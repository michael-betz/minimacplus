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
#include "ncr.h"
#include "hd.h"

// typedef struct {
// 	FILE *f;
// 	int size;
// } HdPriv;

// const uint8_t inq_resp[95]={
// 	0, //HD
// 	0, //0x80 if removable
// 	0x49, //Obsolete SCSI standard 1 all the way
// 	0, //response version etc
// 	31, //extra data
// 	0,0, //reserved
// 	0, //features
// 	'A','P','P','L','E',' ',' ',' ', //vendor id
// 	'2','0','S','C',' ',' ',' ',' ', //prod id
// 	'1','.','0',' ',' ',' ',' ',' ', //prod rev lvl
// };

// static int hdScsiCmd(SCSITransferData *data, unsigned int cmd, unsigned int len, unsigned int lba, void *arg) {
// 	int ret=0;
// 	HdPriv *hd=(HdPriv*)arg;

// 	// for (int x=0; x<32; x++)
// 	// 	printf("%02X ", data->cmd[x]);
// 	// printf("\n");

// 	if (cmd==0x8 || cmd==0x28) { //read
// 		printf("HD:  Read %2d blocks from LBA %5d.\n", len, lba);

// 		fseek(hd->f, lba * 512, SEEK_SET);
// 		if (len * 512 > 32 * 1024) {
// 			printf("HD BUFFER OVERFLOW!!!\n");
// 			abort();
// 		}
// 		size_t r_ret = fread(data->data, 512, len, hd->f);
// 		if (r_ret != len) {
// 			printf("HD ERROR: Read %d blocks, got %ld blocks.\n", len, r_ret);
// 			return 0;
// 		}
// 		// hexdump(data->data, len * 512);
// 		ret = len * 512;
// 	} else if (cmd==0xA || cmd==0x2A) { // write
// 		printf("HD: Write %2d blocks   to LBA %5d.\n", len, lba);
// 		fseek(hd->f, lba*512, SEEK_SET);
// 		fwrite(data->data, 512, len, hd->f);
// 		ret = 0;
// 	} else if (cmd==0x12) { //inquiry
// 		printf("HD: Inquery\n");
// 		memcpy(data->data, inq_resp, sizeof(inq_resp));
// 		return 95;
// 	} else if (cmd==0x25) { //read capacity
// 		int lbacnt=hd->size/512;
// 		data->data[0]=(lbacnt>>24);
// 		data->data[1]=(lbacnt>>16);
// 		data->data[2]=(lbacnt>>8);
// 		data->data[3]=(lbacnt>>0);
// 		data->data[4]=0;
// 		data->data[5]=0;
// 		data->data[6]=2; //512
// 		data->data[7]=0;
// 		ret=8;
// 		printf("HD: Read capacity (%d)\n", lbacnt);
// 	} else {
// 		printf("********** hdScsiCmd: unrecognized command %x\n", cmd);
// 	}
// 	data->cmd[0]=0; //status
// 	data->msg[0]=0;
// 	return ret;
// }

// SCSIDevice *hdCreate() {
// 	const char* file = "hd.img";
// 	SCSIDevice *ret=malloc(sizeof(SCSIDevice));
// 	memset(ret, 0, sizeof(SCSIDevice));
// 	HdPriv *hd=malloc(sizeof(HdPriv));
// 	memset(hd, 0, sizeof(HdPriv));
// 	hd->f=fopen(file, "r+");
// 	if (hd->f<=0) {
// 		perror(file);
// 		exit(0);
// 	}
// 	hd->size = fseek(hd->f, 0, SEEK_END);
// 	ret->arg=hd;
// 	ret->scsiCmd=hdScsiCmd;
// 	return ret;
// }


int dsk_read_lba(disk_t *dsk, void *buf, uint32_t i, uint32_t n)
{
	esp_partition_t *part = (esp_partition_t *)dsk->ext;

	if ((i + n) > dsk->blocks)
		return 1;

	if (esp_partition_read(part, 512 * i, buf, 512 * n) != ESP_OK)
		return 1;

	return 0;
}

static int dsk_part_write (disk_t *dsk, const void *buf, uint32_t i, uint32_t n)
{
	esp_partition_t *part = (esp_partition_t *)dsk->ext;
	const uint8_t *data = buf;
	// unsigned erase_size = part->erase_size;
	const unsigned erase_size = 4096;

	if (dsk->readonly) {
		return (1);
	}

	if ((i + n) > dsk->blocks) {
		return (1);
	}

	// uint8_t *secdat = malloc(erase_size);
	uint8_t secdat[erase_size];
	// if (secdat == NULL)
	// 	return 1;

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

	// free(secdat);

	return (0);
}
