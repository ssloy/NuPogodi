#ifndef __CAPTURE_H__
#define __CAPTURE_H__


void open_device(char *dev_name);
int init_device_print_caps();
int init_mmap();
void start_capturing(void);
int capture_image(uint8_t *out);
void uninit_device();
void close_device();

/*
int main() {
    open_device((char *)"/dev/video0");

    if (init_device_print_caps())
        return -1;

    if (init_mmap())
        return -1;

    start_capturing();
    capture_image();
    uninit_device();

    close_device();
    return 0;
}

*/

#endif //__CAPTURE_H__
