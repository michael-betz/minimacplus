#pragma once

void tmeStartEmu(void *rom);
void tmeMouseMovement(int dx, int dy, int btn);

// Platform dependent (implementation can be found in ./esp or in ../sdl_sim)

// Return ram area, which is array of size TMERAMSIZE
unsigned char *ramInit();

// should be called every (emulated) second. Prints stats.
void printFps(unsigned program_counter);

unsigned int m68k_read_memory_16(unsigned int address);
unsigned int m68k_read_memory_32(unsigned int address);
