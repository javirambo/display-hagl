/*
 * Javier
 */

#include <stdint.h>
#include "rect.h"
#include "hal/hagl_hal.h"

#ifndef _BITMAP_H
#define _BITMAP_H

typedef color_t** pixels_t;

// Bitmap para imagenes rgb565
typedef struct
{
	uint16_t width;		// ancho en pixels (1 pix =2 bytes)
	uint16_t height;	// alto en pixels
	pixels_t pixels;	// buffer para rgb565 (array de punteros a lineas)
	uint32_t transparentColor; // color entre 0 y FFFF, no se pintará este color (-1 no es transparente)
	uint8_t needsFree;	// =1 si es gimp image (no tengo que hacerle free al buffer)
} bitmap_t;

bitmap_t *bitmap_new(int ancho, int alto);
bitmap_t* bitmap_init(int width, int height, void *buffer);
void bitmap_delete(bitmap_t *bmp);
void bitmap_blit(int16_t x0, int16_t y0, bitmap_t *src, bitmap_t *dst, RECT *rect);
void bitmap_shift(uint8_t direction, RECT * window, bitmap_t * bmp);

#endif /* _BITMAP_H */
