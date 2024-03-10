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

//We need speed here!
#pragma GCC optimize ("O3")

#define DO_RESCALE 1

#if DO_RESCALE
	// Floating-point number, actually x/32. Divide mac reso by this to get lcd reso.
	#define SCALE_FACT 51
#else
	#define SCALE_FACT 32
#endif

static uint8_t mask[512];

static void calcLut() {
	for (int i=0; i<512; i++)
		mask[i] = (1 << (7 - (i & 7)));
}

//Returns 0-1024
static int IRAM_ATTR findMacVal(uint8_t *data, int x, int y) {
	int a,b,c,d;
	int v=0;
	int rx=x/32;
	int ry=y/32;

	if (ry>=342) return 0;

	a=data[ry*(512/8)+rx/8]&mask[rx];
	rx++;
	b=data[ry*(512/8)+rx/8]&mask[rx];
	rx--; ry++;
	if (ry<342) {
		c=data[ry*(512/8)+rx/8]&mask[rx];
		rx++;
		d=data[ry*(512/8)+rx/8]&mask[rx];
	} else {
		c=1;
		d=1;
	}

	if (!a) v+=(31-(x&31))*(31-(y&31));
	if (!b) v+=(x&31)*(31-(y&31));
	if (!c) v+=(31-(x&31))*(y&31);
	if (!d) v+=(x&31)*(y&31);

	return v;
}


// Even pixels: a
//  RRBB
//   GG
//
// Odd pixels: b
//   GG
//  RRBB
//
// Even lines start with an even pixel, odd lines with an odd pixel.
//
// Due to the weird buildup, a horizontal subpixel actually is 1/3rd real pixel wide!

#if DO_RESCALE

static uint16_t IRAM_ATTR findPixelVal(uint8_t *data, int x, int y) {
	int sx=(x*SCALE_FACT); //32th is 512/320 -> scale 512 mac screen to 320 width
	int sy=(y*SCALE_FACT);
	//sx and sy are now 27.5 fixed point values for the 'real' mac-like components
	int r,g,b;
	if (((x+y)&1)) {
		//pixel a
		r=findMacVal(data, sx, sy);
		b=findMacVal(data, sx+(SCALE_FACT/3)*2, sy);
		g=findMacVal(data, sx+(SCALE_FACT/3), sy+(SCALE_FACT/2));
	} else {
		//pixel b
		r=findMacVal(data, sx, sy+10);
		b=findMacVal(data, sx+(SCALE_FACT/3)*2, sy+(SCALE_FACT/1));
		g=findMacVal(data, sx+(SCALE_FACT/3), sy);
	}
	return ((r>>5)<<0)|((g>>4)<<5)|((b>>5)<<11);
}

#else
//Stupid 1-to-1 routine
static uint16_t IRAM_ATTR findPixelVal(uint8_t *data, unsigned x, unsigned y) {
	// Something is quite wrong here :(
	// data is the 512 x 342 x 1 bit image from the emulator
	// x and y varies from 0 to 319
	// return the color in RGB565 of the pixel at x, y

	// going 8 pixels to the right means incrementing by one byte
	// going 1 pixel down means incrementing by 512 / 8 = 64 bytes
	uint8_t tmp = data[x / 8 + y * 64];

	// to find the pixel we need to return the (x % 8)th bit
	uint8_t mask = 1 << (7 - (x & 7));
	return (tmp & mask) ? 0xffff : 0;
	// return (data[y * 64 + (x >> 3)] & (1 << ((7 - x) & 7))) ? 0 : 0xffff;
}

#endif

volatile static uint8_t *currFbPtr = NULL;
SemaphoreHandle_t dispSem = NULL;

// TODO completely glitched up display when this is changed to larger values than 32. Why?
#define LINESPERBUF 32

//Use this to move the display area down.
#define YOFFSET 0

static void IRAM_ATTR displayTask(void *arg) {
	uint8_t *img = malloc((LINESPERBUF * 320 * 2));
	assert(img);

	calcLut();

	uint8_t *oldImg = malloc(512 * 342 / 8);
	assert(oldImg);

	int firstrun = 1;
	setColRange(0, 319);

	while(1) {
		// mipiResync();

		// Wait for emulator to release the display memory
		xSemaphoreTake(dispSem, portMAX_DELAY);
		uint8_t *myData = (uint8_t*)currFbPtr;

		int ystart, yend;
		if (!firstrun) {
			for (ystart=0; ystart < 342; ystart++) {
				if (memcmp(oldImg + 64 * ystart, myData + 64 * ystart, 64) != 0)
					break;
			}
			for (yend = 342 - 1; yend >= ystart; --yend) {
				if (memcmp(oldImg + 64 * yend, myData + 64 * yend, 64) != 0)
					break;
			}
			if (ystart == 342) {
				//No need for update
				yend = 342;
			} else {
				//Only copy changed bits of data to changebuffer
				memcpy(oldImg + ystart * 64, myData + ystart * 64, (yend - ystart) * 64);

				ystart = (ystart * 32) / SCALE_FACT - 1;
				yend = (yend * 32) / SCALE_FACT + 2;
				if (ystart < 0)
					ystart = 0;
				// printf("disp: updating lines %d to %d\n", ystart, yend);
			}
		} else {
			ystart=0; yend=320;
		}
		memcpy(oldImg, myData, 512 * 342 / 8);

		if (ystart != yend) {
			// don't write too many lines into the framebuffer (it will wrap around)
			if (yend > 320)
				yend = 320;
			setRowRange(ystart + YOFFSET, 319);
			uint8_t cmd = 0x2c;  // start sending data
			uint8_t *p = img;
			int l = 0;
			for (int y=ystart; y<yend; y++) {
				for (int x=0; x<320; x++) {
					uint16_t v = findPixelVal(oldImg, x, y);
					*p++ = v;
					*p++ = v >> 8;
				}
				l++;
				if (l >= LINESPERBUF || y >= yend - 1) {
					mipiDsiSendLong(0x39, cmd, img, l * 320 * 2);
					cmd = 0x3c;  // continue sending data
					l = 0;
					p = img;
				}
			}
		}
		firstrun = 0;
	}
}

// Functions below are called by the emulator task

void dispDraw(uint8_t *mem) {
	currFbPtr = mem;
	xSemaphoreGive(dispSem);
}

void dispInit() {
	mipiInit();
	initOled();
    dispSem = xSemaphoreCreateBinary();
	xTaskCreatePinnedToCore(&displayTask, "display", 3000, NULL, 5, NULL, 1);
}
