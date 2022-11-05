#ifndef _GW_ROMLOADER_H_
#define _GW_ROMLOADER_H_
#include "gw_def.h"
#include <stdint.h>
#include <unistd.h>

#define LZ4_MAGIC         "\x04\x22\x4D\x18"
#define LZ4_MAGIC_SIZE    4
#define LZ4_FLG_SIZE      1
#define LZ4_BD_SIZE       1
#define LZ4_CONTENT_SIZE  8
#define LZ4_DICTID_SIZE   4
#define LZ4_HC_SIZE       1
#define LZ4_ENDMARK_SIZE  4
#define LZ4_CHECKSUM_SIZE 4
#define LZ4_FRAME_SIZE    4

#define LZ4_FLG_MASK_DICTID     0x1
#define LZ4_FLG_MASK_C_CHECKSUM 0x4
#define LZ4_FLG_MASK_C_SIZE     0x8
#define LZ4_FLG_MASK_B_CHECKSUM 0x10

#define LZ4_FLG_OFFSET          (LZ4_MAGIC_SIZE)
#define LZ4_CONTENT_SIZE_OFFSET (LZ4_MAGIC_SIZE + LZ4_FLG_SIZE + LZ4_BD_SIZE)


typedef struct gwromheader_s
{
    char cpu_name[8];
    char rom_signature[8];
    u8   time_hour_address_msb;
    u8   time_hour_address_lsb;
    u8   time_min_address_msb;
    u8   time_min_address_lsb;
    u8   time_sec_address_msb;
    u8   time_sec_address_lsb;
    u8   time_hour_msb_pm;
    u8   byte_spare1;
    u32  flags;
    u32  background_pixel;
    u32  background_pixel_size;
    u32  segments_pixel;
    u32  segments_pixel_size;
    u32  segments_offset;
    u32  segments_offset_size;
    u32  segments_x;
    u32  segments_x_size;
    u32  segments_y;
    u32  segments_y_size;
    u32  segments_height;
    u32  segments_height_size;
    u32  segments_width;
    u32  segments_width_size;
    u32  melody;
    u32  melody_size;
    u32  program;
    u32  program_size;
    u32  keyboard;
    u32  keyboard_size;
} gwromheader_t;

extern u8  *gw_rom_base;
extern u16 *gw_background;
extern u8  *gw_segments;
extern u16 *gw_segments_x;
extern u16 *gw_segments_y;
extern u16 *gw_segments_width;
extern u16 *gw_segments_height;
extern u32 *gw_segments_offset;
extern u8  *gw_program;
extern u8  *gw_melody;
extern u32 *gw_keyboard;
extern bool gw_rotate;

extern gwromheader_t gw_head;

bool gw_romloader(u8 *rom, uint32 _len);
#endif
