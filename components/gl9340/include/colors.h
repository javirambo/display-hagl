/*
 * colors.h
 *
 *  Created on: 10 feb. 2022
 *      Author: Javier
 */
#include "sdkconfig.h"

#ifndef COMPONENTS_HAGL_INCLUDE_COLORS_H_
#define COMPONENTS_HAGL_INCLUDE_COLORS_H_

//#ifdef CONFIG_MIPI_DCS_ADDRESS_MODE_BGR_SELECTED

#define BLACK       0x0000 // 0, 0, 0
#define NAVY        0x1000 // 0, 0, 128
#define DARKGREEN   0x0004 // 0, 128, 0
#define DARKCYAN    0x1004 // 0, 128, 128
#define MAROON      0x0080 // 128, 0, 0
#define PURPLE      0x1080 // 128, 0, 128
#define OLIVE       0x0084 // 128, 128, 0
#define LIGHTGREY   0x1084 // 128, 128, 128
#define DARKGREY    0x18C6 // 192, 192, 192
#define BLUE        0x1F00 // 0, 0, 255
#define GREEN       0xE007 // 0, 255, 0
#define CYAN        0xFF07 // 0, 255, 255
#define RED         0x00F8 // 255, 0, 0
#define MAGENTA     0x1FF8 // 255, 0, 255
#define YELLOW      0xE0FF // 255, 255, 0
#define WHITE       0xFFFF // 255, 255, 255
#define ORANGE      0x82f9 // 255, 50, 20
#define ORANGEYEL   0x20fd // 255, 165, 0
#define GREENYELLOW 0xf52f // 47, 255,  173
#define PINK        0x38fe // f9 c6 c3

/*#else

#define BLACK       0x0000      //   0,   0,   0  /
#define NAVY        0x000F      //   0,   0, 128  /
#define DARKGREEN   0x03E0      //   0, 128,   0  /
#define DARKCYAN    0x03EF      //   0, 128, 128  /
#define MAROON      0x7800      // 128,   0,   0  /
#define PURPLE      0x780F      // 128,   0, 128  /
#define OLIVE       0x7BE0      // 128, 128,   0  /
#define LIGHTGREY   0xC618      // 192, 192, 192  /
#define DARKGREY    0x7BEF      // 128, 128, 128  /
#define BLUE        0x001F      //   0,   0, 255  /
#define GREEN       0x07E0      //   0, 255,   0  /
#define CYAN        0x07FF      //   0, 255, 255  /
#define RED         0xF800      // 255,   0,   0  /
#define MAGENTA     0xF81F      // 255,   0, 255  /
#define YELLOW      0xFFE0      // 255, 255,   0  /
#define WHITE       0xFFFF      // 255, 255, 255  /
#define ORANGE      0xFD20      // 255, 165,   0  /
#define GREENYELLOW 0xAFE5      // 173, 255,  47  /
#define PINK        0xF81F

#endif*/

#endif /* COMPONENTS_HAGL_INCLUDE_COLORS_H_ */
