/*

 MIT License

 Copyright (c) 2017-2018 Espressif Systems (Shanghai) PTE LTD
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

 This code is based on Espressif provided SPI Master example which was
 released to Public Domain: https://goo.gl/ksC2Ln

 This file is part of the MIPI DCS Display Driver:
 https://github.com/tuupola/esp_mipi

 SPDX-License-Identifier: MIT

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <driver/spi_master.h>
#include <driver/ledc.h>
#include <soc/gpio_struct.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include "sdkconfig.h"
#include "hal/mipi_dcs.h"
#include "hal/mipi_display.h"
#include "colors.h"

static const char *TAG = "mipi_display";
static const uint8_t DELAY_BIT = 1 << 7;

static SemaphoreHandle_t mutex;

DRAM_ATTR static const mipi_init_command_t init_commands[] = {
	{ 0xC0, { 0x23 }, 1 }, 					//Power Control 1
	{ 0xC1, { 0x10 }, 1 },					//Power Control 2
	{ 0xC5, { 0x3E, 0x28 }, 2 },			//VCOM Control 1
	{ 0xC7, { 0x86 }, 1 },					//VCOM Control 2

	{ MIPI_DCS_SET_ADDRESS_MODE, { MIPI_DISPLAY_ADDRESS_MODE }, 1 }, //Memory Access Control

	{ 0x3a, { 0x55 }, 1 },					//Pixel Format Set //65K color: 16-bit/pixel
	{ 0x20, { 0 }, 0 },						//Display Inversion OFF
	{ 0xb1, { 0, 0x18 }, 2 },				//Frame Rate Control
	{ 0xB6, { 0x08, 0xA2, 0x27, 0 }, 4 },	//Display Function Control  REV:1 GS:0 SS:0 SM:0
	{ 0x26, { 1 }, 1 }, 					//Gamma Set

	//Positive Gamma Correction
	{ 0xE0, { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 }, 15 },

	//Negative Gamma Correction
	{ 0xE1, { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F }, 15 },

	{ MIPI_DCS_EXIT_SLEEP_MODE, { 0 }, 0 | DELAY_BIT },	//Sleep Out
	{ MIPI_DCS_SET_DISPLAY_ON, { 0 }, 0 | DELAY_BIT },	//Display ON

	{ 0, { 0 }, 0xff }	// **End of commands**
};

static void mipi_display_write_command(spi_device_handle_t spi, const uint8_t command)
{
	spi_transaction_t transaction;
	memset(&transaction, 0, sizeof(transaction));

	/* Command is 1 byte ie 8 bits */
	transaction.length = 8;
	/* The data is the command itself */
	transaction.tx_buffer = &command;
	/* DC needs to be set to 0. */
	transaction.user = (void*) 0;
	ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &transaction));
}

/* Uses spi_device_transmit, which waits until the transfer is complete. */
static void mipi_display_write_data(spi_device_handle_t spi, const uint8_t *data, size_t length)
{
	if (0 == length)
		return;

	spi_transaction_t transaction;
	memset(&transaction, 0, sizeof(transaction));

	/* Length in bits */
	transaction.length = length * 8;
	transaction.tx_buffer = data;
	/* DC needs to be set to 1 */
	transaction.user = (void*) 1;

	ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &transaction));
}

// si son mas de 4 bytes usar mipi_display_read_data_large
static void mipi_display_read_data(spi_device_handle_t spi, uint8_t *data, size_t length)
{
	if (0 == length)
		return;

	spi_transaction_t transaction;
	memset(&transaction, 0, sizeof(transaction));

	/* Length in bits */
	transaction.length = length * 8;
	transaction.rxlength = length * 8;
	transaction.rx_buffer = data;
	transaction.flags = SPI_TRANS_USE_RXDATA;
	/* DC needs to be set to 1 */
	transaction.user = (void*) 1;

	ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &transaction));
	//ESP_ERROR_CHECK(spi_device_transmit(spi, &transaction));
}


/* This function is called in irq context just before a transmission starts. */
/* It will set the DC line to the value indicated in the user field. */
static void mipi_display_pre_callback(spi_transaction_t *transaction)
{
	uint32_t dc = (uint32_t) transaction->user;
	gpio_set_level(CONFIG_MIPI_DISPLAY_PIN_DC, dc);
}

static void mipi_display_spi_master_init(spi_device_handle_t *spi)
{
	spi_bus_config_t buscfg = {
		.miso_io_num = CONFIG_MIPI_DISPLAY_PIN_MISO,
		.mosi_io_num = CONFIG_MIPI_DISPLAY_PIN_MOSI,
		.sclk_io_num = CONFIG_MIPI_DISPLAY_PIN_CLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		/* Max transfer size in bytes */
		.max_transfer_sz = SPI_MAX_TRANSFER_SIZE
	};
	spi_device_interface_config_t devcfg = {
		.clock_speed_hz = CONFIG_MIPI_DISPLAY_SPI_CLOCK_SPEED_HZ,
		.mode = 0,
		.spics_io_num = CONFIG_MIPI_DISPLAY_PIN_CS,
		.queue_size = 64,
		/* Handles the D/C line */
		.pre_cb = mipi_display_pre_callback,
		.flags = SPI_DEVICE_NO_DUMMY
	};

	/* ESP32S2 requires DMA channel to match the SPI host. */
	ESP_ERROR_CHECK(spi_bus_initialize(CONFIG_MIPI_DISPLAY_SPI_HOST, &buscfg, CONFIG_MIPI_DISPLAY_SPI_HOST));
	ESP_ERROR_CHECK(spi_bus_add_device(CONFIG_MIPI_DISPLAY_SPI_HOST, &devcfg, spi));
}

void mipi_display_init(spi_device_handle_t *spi)
{
	uint8_t cmd = 0;

	mutex = xSemaphoreCreateMutex();

	gpio_set_direction(CONFIG_MIPI_DISPLAY_PIN_CS, GPIO_MODE_OUTPUT);
	gpio_set_level(CONFIG_MIPI_DISPLAY_PIN_CS, 0);
	gpio_set_direction(CONFIG_MIPI_DISPLAY_PIN_DC, GPIO_MODE_OUTPUT);

	/* Init spi driver. */
	mipi_display_spi_master_init(spi);
	vTaskDelay(100 / portTICK_RATE_MS);

	/* Reset the display. */
	if (CONFIG_MIPI_DISPLAY_PIN_RST > 0)
	{
		gpio_set_direction(CONFIG_MIPI_DISPLAY_PIN_RST, GPIO_MODE_OUTPUT);
		gpio_set_level(CONFIG_MIPI_DISPLAY_PIN_RST, 0);
		vTaskDelay(100 / portTICK_RATE_MS);
		gpio_set_level(CONFIG_MIPI_DISPLAY_PIN_RST, 1);
		vTaskDelay(100 / portTICK_RATE_MS);
	}

	/* Send all the commands. */
	while (init_commands[cmd].count != 0xff)
	{
		mipi_display_write_command(*spi, init_commands[cmd].command);
		mipi_display_write_data(*spi, init_commands[cmd].data, init_commands[cmd].count & 0x1F);
		if (init_commands[cmd].count & DELAY_BIT)
		{
			ESP_LOGD(TAG, "Delaying after command 0x%02x", (uint8_t )init_commands[cmd].command);
			vTaskDelay(200 / portTICK_RATE_MS);
		}
		cmd++;
	}

	/* Enable backlight */
	if (CONFIG_MIPI_DISPLAY_PIN_BL > 0)
	{
		gpio_set_direction(CONFIG_MIPI_DISPLAY_PIN_BL, GPIO_MODE_OUTPUT);
		gpio_set_level(CONFIG_MIPI_DISPLAY_PIN_BL, 1);

		/* Enable backlight PWM */
		if (CONFIG_MIPI_DISPLAY_PWM_BL > 0)
		{
			ESP_LOGI(TAG, "Initializing backlight PWM");
			ledc_timer_config_t timercfg = {
				.duty_resolution = LEDC_TIMER_13_BIT,
				.freq_hz = 9765,
				.speed_mode = LEDC_LOW_SPEED_MODE,
				.timer_num = LEDC_TIMER_0,
				.clk_cfg = LEDC_AUTO_CLK,
			};

			ledc_timer_config(&timercfg);

			ledc_channel_config_t channelcfg = {
				.channel = LEDC_CHANNEL_0,
				.duty = CONFIG_MIPI_DISPLAY_PWM_BL,
				.gpio_num = CONFIG_MIPI_DISPLAY_PIN_BL,
				.speed_mode = LEDC_LOW_SPEED_MODE,
				.hpoint = 0,
				.timer_sel = LEDC_TIMER_0,
			};

			ledc_channel_config(&channelcfg);
		}

	}

	ESP_LOGI(TAG, "Display initialized.");

	spi_device_acquire_bus(*spi, portMAX_DELAY);
}

static void mipi_set_address_window(spi_device_handle_t spi, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	static int32_t prev_x1, prev_x2, prev_y1, prev_y2;

	/* Change column address only if it has changed. */
	if ((prev_x1 != x1 || prev_x2 != x2))
	{
		mipi_display_write_command(spi, MIPI_DCS_SET_COLUMN_ADDRESS);
		uint8_t col[4] = { x1 >> 8, x1 & 0xff, x2 >> 8, x2 & 0xff };
		mipi_display_write_data(spi, col, 4);

		prev_x1 = x1;
		prev_x2 = x2;
	}

	/* Change page address only if it has changed. */
	if ((prev_y1 != y1 || prev_y2 != y2))
	{
		mipi_display_write_command(spi, MIPI_DCS_SET_PAGE_ADDRESS);
		uint8_t row[4] = { y1 >> 8, y1 & 0xff, y2 >> 8, y2 & 0xff };
		mipi_display_write_data(spi, row, 4);

		prev_y1 = y1;
		prev_y2 = y2;
	}
}

void mipi_display_write(spi_device_handle_t spi, uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint8_t *buffer)
{
	if (0 == w || 0 == h)
		return;

	int32_t x2 = x1 + w - 1;
	int32_t y2 = y1 + h - 1;
	uint32_t size = w * h * 2;

	xSemaphoreTake(mutex, portMAX_DELAY);
	mipi_set_address_window(spi, x1, y1, x2, y2);
	mipi_display_write_command(spi, MIPI_DCS_WRITE_MEMORY_START);
	mipi_display_write_data(spi, buffer, size);
	xSemaphoreGive(mutex);
}

/**
 * Esta la agregu?? yo.
 * El buffer tiene que tener espacio para (ancho * alto * 2) bytes (pixels)
 * ESTA FUNCION NO ANDA, Y DEBE SER PORQUE EL DISPLAY NO ACEPTA LA LECTURA DE LA MEMORIA.
 * No es estandar MIPI. Hay que ver cual seria....
 */
void mipi_get_pixel_data(spi_device_handle_t spi, uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint8_t *buffer)
{
	if (0 == w || 0 == h)
		return;

	uint8_t pixel[4] = { 0, 0, 0, 0 };
	xSemaphoreTake(mutex, portMAX_DELAY);
	uint16_t *p = (uint16_t*) buffer;
	for (int x = 0; x < w; ++x)
	{
		for (int y = 0; y < h; ++y)
		{
			mipi_set_address_window(spi, x1 + x, y1 + y, x1 + x, y1 + y);
			//	mipi_display_write_command(spi, 0xd9); // secret command ?
			mipi_display_write_command(spi, MIPI_DCS_READ_MEMORY_START); // ram read
			mipi_display_read_data(spi, pixel, 4);
			*p++ = rgb565(pixel[1], pixel[2], pixel[3]);
		}
	}
	xSemaphoreGive(mutex);
}

/**
 * Envia comandos o setea. Depende del tipo de comando,
 * pero lo uso para leer datos del display.
 */
void mipi_display_ioctl(spi_device_handle_t spi, const uint8_t command, uint8_t *data, size_t size)
{
	xSemaphoreTake(mutex, portMAX_DELAY);

	switch (command)
	{
		case MIPI_DCS_GET_COMPRESSION_MODE:
			case MIPI_DCS_GET_DISPLAY_ID:
			case MIPI_DCS_GET_RED_CHANNEL:
			case MIPI_DCS_GET_GREEN_CHANNEL:
			case MIPI_DCS_GET_BLUE_CHANNEL:
			case MIPI_DCS_GET_DISPLAY_STATUS:
			case MIPI_DCS_GET_POWER_MODE:
			case MIPI_DCS_GET_ADDRESS_MODE:
			case MIPI_DCS_GET_PIXEL_FORMAT:
			case MIPI_DCS_GET_DISPLAY_MODE:
			case MIPI_DCS_GET_SIGNAL_MODE:
			case MIPI_DCS_GET_DIAGNOSTIC_RESULT:
			case MIPI_DCS_GET_SCANLINE:
			case MIPI_DCS_GET_DISPLAY_BRIGHTNESS:
			case MIPI_DCS_GET_CONTROL_DISPLAY:
			case MIPI_DCS_GET_POWER_SAVE:
			case MIPI_DCS_READ_DDB_START:
			case MIPI_DCS_READ_DDB_CONTINUE:
			mipi_display_write_command(spi, command);
			mipi_display_read_data(spi, data, size);
			break;
		default:
			mipi_display_write_command(spi, command);
			mipi_display_write_data(spi, data, size);
	}

	xSemaphoreGive(mutex);
}

void mipi_display_close(spi_device_handle_t spi)
{
	spi_device_release_bus(spi);
}
