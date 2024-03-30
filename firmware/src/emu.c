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
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdbool.h>
#include <byteswap.h>
#include "emu.h"
#include "tmeconfig.h"
#include "m68k.h"
#include "disp.h"
#include "iwm.h"
#include "via.h"
#include "scc.h"
#include "macrtc.h"
#include "scsi.h"
#include "snd.h"
#include "mouse.h"
#include "localtalk.h"

unsigned char *macRom;
unsigned char *macRam;

int rom_remap, video_remap=0, audio_remap=0, audio_volume=0, audio_en=0;

mac_scsi_t scsi;

void m68k_instruction() {
	unsigned int pc=m68k_get_reg(NULL, M68K_REG_PC);
	printf("Mon: %x\n", pc);
	int ok=0;
	if (pc < 0x400000) {
		if (rom_remap) {
			ok=1;
		}
	} else if (pc >= 0x400000 && pc<0x500000) {
		ok=1;
	}
	if (!ok) return;
	pc&=0x1FFFF;
	if (pc==0x7DCC) printf("Mon: SCSIReadSectors\n");
	if (pc==0x7E4C) printf("Mon: SCSIReadSectors exit OK\n");
	if (pc==0x7E56) printf("Mon: SCSIReadSectors exit FAIL\n");
}

typedef uint8_t (*PeripAccessCb)(unsigned int address, int data, int isWrite);

uint8_t unhandledAccessCb(unsigned int address, int data, int isWrite) {
	unsigned int pc=m68k_get_reg(NULL, M68K_REG_PC);
	printf("Unhandled %s @ 0x%X! PC=0x%X\n", isWrite?"write":"read", address, pc);
	return 0xff;
}

uint8_t bogusReadCb(unsigned int address, int data, int isWrite) {
	if (isWrite) return 0;
	return address^(address>>8)^(address>>16);
}

uint8_t scsiAccessCb(unsigned int address, int data, int isWrite) {
	if (isWrite) {
		mac_scsi_set_uint8(&scsi, address, data);
		return 0;
	} else {
		return mac_scsi_get_uint8(&scsi, address);
	}
}

uint8_t sscAccessCb(unsigned int address, int data, int isWrite) {
	if (isWrite) {
		sccWrite(address, data);
		return 0;
	} else {
		return sccRead(address);
	}
}

uint8_t iwmAccessCb(unsigned int address, int data, int isWrite) {
	if (isWrite) {
		iwmWrite((address>>9)&0xf, data);
		return 0;
	} else {
		return iwmRead((address>>9)&0xf);;
	}
}

uint8_t viaAccessCb(unsigned int address, int data, int isWrite) {
	if (isWrite) {
		viaWrite((address>>9)&0xf, data);
		return 0;
	} else {
		return viaRead((address>>9)&0xf);
	}
}

#define FLAG_RO (1 << 0)

typedef struct {
	uint8_t *memAddr;
	union {
		PeripAccessCb cb;
		int flags;
	};
} MemmapEnt;

#define MEMMAP_ES 0x20000 //entry size
#define MEMMAP_MAX_ADDR 0x1000000
//Memmap describing 128 128K blocks of memory, from 0 to 0x1000000 (16MiB).
MemmapEnt memmap[MEMMAP_MAX_ADDR/MEMMAP_ES];

static void regenMemmap(int remapRom) {
	int i;
	//Default handler
	for (i=0; i<MEMMAP_MAX_ADDR/MEMMAP_ES; i++) {
		memmap[i].memAddr=0;
		memmap[i].cb=unhandledAccessCb;
	}

	//0-0x400000 is RAM, or ROM when remapped
	if (remapRom) {
		memmap[0].memAddr=macRom;
		memmap[0].flags = FLAG_RO;
		for (i=1; i<0x400000/MEMMAP_ES; i++) {
			//Do not point at ROM again, but at... something else. Abuse RAM here.
			//If pointed at ROM again, ROM will think this machine does not have SCSI.
			memmap[i].memAddr=NULL;
			memmap[i].cb=bogusReadCb;
		}
	} else {
		for (i=0; i<0x400000/MEMMAP_ES; i++) {
			memmap[i].memAddr=&macRam[(i*MEMMAP_ES)&(TME_RAMSIZE-1)];
			memmap[i].flags = 0;
		}
	}

	//0x40000-0x50000 is ROM
	memmap[0x400000/MEMMAP_ES].memAddr=macRom;
	memmap[0x400000/MEMMAP_ES].flags = FLAG_RO;
	for (i=0x400000/MEMMAP_ES+1; i<0x500000/MEMMAP_ES; i++) {
		//Again, point to crap or SCSI won't work.
		memmap[i].memAddr=0;
		memmap[i].cb=bogusReadCb;
	}

	//0x580000-0x600000 is SCSI controller
	for (i=0x580000/MEMMAP_ES; i<0x600000/MEMMAP_ES; i++) {
		memmap[i].memAddr=NULL;
		memmap[i].cb=scsiAccessCb;
	}

	//0x600000-0x700000 is RAM
	for (i=0x600000/MEMMAP_ES; i<0x700000/MEMMAP_ES; i++) {
		memmap[i].memAddr=&macRam[(i*MEMMAP_ES)&(TME_RAMSIZE-1)];
		memmap[i].flags = 0;
	}

	//0x800000-0xC00000 is SSC
	for (i=0x800000/MEMMAP_ES; i<0xC00000/MEMMAP_ES; i++) {
		memmap[i].memAddr=NULL;
		memmap[i].cb=sscAccessCb;
	}

	//0xC00000-0xE00000 is IWM
	for (i=0xc00000/MEMMAP_ES; i<0xe00000/MEMMAP_ES; i++) {
		memmap[i].memAddr=NULL;
		memmap[i].cb=iwmAccessCb;
	}
	//0xE80000-0xF00000 is VIA
	for (i=0xE80000/MEMMAP_ES; i<0xF00000/MEMMAP_ES; i++) {
		memmap[i].memAddr=NULL;
		memmap[i].cb=viaAccessCb;
	}
}

uint8_t *macFb[2], *macSnd[2];

#define MMAP_RAM_PTR(ent, addr) &ent->memAddr[addr & (MEMMAP_ES - 1)]

const inline static MemmapEnt *getMmmapEnt(const unsigned int address) {
	if (address>=MEMMAP_MAX_ADDR) return &memmap[127];
	return &memmap[address/MEMMAP_ES];
}

unsigned int m68k_read_memory_8(unsigned int address) {
	const MemmapEnt *mmEnt=getMmmapEnt(address);
	if (mmEnt->memAddr) {
		uint8_t *p;
		p=(uint8_t*)MMAP_RAM_PTR(mmEnt, address);
		return *p;
	} else {
		return mmEnt->cb(address, 0, 0);
	}
}

unsigned int m68k_read_memory_16(unsigned int address) {
	const MemmapEnt *mmEnt = getMmmapEnt(address);
	// if ((address & 1) != 0)
	// 	printf("%s: Unaligned access to %x!\n", __FUNCTION__, address);

	if (mmEnt->memAddr) {
		uint16_t *p;
		p = (uint16_t*)MMAP_RAM_PTR(mmEnt, address);
		return __bswap_16(*p);
	} else {
		unsigned int ret;
		ret=mmEnt->cb(address, 0, 0)<<8;
		ret|=mmEnt->cb(address+1, 0, 0);
		return ret;
	}
}

unsigned int m68k_read_memory_32(unsigned int address) {
	uint16_t a=m68k_read_memory_16(address);
	uint16_t b=m68k_read_memory_16(address+2);
	return (a<<16)|b;
}

void m68k_write_memory_8(unsigned int address, unsigned int value) {
	const MemmapEnt *mmEnt=getMmmapEnt(address);
	if (mmEnt->memAddr) {
		uint8_t *p;
		p=(uint8_t*)MMAP_RAM_PTR(mmEnt, address);
		*p=value;
	} else {
		mmEnt->cb(address, value, 1);
	}
}

void m68k_write_memory_16(unsigned int address, unsigned int value) {
	const MemmapEnt *mmEnt = getMmmapEnt(address);
	// These printfs cause Panic with stack corruption at boot???
	// if ((address & 1) != 0)
	// 	printf("%s: Unaligned access to %x!\n", __FUNCTION__, address);

	if (mmEnt->memAddr) {
		if (mmEnt->flags & FLAG_RO) {
			printf("%s: %x is read-only!\n", __FUNCTION__, address);
			return;
		}
		uint16_t *p;
		p = (uint16_t*)MMAP_RAM_PTR(mmEnt, address);
		*p = __bswap_16(value);
	} else {
		mmEnt->cb(address, (value>>8)&0xff, 1);
		mmEnt->cb(address+1, (value>>0)&0xff, 1);
	}
}

void m68k_write_memory_32(unsigned int address, unsigned int value) {
	m68k_write_memory_16(address, value>>16);
	m68k_write_memory_16(address+2, value);
}

unsigned char *m68k_pcbase=NULL;

void m68k_pc_changed_handler_function(unsigned int address) {
//	printf("m68k_pc_changed_handler_function %x\n", address);
	const MemmapEnt *mmEnt=getMmmapEnt(address);
	if (mmEnt->memAddr) {
		uint8_t *p;
		p=(uint8_t*)MMAP_RAM_PTR(mmEnt, address);
		m68k_pcbase=p-address;
	} else {
		printf("PC not in mem!\n");
		abort();
	}
}

static void scsi_init()
{
	printf("Creating HD and registering it...\n");
	mac_scsi_init(&scsi);

	scsi.dev[SCSI_DEVICE0_ID] = disk_init(SCSI_DEVICE0_PART_NAME);
	if (scsi.dev[SCSI_DEVICE0_ID] == NULL) {
		printf("**** Couldn't get disk_0 :(\n");
	}

	// scsi.dev[SCSI_DEVICE1_ID] = disk_init(SCSI_DEVICE1_PART_NAME);
	// if (scsi.dev[SCSI_DEVICE1_ID] == NULL) {
	// 	printf("**** Couldn't get disk_1 :(\n");
	// }
}

void tmeStartEmu(void *rom) {
	int ca1=0, ca2=0;
	int x, frame=0;
	int cyclesPerSec=0;

	macRom = rom;
	macRam = ramInit();

	macFb[0] = &macRam[TME_SCREENBUF];
	macFb[1] = &macRam[TME_SCREENBUF_ALT];
	macSnd[0] = &macRam[TME_SNDBUF];
	macSnd[1] = &macRam[TME_SNDBUF_ALT];

	rom_remap = 1;
	regenMemmap(1);

	scsi_init();

	viaClear(VIA_PORTA, 0x7F);
	viaSet(VIA_PORTA, 0x80);
	viaClear(VIA_PORTA, 0xFF);
	viaSet(VIA_PORTB, (1<<3));
	sccInit();

	printf("Initializing m68k...\n");
	m68k_pc_changed_handler_function(0x0);
	m68k_init();

	printf("Setting CPU type and resetting...\n");
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	m68k_pulse_reset();

	printf("Sound and display init...\n");
	sndInit();
	dispInit();
	// localtalkInit();
	// mouse_init();

	// #if TME_DISABLE_MEMTEST
	// 	printf("disabling startup memory test\n");
	// 	// doesn't work, tries to write to read-only ROM!
	// 	m68k_write_memory_32(0x02ae, 0x00400000);
	// #endif

	printf("Done! Running.\n");
	while(1) {
		for (x=0; x<8000000/60; x+=1000) {
			m68k_execute(1000);

			// should run at 783.36KHz
			viaStep(100);
			sccTick(100);

			mouseTick();

			//Sound handler keeps track of real time, if its buffer is empty we should be done with the video frame.
			if (x > (8000000 / 120) && sndDone())
				break;
		}
		cyclesPerSec += x;

		// do these every frame
		dispDraw(macFb[video_remap ? 1 : 0]);
		// mouse_read();
		sndPush(macSnd[audio_remap ? 1 : 0], audio_en ? audio_volume : 0);
		localtalkTick();
		frame++;

		ca1 ^= 1;

		viaControlWrite(VIA_CA1, ca1);

		// every one second ...
		if (frame == 59) {
			ca2 ^= 1;
			viaControlWrite(VIA_CA2, ca2);
		}

		if (frame >= 60) {
			ca2 ^= 1;
			viaControlWrite(VIA_CA2, ca2);
			rtcTick();
			frame=0;
			printFps(m68k_get_reg(NULL, M68K_REG_PC));
			cyclesPerSec = 0;
		}
	}
}

void viaIrq(int req) {
	// printf("IRQ %d\n", req);
	m68k_set_irq(req ? 1 : 0);
}

void sccIrq(int req) {
	// printf("IRQ %d\n", req);
	m68k_set_irq(req ? 2 : 0);
}

void viaCbPortAWrite(unsigned int val) {
	static int writes = 0;
	if ((writes++) == 0)
		val = 0x67;
	// printf("VIA PORTA WRITE %x\n", val);
	video_remap = (val & (1 << 6)) ? 1 : 0;
	rom_remap = (val & (1 << 4)) ? 1 : 0;
	audio_remap = (val & (1 << 3)) ? 1 : 0;
	audio_volume = (val & 7);
	iwmSetHeadSel(val & (1 << 5));
	regenMemmap(rom_remap);
}

void viaCbPortBWrite(unsigned int val) {
	// printf("VIA PORTB WRITE %x\n", val);
	int b;
	b = rtcCom(val & 4, val & 1, val & 2);
	if (b)
		viaSet(VIA_PORTB, 1);
	else
		viaClear(VIA_PORTB, 1);
	audio_en = !(val & (1 << 7));
}
