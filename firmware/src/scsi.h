/*****************************************************************************
 * pce                                                                       *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/arch/macplus/scsi.h                                      *
 * Created:     2007-11-13 by Hampa Hug <hampa@hampa.ch>                     *
 * Copyright:   (C) 2007-2014 Hampa Hug <hampa@hampa.ch>                     *
 *****************************************************************************/

/*****************************************************************************
 * This program is free software. You can redistribute it and / or modify it *
 * under the terms of the GNU General Public License version 2 as  published *
 * by the Free Software Foundation.                                          *
 *                                                                           *
 * This program is distributed in the hope  that  it  will  be  useful,  but *
 * WITHOUT  ANY   WARRANTY,   without   even   the   implied   warranty   of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  General *
 * Public License for more details.                                          *
 *****************************************************************************/

#pragma once

#include <stdint.h>

#define N_DSKS 1

enum {
	PCE_DISK_NONE,
	PCE_DISK_RAW,
	PCE_DISK_RAM,
	PCE_DISK_PCE,
	PCE_DISK_DOSEMU,
	PCE_DISK_COW,
	PCE_DISK_PSI,
	PCE_DISK_QED,
	PCE_DISK_PBI,
	PCE_DISK_CHD,
	PCE_DISK_PRI
};


struct disk_s;

typedef int (*dsk_read_f) (struct disk_s *dsk, void *buf, uint32_t i, uint32_t n);

typedef int (*dsk_write_f) (struct disk_s *dsk, const void *buf, uint32_t i, uint32_t n);

typedef int (*dsk_get_msg_f) (struct disk_s *dsk, const char *msg, char *val, unsigned max);
typedef int (*dsk_set_msg_f) (struct disk_s *dsk, const char *msg, const char *val);


typedef struct disk_s {
	unsigned      type;

	void          (*del) (struct disk_s *dsk);
	dsk_read_f    read;
	dsk_write_f   write;
	dsk_get_msg_f get_msg;
	dsk_set_msg_f set_msg;

	uint32_t      blocks;

	uint32_t      c;
	uint32_t      h;
	uint32_t      s;

	uint32_t      visible_c;
	uint32_t      visible_h;
	uint32_t      visible_s;

	char          readonly;

	char          *fname;

	void          *ext;
} disk_t;

typedef struct mac_scsi_s {
	unsigned      phase;

	unsigned char odr;
	unsigned char csd;
	unsigned char icr;
	unsigned char mr2;
	unsigned char tcr;
	unsigned char csb;
	unsigned char ser;
	unsigned char bsr;

	unsigned char status;
	unsigned char message;

	unsigned      cmd_i;
	unsigned      cmd_n;
	unsigned char cmd[16];

	unsigned long buf_i;
	unsigned long buf_n;
	unsigned long buf_max;
	unsigned char *buf;

	unsigned      sel_drv;

	unsigned long addr_mask;
	unsigned      addr_shift;

	void          (*cmd_start) (struct mac_scsi_s *scsi);
	void          (*cmd_finish) (struct mac_scsi_s *scsi);

	unsigned char set_int_val;
	void          *set_int_ext;
	void          (*set_int) (void *ext, unsigned char val);

	disk_t  	  *dev[8];  // NULL if invalid or points to a disk_t otherwise
} mac_scsi_t;


void mac_scsi_init (mac_scsi_t *scsi);
void mac_scsi_free (mac_scsi_t *scsi);

void mac_scsi_set_int_fct (mac_scsi_t *scsi, void *ext, void *fct);

unsigned char mac_scsi_get_uint8 (void *ext, unsigned long addr);
unsigned short mac_scsi_get_uint16 (void *ext, unsigned long addr);

void mac_scsi_set_uint8 (void *ext, unsigned long addr, unsigned char val);
void mac_scsi_set_uint16 (void *ext, unsigned long addr, unsigned short val);

void mac_scsi_reset (mac_scsi_t *scsi);

void dsk_init (disk_t *dsk, void *ext, uint32_t n, uint32_t c, uint32_t h, uint32_t s);


// Define how to read and write from image file and its size
disk_t *disk_init(const char *part_name);
