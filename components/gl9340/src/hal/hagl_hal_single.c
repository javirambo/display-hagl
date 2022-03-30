/*

 MIT License

 Copyright (c) 2019-2020 Mika Tuupola

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

 This file is part of the MIPI DCS HAL for HAGL graphics library:
 https://github.com/tuupola/hagl_esp_mipi/

 SPDX-License-Identifier: MIT

 -cut-

 This is the HAL used when buffering is disabled. I call this single buffered
 since I consider the GRAM of the display driver chip to be the framebuffer.

 Note that all coordinates are already clipped in the main library itself.
 HAL does not need to validate the coordinates, they can alway be assumed
 valid.

 */

#include "sdkconfig.h"
#include "hal/hagl_hal.h"

#ifdef CONFIG_HAGL_HAL_NO_BUFFERING

#include "hal/mipi_display.h"
#include <freertos/semphr.h>

static spi_device_handle_t spi;

void hagl_hal_init()
{
	mipi_display_init(&spi);
}

void hagl_hal_put_pixel(int16_t x0, int16_t y0, color_t color)
{
	mipi_display_write(spi, x0, y0, 1, 1, (uint8_t*) &color);
}

color_t hagl_hal_get_pixel(int16_t x0, int16_t y0)
{
	color_t color;
	mipi_get_pixel_data(spi, x0, y0, 1, 1, (uint8_t*) &color);
	return color;
}

void hagl_hal_blit(uint16_t x0, uint16_t y0, bitmap_t *src)
{
	for (int y = 0; y < src->height; y++)
		mipi_display_write(spi, x0, y0 + y, src->width, 1, (uint8_t*) src->pixels[y]);
}

void hagl_hal_hline(int16_t x0, int16_t y0, uint16_t width, color_t color)
{
	static color_t line[DISPLAY_WIDTH];
	color_t *ptr = line;
	for (uint16_t x = 0; x < width; x++)
		*(ptr++) = color;
	mipi_display_write(spi, x0, y0, width, 1, (uint8_t*) line);
}

void hagl_hal_vline(int16_t x0, int16_t y0, uint16_t height, color_t color)
{
	static color_t line[DISPLAY_HEIGHT];
	color_t *ptr = line;
	for (uint16_t x = 0; x < height; x++)
		*(ptr++) = color;
	mipi_display_write(spi, x0, y0, 1, height, (uint8_t*) line);
}

void hagl_hal_get_pixel_data(RECT *rect, uint8_t *destination_buffer)
{
	mipi_get_pixel_data(spi, rect->x0, rect->y0, rect->w, rect->h, destination_buffer);
}

/*
 * Agrego una forma de enviar comandos al display.
 * (tambien puedo leer algunos registros y retornan en data)
 * Javier.
 */
void hagl_hal_control(uint8_t cmd, uint8_t *data, int data_len)
{
	return mipi_display_ioctl(spi, cmd, data, data_len);
}

#endif /* CONFIG_HAGL_HAL_NO_BUFFERING */
