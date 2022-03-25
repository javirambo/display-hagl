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

#ifndef _gl_H
#define _gl_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>

#include "hal/hagl_hal.h"
#include "colors.h"
#include "rect.h"
#include "bitmap.h"
#include "rect.h"
#include "fontx.h"

#define GL_OK                  (0)
#define GL_ERR_GENERAL         (1)
#define GL_ERR_FILE_IO         (2)
#define GL_ERR_TJPGD           (100)

//////////////////////////////////////////////////////////////////////
////////// FUNCIONES GENERALES
//////////////////////////////////////////////////////////////////////

/*
 * Inicializa el display (el driver SPI) y envía settings al display.
 * Si tiene doble o triple buffers, los crea.
 */
bitmap_t* gl_init();

/*
 * Solo se usa con doble o triple buffer.
 * Escribe los buffers en el display.
 * Debe llamarse desde una tarea cada N frames por segundo.
 */
void gl_flush();

/*
 * (Aparentemente no lo usa nadie....ni está implementado....)
 */
void gl_close();

/**
 * Convert RGB to color
 *
 * Returns color type  defined by the HAL. Most often it is an
 * uint16_t RGB565 color.
 *
 * @return color
 */
color_t gl_color(uint8_t r, uint8_t g, uint8_t b);

/**
 * Set the clip window
 *
 * Clip windows restricts the drawable area. It does not affect
 * the coordinates.
 *
 * @param x0
 * @param y0
 * @param x1
 * @param y1
 */
void gl_set_clip_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * Clear area of the current clip window
 */
void gl_clear_clip_window();
void gl_fill_clip_window(color_t color);

/**
 * Clear the display
 */
void gl_clear_screen();
void gl_fill_screen(color_t color);

/**
 * Put a single pixel
 *
 * Output will be clipped to the current clip window.
 * @param x0
 * @param y0
 * @param color
 */
void gl_put_pixel(int16_t x0, int16_t y0, color_t color);

/**
 * Get a single pixel
 *
 * Input will be clipped to the current clip window. In case of
 * error or if HAL does not support this feature returns black.
 * @param x0
 * @param y0
 * @return color at the given location
 */
color_t gl_get_pixel(int16_t x0, int16_t y0);

//////////////////////////////////////////////////////////////////////
////////// FUNCIONES PARA IMAGENES
//////////////////////////////////////////////////////////////////////

void gl_blit(int16_t x0, int16_t y0, bitmap_t *bitmap);

uint32_t gl_get_jpeg_size(uint16_t *w, uint16_t *h, const uint8_t jpeg_data[], uint32_t data_size);

uint32_t gl_show_jpeg_data(int16_t x, int16_t y, const uint8_t jpeg_data[], uint32_t data_size);

uint32_t gl_show_image_file(int16_t x0, int16_t y0, const char *filename);

uint32_t gl_show_partial_image(uint16_t x0, uint16_t y0, bitmap_t *bitmap, RECT *img_rect, uint16_t transparentColor);

uint32_t gl_load_gimp_image(void *gimp_img, bitmap_t *bmp);

#if 0
// ESTO NO FUNCIONA PORQUE NO SE PUEDE LEER LA RAM DEL DISPLAY
uint32_t save_screen(RECT *rect, bitmap_t *target);
#endif

// OJO HACER FREE AL BMP!
uint32_t gl_load_image(const char *filename, bitmap_t *bitmap);

void gl_free_bitmap_buffer(bitmap_t *bitmap);
void gl_free_bitmap(bitmap_t *bitmap);

//////////////////////////////////////////////////////////////////////
////////// FUNCIONES PARA DIBUJOS
//////////////////////////////////////////////////////////////////////

/**
 * Draw a line
 *
 * Output will be clipped to the current clip window.
 *
 * @param x0
 * @param y0
 * @param x1
 * @param y1
 * @param color
 */
void gl_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color);

/**
 * Draw a vertical line
 *
 * Output will be clipped to the current clip window.
 *
 * @param x0
 * @param y0
 * @param width
 * @param color
 */
void gl_draw_hline(int16_t x0, int16_t y0, uint16_t width, color_t color);

/**
 * Draw a horizontal line
 *
 * Output will be clipped to the current clip window.
 *
 * @param x0
 * @param y0
 * @param height
 * @param color
 */
void gl_draw_vline(int16_t x0, int16_t y0, uint16_t height, color_t color);

/**
 * Draw a rectangle
 *
 * Output will be clipped to the current clip window.
 *
 * @param x0
 * @param y0
 * @param x1
 * @param y1
 * @param color
 */
void gl_draw_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color);

/**
 * Draw a filled rectangle
 *
 * Output will be clipped to the current clip window.
 *
 * @param x0
 * @param y0
 * @param x1
 * @param y1
 * @param color
 */
void gl_fill_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color);

/**
 * Draw a circle
 *
 * Output will be clipped to the current clip window.
 *
 * @param x0 center X
 * @param y0 center Y
 * @param r radius
 * @param color
 */
void gl_draw_circle(int16_t x0, int16_t y0, int16_t r, color_t color);

/**
 * Draw a filled circle
 *
 * Output will be clipped to the current clip window.
 *
 * @param x0 center X
 * @param y0 center Y
 * @param r radius
 * @param color
 */
void gl_fill_circle(int16_t x0, int16_t y0, int16_t r, color_t color);

/**
 * Draw an ellipse
 *
 * Output will be clipped to the current clip window.
 *
 * @param x0 center X
 * @param y0 center Y
 * @param a vertical radius
 * @param b horizontal radius
 * @param color
 */
void gl_draw_ellipse(int16_t x0, int16_t y0, int16_t a, int16_t b, color_t color);

/**
 * Draw a filled ellipse
 *
 * Output will be clipped to the current clip window.
 *
 * @param x0 center X
 * @param y0 center Y
 * @param a vertical radius
 * @param b horizontal radius
 * @param color
 */
void gl_fill_ellipse(int16_t x0, int16_t y0, int16_t a, int16_t b, color_t color);

/**
 * Draw a polygon
 *
 * Output will be clipped to the current clip window. Polygon does
 * not need to be convex. They can also be concave or complex.
 *
 * color_t color = gl_color(0, 255, 0);
 * int16_t vertices[10] = {x0, y0, x1, y1, x2, y2, x3, y3, x4, y4};
 * gl_draw_polygon(5, vertices, color);
 *
 * @param amount number of vertices
 * @param vertices pointer to (an array) of vertices
 * @param color
 */
void gl_draw_polygon(int16_t amount, int16_t *vertices, color_t color);

/**
 * Draw a filled polygon
 *
 * Output will be clipped to the current clip window. Polygon does
 * not need to be convex. They can also be concave or complex.
 *
 * color_t color = gl_color(0, 255, 0);
 * int16_t vertices[10] = {x0, y0, x1, y1, x2, y2, x3, y3, x4, y4};
 * gl_draw_polygon(5, vertices, color);
 *
 * @param amount number of vertices
 * @param vertices pointer to (an array) of vertices
 * @param color
 */
void gl_fill_polygon(int16_t amount, int16_t *vertices, color_t color);

/**
 * Draw a triangle
 *
 * Output will be clipped to the current clip window. Internally
 * uses gl_draw_polygon() to draw the triangle.
 *
 * @param x0
 * @param y0
 * @param x1
 * @param y1
 * @param x2
 * @param y3
 * @param color
 */
void gl_draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, color_t color);

/**
 * Draw a filled triangle
 *
 * Output will be clipped to the current clip window. Internally
 * uses gl_fill_polygon() to draw the triangle.
 *
 * @param x0
 * @param y0
 * @param x1
 * @param y1
 * @param x2
 * @param y3
 * @param color
 */
void gl_fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, color_t color);

/**
 * Draw a rounded rectangle
 *
 * Output will be clipped to the current clip window.
 *
 * @param x0
 * @param y0
 * @param x0
 * @param y0
 * @param r corner radius
 * @param color
 */
void gl_draw_rounded_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t r, color_t color);

/**
 * Draw a filled rounded rectangle
 *
 * Output will be clipped to the current clip window.
 *
 * @param x0
 * @param y0
 * @param x1
 * @param y1
 * @param r corner radius
 * @param color
 */
void gl_fill_rounded_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t r, color_t color);

//////////////////////////////////////////////////////////////////////
/////// FUNCIONES PARA TEXTO
//////////////////////////////////////////////////////////////////////

typedef struct
{
	uint16_t x; 	// posicion actual de la font
	uint16_t y; 	//  "
	color_t bg; 	// color de fondo
	color_t fg; 	// color de la letra
	uint8_t *fx;	// font seleccionada
} font_screen_t;

typedef struct
{
	//RECT *rect;
	bitmap_t *bmp;
	font_screen_t font;
	int line_nbr;  	// numero de linea que se esta imprimiendo, si se pasa del max hace scroll.
	int line_max; 	// lineas verticales maximas.
	int line_h; 	// alto de la linea (cant de pixels a escrolar)
} terminal_t;

void gl_set_font(const uint8_t *font);
void gl_set_font_color(color_t fg, color_t bg);
void gl_set_font_pos(uint16_t x, uint16_t y);
int gl_print(const char *str);

uint8_t gl_get_glyph(char code, color_t fg, color_t bg, bitmap_t *bitmap, uint8_t *fx);

void gl_init_terminal(int x, int y, int w, int h, const uint8_t *fx, color_t fg, color_t bg, terminal_t *term);
int gl_terminal_print(terminal_t *term, char *text);
void gl_destroy_terminal(terminal_t *term);

#ifdef __cplusplus
}
#endif

#endif /* _gl_H */
