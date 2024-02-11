//Stuff for a host-build of TME
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
#include <SDL2/SDL.h>
#include "mouse.h"

const int SCREEN_WIDTH = 512;
const int SCREEN_HEIGHT = 342;

SDL_Window* win = NULL;
SDL_Surface* surf = NULL;
SDL_Surface* drwsurf=NULL;

void sdlDie() {
	printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
	exit(0);
}

void dispInit() {
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1");

	win=SDL_CreateWindow( "TME", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
	if (win == 0)
		sdlDie();

	drwsurf = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
	SDL_SetRelativeMouseMode(1);
}

void sdlDispAudioInit() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
		sdlDie();
}


void handleInput() {
	static int btn=0;
	SDL_Event ev;
	while(SDL_PollEvent(&ev)) {
		switch (ev.type) {
			case SDL_QUIT:
				exit(0);
				break;
			case SDL_MOUSEMOTION:
				mouseMove(ev.motion.xrel, ev.motion.yrel, btn);
				break;
			case SDL_MOUSEBUTTONUP:
				btn = 0;
				mouseMove(0, 0, btn);
				break;
			case SDL_MOUSEBUTTONDOWN:
				btn = 1;
				mouseMove(0, 0, btn);
				break;
		}
	}
}

void dispDraw(uint8_t *mem) {
	int x, y, z;
	SDL_LockSurface(drwsurf);
	uint32_t *pixels=(uint32_t*)drwsurf->pixels;
	for (y=0; y<SCREEN_HEIGHT; y++) {
		for (x=0; x<SCREEN_WIDTH; x+=8) {
			for (z=0x80; z!=0; z>>=1) {
				if (*mem&z) *pixels=0xFF000000; else *pixels=0xFFFFFFFF;
				pixels++;
			}
			mem++;
		}
	}
	SDL_UnlockSurface(drwsurf);
	surf = SDL_GetWindowSurface(win);
	SDL_BlitSurface(drwsurf, NULL, surf, NULL);
	SDL_UpdateWindowSurface(win);

	//Also handle mouse here.
	handleInput();
}

