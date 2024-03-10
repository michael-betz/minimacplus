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
