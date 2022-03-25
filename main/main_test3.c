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

#include "fonts/font10x20-ISO8859-7.h"
#include "fonts/font10x20_ISO8859_1.h"
#include "fonts/font5x7.h"
#include "fonts/font5x8.h"
#include "fonts/font6x9.h"
#include "fonts/font8x13O-ISO8859-13.h"
#include "fonts/font9x15B-ISO8859-13.h"
#include "fonts/font9x18-ISO8859-13.h"
#include "gl.h"
#include "fsTools.h" // (ahora hagl abre imagenes con la fsTools)
#include "chancha.h"
#include "font_squid_30.h"
#include "iconos.h"
#include "font_agency_50.h"

static const char *TAG = "main";

void fonts_demo()
{
	int i = 0;
	const char *text = "YO ♥ MTV raps";
	hagl_put_text(text, 0, i += 20, 0xffff, font5x8);
	hagl_put_text(text, 0, i += 20, 0xffff, font10x20_ISO8859_1);
	hagl_put_text(text, 0, i += 20, 0xffff, font10x20_ISO8859_7);
	hagl_put_text(text, 0, i += 20, 0xffff, font8x13O_ISO8859_13);
	hagl_put_text(text, 0, i += 20, 0xffff, font9x15B_ISO8859_13);
	hagl_put_text(text, 0, i += 20, 0xffff, font9x18_ISO8859_13);
}

bitmap_t chancha;
bitmap_t font30;
bitmap_t iconos1;
bitmap_t *font_agency;

static void mostrar_numero(int x, int y, int num, int *config, bitmap_t *bmp)
{
	uint16_t imgx = config[num * 2];
	uint16_t x2 = config[num * 2 + 1] + 1; // el derecho esta incluido
	uint16_t imgy = 0;
	uint16_t imgw = x2 - imgx;
	uint16_t imgh = font30.height;
	uint16_t transparentColor = 0xffff; // blanco transparente
	hagl_show_image(x, y, imgx, imgy, imgw, imgh, bmp, transparentColor);
}

void new_font_demo()
{
	int num = 2; // numero a mostrar
	int x = 120;
	int y = 120;
	uint16_t imgx = font_squid_30_config[num * 2];
	uint16_t x2 = font_squid_30_config[num * 2 + 1] + 1; // el derecho esta incluido
	uint16_t imgy = 0;
	uint16_t imgw = x2 - imgx;
	uint16_t imgh = font30.height;
	uint16_t transparentColor = 0xffff; // blanco transparente
	hagl_show_image(x, y, imgx, imgy, imgw, imgh, &font30, transparentColor);

	num = 1;
	imgx = iconos_config[num * 4];
	imgy = iconos_config[num * 4 + 1];
	imgw = iconos_config[num * 4 + 2];
	imgh = iconos_config[num * 4 + 3];
	transparentColor = 0xffff; // blanco transparente
	hagl_show_image(10, 10, imgx, imgy, imgw, imgh, &iconos1, transparentColor);

}

void demo_task(void *params)
{
// transformo la estructura de GIMP a mi estructura bitmap (y hago swap de los bytes):
	//hagl_load_gimp_image(&chancha, chancha_parada.width, chancha_parada.height, chancha_parada.pixel_data);
	//hagl_load_gimp_image(&font30, font_squid_30.width, font_squid_30.height, font_squid_30.pixel_data);
	//hagl_load_gimp_image(&iconos1, iconos.width, iconos.height, iconos.pixel_data);
	//hagl_load_gimp_image(&font_agency, font_agency_50.width, font_agency_50.height, font_agency_50.pixel_data);



	int numero = 0;
	while (1)
	{
		hagl_fill_screen(WHITE);

		hagl_blit(esp_random() % 320 - 72, random() % 240 - 51, font_agency);

		//fonts_demo();

		// carga una imagen jpeg desde el FS, y la muestra en el display:
		//hagl_load_jpg(esp_random() % 320 - 72, random() % 240 - 51, "chancha-parada.jpg");

		// solo el blit de un bmp:
		//hagl_blit(esp_random() % 320 - 72, random() % 240 - 51, &chancha);

		//new_font_demo();

		//mostrar_numero(100, 100, numero++ %10, font_agency_50_config, &font_agency);

		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}

void app_main()
{
	hagl_init();
	fs_init();

	// cargo las imagenes a usar (si no se usan más hay que liberar la memoria!)
	font_agency = gl_load_image("chancha-parada.jpg");

	xTaskCreatePinnedToCore(demo_task, "Demo", 12048, NULL, 1, NULL, 1);
}
