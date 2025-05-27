#pragma once

//IO pins
#define GPIO_D0N_LS 4
#define GPIO_D0P_LS 33
#define GPIO_D0_HS 32
#define GPIO_CLKP_LS 25
#define GPIO_CLKN_LS 27
#define GPIO_FF_NRST 14
#define GPIO_FF_CLK 12
#define GPIO_NRST 5

// convert RGB888 to RGB565
#define RGB(r, g, b) \
    (((b >> 3) & 0x1F) << 11) | \
    (((g >> 2) & 0x3F) << 5 ) | \
    ((r >> 3) & 0x1F)

void initOled();

// max range is 0, 319
void setColRange(int xstart, int xend);

// max range is 0, 319
void setRowRange(int ystart, int yend);

// Anything below 100 is basically off
void set_brightness(uint8_t val);

// coordinates are from 0 to 319
void fillRect(int x0, int x1, int y0, int y1, uint16_t color);
