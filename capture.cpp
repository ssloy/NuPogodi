#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

struct buffer {
    void   *start;
    size_t  length;
};

struct buffer          *buffers;
static unsigned int     n_buffers;
static int              fd = -1;
static uint8_t *out_buffer = NULL;

static int xioctl(int fd, int request, void *arg) {
    int r;

    do r = ioctl(fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

static void errno_exit(const char *s) {
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}


int init_device_print_caps() {
    struct v4l2_capability caps = {};
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &caps)) {
        perror("Querying Capabilities error: ");
        return -1;
    }

    printf("Driver Caps:\n"
            "  Driver: \"%s\"\n"
            "  Card: \"%s\"\n"
            "  Bus: \"%s\"\n"
            "  Version: %d.%d\n"
            "  Capabilities: %08x\n",
            caps.driver,
            caps.card,
            caps.bus_info,
            (caps.version>>16)&&0xff,
            (caps.version>>24)&&0xff,
            caps.capabilities);

    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "it is not a video capture device\n");
        return -1;
    }

    if (!(caps.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "the device does not support streaming i/o\n");
        return -1;
    }


    struct v4l2_cropcap cropcap = {0};
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        perror("Querying Cropping Capabilities");
        return -1;
    }

    printf( "Camera Cropping:\n"
            "  Bounds: %dx%d+%d+%d\n"
            "  Default: %dx%d+%d+%d\n"
            "  Aspect: %d/%d\n",
            cropcap.bounds.width, cropcap.bounds.height, cropcap.bounds.left, cropcap.bounds.top,
            cropcap.defrect.width, cropcap.defrect.height, cropcap.defrect.left, cropcap.defrect.top,
            cropcap.pixelaspect.numerator, cropcap.pixelaspect.denominator);





    struct v4l2_fmtdesc fmtdesc = {0};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    char fourcc[5] = {0};
    char c, e;
    printf("  FMT : CE Desc\n--------------------\n");
    while (0 == xioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
//        if (fmtdesc.pixelformat == V4L2_PIX_FMT_YUV422P) {
//        fmt.pix.pixelformat = fmtdesc.pixelformat;
//        }

        strncpy(fourcc, (char *)&fmtdesc.pixelformat, 4);
        c = fmtdesc.flags & 1? 'C' : ' ';
        e = fmtdesc.flags & 2? 'E' : ' ';
        printf("  %s: %c%c %s\n", fourcc, c, e, fmtdesc.description);
        fmtdesc.index++;
    }

    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 320;
    fmt.fmt.pix.height = 240;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV422P;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;


    // trik specific call
    v4l2_std_id stdid = V4L2_STD_625_50;
    if (xioctl(fd, VIDIOC_S_STD, &stdid)) {
        perror("VIDIOC_S_STD failed: ");
        return -1;
    }

    // trik specific call
    if (-1 == xioctl(fd, VIDIOC_TRY_FMT, &fmt)) {
        perror("Trying Pixel Format");
        return 1;
    }

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) {
        perror("Setting Pixel Format");
        return 1;
    }

    strncpy(fourcc, (char *)&fmt.fmt.pix.pixelformat, 4);
    printf( "Selected Camera Mode:\n"
            "  Width: %d\n"
            "  Height: %d\n"
            "  PixFmt: %s\n"
            "  Field: %d\n",
            fmt.fmt.pix.width,
            fmt.fmt.pix.height,
            fourcc,
            fmt.fmt.pix.field);

    return 0;
}

void open_device(char *dev_name) {
    struct stat st;

    if (-1 == stat(dev_name, &st)) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no device\n", dev_name);
        exit(EXIT_FAILURE);
    }

    fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);
    if (-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}


void close_device() {
    if (-1 == close(fd))
        errno_exit("close");
    fd = -1;
}

int init_mmap() {
    struct v4l2_requestbuffers req = {0};
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        perror("Requesting Buffer");
        return -1;
    }

    buffers = (buffer *)calloc(req.count, sizeof(*buffers));

    for (n_buffers = 0; n_buffers<req.count; ++n_buffers) {
        struct v4l2_buffer buf = {0};

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
            perror("VIDIOC_QUERYBUF");
            return -1;
        }

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL /* start anywhere */, buf.length, PROT_READ | PROT_WRITE /* required */, MAP_SHARED /* recommended */, fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start) {
            perror("mmap");
            return -1;
        }
    }

    return 0;
}

void uninit_device() {
    unsigned int i;

    for (i = 0; i < n_buffers; ++i)
        if (-1 == munmap(buffers[i].start, buffers[i].length))
            errno_exit("munmap");

    free(buffers);
}


 void start_capturing(void) {
    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf = {0};

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
        errno_exit("VIDIOC_STREAMON");
}

static int read_frame() {
    struct v4l2_buffer buf = {0};
    unsigned int i;

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
            case EAGAIN:
                return EAGAIN;

            case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

            default:
                errno_exit("VIDIOC_DQBUF");
        }
    }

    assert(buf.index < n_buffers);

    fprintf(stderr, "Debug info: read buffer index %d %d\n", buf.index, buf.bytesused);
    memcpy(out_buffer, buffers[buf.index].start, buf.bytesused);

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        errno_exit("VIDIOC_QBUF");
    return 0;
}


int capture_image(uint8_t *out) {
    out_buffer = out;
    for (;;) {
        fd_set fds;
        struct timeval tv;
        int r;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r) {
            if (EINTR == errno)
                continue;
            perror("select");
        }

        if (0 == r) {
            fprintf(stderr, "select timeout\n");
            return -1;
        }

        int ret = read_frame();
        if (EAGAIN==ret) continue;
        return ret;
    }
}


