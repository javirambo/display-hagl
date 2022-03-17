/*
 MIT License

 Copyright (c) 2018-2021 Mika Tuupola

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 -cut-

 This file is part of the HAGL graphics library:
 https://github.com/tuupola/hagl

 ... y modificada por Javier.

 SPDX-License-Identifier: MIT
 */

#include "../../gl9340/include/gl.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <esp_log.h>

#include "fsTools.h"
#include "../../gl9340/include/bitmap.h"
#include "../../gl9340/include/clip.h"
#include "../../gl9340/include/jpeg_decomp.h"
#include "../../gl9340/include/rgb565.h"

static const char *TAG = "gl";

////////////////////////////////////////////////////////////////////////

#define ABS(x)  ((x) > 0 ? (x) : -(x))

#define GL_CHAR_BUFFER_SIZE    (16 * 16 * DISPLAY_DEPTH / 2)

extern RECT clip_window;

typedef struct
{
	FILE *fp;
	int16_t x0;
	int16_t y0;
	uint32_t array_index;
	uint32_t array_size;
	uint8_t *array_data;
	bitmap_t *image; // para cargarlo desde FS (ojo, hacer un free del buffer si no se usa mas)
} tjpgd_iodev_t;

RECT clip_window = {
	.x0 = 0, .y0 = 0, .x1 = DISPLAY_WIDTH - 1, .y1 = DISPLAY_HEIGHT - 1,
	.w = DISPLAY_WIDTH, .h = DISPLAY_HEIGHT
};

#define JPG_WORKSPACE_SIZE 4000
static uint8_t workspace[JPG_WORKSPACE_SIZE] __attribute__((aligned(4)));

static bool str_ends_with(const char *name, char *ext)
{
	char *dot = strrchr(name, '.');
	return dot && !strcmp(dot, ext);
}

////////////////////////////////////////////////////////////////////////
//////  GENERAL
////////////////////////////////////////////////////////////////////////

color_t gl_color(uint8_t r, uint8_t g, uint8_t b)
{
	return rgb565(r, g, b);
}

bitmap_t* gl_init()
{
#ifdef HAGL_HAS_HAL_INIT
	bitmap_t *bb = hagl_hal_init();
	gl_clear_screen();
	return bb;
#else
    gl_clear_screen();
    return NULL;
#endif
}

void gl_flush()
{
#ifdef HAGL_HAS_HAL_FLUSH
    gl_hal_flush();
#endif
}

void gl_close()
{
#ifdef HAGL_HAS_HAL_CLOSE
    gl_hal_close();
#endif
}

inline void gl_set_clip_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	clip_window.x0 = x0;
	clip_window.y0 = y0;
	clip_window.x1 = x1;
	clip_window.y1 = y1;
}

inline void gl_clear_clip_window()
{
	gl_fill_clip_window(BLACK);
}

inline void gl_fill_clip_window(color_t color)
{
	gl_fill_rectangle(clip_window.x0, clip_window.y0, clip_window.x1, clip_window.y1, color);
}

void gl_put_pixel(int16_t x0, int16_t y0, color_t color)
{
	/* x0 or y0 is before the edge, nothing to do. */
	if ((x0 < clip_window.x0) || (y0 < clip_window.y0))
	{
		return;
	}

	/* x0 or y0 is after the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (y0 > clip_window.y1))
	{
		return;
	}

	/* If still in bounds set the pixel. */
	hagl_hal_put_pixel(x0, y0, color);
}

color_t gl_get_pixel(int16_t x0, int16_t y0)
{
	/* x0 or y0 is before the edge, nothing to do. */
	if ((x0 < clip_window.x0) || (y0 < clip_window.y0))
	{
		return gl_color(0, 0, 0);
	}

	/* x0 or y0 is after the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (y0 > clip_window.y1))
	{
		return gl_color(0, 0, 0);
	}

#ifdef gl_HAS_HAL_GET_PIXEL
    return hagl_hal_get_pixel(x0, y0);
#else
	return gl_color(0, 0, 0);
#endif /* gl_HAS_HAL_GET_PIXEL */
}

inline void gl_clear_screen()
{
	gl_fill_screen(BLACK);
}

void gl_fill_screen(color_t color)
{
#ifdef HAGL_HAS_HAL_CLEAR_SCREEN
    hagl_hal_clear_screen();
#else
	uint16_t x0 = clip_window.x0;
	uint16_t y0 = clip_window.y0;
	uint16_t x1 = clip_window.x1;
	uint16_t y1 = clip_window.y1;

	gl_set_clip_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
	gl_fill_rectangle(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, color);
	gl_set_clip_window(x0, y0, x1, y1);
#endif
}

////////////////////////////////////////////////////////////////////////
/////  COSAS DE IMAGENES
////////////////////////////////////////////////////////////////////////

/**
 * Lo usa el decoder: lee desde un archivo.
 */
static unsigned int jpg_file_reader(JDEC *decoder, uint8_t *buffer, unsigned int size)
{
	tjpgd_iodev_t *device = (tjpgd_iodev_t*) decoder->device;
	if (buffer)
		return (uint16_t) fread(buffer, 1, size, device->fp);
	else
		// tjpgd.c => Null pointer specifies to skip bytes of stream
		return fseek(device->fp, size, SEEK_CUR) ? 0 : size;
}

// copio cada rectangulo jpeg al destino que es el pedacito que le corresponde en el bmp.
static int jpg_memcpy(JDEC *decoder, void *bitmap, JRECT *rectangle)
{
	tjpgd_iodev_t *device = (tjpgd_iodev_t*) decoder->device;
	uint8_t *pimg = device->image->buffer;
	uint8_t *pbmp = ((uint8_t*) bitmap);
	int w = (rectangle->right - rectangle->left + 1) * 2;
	int l = rectangle->left * 2;
	for (int y = rectangle->top; y <= rectangle->bottom; y++)
	{
		memcpy(pimg + l + device->image->pitch * y, pbmp, w);
		pbmp += w;
	}
	return 1;
}

/**
 * Lo usa el gl_draw_jpg
 */
static unsigned int jpg_input(JDEC *decoder, uint8_t *buf, unsigned int len)
{
	tjpgd_iodev_t *device = (tjpgd_iodev_t*) decoder->device;

	// Avoid running off end of array
	if (device->array_index + len > device->array_size)
	{
		len = device->array_size - device->array_index;
	}

	// If buf is valid then copy len bytes to buffer
	if (buf)
		memcpy(buf, (const uint8_t*) (device->array_data + device->array_index), len);

	// Move pointer
	device->array_index += len;
	return len;
}

/**
 * Lo usa el decoder para mostrar en pantalla
 * y el gl_draw_jpg
 */
static int jpg_data_blit(JDEC *decoder, void *bitmap, JRECT *rectangle)
{
	tjpgd_iodev_t *device = (tjpgd_iodev_t*) decoder->device;
	uint16_t width = (rectangle->right - rectangle->left) + 1;
	uint16_t height = (rectangle->bottom - rectangle->top) + 1;

	bitmap_t block = {
		.width = width,
		.height = height,
		.depth = DISPLAY_DEPTH,
		.pitch = width * (DISPLAY_DEPTH / 8),
		.size = width * (DISPLAY_DEPTH / 8) * height,
		.buffer = (uint8_t*) bitmap };

	gl_blit(rectangle->left + device->x0, rectangle->top + device->y0, &block);
	return 1;
}

/*
 * Carga una imagen desde archivo al bmp.
 * TODO hacer una seleccion de tipo de decores dependiendo de la extension del archivo
 * (por ahora es solo para jpg)
 * OJO, HACER FREE DEL BUFFER DEL BITMAP!!
 */
uint32_t gl_load_image(const char *filename, bitmap_t *bmp)
{
	JDEC decoder;
	JRESULT result;
	tjpgd_iodev_t device;
	device.x0 = 0;
	device.y0 = 0;
	device.fp = fs_open_file(filename, "rb");

	if (!str_ends_with(filename, ".jpg"))
		return GL_ERR_FILE_IO;

	if (!device.fp)
		return GL_ERR_FILE_IO;

	result = jd_prepare(&decoder, jpg_file_reader, workspace, JPG_WORKSPACE_SIZE, (void*) &device);
	if (result == JDR_OK)
	{
		//-1- creo el bitmap con sus tamaños y cosas de la imagen:
		bmp->width = decoder.width;
		bmp->height = decoder.height;
		bmp->depth = DISPLAY_DEPTH;
		bmp->pitch = decoder.width * (DISPLAY_DEPTH / 8);
		bmp->size = decoder.width * (DISPLAY_DEPTH / 8) * decoder.height;
		bmp->buffer = malloc(bmp->size * DISPLAY_DEPTH / 8);
		//-2- copio la imagen en el buffer:
		device.image = bmp;
		result = jd_decomp(&decoder, jpg_memcpy, 0);
		if (JDR_OK != result)
		{
			fclose(device.fp);
			return GL_ERR_TJPGD + result;
		}
	}
	else
	{
		fclose(device.fp);
		return GL_ERR_TJPGD + result;
	}
	fclose(device.fp);
	return GL_OK;
}

/*
 * Free solo del buffer del bitmap_t
 */
void gl_free_bitmap_buffer(bitmap_t *bitmap)
{
	if (bitmap && bitmap->buffer)
		free(bitmap->buffer);
}

/*
 * Por si el bitmap_t se creó con malloc
 */
void gl_free_bitmap(bitmap_t *bitmap)
{
	if (bitmap)
	{
		if (bitmap->buffer)
			free(bitmap->buffer);
		free(bitmap);
	}
}

/*
 * Carga una imagen desde archivo y muestra en el display.
 * * TODO determinar el tipo de imagen con la extension.
 *
 * Output will be clipped to the current clip window. Does not do
 * any scaling. Currently supports only baseline jpg images.
 */
uint32_t gl_show_image_file(int16_t x0, int16_t y0, const char *filename)
{
	JDEC decoder;
	JRESULT result;
	tjpgd_iodev_t device;
	device.x0 = x0;
	device.y0 = y0;

	if (!str_ends_with(filename, ".jpg"))
		return GL_ERR_FILE_IO;

	device.fp = fs_open_file(filename, "rb");
	if (!device.fp)
		return GL_ERR_FILE_IO;

	result = jd_prepare(&decoder, jpg_file_reader, workspace, JPG_WORKSPACE_SIZE, (void*) &device);
	if (result == JDR_OK)
	{
		/***** Muestra la imagen centrada, si x=-1 o y=-1 , @javier ****/
		if (x0 < 0)
			device.x0 = (DISPLAY_WIDTH - decoder.width) / 2;
		if (y0 < 0)
			device.y0 = (DISPLAY_HEIGHT - decoder.height) / 2;
		result = jd_decomp(&decoder, jpg_data_blit, 0);
		if (JDR_OK != result)
		{
			fclose(device.fp);
			return GL_ERR_TJPGD + result;
		}
	}
	else
	{
		fclose(device.fp);
		return GL_ERR_TJPGD + result;
	}

	fclose(device.fp);
	return GL_OK;
}

/**
 *  Muestra una imagen en el display.
 *  La imagen es un jpeg que viene en un buffer.
 *
 *  @param x			posicion x de la imagen (si es -1 la centra en X)
 *  @param y			posicion y de la imagen (si es -1 la centra en Y)
 *  @param jpeg_data	buffer con la imagen jpeg
 *  @param data_size	tamaño
 */
uint32_t gl_show_jpeg_data(int16_t x, int16_t y, const uint8_t jpeg_data[], uint32_t data_size)
{
	tjpgd_iodev_t dev;
	dev.array_index = 0;
	dev.array_data = (uint8_t*) jpeg_data;
	dev.array_size = data_size;
	dev.x0 = x;
	dev.y0 = y;

	JDEC jdec;
	// Analyse input data
	JRESULT jresult = jd_prepare(&jdec, jpg_input, workspace, JPG_WORKSPACE_SIZE, (void*) &dev);

	if (jresult == JDR_OK)
	{
		/***** Muestra la imagen centrada, si x=-1 o y=-1 , @javier ****/
		if (x < 0)
			dev.x0 = (DISPLAY_WIDTH - jdec.width) / 2;
		if (y < 0)
			dev.y0 = (DISPLAY_HEIGHT - jdec.height) / 2;
		// Extract image and render
		jresult = jd_decomp(&jdec, jpg_data_blit, 0);
	}
	else if (jresult == JDR_FMT3)
	{
		ESP_LOGE(TAG, "JPEG NO SOPORTADO. USAR NO PROGRESIVO");
	}
	return jresult;
}

/**
 *  Obtiene el tamaño de la imagen.
 *  La imagen es un jpeg que viene en un buffer.
 */
uint32_t gl_get_jpeg_size(uint16_t *w, uint16_t *h, const uint8_t jpeg_data[], uint32_t data_size)
{
	tjpgd_iodev_t dev;
	dev.array_index = 0;
	dev.array_data = (uint8_t*) jpeg_data;
	dev.array_size = data_size;

	JDEC jdec;
	if (JDR_OK == jd_prepare(&jdec, jpg_input, workspace, JPG_WORKSPACE_SIZE, (void*) &dev))
	{
		*w = jdec.width;
		*h = jdec.height;
	}
	else
	{
		*w = 0;
		*h = 0;
	}
	return GL_OK;
}

/*
 * Cuando se exportan de GIMP a .c las imagenes vienen en Big Endian.
 * Esto hace un swap de los bytes del buffer de la imagen,
 * y quedan asi dados vuelta para usar donde sea.
 *
 * (Accedo a los campos de la estructura del archivo GIMP mediante punteros)
 */
uint32_t gl_load_gimp_image(void *gimp_struct, bitmap_t *bmp)
{
	unsigned int *pgs = gimp_struct;
	unsigned int width = *pgs++;
	unsigned int height = *pgs++;
	unsigned int bytes_per_pixel = *pgs;
	unsigned char *pixel_data = gimp_struct + sizeof(unsigned int) * 3;

	ESP_LOGE(TAG, "%d", bytes_per_pixel);

	// flag para no volver a invertir los datos de la imagen.
	if (bytes_per_pixel == 0)
		return GL_OK;
	*pgs = 0;

	bmp->width = width;
	bmp->height = height;
	bmp->size = width * height;
	bmp->buffer = pixel_data;

	uint16_t *p = (uint16_t*) bmp->buffer;
	uint16_t *end = p + bmp->size;
	while (p < end)
	{
		*p = (*p << 8) | (*p >> 8);
		p++;
	}
	return GL_OK;
}

/**
 * Blit a bitmap to the display
 *
 * Output will be clipped to the current clip window.
 *
 * @param x0
 * @param y0
 * @param source pointer to a bitmap
 */
void gl_blit(int16_t x0, int16_t y0, bitmap_t *source)
{
#ifdef HAGL_HAS_HAL_BLIT
	/* Check if bitmap is inside clip windows bounds */
	if ((x0 < clip_window.x0) || (y0 < clip_window.y0) || (x0 + source->width > clip_window.x1) || (y0 + source->height > clip_window.y1))
	{
		/* Out of bounds, use local pixel fallback. */
		color_t color;
		color_t *ptr = (color_t*) source->buffer;

		for (uint16_t y = 0; y < source->height; y++)
		{
			for (uint16_t x = 0; x < source->width; x++)
			{
				color = *(ptr++);
				gl_put_pixel(x0 + x, y0 + y, color);
			}
		}
	}
	else
	{
		/* Inside of bounds, can use HAL provided blit. */
		hagl_hal_blit(x0, y0, source);
	}
#else
    color_t color;
    color_t *ptr = (color_t *) source->buffer;

    for (uint16_t y = 0; y < source->height; y++) {
        for (uint16_t x = 0; x < source->width; x++) {
            color = *(ptr++);
            gl_put_pixel(x0 + x, y0 + y, color);
        }
    }
#endif
}

/*
 * Muestra una porcion de la imagen del bmp en la pos x0,y0 de la pantalla.
 * La porcion de la imagen está dada por el RECT.
 * Los bits = transparentColor no los pinta en el display.
 *
 * El RECT tiene que estar normalizado (x/y/ancho y alto)
 */
uint32_t gl_show_partial_image(uint16_t x0, uint16_t y0, bitmap_t *image, RECT *rect, uint16_t transparentColor)
{
	uint16_t *bmp = (uint16_t*) image->buffer;
	int h = rect->h;
	int w = rect->w;
	int pitch = image->width - w;
	bmp += rect->x0; // posiciono la x
	bmp += (rect->y0 * image->width); // posiciono la y

	for (int fy = 0; fy < h; fy++)
	{
		for (int fx = 0; fx < w; fx++)
		{
			// colores del fondo
			/*pix = gl_get_pixel(x + fx, y + fy);
			 bb = pix & 0xF800;
			 bg = pix & 0x07E0;
			 br = pix & 0x001F;

			 // colores de la letra
			 fb = 0xF800 - (*bmp & 0xF800);
			 fg = 0x07E0 - (*bmp & 0x07E0);
			 fr = 0x001F - (*bmp & 0x001F);
			 alfa = ((float) fg) / (float) 0x3f; // 0..1*/

			//alfa = (*pbmp & 0x07E0) >> 5;
			if (*bmp != transparentColor)
			{
				//fg = (*pbmp * alfa) + (bg * (0x1f - alfa));
				//	pix = ((br * fr / 0x1f) & 0xf800) | ((bg * fg / 0x2f) & 0x07e0) | ((bb * fb / 0x1f) & 0x001f);
				//pix = ((uint16_t) ((float) fg * alfa + (float) bg * (1 - alfa))) & 0x07e0;
				gl_put_pixel(x0 + fx, y0 + fy, *bmp);
			}
			bmp++; // sig pixel horiz
		}
		bmp += pitch; // sig linea
	}
	return GL_OK;
}

//////////////////////////////////////////////////////////////////////
////////// COSAS DE SHAPES Y DIBUJOS
//////////////////////////////////////////////////////////////////////

void gl_draw_hline(int16_t x0, int16_t y0, uint16_t w, color_t color)
{
#ifdef HAGL_HAS_HAL_HLINE
	int16_t width = w;

	/* x0 or y0 is over the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (y0 > clip_window.y1) || (y0 < clip_window.y0))
	{
		return;
	}

	/* x0 is left of clip window, ignore start part. */
	if (x0 < clip_window.x0)
	{
		width = width + x0;
		x0 = clip_window.x0;
	}

	/* Everything outside clip window, nothing to do. */
	if (width < 0)
	{
		return;
	}

	/* Cut anything going over right edge of clip window. */
	if (((x0 + width) > clip_window.x1))
	{
		width = width - (x0 + width - clip_window.x1);
	}

	hagl_hal_hline(x0, y0, width, color);
#else
    hagl_draw_line(x0, y0, x0 + w, y0, color);
#endif
}

/*
 * Draw a vertical line with given color. If HAL supports it uses
 * hardware vline drawing. If not falls back to vanilla line drawing.
 */
void gl_draw_vline(int16_t x0, int16_t y0, uint16_t h, color_t color)
{
#ifdef HAGL_HAS_HAL_VLINE
	int16_t height = h;

	/* x0 or y0 is over the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (x0 < clip_window.x0) || (y0 > clip_window.y1))
	{
		return;
	}

	/* y0 is top of clip window, ignore start part. */
	if (y0 < clip_window.y0)
	{
		height = height + y0;
		y0 = clip_window.y0;
	}

	/* Everything outside clip window, nothing to do. */
	if (height < 0)
	{
		return;
	}

	/* Cut anything going over right edge. */
	if (((y0 + height) > clip_window.y1))
	{
		height = height - (y0 + height - clip_window.y1);
	}

	hagl_hal_vline(x0, y0, height, color);
#else
    hagl_draw_line(x0, y0, x0, y0 + h, color);
#endif
}

/*
 * Draw a line using Bresenham's algorithm with given color.
 */
void gl_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color)
{
	/* Clip coordinates to fit clip window. */
	if (false == clip_line(&x0, &y0, &x1, &y1, clip_window))
	{
		return;
	}

	int16_t dx;
	int16_t sx;
	int16_t dy;
	int16_t sy;
	int16_t err;
	int16_t e2;

	dx = ABS(x1 - x0);
	sx = x0 < x1 ? 1 : -1;
	dy = ABS(y1 - y0);
	sy = y0 < y1 ? 1 : -1;
	err = (dx > dy ? dx : -dy) / 2;

	while (1)
	{
		gl_put_pixel(x0, y0, color);

		if (x0 == x1 && y0 == y1)
		{
			break;
		};

		e2 = err + err;

		if (e2 > -dx)
		{
			err -= dy;
			x0 += sx;
		}

		if (e2 < dy)
		{
			err += dx;
			y0 += sy;
		}
	}
}

/*
 * Draw a rectangle with given color.
 */
void gl_draw_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color)
{
	/* Make sure x0 is smaller than x1. */
	if (x0 > x1)
	{
		x0 = x0 + x1;
		x1 = x0 - x1;
		x0 = x0 - x1;
	}

	/* Make sure y0 is smaller than y1. */
	if (y0 > y1)
	{
		y0 = y0 + y1;
		y1 = y0 - y1;
		y0 = y0 - y1;
	}

	/* x1 or y1 is before the edge, nothing to do. */
	if ((x1 < clip_window.x0) || (y1 < clip_window.y0))
	{
		return;
	}

	/* x0 or y0 is after the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (y0 > clip_window.y1))
	{
		return;
	}

	uint16_t width = x1 - x0 + 1;
	uint16_t height = y1 - y0 + 1;

	gl_draw_hline(x0, y0, width, color);
	gl_draw_hline(x0, y1, width, color);
	gl_draw_vline(x0, y0, height, color);
	gl_draw_vline(x1, y0, height, color);
}

/*
 * Draw a filled rectangle with given color.
 */
void gl_fill_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color)
{
	/* Make sure x0 is smaller than x1. */
	if (x0 > x1)
	{
		x0 = x0 + x1;
		x1 = x0 - x1;
		x0 = x0 - x1;
	}

	/* Make sure y0 is smaller than y1. */
	if (y0 > y1)
	{
		y0 = y0 + y1;
		y1 = y0 - y1;
		y0 = y0 - y1;
	}

	/* x1 or y1 is before the edge, nothing to do. */
	if ((x1 < clip_window.x0) || (y1 < clip_window.y0))
	{
		return;
	}

	/* x0 or y0 is after the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (y0 > clip_window.y1))
	{
		return;
	}

	x0 = max(x0, clip_window.x0);
	y0 = max(y0, clip_window.y0);
	x1 = min(x1, clip_window.x1);
	y1 = min(y1, clip_window.y1);

	uint16_t width = x1 - x0 + 1;
	uint16_t height = y1 - y0 + 1;

	for (uint16_t i = 0; i < height; i++)
	{
#ifdef HAGL_HAS_HAL_HLINE
		/* Already clipped so can call HAL directly. */
		hagl_hal_hline(x0, y0 + i, width, color);
#else
        hagl_draw_hline(x0, y0 + i, width, color);
#endif
	}
}

void gl_draw_circle(int16_t xc, int16_t yc, int16_t r, color_t color)
{
	int16_t x = 0;
	int16_t y = r;
	int16_t d = 3 - 2 * r;

	gl_put_pixel(xc + x, yc + y, color);
	gl_put_pixel(xc - x, yc + y, color);
	gl_put_pixel(xc + x, yc - y, color);
	gl_put_pixel(xc - x, yc - y, color);
	gl_put_pixel(xc + y, yc + x, color);
	gl_put_pixel(xc - y, yc + x, color);
	gl_put_pixel(xc + y, yc - x, color);
	gl_put_pixel(xc - y, yc - x, color);

	while (y >= x)
	{
		x++;

		if (d > 0)
		{
			y--;
			d = d + 4 * (x - y) + 10;
		}
		else
		{
			d = d + 4 * x + 6;
		}

		gl_put_pixel(xc + x, yc + y, color);
		gl_put_pixel(xc - x, yc + y, color);
		gl_put_pixel(xc + x, yc - y, color);
		gl_put_pixel(xc - x, yc - y, color);
		gl_put_pixel(xc + y, yc + x, color);
		gl_put_pixel(xc - y, yc + x, color);
		gl_put_pixel(xc + y, yc - x, color);
		gl_put_pixel(xc - y, yc - x, color);
	}
}

void gl_fill_circle(int16_t x0, int16_t y0, int16_t r, color_t color)
{
	int16_t x = 0;
	int16_t y = r;
	int16_t d = 3 - 2 * r;

	while (y >= x)
	{
		gl_draw_hline(x0 - x, y0 + y, x * 2, color);
		gl_draw_hline(x0 - x, y0 - y, x * 2, color);
		gl_draw_hline(x0 - y, y0 + x, y * 2, color);
		gl_draw_hline(x0 - y, y0 - x, y * 2, color);
		x++;

		if (d > 0)
		{
			y--;
			d = d + 4 * (x - y) + 10;
		}
		else
		{
			d = d + 4 * x + 6;
		}
	}
}

void gl_draw_ellipse(int16_t x0, int16_t y0, int16_t a, int16_t b, color_t color)
{
	int16_t wx, wy;
	int32_t xa, ya;
	int32_t t;
	int32_t asq = a * a;
	int32_t bsq = b * b;

	gl_put_pixel(x0, y0 + b, color);
	gl_put_pixel(x0, y0 - b, color);

	wx = 0;
	wy = b;
	xa = 0;
	ya = asq * 2 * b;
	t = asq / 4 - asq * b;

	while (1)
	{
		t += xa + bsq;

		if (t >= 0)
		{
			ya -= asq * 2;
			t -= ya;
			wy--;
		}

		xa += bsq * 2;
		wx++;

		if (xa >= ya)
		{
			break;
		}

		gl_put_pixel(x0 + wx, y0 - wy, color);
		gl_put_pixel(x0 - wx, y0 - wy, color);
		gl_put_pixel(x0 + wx, y0 + wy, color);
		gl_put_pixel(x0 - wx, y0 + wy, color);
	}

	gl_put_pixel(x0 + a, y0, color);
	gl_put_pixel(x0 - a, y0, color);

	wx = a;
	wy = 0;
	xa = bsq * 2 * a;

	ya = 0;
	t = bsq / 4 - bsq * a;

	while (1)
	{
		t += ya + asq;

		if (t >= 0)
		{
			xa -= bsq * 2;
			t = t - xa;
			wx--;
		}

		ya += asq * 2;
		wy++;

		if (ya > xa)
		{
			break;
		}

		gl_put_pixel(x0 + wx, y0 - wy, color);
		gl_put_pixel(x0 - wx, y0 - wy, color);
		gl_put_pixel(x0 + wx, y0 + wy, color);
		gl_put_pixel(x0 - wx, y0 + wy, color);
	}
}

void gl_fill_ellipse(int16_t x0, int16_t y0, int16_t a, int16_t b, color_t color)
{
	int16_t wx, wy;
	int32_t xa, ya;
	int32_t t;
	int32_t asq = a * a;
	int32_t bsq = b * b;

	gl_put_pixel(x0, y0 + b, color);
	gl_put_pixel(x0, y0 - b, color);

	wx = 0;
	wy = b;
	xa = 0;
	ya = asq * 2 * b;
	t = asq / 4 - asq * b;

	while (1)
	{
		t += xa + bsq;

		if (t >= 0)
		{
			ya -= asq * 2;
			t -= ya;
			wy--;
		}

		xa += bsq * 2;
		wx++;

		if (xa >= ya)
		{
			break;
		}

		gl_draw_hline(x0 - wx, y0 - wy, wx * 2, color);
		gl_draw_hline(x0 - wx, y0 + wy, wx * 2, color);
	}

	gl_draw_hline(x0 - a, y0, a * 2, color);

	wx = a;
	wy = 0;
	xa = bsq * 2 * a;

	ya = 0;
	t = bsq / 4 - bsq * a;

	while (1)
	{
		t += ya + asq;

		if (t >= 0)
		{
			xa -= bsq * 2;
			t = t - xa;
			wx--;
		}

		ya += asq * 2;
		wy++;

		if (ya > xa)
		{
			break;
		}

		gl_draw_hline(x0 - wx, y0 - wy, wx * 2, color);
		gl_draw_hline(x0 - wx, y0 + wy, wx * 2, color);
	}
}

void gl_draw_polygon(int16_t amount, int16_t *vertices, color_t color)
{

	for (int16_t i = 0; i < amount - 1; i++)
	{
		gl_draw_line(vertices[(i << 1) + 0], vertices[(i << 1) + 1], vertices[(i << 1) + 2], vertices[(i << 1) + 3], color);
	}
	gl_draw_line(vertices[0], vertices[1], vertices[(amount << 1) - 2], vertices[(amount << 1) - 1], color);
}

/* Adapted from  http://alienryderflex.com/polygon_fill/ */
void gl_fill_polygon(int16_t amount, int16_t *vertices, color_t color)
{
	uint16_t nodes[64];
	int16_t y;

	float x0;
	float y0;
	float x1;
	float y1;

	int16_t miny = DISPLAY_HEIGHT;
	int16_t maxy = 0;

	for (uint8_t i = 0; i < amount; i++)
	{
		if (miny > vertices[(i << 1) + 1])
		{
			miny = vertices[(i << 1) + 1];
		}
		if (maxy < vertices[(i << 1) + 1])
		{
			maxy = vertices[(i << 1) + 1];
		}
	}

	/*  Loop through the rows of the image. */
	for (y = miny; y < maxy; y++)
	{

		/*  Build a list of nodes. */
		int16_t count = 0;
		int16_t j = amount - 1;

		for (int16_t i = 0; i < amount; i++)
		{
			x0 = vertices[(i << 1) + 0];
			y0 = vertices[(i << 1) + 1];
			x1 = vertices[(j << 1) + 0];
			y1 = vertices[(j << 1) + 1];

			if ((y0 < (float) y && y1 >= (float) y) || (y1 < (float) y && y0 >= (float) y))
			{
				nodes[count] = (int16_t) (x0 + (y - y0) / (y1 - y0) * (x1 - x0));
				count++;
			}
			j = i;
		}

		/* Sort the nodes, via a simple â€œBubbleâ€� sort. */
		int16_t i = 0;
		while (i < count - 1)
		{
			if (nodes[i] > nodes[i + 1])
			{
				int16_t swap = nodes[i];
				nodes[i] = nodes[i + 1];
				nodes[i + 1] = swap;
				if (i)
				{
					i--;
				}
			}
			else
			{
				i++;
			}
		}

		/* Draw lines between nodes. */
		for (int16_t i = 0; i < count; i += 2)
		{
			int16_t width = nodes[i + 1] - nodes[i];
			gl_draw_hline(nodes[i], y, width, color);
		}
	}
}

void gl_draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, color_t color)
{
	int16_t vertices[6] =
			{ x0, y0, x1, y1, x2, y2 };
	gl_draw_polygon(3, vertices, color);
}

void gl_fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, color_t color)
{
	int16_t vertices[6] =
			{ x0, y0, x1, y1, x2, y2 };
	gl_fill_polygon(3, vertices, color);
}

void gl_draw_rounded_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t r, color_t color)
{

	uint16_t width, height;
	int16_t x, y, d;

	/* Make sure x0 is smaller than x1. */
	if (x0 > x1)
	{
		x0 = x0 + x1;
		x1 = x0 - x1;
		x0 = x0 - x1;
	}

	/* Make sure y0 is smaller than y1. */
	if (y0 > y1)
	{
		y0 = y0 + y1;
		y1 = y0 - y1;
		y0 = y0 - y1;
	}

	/* x1 or y1 is before the edge, nothing to do. */
	if ((x1 < clip_window.x0) || (y1 < clip_window.y0))
	{
		return;
	}

	/* x0 or y0 is after the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (y0 > clip_window.y1))
	{
		return;
	}

	/* Max radius is half of shortest edge. */
	width = x1 - x0 + 1;
	height = y1 - y0 + 1;
	r = min(r, min(width / 2, height / 2));

	gl_draw_hline(x0 + r, y0, width - 2 * r, color);
	gl_draw_hline(x0 + r, y1, width - 2 * r, color);
	gl_draw_vline(x0, y0 + r, height - 2 * r, color);
	gl_draw_vline(x1, y0 + r, height - 2 * r, color);

	x = 0;
	y = r;
	d = 3 - 2 * r;

	while (y >= x)
	{
		x++;

		if (d > 0)
		{
			y--;
			d = d + 4 * (x - y) + 10;
		}
		else
		{
			d = d + 4 * x + 6;
		}

		/* Top right */
		gl_put_pixel(x1 - r + x, y0 + r - y, color);
		gl_put_pixel(x1 - r + y, y0 + r - x, color);

		/* Top left */
		gl_put_pixel(x0 + r - x, y0 + r - y, color);
		gl_put_pixel(x0 + r - y, y0 + r - x, color);

		/* Bottom right */
		gl_put_pixel(x1 - r + x, y1 - r + y, color);
		gl_put_pixel(x1 - r + y, y1 - r + x, color);

		/* Bottom left */
		gl_put_pixel(x0 + r - x, y1 - r + y, color);
		gl_put_pixel(x0 + r - y, y1 - r + x, color);
	}
}

void gl_fill_rounded_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t r, color_t color)
{
	uint16_t width, height;
	int16_t rx0, ry0, rx1, x, y, d;

	/* Make sure x0 is smaller than x1. */
	if (x0 > x1)
	{
		x0 = x0 + x1;
		x1 = x0 - x1;
		x0 = x0 - x1;
	}

	/* Make sure y0 is smaller than y1. */
	if (y0 > y1)
	{
		y0 = y0 + y1;
		y1 = y0 - y1;
		y0 = y0 - y1;
	}

	/* x1 or y1 is before the edge, nothing to do. */
	if ((x1 < clip_window.x0) || (y1 < clip_window.y0))
	{
		return;
	}

	/* x0 or y0 is after the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (y0 > clip_window.y1))
	{
		return;
	}

	/* Max radius is half of shortest edge. */
	width = x1 - x0 + 1;
	height = y1 - y0 + 1;
	r = min(r, min(width / 2, height / 2));

	x = 0;
	y = r;
	d = 3 - 2 * r;

	while (y >= x)
	{
		x++;

		if (d > 0)
		{
			y--;
			d = d + 4 * (x - y) + 10;
		}
		else
		{
			d = d + 4 * x + 6;
		}

		/* Top  */
		ry0 = y0 + r - x;
		rx0 = x0 + r - y;
		rx1 = x1 - r + y;
		width = rx1 - rx0;
		gl_draw_hline(rx0, ry0, width, color);

		ry0 = y0 + r - y;
		rx0 = x0 + r - x;
		rx1 = x1 - r + x;
		width = rx1 - rx0;
		gl_draw_hline(rx0, ry0, width, color);

		/* Bottom */
		ry0 = y1 - r + y;
		rx0 = x0 + r - x;
		rx1 = x1 - r + x;
		width = rx1 - rx0;
		gl_draw_hline(rx0, ry0, width, color);

		ry0 = y1 - r + x;
		rx0 = x0 + r - y;
		rx1 = x1 - r + y;
		width = rx1 - rx0;
		gl_draw_hline(rx0, ry0, width, color);
	}

	/* Center */
	gl_fill_rectangle(x0, y0 + r, x1, y1 - r, color);
}

//////////////////////////////////////////////////////////////////////
/////// COSAS DE TEXTO
//////////////////////////////////////////////////////////////////////

uint8_t gl_put_char(char code, int16_t x0, int16_t y0, color_t color, const uint8_t *font)
{
	uint8_t set, status;
	color_t buffer[GL_CHAR_BUFFER_SIZE];
	bitmap_t bitmap;
	fontx_glyph_t glyph;

	status = fontx_glyph(&glyph, (wchar_t) code, font);

	if (0 != status)
	{
		return 0;
	}

	bitmap.width = glyph.width, bitmap.height = glyph.height, bitmap.depth = DISPLAY_DEPTH,

	bitmap_init(&bitmap, (uint8_t*) buffer);

	color_t *ptr = (color_t*) bitmap.buffer;

	for (uint8_t y = 0; y < glyph.height; y++)
	{
		for (uint8_t x = 0; x < glyph.width; x++)
		{
			set = *(glyph.buffer + x / 8) & (0x80 >> (x % 8));
			if (set)
			{
				*(ptr++) = color;
			}
			else
			{
				*(ptr++) = 0x0000;
			}
		}
		glyph.buffer += glyph.pitch;
	}

	gl_blit(x0, y0, &bitmap);

	return bitmap.width;
}

/*
 * Write a string of text by calling gl_put_char() repeadetly. CR and LF
 * continue from the next line.
 */

uint16_t gl_put_text(const char *str, int16_t x0, int16_t y0, color_t color, const unsigned char *font)
{
	char temp;
	uint8_t status;
	uint16_t original = x0;
	fontx_meta_t meta;

	status = fontx_meta(&meta, font);
	if (0 != status)
	{
		return 0;
	}

	do
	{
		temp = *str++;
		if (13 == temp || 10 == temp)
		{
			x0 = 0;
			y0 += meta.height;
		}
		else
		{
			x0 += gl_put_char(temp, x0, y0, color, font);
		}
	} while (*str != 0);

	return x0 - original;
}

uint8_t gl_get_glyph(char code, color_t color, bitmap_t *bitmap, const uint8_t *font)
{
	uint8_t status, set;
	fontx_glyph_t glyph;

	status = fontx_glyph(&glyph, code, font);

	if (0 != status)
	{
		return status;
	}

	/* Initialise bitmap dimensions. */
	bitmap->depth = DISPLAY_DEPTH, bitmap->width = glyph.width, bitmap->height = glyph.height, bitmap->pitch = bitmap->width * (bitmap->depth / 8);
	bitmap->size = bitmap->pitch * bitmap->height;

	color_t *ptr = (color_t*) bitmap->buffer;

	for (uint8_t y = 0; y < glyph.height; y++)
	{
		for (uint8_t x = 0; x < glyph.width; x++)
		{
			set = *(glyph.buffer) & (0x80 >> (x % 8));
			if (set)
			{
				*(ptr++) = color;
			}
			else
			{
				*(ptr++) = 0x0000;
			}
		}
		glyph.buffer += glyph.pitch;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
//// FIN DE GL
//////////////////////////////////////////////////////////////////////
