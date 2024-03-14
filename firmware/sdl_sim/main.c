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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include "emu.h"
#include "tmeconfig.h"
#include "snd.h"
#include "disp.h"
#include "emu.h"
#include "macrtc.h"
#include "m68k.h"

static void *loadRom(char *file) {
	int i;
	char *ret = malloc(TME_ROMSIZE);
	int f = open(file, O_RDONLY);
	i = read(f, ret, TME_ROMSIZE);
	if (i <= 0) {
		printf("couldn't read %s\n", file);
		perror("reading rom");
		exit(-1);
	}
	printf("loaded 0x%06x bytes from %s into ROM\n", i, file);
	return ret;
}

void saveRtcMem(char *data) {
	FILE *f = fopen("pram.dat", "wb");
	if (f == NULL) {
		printf("Error saving RTC-mem to pram.dat\n");
		return;
	}
	fwrite(data, 32, 1, f);
	fclose(f);
}

//Should be called every second.
void printFps(unsigned cycles) {
	return;
	struct timeval tv;
	static struct timeval oldtv;
	gettimeofday(&tv, NULL);
	if (oldtv.tv_sec!=0) {
		long msec=(tv.tv_sec-oldtv.tv_sec)*1000;
		msec+=(tv.tv_usec-oldtv.tv_usec)/1000;
		unsigned pc = m68k_get_reg(NULL, M68K_REG_PC);
		printf(
			"pc: %08x, cycles: %6d, speed: %3d%%\n",
			pc,
			cycles,
			(int)(100000/msec)
		);
	}
	oldtv.tv_sec=tv.tv_sec;
	oldtv.tv_usec=tv.tv_usec;
}

int main(int argc, char **argv) {
	printf("Need rom.bin and hdd.img to work\n");

	void *rom = loadRom("rom.bin");
	FILE *f = fopen("pram.dat", "r");
	if (f != NULL) {
		char data[32];
		fread(data, 32, 1, f);
		rtcInit(data);
		fclose(f);
		printf("Loaded RTC data from pram.dat\n");
	}
	sdlDispAudioInit();
	tmeStartEmu(rom);
}
