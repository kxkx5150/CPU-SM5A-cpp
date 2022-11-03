#ifndef _GW_GRAPHIC_H_
#define _GW_GRAPHIC_H_
#include "gw_def.h"

#define NB_SEGS_COL   4
#define NB_SEGS_ROW   16
#define NB_SEGS_BUS   3
#define ADD_SEGA_BASE 0x60
#define ADD_SEGB_BASE 0x70
#define ADD_SEGC_BASE 0x50

#define GW_RGB565_32BITS 0x07E0F81F
#define GW_MASK_RGB565_R 0xF800
#define GW_MASK_RGB565_G 0x07E0
#define GW_MASK_RGB565_B 0x001F
#define SEG_WHITE_COLOR  0xff
#define SEG_BLACK_COLOR  0x0

void gw_gfx_init();
void gw_gfx_sm500_rendering(uint16 *framebuffer);
void gw_gfx_sm510_rendering(uint16 *framebuffer);

#endif
