//Stuff for a host build of TME
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
#include <stdlib.h>
#include <string.h>
#include "esp_partition.h"
#include "scsi.h"

int image_read(disk_t *dsk, void *buf, uint32_t i, uint32_t n)
{
	esp_partition_t *part = (esp_partition_t *)dsk->ext;

	if ((i + n) > dsk->blocks)
		return 1;

	if (esp_partition_read(part, 512 * i, buf, 512 * n) != ESP_OK)
		return 1;

	return 0;
}

int image_write(disk_t *dsk, const void *buf, uint32_t i, uint32_t n)
{
	printf("image_write(%ld, %ld)\n", i, n);
	esp_partition_t *part = (esp_partition_t *)dsk->ext;
	const uint8_t *data = buf;
	// const unsigned erase_size = part->erase_size;
	const unsigned erase_size = 4096;

	if ((i + n) > dsk->blocks) {
		return (1);
	}

	uint8_t *secdat = malloc(erase_size);
	if (secdat == NULL)
		return 1;

	unsigned int lbaStart = i & (~7);
	unsigned int lbaOff = i & 7;

	if (esp_partition_read(part, lbaStart * 512, secdat, erase_size) != ESP_OK)
		return 1;

	if (esp_partition_erase_range(part, lbaStart * 512, erase_size) != ESP_OK)
		return 1;

	for (int i = 0; i < 512; i++)
		secdat[lbaOff * 512 + i] = data[i];

	if (esp_partition_write(part, lbaStart * 512, secdat, erase_size) != ESP_OK)
		return 1;

	free(secdat);

	return (0);
}

disk_t *disk_init(const char *part_name)
{
	disk_t *dsk = malloc(sizeof(disk_t));
	if (dsk == NULL) {
		return (NULL);
	}

	printf("disk_init(%s) ", part_name);
	const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, part_name);
	if (part == 0) {
		printf("*** couldn't find partition %s\n", part_name);
		return NULL;
	}
	// Get size of partition
	long cnt = part->size / 512;
	if (cnt <= 0) {
		printf("invalid size %ld\n", cnt);
		return (NULL);
	}
	printf("%ld blocks\n", cnt);

	dsk_init (dsk, (void *)part, cnt, 0, 0, 0);
	dsk->read = image_read;
	dsk->write = image_write;

	return dsk;
}
