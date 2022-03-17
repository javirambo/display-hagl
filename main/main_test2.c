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

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_task_wdt.h>

#include "../components/gl9340/include/aps.h"
#include "../components/gl9340/include/fonts/font10x20-ISO8859-7.h"
#include "../components/gl9340/include/fonts/font10x20_ISO8859_1.h"
#include "../components/gl9340/include/fonts/font5x7.h"
#include "../components/gl9340/include/fonts/font5x8.h"
#include "../components/gl9340/include/fonts/font6x9.h"
#include "../components/gl9340/include/fonts/font8x13O-ISO8859-13.h"
#include "../components/gl9340/include/fonts/font9x15B-ISO8859-13.h"
#include "../components/gl9340/include/fonts/font9x18-ISO8859-13.h"
#include "../components/gl9340/include/fps.h"
#include "hagl.h"
#include "fsTools.h" // (ahora hagl abre imagenes con la fsTools)

static const char *TAG = "main";
static char primitive[18][32] =
		{
			"CHANCHAS",
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
	char message[128];

#ifdef HAGL_HAL_USE_BUFFERING
    while (1) {
        fx_fps = aps(drawn);
        drawn = 0;

        hagl_set_clip_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);

        sprintf(message, "%.3f %s PER SECOND       ", fx_fps, primitive[current_demo]);
        hagl_put_text(message, 6, 4, color, font6x9);
        sprintf(message, "%.3f FPS  ", fb_fps);
        hagl_put_text(message, DISPLAY_WIDTH - 56, DISPLAY_HEIGHT - 14, color, font6x9);

        hagl_set_clip_window(0, 20, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 21);

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
#else
	while (1)
	{
		fx_fps = aps(drawn);
		drawn = 0;

		sprintf(message, "%.3f %s PER SECOND       ", fx_fps, primitive[current_demo]);
		hagl_set_clip_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
		hagl_put_text(message, 8, 4, color, font10x20_ISO8859_7);
		hagl_set_clip_window(0, 20, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 21);

		vTaskDelay(1000 / portTICK_RATE_MS);
	}
#endif
	vTaskDelete(NULL);
}

void switch_task(void *params)
{
	while (1)
	{
		ESP_LOGI(TAG, "%.2f %s per second", fx_fps, primitive[current_demo]);

		current_demo = (current_demo + 1) % 17;
		/*if (current_demo == 15)
		 current_demo = 16;
		 else if (current_demo == 16)
		 current_demo = 17;
		 else
		 current_demo = 15;
		 */
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
/*
void put_text_demo()
{
	int16_t x0 = (rand() % DISPLAY_WIDTH + 20) - 80;
	int16_t y0 = (rand() % DISPLAY_HEIGHT + 20) - 20;

	color_t colour = rand() % 0xffff;

	char * text = "HOLA";
	hagl_put_text(text, x0, y0, colour, font6x9);
}*/

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
	const char *text = "YO ♥ MTV raps";
	hagl_put_text(text, 0, i += 20, 0xffff, font5x8);
	hagl_put_text(text, 0, i+=20, 0xffff, font10x20_ISO8859_1);
	hagl_put_text(text, 0, i+=20, 0xffff, font10x20_ISO8859_7);
	hagl_put_text(text, 0, i+=20, 0xffff, font8x13O_ISO8859_13);
	hagl_put_text(text, 0, i+=20, 0xffff, font9x15B_ISO8859_13);
	hagl_put_text(text, 0, i += 20, 0xffff, font9x18_ISO8859_13);
}

uint8_t jpg_data[3400]; // para que entre el chancha-parada.jpg
size_t jpg_size;
void font_bmp_demo()
{
	// mando un comando al display:
	//hagl_hal_control(0x36, &data[contador], 1);

	/*size_t jpg_size = fs_load_into_buf("chancha-parada.jpg", jpg_data, 3400);

	 int w,h;
	 hagl_get_jpg_size(&w, &h, jpg_data, 3400);

	 hagl_fill_rectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, BLACK);

	 for (int y = 0; y < 240-w; y+=2)
	 {*/
	// 71x50
	//hagl_draw_jpg(esp_random() % 320 - 72, random() % 240 - 51, jpg_data, jpg_size);
	hagl_load_jpg(esp_random() % 320 - 72, random() % 240 - 51, "chancha-parada.jpg");
	/*}

	 vTaskDelay(2000 / portTICK_RATE_MS);*/
}

void demo_task(void *params)
{
	void (*demo[18])();

	demo[0] = font_bmp_demo;
	//demo[1] = rgb_demo;
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
	//demo[16] = put_text_demo;
	//demo[17] = fonts_demo;

	uint8_t rx[10];
	hagl_hal_control(0x04, rx, 4); //Read Display Identification Information
	ESP_LOGE(TAG, "DISPLAY INFORMATION: Manufacturer ID:%02X Driver version:%02X Driver ID:%02X", rx[1], rx[2], rx[3]);
	/*	hagl_hal_control(0x09, rx, 5); //Read Display Identification Information
	 ESP_LOGE(TAG, "DISPLAY STATUS: %02X %02X %02X %02X", rx[1], rx[2], rx[3], rx[4]);
	 hagl_hal_control(0x0A, rx, 2); //Read Display Power Mode
	 ESP_LOGE(TAG, "DISPLAY Power Mode: %02X", rx[1]);
	 */

	//jpg_size = fs_load_into_buf("chancha-parada.jpg", jpg_data, 3400);

	while (1)
	{
		current_demo = 0; // OJO ACA, BLOQUEO ESTE TEST!
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

	fs_init();

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
		xTaskCreatePinnedToCore(demo_task, "Demo", 12048, NULL, 1, NULL, 1);
		xTaskCreatePinnedToCore(switch_task, "Switch", 2048, NULL, 2, NULL, 1);
#endif /* CONFIG_IDF_TARGET_ESP32S2 */
	}
	else
	{
		ESP_LOGE(TAG, "No mutex?");
	}
}
