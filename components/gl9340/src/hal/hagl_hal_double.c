/*
 Javier
 */
#include "sdkconfig.h"

#ifdef CONFIG_HAGL_HAL_USE_DOUBLE_BUFFERING

#include <esp_log.h>
#include "hal/hagl_hal.h"
#include "hal/mipi_display.h"
#include "bitmap.h"
#include <freertos/semphr.h>

#ifdef CONFIG_HAGL_HAL_LOCK_WHEN_FLUSHING
static SemaphoreHandle_t mutex;
#endif

static bitmap_t *fore_bitmap;
static spi_device_handle_t spi;
//static const char *TAG = "hagl_esp_mipi";

void hagl_hal_init()
{
	mipi_display_init(&spi);
#ifdef CONFIG_HAGL_HAL_LOCK_WHEN_FLUSHING
	mutex = xSemaphoreCreateMutex();
#endif
	fore_bitmap = bitmap_new(DISPLAY_WIDTH, DISPLAY_HEIGHT, NULL);
}

void hagl_hal_flush()
{
#ifdef CONFIG_HAGL_HAL_LOCK_WHEN_FLUSHING
	xSemaphoreTake(mutex, portMAX_DELAY);
#endif

	for (int y = 0; y < fore_bitmap->height; y++)
		mipi_display_write(spi, 0, y, fore_bitmap->width, 1, (uint8_t*) fore_bitmap->pixels[y]);

#ifdef CONFIG_HAGL_HAL_LOCK_WHEN_FLUSHING
	xSemaphoreGive(mutex);
#endif
}

void hagl_hal_put_pixel(int16_t x0, int16_t y0, color_t color)
{
#ifdef CONFIG_HAGL_HAL_LOCK_WHEN_FLUSHING
	xSemaphoreTake(mutex, portMAX_DELAY);
#endif

	fore_bitmap->pixels[y0][x0] = color;

#ifdef CONFIG_HAGL_HAL_LOCK_WHEN_FLUSHING
	xSemaphoreGive(mutex);
#endif
}

// hace un scroll de la pantalla, solo en la ventana dada.
void hagl_hal_scroll(uint8_t direction, RECT *window, uint16_t pixels, color_t background_color)
{
	for (int i = 0; i < pixels; i++)
	{
		// escrolea el bitmap en la direccion dada, solo un pixel,
		bitmap_shift(direction, window, fore_bitmap);
		// debo rellenar la linea o columna liberada con el color de fondo:
		switch (direction)
		{
			case SCROLL_UP:
				hagl_hal_hline(window->x0, window->y0 + window->h - 1, window->w, background_color);
				break;
			case SCROLL_DOWN:
				hagl_hal_hline(window->x0, window->y0, window->w, background_color);
				break;
			case SCROLL_LEFT:
				hagl_hal_vline(window->x0 + window->w - 1, window->y0, window->h, background_color);
				break;
			case SCROLL_RIGHT:
				hagl_hal_vline(window->x0, window->y0, window->h, background_color);
				break;
		}
	}
}

void hagl_hal_blit(uint16_t x0, uint16_t y0, bitmap_t *src, RECT *rect)
{
#ifdef CONFIG_HAGL_HAL_LOCK_WHEN_FLUSHING
	xSemaphoreTake(mutex, portMAX_DELAY);
#endif

	bitmap_blit(x0, y0, src, fore_bitmap, rect);

#ifdef CONFIG_HAGL_HAL_LOCK_WHEN_FLUSHING
	xSemaphoreGive(mutex);
#endif
}

void hagl_hal_scale_blit(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, bitmap_t *src)
{
#ifdef CONFIG_HAGL_HAL_LOCK_WHEN_FLUSHING
	xSemaphoreTake(mutex, portMAX_DELAY);
#endif
	// TODO
	//bitmap_scale_blit(x0, y0, w, h, src, &fore_bitmap);

#ifdef CONFIG_HAGL_HAL_LOCK_WHEN_FLUSHING
	xSemaphoreGive(mutex);
#endif
}

void hagl_hal_hline(int16_t x0, int16_t y0, uint16_t width, color_t color)
{
#ifdef CONFIG_HAGL_HAL_LOCK_WHEN_FLUSHING
	xSemaphoreTake(mutex, portMAX_DELAY);
#endif

	for (uint16_t x = x0; x < x0 + width; x++)
		fore_bitmap->pixels[y0][x] = color;

#ifdef CONFIG_HAGL_HAL_LOCK_WHEN_FLUSHING
	xSemaphoreGive(mutex);
#endif
}

void hagl_hal_vline(int16_t x0, int16_t y0, uint16_t height, color_t color)
{
#ifdef CONFIG_HAGL_HAL_LOCK_WHEN_FLUSHING
	xSemaphoreTake(mutex, portMAX_DELAY);
#endif

	for (uint16_t y = y0; y < y0 + height; y++)
		fore_bitmap->pixels[y][x0] = color;

#ifdef CONFIG_HAGL_HAL_LOCK_WHEN_FLUSHING
	xSemaphoreGive(mutex);
#endif
}

#endif /* CONFIG_HAGL_HAL_USE_DOUBLE_BUFFERING */
