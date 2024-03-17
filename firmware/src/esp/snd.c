/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/dac_continuous.h"
#include "driver/gpio.h"

static dac_continuous_handle_t dac_handle;

// powers of 2 plzthx
#define BUFLEN 2048  // emulator --> sample ringbuffer
#define SND_CHUNKSZ 128  // ringbuffer --> DMA buffer chunksz
#define IO_AMP_DIS 2  // Amplifier disable pin

static uint8_t buf[BUFLEN];
static volatile int wp = 256, rp = 0;

static int bufLen() {
	return (wp - rp) & (BUFLEN - 1);
}

int sndDone() { // returns 1 when stuff can be written to buffer
//	printf("sndpoll %d\n", bufLen());
	return bufLen() < (BUFLEN - 400);
}

volatile static int myVolume;

void sndTask(void *arg) {
	static uint8_t tmpb[SND_CHUNKSZ] = {0};
	printf("Sound task started.\n");
	while (1) {
		int volume = (int)myVolume;
		if (volume < 0)
			volume = 0;

		if (volume > 7)
			volume = 7;

		for (int j=0; j<SND_CHUNKSZ; j++) {
			int s = buf[rp];
			s = ((s - 128) >> (7 - volume));
			// s = s / 16;
			tmpb[j] = s + 128;
			rp++;
			if (rp >= BUFLEN)
				rp = 0;
		}

		unsigned n_written = 0;
		ESP_ERROR_CHECK(dac_continuous_write(
			dac_handle, tmpb, sizeof(tmpb), &n_written, -1
		));
//		printf("snd %d\n", rp);
	}
}

int sndPush(uint8_t *data, int volume) {
	static int ampPowerTimeout = 0;

	while (!sndDone())
		usleep(1000);

	myVolume = volume;
	for (int i=0; i<370; i++) {
		buf[wp++] = volume ? *data : 128;
		data += 2;
		if (wp >= BUFLEN)
			wp = 0;
	}
	if (volume) {
		gpio_set_level(IO_AMP_DIS, 0);
		ampPowerTimeout = 11;
	} else {
		if (ampPowerTimeout == 1)
			gpio_set_level(IO_AMP_DIS, 1);
		if (ampPowerTimeout > 0)
			ampPowerTimeout--;
	}
	return 1;
}

void sndInit() {
    dac_continuous_config_t cont_cfg = {
        .chan_mask = DAC_CHANNEL_MASK_CH0,
        .desc_num = 8,
        .buf_size = SND_CHUNKSZ,
        .freq_hz = 22200,
        .offset = 0,  // DC offset?
        .clk_src = SOC_MOD_CLK_PLL_D2,
        /* Assume the data in buffer is 'A B C D E F'
         * DAC_CHANNEL_MODE_SIMUL:
         *      - channel 0: A B C D E F
         *      - channel 1: A B C D E F
         * DAC_CHANNEL_MODE_ALTER:
         *      - channel 0: A C E
         *      - channel 1: B D F
         */
        .chan_mode = DAC_CHANNEL_MODE_SIMUL,
    };
    // Allocate continuous channels
    ESP_ERROR_CHECK(dac_continuous_new_channels(&cont_cfg, &dac_handle));

    // Enable the continuous channels
    ESP_ERROR_CHECK(dac_continuous_enable(dac_handle));

    // Init GPIO to drive amplifier enable signal
	gpio_config_t io_conf_amp={
		.intr_type = GPIO_INTR_DISABLE,
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = 1,
		.pin_bit_mask = (1 << IO_AMP_DIS)
	};
	gpio_config(&io_conf_amp);

	xTaskCreatePinnedToCore(&sndTask, "snd", 6 * 1024, NULL, 5, NULL, 1);
}


