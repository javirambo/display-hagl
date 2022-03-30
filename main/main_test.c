/*
 *
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

#include "fsTools.h"
#include "chancha.h"
#include "gl.h"
#include "fonts/font6x9.h"

static const char *TAG = "main";

static SemaphoreHandle_t mutex;

static void sleep()
{
	ESP_LOGI(TAG, "Heap despues: %d", esp_get_free_heap_size());
	vTaskDelay(5000 / portTICK_RATE_MS);
	ESP_LOGI(TAG, "Heap antes  : %d", esp_get_free_heap_size());
}

void demo_task(void *params)
{
	int alto = 100;
	int ancho = 100;

	gl_fill_screen(RED);

	// cls
	//for (uint16_t y = 0; y < alto; y++)
	//	hagl_hal_hline(0, y, ancho, YELLOW);

	bitmap_t bmp;
	new_bitmap(ancho, alto, &bmp);
	bmp.transparentColor = BLUE;
	bmp.flags = BMP_TRANSPARENT;
	bitmap_t bmp2;
	new_bitmap(ancho, alto, &bmp2);

	// fondo rojo del bmp:
	for (int x = 0; x < bmp.width; x++)
		for (int y = 0; y < bmp.height; y++)
		{
			bmp.pixels[y][x] = YELLOW;
			bmp2.pixels[y][x] = RED;
		}

	//dibujo sobre el bmp:
	for (uint8_t x = 50; x < 75; x++)
		for (uint8_t y = 50; y < 75; y++)
		{
			bmp.pixels[y][x] = BLUE;
			bmp2.pixels[y + 15][x + 15] = GREEN;
		}

	while (1)
	{
		gl_blit(0, 0, &bmp);
		vTaskDelay(666 / portTICK_RATE_MS);
		gl_blit(0, 0, &bmp2);
		vTaskDelay(666 / portTICK_RATE_MS);
	}

	vTaskDelete(NULL);
}

/*
 * Flushes the framebuffer to display in a loop. This demo is
 * capped to 30 fps.
 */
void framebuffer_task(void *params)
{
	const TickType_t frequency = 1000 / 30 / portTICK_RATE_MS;
	TickType_t last = xTaskGetTickCount();
	while (1)
	{
		xSemaphoreTake(mutex, portMAX_DELAY);
		gl_flush();
		xSemaphoreGive(mutex);
		vTaskDelayUntil(&last, frequency);
	}
	vTaskDelete(NULL);
}

void app_main()
{
	gl_init();
	fs_init();

	mutex = xSemaphoreCreateMutex();

	ESP_LOGI(TAG, "Heap al arrancar: %d", esp_get_free_heap_size());
	xTaskCreatePinnedToCore(demo_task, "Demo", 16384, NULL, 1, NULL, 1);
	xTaskCreatePinnedToCore(framebuffer_task, "Framebuffer", 8192, NULL, 1, NULL, 0);
}
