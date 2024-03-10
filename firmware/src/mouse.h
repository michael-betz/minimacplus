#pragma once

#define MOUSE_QXA (1<<0)
#define MOUSE_QXB (1<<1)
#define MOUSE_QYA (1<<2)
#define MOUSE_QYB (1<<3)
#define MOUSE_BTN (1<<4)

// Generates emulated encoder waveform, drives VIA_PORTB
void mouseMove(int dx, int dy, int btn);
void mouseTick();

// Reads real mouse position from hardware. Calls mouseMove()
void mouse_init();
void mouse_read();
