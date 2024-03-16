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
#include "scsi.h"

static void image_del (disk_t *dsk)
{
	FILE *fp = (FILE *)dsk->ext;
	fclose(fp);
}

static int image_read (disk_t *dsk, void *buf, uint32_t i, uint32_t n)
{
	// printf("hd_read %04x %d\n", i, n);
	FILE *fp = (FILE *)dsk->ext;

	if ((i + n) > dsk->blocks)
		return 1;

	if(fseek(fp, 512 * i, SEEK_SET) != 0) {
		perror("read-fseek failed");
		return 1;
	}

	n *= 512;
	size_t r = fread(buf, 1, n, fp);
	if (r != n) {
		printf("fread failed: %ld vs %d\n", r, n);
		return 1;
	}

	return 0;
}

static int image_write (disk_t *dsk, const void *buf, uint32_t i, uint32_t n)
{
	printf("hd_write %04x %d\n", i, n);
	FILE *fp = (FILE *)dsk->ext;

	if ((i + n) > dsk->blocks)
		return 1;

	if(fseek(fp, 512 * i, SEEK_SET) != 0) {
		perror("write-fseek failed");
		return 1;
	}

	n *= 512;
	size_t r = fwrite(buf, 1, n, fp);
	if (r != n) {
		printf("fwrite failed: %ld vs %d\n", r, n);
		return 1;
	}

	return 0;
}

disk_t *disk_init(const char *part_name)
{
	disk_t *dsk = malloc(sizeof(disk_t));
	if (dsk == NULL) {
		return (NULL);
	}

	printf("disk_init(%s) ", part_name);
	FILE *fp = fopen (part_name, "r+b");
	if (fp == NULL) {
		perror("couldn't open");
		return (NULL);
	}

	// Get size of file
	fseek (fp, 0, SEEK_END);
	long cnt = ftell(fp);
	cnt /= 512;
	if (cnt <= 0) {
		printf("invalid size %ld\n", cnt);
		fclose (fp);
		return (NULL);
	}

	printf("%ld blocks\n", cnt);

	dsk_init (dsk, (void *)fp, cnt, 0, 0, 0);
	dsk->type = PCE_DISK_RAW;
	dsk->readonly = 0;
	dsk->fname = NULL;  // part_name;
	dsk->del = image_del;
	dsk->read = image_read;
	dsk->write = image_write;
	return dsk;
}
