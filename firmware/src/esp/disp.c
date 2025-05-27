/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"

#include "mipi_dsi.h"
#include "oled.h"
#include "emu.h"

//We need speed here!
#pragma GCC optimize ("O3")

//Stupid 1-to-1 routine
static uint16_t IRAM_ATTR findPixelVal(uint8_t *data, unsigned x, unsigned y) {
	// Something is quite wrong here :(
	// data is the XSIZE_M x YSIZE_M x 1 bit image from the emulator
	// x and y varies from 0 to 319
	// return the color in RGB565 of the pixel at x, y

	// going 8 pixels to the right means incrementing by one byte
	// going 1 pixel down means incrementing by XSIZE_M / 8 = 64 bytes
	uint8_t tmp = data[x / 8 + y * 64];

	// to find the pixel we need to return the (x % 8)th bit
	uint8_t mask = 0x80 >> (x & 7);
	return (tmp & mask) ? 0 : 0xffff;
	// return (data[y * 64 + (x >> 3)] & (1 << ((7 - x) & 7))) ? 0 : 0xffff;
}

TaskHandle_t th_display = NULL;

//Use this to move the display area down.
#define YOFFSET 0

// Size of the displayed area
#define XSIZE 320
#define YSIZE 250

// Size of the Macintosh screen
#define XSIZE_M 512
#define YSIZE_M 342

// TODO completely glitched up display when this is changed to larger values than 32. Why?
#define LINESPERBUF 32

static void IRAM_ATTR displayTask(void *arg) {
	uint8_t *img = malloc(LINESPERBUF * XSIZE * 2);
	// static uint8_t img[LINESPERBUF * XSIZE * 2];
	assert(img);
	memset(img, 0, LINESPERBUF * XSIZE * 2);

	uint8_t *oldImg = malloc(XSIZE_M * YSIZE_M / 8);
	// static uint8_t oldImg[XSIZE_M * YSIZE_M / 8];
	assert(oldImg);

	fillRect(0, 319, 0, 319, 0);

	// int firstrun = 1;
	setColRange(0, XSIZE - 1);
	setRowRange(YOFFSET, YOFFSET + YSIZE - 1);

	while(1) {
		uint8_t *myData = NULL;

		// mipiResync();

		// Wait for emulator to release the display memory
		xTaskNotifyWait(0, 0, (uint32_t*)(&myData), portMAX_DELAY);

		memcpy(oldImg, myData, XSIZE_M * YSIZE_M / 8);

		// Get mouse position, calculate draw window offset
		int m_y = m68k_read_memory_16(0x0830);
		int m_x = m68k_read_memory_16(0x0832);

		if (m_x > XSIZE_M - XSIZE / 2)
			m_x = XSIZE_M - XSIZE / 2;

		if (m_y > YSIZE_M - YSIZE / 2)
			m_y = YSIZE_M - YSIZE / 2;

		int o_x = 0, o_y = 0;

		if (m_x > XSIZE / 2)
			o_x = m_x - XSIZE / 2;

		if (m_y > YSIZE / 2)
			o_y = m_y - YSIZE / 2;


		// int ystart = 0;
		// int yend = YSIZE;
		// if (!firstrun) {
		// 	for (ystart=0; ystart < YSIZE_M; ystart++) {
		// 		if (memcmp(oldImg + 64 * ystart, myData + 64 * ystart, 64) != 0)
		// 			break;
		// 	}
		// 	for (yend = YSIZE_M - 1; yend >= ystart; --yend) {
		// 		if (memcmp(oldImg + 64 * yend, myData + 64 * yend, 64) != 0)
		// 			break;
		// 	}
		// 	if (ystart == YSIZE_M) {
		// 		//No need for update
		// 		yend = YSIZE_M;
		// 	} else {
		// 		//Only copy changed bits of data to changebuffer
		// 		memcpy(oldImg + ystart * 64, myData + ystart * 64, (yend - ystart) * 64);

		// 		ystart = ystart - 1;
		// 		yend = yend + 2;
		// 		if (ystart < 0)
		// 			ystart = 0;
		// 		// printf("disp: updating lines %d to %d\n", ystart, yend);
		// 	}
		// } else {
		// 	ystart=0; yend=XSIZE;
		// }
		// memcpy(oldImg, myData, XSIZE_M * YSIZE_M / 8);

		// if (ystart != yend) {
			// don't write too many lines into the framebuffer (it will wrap around)
			// if (yend > XSIZE)
			// 	yend = XSIZE;
			// setRowRange(ystart + YOFFSET, yend + YOFFSET);
			uint8_t cmd = 0x2c;  // start sending data
			uint8_t *p = img;
			int l = 0;
			for (int y=YOFFSET; y<YOFFSET + YSIZE; y++) {
				for (int x=0; x<XSIZE; x++) {
					uint16_t v = findPixelVal(oldImg, x + o_x, y + o_y);
					// uint16_t v = ((oldImg[x / 8 + y * 64]) & (0x80 >> (x & 7))) ? 0 : 0xffff;
					*p++ = v;
					*p++ = v >> 8;
				}
				l++;
				if (l >= LINESPERBUF || y >= (YOFFSET + YSIZE - 1)) {
					mipiDsiSendLong(0x39, cmd, img, l * XSIZE * 2);
					cmd = 0x3c;  // continue sending data
					l = 0;
					p = img;
				}
			}
		}
	// 	firstrun = 0;
	// }
}

// Functions below are called by the emulator task

void dispDraw(uint8_t *mem) {
	xTaskNotify(th_display, (uint32_t)mem, eSetValueWithOverwrite);
}

void dispInit() {
	mipiInit();
	initOled();
	// set_brightness(5);
	xTaskCreatePinnedToCore(&displayTask, "display", 6 * 1024, NULL, 5, &th_display, 1);
}
