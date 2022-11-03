#include <string.h>
#include "gw_def.h"
#include "gw_graphic.h"
#include "gw_system.h"
#include "gw_romloader.h"
#include "../cpu/sm510.h"


uint16 *gw_graphic_framebuffer = 0;
uint16 *source_mixer           = 0;
uint8   SEG_TRANSPARENT_COLOR  = 0;
bool    deflicker_enabled      = false;


void (*update_segment)(uint8 segment_nb, bool segment_state);


uint16 rgb_multiply_8bits(uint32 bg, uint32 sg)
{
    uint32 bg_r = (bg & GW_MASK_RGB565_R) >> 11;
    uint32 bg_g = (bg & GW_MASK_RGB565_G) >> 5;
    uint32 bg_b = (bg & GW_MASK_RGB565_B);

    bg_r = (bg_r * sg) >> 8;
    bg_g = (bg_g * sg) >> 8;
    bg_b = (bg_b * sg) >> 8;
    return (uint16)(bg_r << 11) | (bg_g << 5) | bg_b;
}
void update_segment_2bits(uint8 segment_nb, bool segment_state)
{
    uint32 segment = gw_segments_offset[segment_nb];
    uint8  cur_pixel, cur_pixelr;
    int    idx = 0;
    if (segment_state == 0)
        return;

    idx     = segment & 0x3;
    segment = segment >> 2;

    uint8 *pixel;
    pixel = &gw_segments[segment];

    uint16 segments_x      = gw_segments_x[segment_nb];
    uint16 segments_y      = gw_segments_y[segment_nb];
    uint16 segments_width  = gw_segments_width[segment_nb];
    uint16 segments_height = gw_segments_height[segment_nb];

    for (int line = segments_y; line < segments_height + segments_y; line++) {
        for (int x = segments_x; x < segments_width + segments_x; x++) {
            cur_pixelr = (pixel[idx >> 2] >> 2 * (idx & 0x3)) & 0x3;
            idx++;
            cur_pixel = cur_pixelr | cur_pixelr << 2 | cur_pixelr << 4 | cur_pixelr << 6;
            if (cur_pixel != SEG_TRANSPARENT_COLOR) {
                if (cur_pixel == 0)
                    cur_pixel = 39;
                gw_graphic_framebuffer[line * GW_SCREEN_WIDTH + x] =
                    rgb_multiply_8bits(source_mixer[line * GW_SCREEN_WIDTH + x], cur_pixel);
            }
        }
    }
}
void update_segment_4bits(uint8 segment_nb, bool segment_state)
{
    uint32 segment = gw_segments_offset[segment_nb];
    uint8  cur_pixel;
    int    idx = 0;

    if (segment_state == 0)
        return;

    idx     = segment & 0x1;
    segment = segment >> 1;
    uint8 *pixel;
    pixel = &gw_segments[segment];

    uint16 segments_x      = gw_segments_x[segment_nb];
    uint16 segments_y      = gw_segments_y[segment_nb];
    uint16 segments_width  = gw_segments_width[segment_nb];
    uint16 segments_height = gw_segments_height[segment_nb];

    for (int line = segments_y; line < segments_height + segments_y; line++) {
        for (int x = segments_x; x < segments_width + segments_x; x++) {
            if ((idx & 0x1) == 0)
                cur_pixel = pixel[idx >> 1] & 0xF0;
            else
                cur_pixel = pixel[idx >> 1] << 4;

            cur_pixel |= cur_pixel >> 4;
            idx++;

            if (cur_pixel != SEG_TRANSPARENT_COLOR)
                gw_graphic_framebuffer[line * GW_SCREEN_WIDTH + x] =
                    rgb_multiply_8bits(source_mixer[line * GW_SCREEN_WIDTH + x], cur_pixel);
        }
    }
}
void update_segment_8bits(uint8 segment_nb, bool segment_state)
{
    uint32 segment = gw_segments_offset[segment_nb];
    uint8  cur_pixel;
    int    idx = 0;

    if (segment_state == 0)
        return;

    uint8 *pixel;
    pixel = &gw_segments[segment];

    uint16 segments_x      = gw_segments_x[segment_nb];
    uint16 segments_y      = gw_segments_y[segment_nb];
    uint16 segments_width  = gw_segments_width[segment_nb];
    uint16 segments_height = gw_segments_height[segment_nb];

    for (int line = segments_y; line < segments_height + segments_y; line++) {
        for (int x = segments_x; x < segments_width + segments_x; x++) {
            cur_pixel = pixel[idx];
            idx++;
            if (cur_pixel != SEG_TRANSPARENT_COLOR)
                gw_graphic_framebuffer[line * GW_SCREEN_WIDTH + x] =
                    rgb_multiply_8bits(source_mixer[line * GW_SCREEN_WIDTH + x], cur_pixel);
        }
    }
}
void gw_gfx_sm510_rendering(uint16 *framebuffer)
{
    uint8 HxA, HxB, HxC;
    uint8 segment_position;
    uint8 segment_state;
    gw_graphic_framebuffer = framebuffer;

    if (gw_head.flags & FLAG_RENDERING_LCD_INVERTED) {
        memset(framebuffer, 0, GW_SCREEN_WIDTH * GW_SCREEN_HEIGHT * 2);
        SEG_TRANSPARENT_COLOR = SEG_BLACK_COLOR;
        source_mixer          = gw_background;
    } else {
        memcpy(framebuffer, gw_background, GW_SCREEN_WIDTH * GW_SCREEN_HEIGHT * 2);
        SEG_TRANSPARENT_COLOR = SEG_WHITE_COLOR;
        source_mixer          = framebuffer;
    }

    for (int seg_y = 0; seg_y < NB_SEGS_ROW; seg_y++) {
        if (deflicker_enabled) {
            HxA = gw_ram_state[ADD_SEGA_BASE + seg_y] & 0xf;
            HxB = gw_ram_state[ADD_SEGB_BASE + seg_y] & 0xf;
            HxC = gw_ram_state[ADD_SEGC_BASE + seg_y] & 0xf;
        } else {
            HxA = gw_ram[ADD_SEGA_BASE + seg_y] & 0xf;
            HxB = gw_ram[ADD_SEGB_BASE + seg_y] & 0xf;
            HxC = gw_ram[ADD_SEGC_BASE + seg_y] & 0xf;
        }

        for (int seg_z = 0; seg_z < NB_SEGS_COL; seg_z++) {
            segment_position = 4 * seg_y + seg_z;

            segment_state = m_bc || !m_bp ? 0 : (HxA & (1 << seg_z)) != 0;
            update_segment(segment_position, segment_state);

            segment_state = m_bc || !m_bp ? 0 : (HxB & (1 << seg_z)) != 0;
            update_segment(segment_position + 64, segment_state);

            segment_state = m_bc || !m_bp ? 0 : (HxC & (1 << seg_z)) != 0;
            update_segment(segment_position + 192, segment_state);
        }
    }

    for (int seg_z = 0; seg_z < NB_SEGS_COL; seg_z++) {
        uint8 blink   = (m_div & 0x4000) ? m_y : 0;
        uint8 seg     = (m_l & ~blink);
        segment_state = (m_bc || !m_bp) ? 0 : seg;
        update_segment(128 + seg_z, ((segment_state & (1 << seg_z)) != 0));
        seg           = (m_x & ~blink);
        segment_state = (m_bc || !m_bp) ? 0 : seg;
        update_segment(132 + seg_z, ((segment_state & (1 << seg_z)) != 0));
    }
}
void gw_gfx_sm500_rendering(uint16 *framebuffer)
{
    uint8 seg;
    gw_graphic_framebuffer = framebuffer;

    if (gw_head.flags & FLAG_RENDERING_LCD_INVERTED) {
        memset(framebuffer, 0, GW_SCREEN_WIDTH * GW_SCREEN_HEIGHT * 2);
        SEG_TRANSPARENT_COLOR = SEG_BLACK_COLOR;
        source_mixer          = gw_background;
    } else {
        memcpy(framebuffer, gw_background, GW_SCREEN_WIDTH * GW_SCREEN_HEIGHT * 2);
        SEG_TRANSPARENT_COLOR = SEG_WHITE_COLOR;
        source_mixer          = framebuffer;
    }

    for (int h = 0; h < 2; h++) {
        for (int o = 0; o < m_o_pins; o++) {
            if (deflicker_enabled)
                seg = h ? m_ox_state[o] : m_o_state[o];
            else
                seg = h ? m_ox[o] : m_o[o];

            update_segment(8 * o + 0 + h, m_bp ? ((seg & 0x1) != 0) : 0);
            update_segment(8 * o + 2 + h, m_bp ? ((seg & 0x2) != 0) : 0);
            update_segment(8 * o + 4 + h, m_bp ? ((seg & 0x4) != 0) : 0);
            update_segment(8 * o + 6 + h, m_bp ? ((seg & 0x8) != 0) : 0);
        }
    }
}
void gw_gfx_init()
{
    flag_lcd_deflicker_level = (gw_head.flags & FLAG_LCD_DEFLICKER_MASK) >> 6;
    deflicker_enabled        = (flag_lcd_deflicker_level != 0);

    update_segment = update_segment_8bits;
    if (gw_head.flags & FLAG_SEGMENTS_4BITS)
        update_segment = update_segment_4bits;

    if (gw_head.flags & FLAG_SEGMENTS_2BITS)
        update_segment = update_segment_2bits;
}
