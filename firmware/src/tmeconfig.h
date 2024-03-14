#pragma once

// for the RMA, on host-build we will use malloc()
// on the esp we will use malloc_caps()

#define TME_ROMSIZE (128 * 1024)

// Emulate an 4MiB MacPlus
#define TME_CACHESIZE (96*1024)
#define TME_RAMSIZE (4*1024*1024)
#define TME_SCREENBUF 0x3FA700
#define TME_SCREENBUF_ALT 0x3F2700
#define TME_SNDBUF 0x3FFD00

// Source: Guide to the Macintosh family hardware
#define TME_SNDBUF_ALT (TME_SNDBUF - 0x5C00)

// Skip the RAM test on boot
#define TME_DISABLE_MEMTEST 1

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
