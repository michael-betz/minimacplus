#pragma once

#define TME_ROMSIZE (128 * 1024)

// Emulate an 2 MiB MacPlus
#define TME_RAMSIZE (2 * 1024 * 1024)

// Source: Guide to the Macintosh family hardware
#define TME_SCREENBUF (TME_RAMSIZE - 0x5900)
#define TME_SCREENBUF_ALT (TME_SCREENBUF - 0x8000)
#define TME_SNDBUF (TME_RAMSIZE - 0x300)
#define TME_SNDBUF_ALT (TME_SNDBUF - 0x5C00)

// Skip the RAM test on boot
// #define TME_DISABLE_MEMTEST 1

#define SCSI_DEVICE_VENDOR "PCE     "
#define SCSI_DEVICE_PRODUCT "PCEDISK         "

#define SCSI_DEVICE0_ID 6
#define SCSI_DEVICE1_ID 7

#ifdef HOSTBUILD
    #define SCSI_DEVICE0_PART_NAME "hd7.img"
    #define SCSI_DEVICE1_PART_NAME ""
#else
    #define SCSI_DEVICE0_PART_NAME "hd"
    #define SCSI_DEVICE1_PART_NAME ""
#endif
