/*

 MIT No Attribution

 Copyright (c) 2018-2020 Mika Tuupola

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 -cut-

 SPDX-License-Identifier: MIT-0

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_task_wdt.h>

#include "sdkconfig.h"
#include "hagl_hal.h"
#include "bitmap.h"
#include "hagl.h"
#include "font6x9.h"
#include "font5x7.h"
#include "font5x8.h"
#include "font10x20_ISO8859_1.h"
#include "font10x20-ISO8859-7.h"
#include "font8x13O-ISO8859-13.h"
#include "font9x18-ISO8859-13.h"
#include "font9x15B-ISO8859-13.h"
#include "fps.h"
#include "aps.h"
#include "pig_resources.h"

static const char *TAG = "main";
static char primitive[18][32] =
		{
			"RGB BARS",
			"PIXELS",
			"LINES",
			"CIRCLES",
			"FILLED CIRCLES",
			"ELLIPSES",
			"FILLED ELLIPSES",
			"TRIANGLES",
			"FILLED TRIANGLES",
			"RECTANGLES",
			"FILLED RECTANGLES",
			"ROUND RECTANGLES",
			"FILLED ROUND RECTANGLES",
			"POLYGONS",
			"FILLED POLYGONS",
			"CHARACTERS",
			"STRINGS",
			"JPEG" };

static SemaphoreHandle_t mutex;
static float fb_fps;
static float fx_fps;
static uint16_t current_demo = 0;
static bitmap_t *bb;
static uint32_t drawn = 0;
/*
 * Flushes the framebuffer to display in a loop. This demo is
 * capped to 30 fps.
 */
void framebuffer_task(void *params)
{
	TickType_t last;
	const TickType_t frequency = 1000 / 30 / portTICK_RATE_MS;

	last = xTaskGetTickCount();

	while (1)
	{
		xSemaphoreTake(mutex, portMAX_DELAY);
		hagl_flush();
		xSemaphoreGive(mutex);
		fb_fps = fps();
		vTaskDelayUntil(&last, frequency);
	}

	vTaskDelete(NULL);
}

/*
 * Displays the info bar on top of the screen.
 */
void fps_task(void *params)
{
	uint16_t color = hagl_color(0, 255, 0);
	wchar_t message[128];

#ifdef HAGL_HAL_USE_BUFFERING
    while (1) {
        fx_fps = aps(drawn);
        drawn = 0;

        hagl_set_clip_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);

        swprintf(message, sizeof(message), L"%.*f %s PER SECOND       ", 0, fx_fps, primitive[current_demo]);
        hagl_put_text(message, 6, 4, color, font6x9);
        swprintf(message, sizeof(message), L"%.*f FPS  ", 1, fb_fps);
        hagl_put_text(message, DISPLAY_WIDTH - 56, DISPLAY_HEIGHT - 14, color, font6x9);

        hagl_set_clip_window(0, 20, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 21);

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
#else
	while (1)
	{
		fx_fps = aps(drawn);
		drawn = 0;

		swprintf(message, sizeof(message), L"%.*f %s PER SECOND       ", 0, fx_fps, primitive[current_demo]);
		hagl_set_clip_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
		hagl_put_text(message, 8, 4, color, font6x9);
		hagl_set_clip_window(0, 20, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 21);

		vTaskDelay(2000 / portTICK_RATE_MS);
	}
#endif
	vTaskDelete(NULL);
}

void switch_task(void *params)
{
	while (1)
	{
		ESP_LOGI(TAG, "%.*f %s per second, FB %.*f FPS", 0, fx_fps, primitive[current_demo], 1, fb_fps);

		//current_demo = (current_demo + 1) % 17;
		if (current_demo == 15)
			current_demo = 16;
		else if (current_demo == 16)
			current_demo = 17;
		else
			current_demo = 15;

		hagl_clear_clip_window();
		aps(APS_RESET);
		drawn = 0;

		vTaskDelay(5000 / portTICK_RATE_MS);
	}

	vTaskDelete(NULL);
}

void polygon_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x1 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y1 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x2 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y2 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x3 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y3 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x4 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y4 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	color_t colour = rand() % 0xffff;
	int16_t vertices[10] =
			{ x0, y0, x1, y1, x2, y2, x3, y3, x4, y4 };
	hagl_draw_polygon(5, vertices, colour);
}

void fill_polygon_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x1 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y1 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x2 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y2 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x3 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y3 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x4 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y4 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	color_t colour = rand() % 0xffff;
	int16_t vertices[10] =
			{ x0, y0, x1, y1, x2, y2, x3, y3, x4, y4 };
	hagl_fill_polygon(5, vertices, colour);
}

void circle_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	uint16_t r = (rand() % 40);
	color_t colour = rand() % 0xffff;
	hagl_draw_circle(x0, y0, r, colour);
}

void fill_circle_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	uint16_t r = (rand() % 40);
	color_t colour = rand() % 0xffff;
	hagl_fill_circle(x0, y0, r, colour);
}

void ellipse_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	uint16_t a = (rand() % 40) + 20;
	uint16_t b = (rand() % 40) + 20;
	color_t colour = rand() % 0xffff;
	hagl_draw_ellipse(x0, y0, a, b, colour);
}

void fill_ellipse_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	uint16_t a = (rand() % 40) + 20;
	uint16_t b = (rand() % 40) + 20;
	color_t colour = rand() % 0xffff;
	hagl_fill_ellipse(x0, y0, a, b, colour);
}

void line_demo()
{
	// strcpy(primitive, "LINES");

	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x1 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y1 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	color_t colour = rand() % 0xffff;
	hagl_draw_line(x0, y0, x1, y1, colour);
}

void rectangle_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x1 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y1 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	color_t colour = rand() % 0xffff;
	hagl_draw_rectangle(x0, y0, x1, y1, colour);
}

void fill_rectangle_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x1 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y1 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	color_t colour = rand() % 0xffff;
	hagl_fill_rectangle(x0, y0, x1, y1, colour);
}

void put_character_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;

	color_t colour = rand() % 0xffff;
	char ascii = rand() % 127;
	hagl_put_char(ascii, x0, y0, colour, font6x9);
}

void put_text_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 80;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;

	color_t colour = rand() % 0xffff;

	hagl_put_text(u"YO¡ MTV raps ♥", x0, y0, colour, font6x9);
}

void put_pixel_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	color_t colour = rand() % 0xffff;
	hagl_put_pixel(x0, y0, colour);
}

void triangle_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x1 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y1 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x2 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y2 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	color_t colour = rand() % 0xffff;
	hagl_draw_triangle(x0, y0, x1, y1, x2, y2, colour);
}

void fill_triangle_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x1 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y1 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x2 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y2 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	color_t colour = rand() % 0xffff;
	hagl_fill_triangle(x0, y0, x1, y1, x2, y2, colour);
}

void rgb_demo()
{
	uint16_t red = hagl_color(255, 0, 0);
	uint16_t green = hagl_color(0, 255, 0);
	uint16_t blue = hagl_color(0, 0, 255);

	int16_t x0 = 0;
	int16_t x1 = DISPLAY_WIDTH / 3;
	int16_t x2 = 2 * x1;

	hagl_fill_rectangle(x0, 0, x1 - 1, DISPLAY_HEIGHT, red);
	hagl_fill_rectangle(x1, 0, x2 - 1, DISPLAY_HEIGHT, green);
	hagl_fill_rectangle(x2, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, blue);
}

void round_rectangle_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x1 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y1 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t r = rand() % 10;
	color_t colour = rand() % 0xffff;
	hagl_draw_rounded_rectangle(x0, y0, x1, y1, r, colour);
}

void fill_round_rectangle_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t x1 = (rand() % DISPLAY_WIDTH + 20) - 20;
	int16_t y1 = (rand() % DISPLAY_HEIGHT + 20) - 20;
	int16_t r = rand() % 10;
	color_t colour = rand() % 0xffff;
	hagl_fill_rounded_rectangle(x0, y0, x1, y1, r, colour);
}

void fonts_demo()
{
	int i = 0;
	hagl_put_text(u"YO¡ MTV raps ♥", 0, i+=20, 0xffff, font5x8);
	hagl_put_text(u"YO¡ MTV raps ♥", 0, i+=20, 0xffff, font10x20_ISO8859_1);
	hagl_put_text(u"YO¡ MTV raps ♥", 0, i+=20, 0xffff, font10x20_ISO8859_7);
	hagl_put_text(u"YO¡ MTV raps ♥", 0, i+=20, 0xffff, font8x13O_ISO8859_13);
	hagl_put_text(u"YO¡ MTV raps ♥", 0, i+=20, 0xffff, font9x15B_ISO8859_13);
	hagl_put_text(u"YO¡ MTV raps ♥", 0, i+=20, 0xffff, font9x18_ISO8859_13);
}

// '0', 32x30px
const uint16_t digits0 []  = {
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xf79e, 0xe71c, 0xd6ba, 0xd69a,
	0xd69a, 0xdedb, 0xe73c, 0xf7be, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xf7be, 0xce59, 0x738e, 0x39e7, 0x2945, 0x10a2, 0x0861,
	0x0861, 0x2104, 0x3186, 0x4a49, 0x94b2, 0xe71c, 0xffdf, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xc638, 0x630c, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x18e3, 0x8430, 0xe71c, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xf79e, 0x94b2, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2124, 0xc618, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xf79e, 0x738e, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x10a2, 0xa534, 0xffdf, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xf79e, 0x738e, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0861, 0xad75, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0x94b2, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6b4d, 0xa514, 0xc638, 0xd6ba,
	0xd69a, 0xbdf7, 0x9492, 0x4a49, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x18e3, 0xce79, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xc638, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4228, 0xc638, 0xffdf, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xf79e, 0x9cd3, 0x2104, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4228, 0xf79e, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xf7be, 0x630c, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x630c, 0xe73c, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xc618, 0x10a2, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xa514, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xce79, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4228, 0xe73c, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xbdf7, 0x18e3, 0x0000, 0x0000, 0x0000, 0x0000, 0x4a69, 0xf7be, 0xffff, 0xffff,
	0xffff, 0xffff, 0x73ae, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc638, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffdf, 0x7bcf, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xce79, 0xffff, 0xffff,
	0xffff, 0xf79e, 0x4208, 0x0000, 0x0000, 0x0000, 0x0000, 0x6b4d, 0xffdf, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xe71c, 0x0861, 0x0000, 0x0000, 0x0000, 0x0000, 0x8c71, 0xffff, 0xffff,
	0xffff, 0xe71c, 0x2945, 0x0000, 0x0000, 0x0000, 0x0000, 0xa534, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffdf, 0x6b4d, 0x0000, 0x0000, 0x0000, 0x0000, 0x4a69, 0xffdf, 0xffff,
	0xffff, 0xd6ba, 0x18e3, 0x0000, 0x0000, 0x0000, 0x0000, 0xce59, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x8c71, 0x0000, 0x0000, 0x0000, 0x0000, 0x4208, 0xf79e, 0xffff,
	0xffff, 0xd69a, 0x0861, 0x0000, 0x0000, 0x0000, 0x0000, 0xd6ba, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x9cd3, 0x0000, 0x0000, 0x0000, 0x0000, 0x39c7, 0xef7d, 0xffff,
	0xffff, 0xd69a, 0x0861, 0x0000, 0x0000, 0x0000, 0x0000, 0xd6ba, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x9cd3, 0x0000, 0x0000, 0x0000, 0x0000, 0x39e7, 0xf79e, 0xffff,
	0xffff, 0xdedb, 0x18e3, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x8430, 0x0000, 0x0000, 0x0000, 0x0000, 0x4208, 0xf7be, 0xffff,
	0xffff, 0xe73c, 0x3186, 0x0000, 0x0000, 0x0000, 0x0000, 0x94b2, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xf7be, 0x52aa, 0x0000, 0x0000, 0x0000, 0x0000, 0x5aeb, 0xffff, 0xffff,
	0xffff, 0xf7be, 0x4a49, 0x0000, 0x0000, 0x0000, 0x0000, 0x528a, 0xf79e, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xce59, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xa514, 0xffff, 0xffff,
	0xffff, 0xffff, 0x9492, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xa514, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xf79e, 0x5acb, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xdefb, 0xffff, 0xffff,
	0xffff, 0xffff, 0xdefb, 0x10a2, 0x0000, 0x0000, 0x0000, 0x0000, 0x2124, 0xce59, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x94b2, 0x0861, 0x0000, 0x0000, 0x0000, 0x0000, 0x632c, 0xffdf, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffdf, 0x8410, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2124, 0xc638, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xf79e, 0x94b2, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xbdd7, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xdefb, 0x18e3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2104, 0x8c51, 0xef5d, 0xffdf, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffdf, 0xd69a, 0x5aeb, 0x0861, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x738e, 0xffdf, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xbdd7, 0x10a2, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2124, 0x738e, 0x94b2, 0xa514,
	0x9cf3, 0x8c51, 0x5aeb, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x39c7, 0xe71c, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffdf, 0x9cf3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x39c7, 0xd69a, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffdf, 0x9cf3, 0x18e3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x39c7, 0xd69a, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffdf, 0xc638, 0x3186, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6b6d, 0xe71c, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xef5d, 0x94b2, 0x39e7, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x5aeb, 0xb5b6, 0xffdf, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xef7d, 0xbdf7, 0x73ae, 0x4a49, 0x39e7, 0x39c7,
	0x39c7, 0x4208, 0x528a, 0x9492, 0xd6ba, 0xf7be, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xf7be, 0xef7d, 0xef5d,
	0xef7d, 0xf79e, 0xffdf, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff
};

// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 976)
const int digitsallArray_LEN = 1;
const uint16_t* digitsallArray[1] = {
	digits0
};

void font_bmp_demo()
{
	hagl_fill_rectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0xffff);
	//hagl_draw_jpg(20, 20, font_squid_30_jpg, RES_SIZE(font_squid_30_jpg));

	bitmap_t block = {
			.width = 32,
			.height = 30,
			.depth = DISPLAY_DEPTH,
			.pitch = 32 * (DISPLAY_DEPTH / 8),
			.size = 32 * (DISPLAY_DEPTH / 8) * 30,
			.buffer = (uint8_t*) digits0 };

	hagl_blit(0, 100, &block);

	vTaskDelay(2000 / portTICK_RATE_MS);
}

void demo_task(void *params)
{
	void (*demo[18])();

	demo[0] = font_bmp_demo;
	//demo[0] = rgb_demo;
	demo[1] = put_pixel_demo;
	demo[2] = line_demo;
	demo[3] = circle_demo;
	demo[4] = fill_circle_demo;
	demo[5] = ellipse_demo;
	demo[6] = fill_ellipse_demo;
	demo[7] = triangle_demo;
	demo[8] = fill_triangle_demo;
	demo[9] = rectangle_demo;
	demo[10] = fill_rectangle_demo;
	demo[11] = round_rectangle_demo;
	demo[12] = fill_round_rectangle_demo;
	demo[13] = polygon_demo;
	demo[14] = fill_polygon_demo;
	demo[15] = put_character_demo;
	//demo[16] = jpeg_demo1;
	demo[16] = put_text_demo;
	demo[17] = fonts_demo;

	//int i = 0;
	while (1)
	{
		//current_demo = 0; // OJO ACA, BLOQUEO ESTE TEST!
		(*demo[current_demo])();
		drawn++;
	}

	vTaskDelete(NULL);
}

void app_main()
{
	ESP_LOGI(TAG, "SDK version: %s", esp_get_idf_version());
	ESP_LOGI(TAG, "Heap when starting: %d", esp_get_free_heap_size());

	bb = hagl_init();
	if (bb)
	{
		ESP_LOGI(TAG, "Back buffer: %dx%dx%d", bb->width, bb->height, bb->depth);
	}

	ESP_LOGI(TAG, "Heap after HAGL init: %d", esp_get_free_heap_size());

	hagl_set_clip_window(0, 20, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 21);
	mutex = xSemaphoreCreateMutex();

	if (NULL != mutex)
	{
#ifdef HAGL_HAL_USE_BUFFERING
        xTaskCreatePinnedToCore(framebuffer_task, "Framebuffer", 8192, NULL, 1, NULL, 0);
#endif
#ifdef CONFIG_IDF_TARGET_ESP32S2
        /* ESP32-S2 has only one core, run everthing in core 0. */
        xTaskCreatePinnedToCore(fps_task, "FPS", 8192, NULL, 2, NULL, 0);
        xTaskCreatePinnedToCore(demo_task, "Demo", 8192, NULL, 1, NULL, 0);
        xTaskCreatePinnedToCore(switch_task, "Switch", 2048, NULL, 2, NULL, 0);
#else
		/* ESP32 has two cores, run demo stuff in core 1. */
		xTaskCreatePinnedToCore(fps_task, "FPS", 8192, NULL, 2, NULL, 1);
		xTaskCreatePinnedToCore(demo_task, "Demo", 8192, NULL, 1, NULL, 1);
		xTaskCreatePinnedToCore(switch_task, "Switch", 2048, NULL, 2, NULL, 1);
#endif /* CONFIG_IDF_TARGET_ESP32S2 */
	}
	else
	{
		ESP_LOGE(TAG, "No mutex?");
	}
}
