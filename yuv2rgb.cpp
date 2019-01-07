#include <stdint.h>

#ifndef MAX
    #define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
    #define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static int32_t U[256], V[256], Y1[256], Y2[256];

void init_yuv422p_table(void) {
    uint16_t i;
    for (i=0; i<256; i++) {
        V[i]  = 15938L*i - 2221300L;
        U[i]  = 20238L*i - 2771300L;
        Y1[i] = 11644L*i;
        Y2[i] = 19837L*i - 311710L;
    }
}

void yuv422p_to_rgb24_(uint8_t y, uint8_t cb, uint8_t cr, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = MAX(0, MIN(255, (V[cr] + Y1[y])/10000L));
    *b = MAX(0, MIN(255, (U[cb] + Y1[y])/10000L));
    *g = MAX(0, MIN(255, (Y2[y] - 5094L*(*r) - 1942L*(*b))/10000L));
}

/*
          w
  +--------------------+
  |Y0Y1Y2Y3...         |
  |...                 |   h
  |...                 |
  |                    |
  +--------------------+
  |U0U1      |
  |...       |   h
  |...       |
  |          |
  +----------+
  |V0V1      |
  |...       |  h
  |...       |
  |          |
  +----------+
      w/2
*/

void yuv422p_to_rgb24(uint8_t *yuv422p, uint8_t *rgb, uint16_t width, uint16_t height) {
    uint32_t i = 0;
    uint8_t *p_y, *p_u, *p_v, *p_rgb;

    static int init_yuv422p = 0;    // just do it once
    if (!init_yuv422p) {
        init_yuv422p_table();
        init_yuv422p = 1;
    }

    p_y = yuv422p;
    p_u = p_y + width*height;
    p_v = p_u + width*height/2;
    p_rgb = rgb;

    for (i=0; i<width*height/2; i++) {
//        yuv422p_to_rgb24_(*(p_y+0), *p_u, *p_v, p_rgb+0, p_rgb+1, p_rgb+2);
//        yuv422p_to_rgb24_(*(p_y+1), *p_u, *p_v, p_rgb+3, p_rgb+4, p_rgb+5);

        yuv422p_to_rgb24_(*(p_y+0), *(p_u+1), *p_u, p_rgb+0, p_rgb+1, p_rgb+2);
        yuv422p_to_rgb24_(*(p_y+1), *(p_u+1), *p_u, p_rgb+3, p_rgb+4, p_rgb+5);

        p_y += 2;
//        p_u++;
//        p_v++;
        p_u+=2;

        p_rgb += 6;
    }
}

