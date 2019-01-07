#include <iostream>
#include <fstream>

#include "yuv2rgb.h"
#include "capture.h"


using namespace std;

#if 0

        inline unsigned clip255(int x) { return x >= 255 ? 255 : (x <= 0)? 0 : x; }

void yuv422pToRgb(uint8_t *yuv422p, uint8_t *rgb, uint16_t width, uint16_t height) {
    const uint8_t *Y = &yuv422p[0];
    const uint8_t *U = &yuv422p[width * height];
    const uint8_t *V = &yuv422p[3 * width * height / 2];

    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; col+=2) {
            int startIndex = row * width + col;
            int y1 = Y[startIndex] - 16;
            int y2 = Y[startIndex+1] - 16;
            int u  = yuv422p[width * height+startIndex+1] - 128;
            int v  = yuv422p[width * height+startIndex+0] - 128;
            int _298y1 = 298 * y1;
            int _298y2 = 298 * y2;
            int _409v  = 409 * v;
            int _100u  = -100 * u;
            int _516u  = 516 * u;
            int _208v  = -208 * v;
            int r1 = clip255 ((_298y1 + _409v + 128) >> 8);
            int g1 = clip255 ((_298y1 + _100u + _208v + 128) >> 8);
            int b1 = clip255 ((_298y1 + _516u + 128) >> 8);
            int r2 = clip255 ((_298y2 + _409v + 128) >> 8);
            int g2 = clip255 ((_298y2 + _100u + _208v + 128) >> 8);
            int b2 = clip255 ((_298y2 + _516u + 128) >> 8);
            rgb[startIndex*3+0] = r1;
            rgb[startIndex*3+1] = g1;
            rgb[startIndex*3+2] = b1;
            rgb[startIndex*3+3] = r2;
            rgb[startIndex*3+4] = g2;
            rgb[startIndex*3+5] = b2;
/*
            rgb[startIndex*3+0] = U[startIndex+1];
            rgb[startIndex*3+1] = U[startIndex+1];
            rgb[startIndex*3+2] = U[startIndex+1];
            rgb[startIndex*3+3] = U[startIndex+1];
            rgb[startIndex*3+4] = U[startIndex+1];
            rgb[startIndex*3+5] = U[startIndex+1];
*/
        }
    }
}

#endif

int main(int argc, char** argv) {
    const int width = 320;
    const int height = 240;

    open_device((char *)"/dev/video0");

    if (init_device_print_caps())
        return -1;

    if (init_mmap())
        return -1;

    start_capturing();
    uint8_t *yuv422p_buf = new uint8_t[width*height*2];
    uint8_t *rgb_buf = new uint8_t[width*height*3];

    capture_image(yuv422p_buf);
    yuv422p_to_rgb24(yuv422p_buf, rgb_buf, width, height);
 
//    yuv422pToRgb(yuv422p_buf, rgb_buf, width, height);



    ofstream image;
    image.open("frame.ppm");
    image << "P6\n" << width << " " << height << "\n255\n";
    image.write((char *)rgb_buf, width*height*3);
    image.close();

    uninit_device();

    close_device();
    delete [] rgb_buf;
    delete [] yuv422p_buf;
    return 0;
}

