#ifndef __YUV2RGB_H__
#define __YUV2RGB_H__

#include <stdint.h>

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

void yuv422p_to_rgb24(uint8_t *yuv422p, uint8_t *rgb, uint16_t width, uint16_t height);

#endif //__YUV2RGB_H__

