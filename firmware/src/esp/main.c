/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_attr.h"

#include "spi_flash_mmap.h"

#include "rom/cache.h"
#include "rom/ets_sys.h"
#include "rom/spi_flash.h"
#include "rom/crc.h"

#include "soc/soc.h"
#include "soc/dport_reg.h"
#include "soc/io_mux_reg.h"
#include "soc/efuse_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_partition.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"

#include "emu.h"
#include "mouse.h"
#include "hexdump.h"
#include "tmeconfig.h"
#include "macrtc.h"
#include "wifi.h"
#include "static_ws.h"
#include "json_settings.h"

unsigned char *romdata;
nvs_handle nvs;

void emuTask(void *pvParameters)
{
	tmeStartEmu(romdata);
}

void saveRtcMem(char *data) {
	esp_err_t err;
	err = nvs_set_blob(nvs, "pram", data, 32);
	if (err!=ESP_OK) {
		printf("NVS: Saving to PRAM failed!");
	}
}

void app_main()
{
	const esp_partition_t* part;
	esp_err_t err;
	uint8_t pram[32];

	// Mount spiffs for *.html and defaults.json
	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 4,
		.format_if_mount_failed = true
	};
	esp_vfs_spiffs_register(&conf);

	// Load settings.json from SPIFFS, try to create file if it doesn't exist
	set_settings_file("/spiffs/settings.json", "/spiffs/default_settings.json");

	// load 32 bytes of PRAM from NVS
	nvs_flash_init();
	err = nvs_open("pram", NVS_READWRITE, &nvs);
	if (err != ESP_OK) {
		printf("NVS: Try erase\n");
		nvs_flash_erase();
		err = nvs_open("pram", NVS_READWRITE, &nvs);
	}

	unsigned int sz = 32;
	err = nvs_get_blob(nvs, "pram", pram, &sz);
	if (err == ESP_OK) {
		rtcInit((char*)pram);
	} else {
		printf("NVS: Cannot load pram!\n");
	}

	// find rom partition and memory-map it
	part = esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, "rom");
	if (part == NULL) {
		printf("*** couldn't find partition %s\n", "rom");
		assert(0);
	}

	spi_flash_mmap_handle_t hrom;
	ESP_ERROR_CHECK(esp_partition_mmap(
		part,
		0,
		TME_ROMSIZE,
		SPI_FLASH_MMAP_DATA,
		(const void**)&romdata,
		&hrom
	));

	printf("Starting emu...\n");
	xTaskCreatePinnedToCore(&emuTask, "emu", 6*1024, NULL, 1, NULL, 0);

	initWifi();
	tryConnect();
}

// called every second by emu.c
void printFps(unsigned cycles) {
	struct timeval tv;
	static struct timeval oldtv;
	gettimeofday(&tv, NULL);
	if (oldtv.tv_sec!=0) {
		long msec=(tv.tv_sec-oldtv.tv_sec)*1000;
		msec+=(tv.tv_usec-oldtv.tv_usec)/1000;
		printf(
			"cycles: %6d, speed: %3d%%, free: %3d kB_8, %3d kB_32\n",
			cycles,
			(int)(100000/msec),
			heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024,
			heap_caps_get_free_size(MALLOC_CAP_32BIT) / 1024
		);
	}
	oldtv.tv_sec=tv.tv_sec;
	oldtv.tv_usec=tv.tv_usec;

	// Feed task watchdog
	vTaskDelay(10 / portTICK_PERIOD_MS);
}



void ws_callback(uint8_t *payload, unsigned len)
{
	char tmpStr[256];
	char *pl = (char*)payload;

	printf("ws_callback(%d)\n", len);
	if (len < 1)
		return;

	// hexdump(payload, len);

	switch (pl[0]) {
		case 'i':  // 'i' command = RSSI
			int rssi = 0;
			esp_wifi_sta_get_rssi(&rssi);
			snprintf(tmpStr, sizeof(tmpStr), "{\"RSSI\": %d}", rssi);
			ws_send((uint8_t*)tmpStr, strnlen(tmpStr, sizeof(tmpStr)));
			break;

		case 'm':  // 'm,12,45,0' update mouse dx, dy, btn
			// split string into 4 tokens at ',' and convert to int
			long temp_dx=0, temp_dy=0, temp_btn=0;
			char *tok = NULL;
			for(unsigned tok_id = 0; tok_id <= 3; tok_id++) {
				tok = strsep(&pl, ",");
				if (tok == NULL) {
					printf("m - parse error!\n");
					return;
				}
				if (tok_id == 1) 		temp_dx = strtol(tok, NULL, 0);
				else if (tok_id == 2)	temp_dy = strtol(tok, NULL, 0);
				else if (tok_id == 3) 	temp_btn = strtol(tok, NULL, 0);
			}
			printf("mouse(%ld, %ld, %ld)\n", temp_dx, temp_dy, temp_btn);
			mouseMove(temp_dx, temp_dy, temp_btn);
			break;
	}
}
