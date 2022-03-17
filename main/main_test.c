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
#include "../components/gl9340/include/gl.h"

static const char *TAG = "main";

static void sleep()
{
	ESP_LOGI(TAG, "Heap despues: %d", esp_get_free_heap_size());
	vTaskDelay(500 / portTICK_RATE_MS);
	gl_fill_screen(BLACK);
	ESP_LOGI(TAG, "Heap antes  : %d", esp_get_free_heap_size());
}

// carga el jpg desde archivo a un bitmap
void demo_1()
{
	bitmap_t chancha;
	uint32_t err = gl_load_image("chancha-parada.jpg", &chancha);

	if (err != GL_OK)
		ESP_LOGE(TAG, "error al cargar la imagen");

	gl_blit(80, 10, &chancha);

	gl_free_bitmap_buffer(&chancha);
	sleep();
}

// testea los colores y figuras (me cambiaba los colores)
void demo_2()
{
	gl_fill_screen(OLIVE);
	gl_fill_circle(160, 120, 80, MAROON);
	gl_fill_rectangle(111, 111, 177, 177, ORANGEYEL);
	gl_fill_rectangle(171, 171, 200, 207, ORANGE);
	sleep();
}

// carga la imagen jpg desde archivo, y la muestra en el display directamente
void demo_3()
{
	gl_draw_rounded_rectangle(0, 0, 319, 239, 35, RED);
	uint32_t err = gl_show_image_file(10, 10, "chancha-parada.jpg");
	if (err != GL_OK)
		ESP_LOGE(TAG, "error al cargar la imagen");
	sleep();
}

// muestra un pedacito de la imagen
void demo_4()
{
	bitmap_t image;
	gl_load_image("chancha-parada.jpg", &image);

	uint16_t transparentColor = BLACK;

	RECT r; // solo muestro este pedacito del bmp
	set_rect_w(10, 10, 25, 25, &r);
	gl_show_partial_image(90, 80, &image, &r, transparentColor);

	gl_free_bitmap_buffer(&image);
	sleep();
}

void demo_5()
{
	bitmap_t image;
	gl_load_gimp_image(&chancha_parada, &image);
	gl_blit(esp_random() % 320 - chancha_parada.width, esp_random() % 240 - chancha_parada.height, &image);
	sleep();
}

void app_main()
{
	gl_init();
	fs_init();
	ESP_LOGI(TAG, "Heap al arrancar: %d", esp_get_free_heap_size());

	while (1)
	{
		demo_1();
		demo_2();
		demo_3();
		demo_4();
		demo_5();
	}
}
