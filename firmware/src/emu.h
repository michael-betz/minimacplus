#pragma once

void tmeStartEmu(void *rom);
void tmeMouseMovement(int dx, int dy, int btn);

// Platform dependent (implementation can be found in ./esp or in ../sdl_sim)


// should be called every (emulated) second. Prints stats.
void printFps(unsigned cycles);
