/*
 * Javier 2022
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <esp_log.h>

#include "fsTools.h"
#include "bitmap.h"
#include "clip.h"
#include "jpeg_decomp.h"
#include "gl.h"

static const char *TAG = "gl";

////////////////////////////////////////////////////////////////////////

#define ABS(x)  ((x) > 0 ? (x) : -(x))

#define GL_CHAR_BUFFER_SIZE    (16 * 16 * DISPLAY_DEPTH / 2)

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

static RECT clip_window = {
	.x0 = 0, .y0 = 0,
	.x1 = DISPLAY_WIDTH - 1, .y1 = DISPLAY_HEIGHT - 1,
	.w = DISPLAY_WIDTH, .h = DISPLAY_HEIGHT
};

#define JPG_WORKSPACE_SIZE 4000
static uint8_t workspace[JPG_WORKSPACE_SIZE] __attribute__((aligned(4)));

// font actual que se usa para la pantalla.
static terminal_t terminal_actual = {
	.x = 0, .y = 0, .bg = BLACK, .fg = WHITE, .isTransparent = 0,
	.rect.x0 = 0, .rect.y0 = 0, .rect.w = DISPLAY_WIDTH, .rect.h = DISPLAY_HEIGHT,
	.rect.x1 = DISPLAY_WIDTH - 1, .rect.y1 = DISPLAY_HEIGHT - 1
};

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

void gl_init()
{
	hagl_hal_init();
	gl_clear_screen();
}

void gl_flush()
{
#ifdef HAGL_HAS_HAL_FLUSH
	hagl_hal_flush();
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
		return;

	/* x0 or y0 is after the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (y0 > clip_window.y1))
		return;

	/* If still in bounds set the pixel. */
	hagl_hal_put_pixel(x0, y0, color);
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
static int jpg_memcpy(JDEC *decoder, void *buffer, JRECT *rectangle)
{
	tjpgd_iodev_t *device = (tjpgd_iodev_t*) decoder->device;
	color_t *bmp = (color_t*) buffer;
	int ancho = rectangle->right - rectangle->left + 1;
	for (int y = rectangle->top; y <= rectangle->bottom; y++)
		for (int x = 0; x < ancho; x++)
			device->image->pixels[y][x + rectangle->left] = *bmp++;
	return 1;
}

/*
 * Carga una imagen desde archivo al bmp.
 * TODO hacer una seleccion de tipo de decodes dependiendo de la extension del archivo
 * (por ahora es solo para jpg)
 * OJO, HACER FREE DEL BUFFER DEL BITMAP!!
 */
bitmap_t* gl_load_image(const char *filename)
{
	bitmap_t *bmp = NULL;
	JDEC decoder;
	JRESULT result;
	tjpgd_iodev_t device;
	device.x0 = 0;
	device.y0 = 0;
	device.fp = fs_open_file(filename, "rb");

	if (!str_ends_with(filename, ".jpg"))
	{
		ESP_LOGE(TAG, "ext file err");
		return NULL;
	}

	if (!device.fp)
	{
		ESP_LOGE(TAG, "file err");
		return NULL;
	}

	result = jd_prepare(&decoder, jpg_file_reader, workspace, JPG_WORKSPACE_SIZE, (void*) &device);
	if (result == JDR_OK)
	{
		//-1- creo el bitmap con sus tamaños y cosas de la imagen:
		bmp = bitmap_new(decoder.width, decoder.height, NULL);

		//-2- copio la imagen en el buffer:
		device.image = bmp;
		result = jd_decomp(&decoder, jpg_memcpy, 0);
		if (JDR_OK != result)
		{
			fclose(device.fp);
			bitmap_delete(bmp);
			return NULL;
		}
	}
	else
	{
		fclose(device.fp);
		return NULL;
	}
	fclose(device.fp);
	return bmp;
}

/*
 * Cuando se exportan de GIMP a .c las imagenes vienen en Big Endian.
 * Esto hace un swap de los bytes del buffer de la imagen,
 * y quedan asi dados vuelta para usar donde sea.
 *
 * (Accedo a los campos de la estructura del archivo GIMP mediante punteros)
 */
bitmap_t* gl_load_gimp_image(void *gimp_struct)
{
	unsigned int *pgs = gimp_struct;
	unsigned int width = *pgs++;
	unsigned int height = *pgs++;
	unsigned int bytes_per_pixel = *pgs;
	unsigned char *pixel_data = gimp_struct + sizeof(unsigned int) * 3;

	// flag para no volver a invertir los datos de la imagen.
	if (bytes_per_pixel == 0)
		return NULL;
	*pgs = 0;

	// voy a usar el buffer estatico del GIMP.
	return bitmap_new(width, height, pixel_data);
}

/*
 * Muestra una porcion de la imagen del bmp en la pos x0,y0 de la pantalla.
 * La porcion de la imagen está dada por el RECT.
 * Los bits = transparentColor no los pinta en el display.
 *
 * IMPORTANTE: NO SE PUEDE LEER LA RAM DEL DISPLAY, POR LO TANTO NO PUEDO HACER
 * TOTALMENTE TRANSPARENTE LA IMAGEN A MOSTRAR.
 * (O SEA, NO PUEDO HACER UN COLOR ENTRE EL BG Y EL FG)
 *
 * El RECT tiene que estar normalizado (x/y/ancho y alto)
 */
void gl_blit(uint16_t x0, uint16_t y0, bitmap_t *image, RECT *rect)
{
	hagl_hal_blit(x0, y0, image, rect);
}

//////////////////////////////////////////////////////////////////////
////////// COSAS DE SHAPES Y DIBUJOS
//////////////////////////////////////////////////////////////////////

void gl_draw_hline(int16_t x0, int16_t y0, uint16_t w, color_t color)
{
	int16_t width = w;

	/* x0 or y0 is over the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (y0 > clip_window.y1) || (y0 < clip_window.y0))
		return;

	/* x0 is left of clip window, ignore start part. */
	if (x0 < clip_window.x0)
	{
		width = width + x0;
		x0 = clip_window.x0;
	}

	/* Everything outside clip window, nothing to do. */
	if (width < 0)
		return;

	/* Cut anything going over right edge of clip window. */
	if (((x0 + width) > clip_window.x1))
		width = width - (x0 + width - clip_window.x1);

	hagl_hal_hline(x0, y0, width, color);
}

/*
 * Draw a vertical line with given color. If HAL supports it uses
 * hardware vline drawing. If not falls back to vanilla line drawing.
 */
void gl_draw_vline(int16_t x0, int16_t y0, uint16_t h, color_t color)
{
	int16_t height = h;

	/* x0 or y0 is over the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (x0 < clip_window.x0) || (y0 > clip_window.y1))
		return;

	/* y0 is top of clip window, ignore start part. */
	if (y0 < clip_window.y0)
	{
		height = height + y0;
		y0 = clip_window.y0;
	}

	/* Everything outside clip window, nothing to do. */
	if (height < 0)
		return;

	/* Cut anything going over right edge. */
	if (((y0 + height) > clip_window.y1))
		height = height - (y0 + height - clip_window.y1);

	hagl_hal_vline(x0, y0, height, color);
}

/*
 * Draw a line using Bresenham's algorithm with given color.
 */
void gl_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color)
{
	/* Clip coordinates to fit clip window. */
	if (false == clip_line(&x0, &y0, &x1, &y1, clip_window))
		return;

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
			break;

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
 * El RECT tiene que estar normalizado
 */
void gl_draw_rect(RECT *rect, color_t color)
{
	gl_draw_rectangle(rect->x0, rect->y0, rect->x1, rect->y1, color);
}

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
		return;

	/* x0 or y0 is after the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (y0 > clip_window.y1))
		return;

	uint16_t width = x1 - x0 + 1;
	uint16_t height = y1 - y0 + 1;

	gl_draw_hline(x0, y0, width, color);
	gl_draw_hline(x0, y1, width, color);
	gl_draw_vline(x0, y0, height, color);
	gl_draw_vline(x1, y0, height, color);
}

/*
 * Draw a filled rectangle with given color.
 * El RECT tiene que estar normalizado
 */
void gl_fill_rect(RECT *rect, color_t color)
{
	gl_fill_rectangle(rect->x0, rect->y0, rect->x1, rect->y1, color);
}

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
		return;

	/* x0 or y0 is after the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (y0 > clip_window.y1))
		return;

	x0 = max(x0, clip_window.x0);
	y0 = max(y0, clip_window.y0);
	x1 = min(x1, clip_window.x1);
	y1 = min(y1, clip_window.y1);

	uint16_t width = x1 - x0 + 1;
	uint16_t height = y1 - y0 + 1;

	for (uint16_t i = 0; i < height; i++)
	{
		/* Already clipped so can call HAL directly. */
		hagl_hal_hline(x0, y0 + i, width, color);
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
		gl_draw_line(vertices[(i << 1) + 0], vertices[(i << 1) + 1], vertices[(i << 1) + 2], vertices[(i << 1) + 3], color);

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

// el RECT tiene que estar normalizado.
void gl_draw_rounded_rect(RECT *rect, int16_t r, color_t color)
{
	gl_draw_rounded_rectangle(rect->x0, rect->y0, rect->x1, rect->y1, r, color);
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
		return;

	/* x0 or y0 is after the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (y0 > clip_window.y1))
		return;

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

// el RECT tiene que estar normalizado.
void gl_fill_rounded_rect(RECT *rect, int16_t r, color_t color)
{
	gl_fill_rounded_rectangle(rect->x0, rect->y0, rect->x1, rect->y1, r, color);
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
		return;

	/* x0 or y0 is after the edge, nothing to do. */
	if ((x0 > clip_window.x1) || (y0 > clip_window.y1))
		return;

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

void gl_set_font(const uint8_t *fx)
{
	terminal_actual.fx = fontx_meta(&(terminal_actual.meta), fx);
}

void gl_set_font_color(uint16_t fg)
{
	terminal_actual.fg = fg;
}

void gl_set_font_colors(uint16_t fg, uint16_t bg)
{
	terminal_actual.fg = fg;
	terminal_actual.bg = bg;
}

void gl_set_transparent()
{
	terminal_actual.isTransparent = 1;
}

void gl_clear_transparent()
{
	terminal_actual.isTransparent = 0;
}

void gl_set_font_pos(uint16_t x, uint16_t y)
{
	terminal_actual.x = clip_window.x0 + x;
	terminal_actual.y = clip_window.y0 + y;
}

// mete en el bitmap la letra 'code' de la font 'font'
// el bitmap tiene que tener espacio para almacenar la font (del tamaño fontY*X*2)
static bitmap_t* gl_get_glyph(terminal_t *term, char code)
{
	fontx_glyph_t glyph;
	if (0 != fontx_glyph(&glyph, (wchar_t) code, term->fx))
		return NULL; // no existe ese caracter!

	char buf[glyph.width * glyph.height * 2];
	bitmap_t *bitmap = bitmap_new(glyph.width, glyph.height, buf);
	if (term->isTransparent)
		bitmap->transparentColor = term->bg;

	uint8_t isForeGround;
	for (uint8_t y = 0; y < glyph.height; y++)
	{
		for (uint8_t x = 0; x < glyph.width; x++)
		{
			isForeGround = *(glyph.buffer + x / 8) & (0x80 >> (x % 8));
			if (isForeGround)
				bitmap->pixels[y][x] = term->fg;
			else
				bitmap->pixels[y][x] = term->bg;
		}
		glyph.buffer += glyph.pitch;
	}
	return bitmap;
}

// solo imprimo dentro del rect definido en la terminal.
// si el texto quiere irse debajo de la linea bottom del window,
// se hará un scroll del rectangulo.
static void gl_put_text(terminal_t *term, const char *str)
{
	char code;
	while (*str != 0)
	{
		code = *str++;

		// los ENTERS modifican la posicion,
		// y verifico que no se imprima fuera de la ventana horizontalmente:
		if (13 == code || 10 == code || term->x + term->meta.width > term->rect.w)
		{
			term->x = 0;
			term->y += term->meta.height;
		}

		// verifico fuera de ventana verticalmente:
		if (term->y + term->meta.height > term->rect.h)
		{
			hagl_hal_scroll(SCROLL_UP, &(term->rect), term->meta.height, term->bg);
			term->x = 0;
			term->y -= term->meta.height;
		}

		// busco el bitmap de la letra a mostrar:
		if (13 != code && 10 != code)
		{
			bitmap_t *bmp = gl_get_glyph(term, code);
			if (bmp)
			{
				gl_blit(term->rect.x0 + term->x, term->rect.y0 + term->y, bmp, NULL);
				term->x += term->meta.width;
				bitmap_delete(bmp);
			}
		}
	}
}

// imprime en pantalla, pero dentro de la ventana clip_window
void gl_print(const char *text)
{
	gl_put_text(&terminal_actual, text);
}

int gl_printf(const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	char temp[200];
	int len = vsnprintf(temp, sizeof(temp) - 1, format, arg);
	temp[sizeof(temp) - 1] = 0;
	gl_put_text(&terminal_actual, temp);
	va_end(arg);
	return len;
}

// se pueden generar terminales diferentes a la por defecto.
void gl_terminal_print(terminal_t *term, char *text)
{
	gl_put_text(term, text);
}

// Podés dividir la pantalla en una zona de texto tipo terminal, donde las letras fuera de los
// márgenes hacen que se acomode el texto.
// Importante: hay que eliminar la terminal con terminal_delete
terminal_t* gl_terminal_new(int x0, int y0, int width, int height, const uint8_t *fx, color_t fg, color_t bg)
{
	terminal_t *terminal = malloc(sizeof(terminal_t));

	// validaciones (porque se va a la mierda!)
	if (x0 + width > DISPLAY_WIDTH)
	{
		width = DISPLAY_WIDTH - x0;
		ESP_LOGW(TAG, "TERMINAL WIDTH > DISPLAY");
	}
	if (y0 + height > DISPLAY_HEIGHT)
	{
		ESP_LOGW(TAG, "TERMINAL HEIGHT > DISPLAY");
		height = DISPLAY_HEIGHT - y0;
	}

	// guardo cosas de la font, ventana, etc:
	fontx_meta(&(terminal->meta), fx);
	terminal->rect.x0 = x0;
	terminal->rect.y0 = y0;
	terminal->rect.w = width;
	terminal->rect.h = height;
	terminal->rect.x1 = x0 + width - 1;
	terminal->rect.y1 = y0 + height - 1;
	terminal->fx = (uint8_t*) fx;
	terminal->fg = fg;
	terminal->bg = bg;
	terminal->x = 0;
	terminal->y = 0;

	// borro la pantalla que ocupa la terminal:
	gl_fill_rect(&(terminal->rect), bg);

	return terminal;
}

void gl_terminal_delete(terminal_t *terminal)
{
	if (terminal)
		free(terminal);
}

//////////////////////////////////////////////////////////////////////
//// FIN DE GL
//////////////////////////////////////////////////////////////////////
