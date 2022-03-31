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
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <esp_event.h>
#include <esp_task_wdt.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_task_wdt.h>

#include "fsTools.h"
#include "chancha.h"
#include "gl.h"
#include "fonts/font6x9.h"
#include "fonts/font8x13O-ISO8859-13.h"
#include "fonts/font9x18-ISO8859-13.h"

static SemaphoreHandle_t mutex;
static const char *TAG = "main";

static void tiempito()
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
	tiempito();
}

// testea los colores y figuras /OK
void demo_2()
{
	gl_fill_screen(OLIVE);
	gl_fill_circle(160, 120, 80, MAROON);
	gl_fill_rectangle(111, 111, 177, 177, ORANGEYEL);
	gl_fill_rectangle(171, 171, 200, 207, ORANGE);
	tiempito();
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

	tiempito();
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
	tiempito();
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
	tiempito();
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
	tiempito();
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

	// scroll de un rectangulo interno
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

	// scroll de toda la pantalla
	set_rect_w(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, &area);
	for (int dir = 0; dir < 4; ++dir)
	{
		pantalla_para_scrolar();
		for (int i = 1; i < 20; ++i)
		{
			gl_flush();
			hagl_hal_scroll(dir, &area, 5, RED);
			vTaskDelay(10 / portTICK_RATE_MS);
		}
	}
	tiempito();
}

// scroll de toda la pantalla
// (o terminal grande como toda la pantalla) /OK
void demo_8()
{
	gl_fill_screen(PURPLE);

	// testeo que el x0+ancho no explote (y en y)

	terminal_t *term = gl_terminal_new(50, 50, 310, 220, font6x9, RED, BLACK);

	for (int i = 0; i < 20; i++)
		gl_terminal_print(term, "HOLA\n");
	for (int i = 0; i < 20; i++)
		gl_terminal_print(term, "MONGO\n");

	gl_terminal_delete(term);
	tiempito();
}

/*
 ---test solo del color_from_ansi---
 I (26129) main: colors => 0000 0000 1084 1084
 I (26129) main: colors => C5D9 C5D9 00F8 00F8
 I (26129) main: colors => A93D A93D E007 E007
 I (26129) main: colors => 20FE 20FE E0FF E0FF
 I (26139) main: colors => 7703 7703 1F00 1F00
 I (26139) main: colors => 2E71 2E71 1FF8 1FF8
 I (26149) main: colors => BD2D BD2D FF07 FF07
 I (26149) main: colors => 79CE 79CE FFFF FFFF
 */
void demo_ansi_colors(terminal_t *term)
{
	for (int i = 0; i < 8; i++)
	{
		ESP_LOGI(TAG, "colors => %04X %04X %04X %04X", color_from_ansi(30 + i), color_from_ansi(40 + i),
				color_from_ansi(90 + i), color_from_ansi(100 + i));
		term->bg = i == 0 ? WHITE : BLACK;
		term->fg = color_from_ansi(30 + i);
		gl_terminal_printf(term, "FG %04X ", term->fg);
		term->fg = color_from_ansi(90 + i);
		gl_terminal_printf(term, "FG %04X ", term->fg);
		term->fg = i == 7 ? BLACK : WHITE;
		term->bg = color_from_ansi(40 + i);
		gl_terminal_printf(term, "BG %04X ", term->bg);
		term->bg = color_from_ansi(100 + i);
		gl_terminal_printf(term, "BG %04X\n", term->bg);
	}
	ESP_LOGI(TAG, "color prohibido => %04X", color_from_ansi(0));
	gl_flush();
	vTaskDelay(5000 / portTICK_RATE_MS);
}

void demo_ansi_tty(terminal_t *term)
{
	static char *names[] = {
		"Black", "Red", "Green", "Yellow",
		"Blue", "Magenta", "Cyan", "White",
		"Bright Black (Gray)", "Bright Red", "Bright Green", "Bright Yellow",
		"Bright Blue", "Bright Magenta", "Bright Cyan", "Bright White" };

	int i;
	char buf[200];

	// cambio colores de letra:
	for (i = 0; i < 8; i++)
	{
		sprintf(buf, "\033[107;%dmFG=%d color %s\033[0m\n", 30 + i, 30 + i, names[i]);
		gl_terminal_print(term, buf);
	}
	for (i = 0; i < 8; i++)
	{
		sprintf(buf, "\033[40;%dmFG=%d color %s\033[0m\n", 90 + i, 90 + i, names[8 + i]);
		gl_terminal_print(term, buf);
	}
	gl_flush();

	vTaskDelay(1000 / portTICK_RATE_MS);

	// cambio los colores de fondo:
	for (i = 0; i < 8; i++)
	{
		sprintf(buf, "\033[97;%dmBG=%d, Blanco sobre %s\033[0m\n", 40 + i, 40 + i, names[i]);
		gl_terminal_print(term, buf);
	}
	for (i = 0; i < 8; i++)
	{
		sprintf(buf, "\033[30;%dmBG=%d Negro sobre %s\033[0m\n", 100 + i, 100 + i, names[8 + i]);
		gl_terminal_print(term, buf);
	}
	gl_flush();
}

// colores controlados por códigos de terminal (\033 ANSI escape sequence)
// ejemplo: "\033[0;31m texto rojo \033[0m"
void demo_9()
{
	static bool toggle = true;
	terminal_t *term = gl_terminal_new(0, 0, 320, 240, font9x18_ISO8859_13, WHITE, BLACK);
	gl_terminal_ansi_enabled(term, toggle);
	toggle = !toggle;
	demo_ansi_colors(term);
	demo_ansi_tty(term);
	gl_terminal_delete(term);
	tiempito();
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
		demo_8();
		demo_9();
	}
}

/*
 * Flushes the framebuffer to display in a loop. This demo is
 * capped to 30 fps.
 */
void framebuffer_task(void *params)
{
	int *fps = (int*) params;
	const TickType_t frequency = 1000 / (*fps) / portTICK_RATE_MS;
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
	int fps = 5;
	ESP_LOGD(TAG, "SDK version: %s", esp_get_idf_version());
	ESP_LOGD(TAG, "Heap when starting: %d", esp_get_free_heap_size());

	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	mutex = xSemaphoreCreateMutex();

	gl_init();
//	gl_clear_screen(rgb565(0xff, 0x96, 0x96));

	xTaskCreatePinnedToCore(framebuffer_task, "Framebuffer", 8192, &fps, 1, NULL, 0);

	fs_init();

//	terminal_t *term = gl_terminal_new(0, 0, 320, 240, font6x9, RED, BLACK);
//	for (int i = 0; i < 20; i++) // ACA EXPLOTA!!!
//	{
//		gl_terminal_print(term, "HOLA\n");
//	}
//	for (int i = 0; i < 20; i++) // ACA EXPLOTA!!!
//	{
//		gl_terminal_print(term, "MONGO\n");
//	}
//		//ESP_LOGD(FSLOG_STARTUP, "debug %d", esp_random());
//	for (;;){}

	ESP_LOGI(TAG, "Heap antes del task: %d", esp_get_free_heap_size());

	xTaskCreatePinnedToCore(demo_task, "Demo", 10000, NULL, 1, NULL, 1);
	//xTaskCreatePinnedToCore(framebuffer_task, "Framebuffer", 8192, &fps, 1, NULL, 0);
}
