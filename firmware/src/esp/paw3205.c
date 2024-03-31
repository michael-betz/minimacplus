/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

// Works for the PAW3204/5 and similar optical mouse sensor chips

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include "rom/ets_sys.h"

#include "mouse.h"


#define PIN_DATA 23
#define PIN_CLK 19

#define DELAY() ets_delay_us(2);


static void pawSync()
{
	printf("pawSync()\n");
	gpio_set_direction(PIN_DATA, GPIO_MODE_INPUT);
	gpio_set_level(PIN_CLK, 0);
	ets_delay_us(2);
	gpio_set_level(PIN_CLK, 1);
	vTaskDelay(100 / portTICK_PERIOD_MS);
}


static void pawWrite8(int adr, int val) {
	int data = ((adr | 0x80) << 8) | val;

	gpio_set_direction(PIN_DATA, GPIO_MODE_OUTPUT);
	for (int mask=0x8000; mask!=0; mask>>=1) {
		gpio_set_level(PIN_DATA, (data & mask) ? 1 : 0);
		gpio_set_level(PIN_CLK, 0);
		DELAY();
		gpio_set_level(PIN_CLK, 1);
		DELAY();
	}
	gpio_set_direction(PIN_DATA, GPIO_MODE_INPUT);
}

static unsigned pawRead8(int adr) {
	unsigned data = adr & 0x7F;
	unsigned mask = 0x80;

	gpio_set_direction(PIN_DATA, GPIO_MODE_OUTPUT);
	for (int i=0; i<8; i++) {
		gpio_set_level(PIN_DATA, ((data & mask) > 0));
		gpio_set_level(PIN_CLK, 0);
		DELAY();

		gpio_set_level(PIN_CLK, 1);
		DELAY();

		mask >>= 1;
	}

	ets_delay_us(10);
	gpio_set_direction(PIN_DATA, GPIO_MODE_INPUT);

	data = 0;
	for (int i=0; i<8; i++) {
		data <<= 1;

		gpio_set_level(PIN_CLK, 0);
		DELAY();

		gpio_set_level(PIN_CLK, 1);
		data |= gpio_get_level(PIN_DATA);
		DELAY();
	}
	return data;
}

static bool is_mouse_found = false;

static void pawInit() {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_DATA) | (1ULL << PIN_CLK);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

	gpio_set_direction(PIN_CLK, GPIO_MODE_OUTPUT);

	for (unsigned i=0; i<10; i++) {
		pawSync();
		is_mouse_found = pawRead8(0x00) == 0x30;
		if (is_mouse_found)
			break;
	}
}


// Below are functions which will be called by the emulator

void mouse_init()
{
	pawInit();
	if (!is_mouse_found)
		printf("mouse_init(): PAW3205 not found!\n");
	else
		printf("mouse_init(): success!\n");
}

void mouse_read()
{
	static unsigned n = 0;

	if (!is_mouse_found)
		return;

	int8_t dx = 0, dy = 0;

	// Read btn status (when DIO line is high impedance)
	gpio_set_level(PIN_CLK, 0);
	ets_delay_us(25);
	unsigned btn = !gpio_get_level(PIN_DATA);

	if (n++ > 180) {
		if (pawRead8(0x00) != 0x30)
			pawSync();
		n = 0;
	}

	unsigned status = pawRead8(0x02);

	// if motion happened
	if (status & (1 << 7)) {
		dx = pawRead8(0x03);
		dy = pawRead8(0x04);
	}

	// printf("mouse_read(): %02x (%4d %4d) %1d \n", pawRead8(0x07), dx, dy, btn);

	mouseMove(dx, -dy, btn);
}
