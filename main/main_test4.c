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
#include "fonts/font8x13O-ISO8859-13.h"

static SemaphoreHandle_t mutex;
static const char *TAG = "main";

static void sleep()
{
	gl_flush();
	ESP_LOGI(TAG, "Heap despues: %d", esp_get_free_heap_size());
	vTaskDelay(5000 / portTICK_RATE_MS);
	gl_fill_screen(BLACK);
	gl_flush();
	ESP_LOGI(TAG, "Heap antes  : %d", esp_get_free_heap_size());
}

// carga el jpg desde archivo a un bitmap /OK
// uso de blit /OK
void demo_1()
{
	gl_draw_rounded_rectangle(0, 0, 319, 239, 35, RED);
	bitmap_t *chancha = gl_load_image("chancha-parada.jpg"); // 71x50

	if (!chancha)
		ESP_LOGE(TAG, "error al cargar la imagen");

	gl_blit((320 - 71) / 2, (240 - 50) / 2, chancha, NULL);

	bitmap_delete(chancha);
	sleep();
}

// testea los colores y figuras /OK
void demo_2()
{
	gl_fill_screen(OLIVE);
	gl_fill_circle(160, 120, 80, MAROON);
	gl_fill_rectangle(111, 111, 177, 177, ORANGEYEL);
	gl_fill_rectangle(171, 171, 200, 207, ORANGE);
	sleep();
}

// testea textos /OK
void demo_3()
{
	gl_fill_screen(BLUE);

	gl_set_font(font6x9);
	gl_set_font_colors(YELLOW, NAVY);
	gl_set_font_pos(20, 20);
	gl_clear_transparent();
	gl_print("HOLA");
	gl_print(" MONGO\n");
	gl_set_transparent();
	gl_printf("PRINTF(%d) ", esp_random());
	gl_printf("TRANSPARENTE");

	sleep();
}

// muestra un pedacito de la imagen. //OK
// muestra fuera de la pantalla. /OK
void demo_4()
{
	gl_fill_screen(BLUE);

	bitmap_t *image = gl_load_image("chancha-parada.jpg");
	image->transparentColor = BLACK;

	RECT r; // solo muestro este pedacito del bmp
	set_rect_w(10, 10, 30, 35, &r);
	gl_blit(90, 80, image, &r);

	set_rect_w(0, 0, 50, 50, &r);
	gl_blit(-9, -9, image, &r);

	set_rect_w(0, 0, 50, 50, &r);
	gl_blit(310, 120, image, &r);

	bitmap_delete(image);
	sleep();
}

// para imagenes bajadas desde GIMP /OK
// muestra fuera de la pantalla. /OK
void demo_5()
{
	bitmap_t *image = gl_load_gimp_image(&chancha_parada);

	int l = 160 - image->width / 2;
	int t = 120 - image->height / 2;

	gl_blit(l, t, image, NULL);
	gl_blit(l, -10, image, NULL);
	gl_blit(310, t, image, NULL);
	gl_blit(-10, t, image, NULL);
	gl_blit(l, 230, image, NULL);

	bitmap_delete(image);
	sleep();
}

// OK
// pruebo hacer scroll de la pantalla
// COMO NO SE PUEDE LEER LA RAM DEL DISPLAY, VOY A USAR UN BITMAP PARA ESCRIBIR EN ÉL,
// Y HACER UN BLIT CON CADA LINEA AGREGADA AL FINAL (simulo una terminal)
void demo_6()
{
	//ESP_LOGI(TAG, "Heap antes: %d", esp_get_free_heap_size());

	gl_fill_screen(RED);
	int w2 = DISPLAY_WIDTH / 2 - 10;
	int h2 = DISPLAY_HEIGHT / 2 - 10;
	terminal_t *term = gl_terminal_new(10, 10, w2, h2, font6x9, YELLOW, BLUE);
	terminal_t *term1 = gl_terminal_new(w2 + 10, 10, w2, h2, font6x9, YELLOW, DARKGREEN);
	terminal_t *term2 = gl_terminal_new(10, h2 + 10, w2, h2, font6x9, YELLOW, PURPLE);
	terminal_t *term3 = gl_terminal_new(w2 + 10, h2 + 10, w2, h2, font8x13O_ISO8859_13, WHITE, MAROON);
	gl_flush(); // (muestro la terminal borrada)

	//ESP_LOGI(TAG, "Heap ahora: %d", esp_get_free_heap_size());
	//gl_terminal_print(term, "A");
	char buf[100];
	for (int i = 1; i < 20; ++i)
	{
		sprintf(buf, "LINEA %d\n", i);
		gl_terminal_print(term, buf);
		gl_flush();
		vTaskDelay(50 / portTICK_RATE_MS);
	}
	for (int i = 1; i < 17; ++i)
	{
		sprintf(buf, "TEXTO HORIZONTAL %d, ", i);
		gl_terminal_print(term1, buf);
		gl_flush();
		vTaskDelay(25 / portTICK_RATE_MS);
	}
	for (int i = 1; i < 20; ++i)
	{
		sprintf(buf, "DOBLE ENTER....%d\n\n", i);
		gl_terminal_print(term2, buf);
		gl_flush();
		vTaskDelay(10 / portTICK_RATE_MS);
	}
	for (int i = 1; i < 10; ++i)
	{
		sprintf(buf, "LINEA un poco mas larga que las demas %d\n", i);
		gl_terminal_print(term3, buf);
		gl_flush();
		vTaskDelay(50 / portTICK_RATE_MS);
	}

	gl_terminal_delete(term);
	gl_terminal_delete(term1);
	gl_terminal_delete(term2);
	gl_terminal_delete(term3);
	sleep();
}

static void pantalla_para_scrolar()
{
	vTaskDelay(1000 / portTICK_RATE_MS);

	gl_fill_screen(YELLOW);
	int w2 = DISPLAY_WIDTH / 2 - 20;
	int h2 = DISPLAY_HEIGHT / 2 - 20;

	RECT r1, r2, r3, r4;
	set_rect_w(20, 20, w2, h2, &r1);
	set_rect_w(w2 + 20, 20, w2, h2, &r2);
	set_rect_w(20, h2 + 20, w2, h2, &r3);
	set_rect_w(w2 + 20, h2 + 20, w2, h2, &r4);

	gl_fill_rect(&r1, BLUE);
	gl_fill_rect(&r2, GREEN);
	gl_fill_rect(&r3, RED);
	gl_fill_rect(&r4, WHITE);

	RECT r;
	set_rect_w(DISPLAY_WIDTH / 4, DISPLAY_HEIGHT / 4, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, &r);
	gl_draw_rect(&r, BLACK);
	resize_rect(&r, 1);
	gl_draw_rect(&r, BLACK);

	vTaskDelay(1000 / portTICK_RATE_MS);
}

// diferentes scrolls /ok
void demo_7()
{
	RECT area;
	set_rect_w(DISPLAY_WIDTH / 4, DISPLAY_HEIGHT / 4, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, &area);

	for (int dir = 0; dir < 4; ++dir)
	{
		pantalla_para_scrolar();
		for (int i = 1; i < 20; ++i)
		{
			gl_flush();
			hagl_hal_scroll(dir, &area, 2, CYAN);
			vTaskDelay(10 / portTICK_RATE_MS);
		}
	}
	sleep();
}

void demo_task(void *params)
{
	ESP_LOGI(TAG, "Heap al arrancar: %d", esp_get_free_heap_size());
	while (1)
	{
		demo_1();
		demo_2();
		demo_3();
		demo_4();
		demo_5();
		demo_6();
		demo_7();
	}
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

	ESP_LOGI(TAG, "Heap antes del task: %d", esp_get_free_heap_size());

	xTaskCreatePinnedToCore(demo_task, "Demo", 10000, NULL, 1, NULL, 1);
	//xTaskCreatePinnedToCore(framebuffer_task, "Framebuffer", 8192, NULL, 1, NULL, 0);
}
