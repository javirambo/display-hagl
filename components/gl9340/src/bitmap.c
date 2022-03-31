/*
 *  Javier 2022
 */

#include <stdlib.h>
#include <stdint.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <string.h>
#include "bitmap.h"

bitmap_t* bitmap_new(int width, int height, void *buffer)
{
	bitmap_t *bmp = calloc(1, sizeof(bitmap_t));
	bmp->width = width;
	bmp->height = height;
	bmp->transparentColor = -1;

	// si le paso un buffer con una imagen, obtengo los punteros a las lineas:
	if (buffer != NULL)
	{
		bmp->pixels = calloc(height, sizeof(color_t*));
		color_t *p = (color_t*) buffer;
		for (int y = 0; y < height; y++)
		{
			bmp->pixels[y] = p;
			for (int x = 0; x < width; x++)
			{
				bmp->pixels[y][x] = (*p << 8) | (*p >> 8);
				p++;
			}
		}
		bmp->needsFree = 0;
	}

	// creo un buffer con el tamaño especificado:
	else
	{
		// espacio para el array de punteros:
		bmp->pixels = heap_caps_malloc(height * sizeof(color_t*), MALLOC_CAP_DMA | MALLOC_CAP_32BIT);
		if (bmp->pixels == NULL)
			ESP_LOGE("*", "no mem!");

		// espacio para cada linea:
		for (int i = 0; i < height; i++)
		{
			bmp->pixels[i] = heap_caps_malloc(width * sizeof(color_t), MALLOC_CAP_DMA | MALLOC_CAP_32BIT);
			if (bmp->pixels[i] == NULL)
				ESP_LOGE("*", "no mem!");
		}
		bmp->needsFree = 1;
	}
	return bmp;
}

void bitmap_delete(bitmap_t *bmp)
{
	if (bmp)
	{
		if (bmp->pixels)
		{
			if (bmp->needsFree)
				for (int i = 0; i < bmp->height; i++)
					heap_caps_free(bmp->pixels[i]);
			heap_caps_free(bmp->pixels);
		}
		free(bmp);
	}
}

/*
 * Blit source bitmap to a destination bitmap.
 */
void bitmap_blit(int16_t x0, int16_t y0, bitmap_t *src, bitmap_t *dst, RECT *rect)
{
	int16_t srcw, srch, x1, y1;

	if (rect == NULL)
	{
		srcw = src->width;
		srch = src->height;
		x1 = 0;
		y1 = 0;
	}
	else
	{
		srcw = rect->w;
		srch = rect->h;
		x1 = rect->x0;
		y1 = rect->y0;
	}

	/* x0 or y0 is over the edge, nothing to do. */
	if ((x0 > dst->width) || (y0 > dst->height))
		return;

	/* x0 is negative, ignore parts outside of screen. */
	if (x0 < 0)
	{
		srcw = srcw + x0;
		x1 = abs(x0);
		x0 = 0;
	}

	/* y0 is negative, ignore parts outside of screen. */
	if (y0 < 0)
	{
		srch = srch + y0;
		y1 = abs(y0);
		y0 = 0;
	}

	/* Ignore everything going over right edge. */
	if (srcw > dst->width - x0)
		srcw = dst->width - x0;

	/* Ignore everything going over bottom edge. */
	if (srch > dst->height - y0)
		srch = dst->height - y0;

	/* Everthing outside viewport, nothing to do. */
	if ((srcw < 0) || (srch < 0))
		return;

	color_t color;
	for (uint16_t y = 0; y < srch; y++)
		for (uint16_t x = 0; x < srcw; x++)
		{
			color = src->pixels[y + y1][x + x1];
			if (color != src->transparentColor)
				dst->pixels[y + y0][x + x0] = color;
		}
}

// scroll de un solo pixel en la direccion especificada:
void bitmap_shift(uint8_t direction, RECT *window, bitmap_t *bmp)
{
	uint16_t y, x;
	switch (direction)
	{
		case SCROLL_UP:
			ESP_LOGE("*","%d %d %d %d", window->x0,window->y0, window->x1,window->y1);
			for (y = window->y0; y < window->y1; y++)
				for (x = window->x0; x <= window->x1; x++)
					bmp->pixels[y][x] = bmp->pixels[y + 1][x];
			break;
		case SCROLL_DOWN:
			for (y = window->y1; y > window->y0; y--)
				for (x = window->x0; x <= window->x1; x++)
					bmp->pixels[y][x] = bmp->pixels[y - 1][x];
			break;
		case SCROLL_LEFT:
			for (x = window->x0; x < window->x1; x++)
				for (y = window->y0; y <= window->y1; y++)
					bmp->pixels[y][x] = bmp->pixels[y][x + 1];
			break;
		case SCROLL_RIGHT:
			for (x = window->x1; x > window->x0; x--)
				for (y = window->y0; y <= window->y1; y++)
					bmp->pixels[y][x] = bmp->pixels[y][x - 1];
			break;
	}
}
