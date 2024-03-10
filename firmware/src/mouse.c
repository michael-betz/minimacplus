/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */
#include <stdio.h>
#include "mouse.h"
#include "via.h"
#include "scc.h"

typedef struct {
	int dx, dy;
	int btn;
	int rpx, rpy;
} Mouse;

static const int quad[4] = {0x0, 0x1, 0x3, 0x2};

Mouse mouse;

#define MAXCDX 50

void mouseMove(int dx, int dy, int btn) {
	mouse.dx+=dx;
	mouse.dy+=dy;
	if (mouse.dx>MAXCDX) mouse.dx=MAXCDX;
	if (mouse.dy>MAXCDX) mouse.dy=MAXCDX;
	if (mouse.dx<-MAXCDX) mouse.dx=-MAXCDX;
	if (mouse.dy<-MAXCDX) mouse.dy=-MAXCDX;
	if (btn) mouse.btn=1; else mouse.btn=0;
}

void mouseTick() {
	if (mouse.dx > 0) {
		mouse.dx--;
		mouse.rpx--;
	}
	if (mouse.dx < 0) {
		mouse.dx++;
		mouse.rpx++;
	}
	if (mouse.dy > 0) {
		mouse.dy--;
		mouse.rpy++;
	}
	if (mouse.dy < 0) {
		mouse.dy++;
		mouse.rpy--;
	}

	unsigned reg = quad[mouse.rpx & 3];
	reg |= quad[mouse.rpy & 3] << 2;
	reg |= mouse.btn << 4;

	// printf("dx %d dy %d reg %x\n", mouse.dx, mouse.dy, reg);

	if (reg & MOUSE_BTN)
		viaClear(VIA_PORTB, (1 << 3));
	else
		viaSet(VIA_PORTB, (1 << 3));

	if (reg & MOUSE_QXB)
		viaClear(VIA_PORTB, (1 << 4));
	else
		viaSet(VIA_PORTB, (1 << 4));

	if (reg & MOUSE_QYB)
		viaClear(VIA_PORTB, (1 << 5));
	else
		viaSet(VIA_PORTB, (1 << 5));

	sccSetDcd(SCC_CHANA, reg & MOUSE_QXA);
	sccSetDcd(SCC_CHANB, reg & MOUSE_QYA);
}
