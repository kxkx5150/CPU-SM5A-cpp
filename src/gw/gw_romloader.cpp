#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "gw_def.h"
#include "gw_system.h"
#include "gw_romloader.h"


gwromheader_t gw_head;

u8 gw_rom[GW_ROM_SIZE_MAX];

u8  *gw_rom_base        = NULL;
u16 *gw_background      = NULL;
u8  *gw_segments        = NULL;
u16 *gw_segments_x      = NULL;
u16 *gw_segments_y      = NULL;
u16 *gw_segments_width  = NULL;
u16 *gw_segments_height = NULL;
u32 *gw_segments_offset = NULL;
u8  *gw_program         = NULL;
u8  *gw_melody          = NULL;
u32 *gw_keyboard        = NULL;


unsigned long lz4_depack(const void *src, void *dst, unsigned long packed_size)
{
    const u8     *in               = (u8 *)src;
    u8           *out              = (u8 *)dst;
    unsigned long dst_size         = 0;
    unsigned long cur              = 0;
    unsigned long prev_match_start = 0;

    if (in[0] == 0) {
        return 0;
    }

    while (cur < packed_size) {
        unsigned long token   = in[cur++];
        unsigned long lit_len = token >> 4;
        unsigned long len     = (token & 0x0F) + 4;
        unsigned long offs;
        unsigned long i;
        if (lit_len == 15) {
            while (in[cur] == 255) {
                lit_len += 255;
                ++cur;
            }
            lit_len += in[cur++];
        }
        for (i = 0; i < lit_len; ++i) {
            out[dst_size++] = in[cur++];
        }
        if (cur == packed_size) {
            if (dst_size >= 5 && lit_len < 5) {
                return 0;
            }
            if (dst_size > 12 && dst_size - prev_match_start < 12) {
                return 0;
            }
            break;
        }
        offs = (unsigned long)in[cur] | ((unsigned long)in[cur + 1] << 8);
        cur += 2;
        if (len == 19) {
            while (in[cur] == 255) {
                len += 255;
                ++cur;
            }
            len += in[cur++];
        }
        prev_match_start = dst_size;
        for (i = 0; i < len; ++i) {
            out[dst_size] = out[dst_size - offs];
            ++dst_size;
        }
    }
    return dst_size;
}
u32 lz4_uncompress(const void *src, void *dst)
{
    const u8 *in  = (u8 *)src;
    u8       *out = (u8 *)dst;

    u32 uncompressed_size      = 0;
    u32 compressed_size        = 0;
    u32 original_size          = 0;
    u32 content_offset         = 0;
    u32 compressed_size_offset = 0;
    u8  flags;

    if (memcmp(&in[0], LZ4_MAGIC, LZ4_MAGIC_SIZE) == 0) {
        memcpy(&flags, &in[LZ4_FLG_OFFSET], sizeof(flags));

        if ((flags & LZ4_FLG_MASK_C_SIZE) != 0) {
            memcpy(&original_size, &in[LZ4_CONTENT_SIZE_OFFSET], sizeof(original_size));
            compressed_size_offset += LZ4_CONTENT_SIZE;
        }

        if ((flags & LZ4_FLG_MASK_DICTID) != 0) {
            compressed_size_offset += LZ4_DICTID_SIZE;
        }

        compressed_size_offset += LZ4_MAGIC_SIZE + LZ4_FLG_SIZE + LZ4_BD_SIZE + LZ4_HC_SIZE;
        content_offset += compressed_size_offset + LZ4_FRAME_SIZE;
        memcpy(&compressed_size, &in[compressed_size_offset], sizeof(compressed_size));
        uncompressed_size = lz4_depack(&in[content_offset], out, compressed_size);

        if ((flags & LZ4_FLG_MASK_C_SIZE) != 0) {
            if (uncompressed_size != original_size)
                uncompressed_size = 0;
        }
    }
    return uncompressed_size;
}
u32 lz4_get_original_size(const void *src)
{
    const u8 *in = (u8 *)src;

    u32 original_size = 0;
    u8  flags;

    if (memcmp(&in[0], LZ4_MAGIC, LZ4_MAGIC_SIZE) == 0) {
        memcpy(&flags, &in[LZ4_FLG_OFFSET], sizeof(flags));
        if ((flags & LZ4_FLG_MASK_C_SIZE) != 0)
            memcpy(&original_size, &in[LZ4_CONTENT_SIZE_OFFSET], sizeof(original_size));
    }
    return original_size;
}
bool gw_romloader(u8 *_rom, uint32 _len)
{
    u8 *src  = (u8 *)_rom;
    u8 *dest = (u8 *)gw_rom;

    u32 rom_size_src  = _len;
    u32 rom_size_dest = _len;

    memset(dest, 0xffff, sizeof(gw_rom));

    if (memcmp(src, ROM_CPU_SM510, 3) == 0) {
        printf("Not compressed : header OK\n");
        memcpy(dest, src, _len);
        rom_size_src = _len;

    } else if (memcmp(src, LZ4_MAGIC, 4) == 0) {
        rom_size_src = lz4_uncompress(src, dest);

        if ((memcmp(dest, ROM_CPU_SM510, 3) == 0)) {
            printf("ROM LZ4 : header OK\n");
        } else {
            printf("ROM LZ4 : header KO\n");
            return false;
        }
    } else {
        printf("Unknow ROM format\n");
        return false;
    }

    memcpy(&gw_head, dest, sizeof(gw_head));
    rom_size_dest = gw_head.keyboard + gw_head.keyboard_size;

    if (rom_size_src != rom_size_dest) {
        printf("ROM ERROR,size=%u,expected=%u\n", rom_size_src, rom_size_dest);
        return false;
    } else {
        printf("ROM size: OK\n");
    }

    if (gw_head.background_pixel_size != 0)
        gw_background = (u16 *)&gw_rom[gw_head.background_pixel];
    else
        gw_background = (u16 *)&gw_rom[rom_size_src];

    gw_segments        = (u8 *)&gw_rom[gw_head.segments_pixel];
    gw_segments_x      = (u16 *)&gw_rom[gw_head.segments_x];
    gw_segments_y      = (u16 *)&gw_rom[gw_head.segments_y];
    gw_segments_width  = (u16 *)&gw_rom[gw_head.segments_width];
    gw_segments_height = (u16 *)&gw_rom[gw_head.segments_height];
    gw_segments_offset = (u32 *)&gw_rom[gw_head.segments_offset];
    gw_program         = (u8 *)&dest[gw_head.program];
    gw_keyboard        = (u32 *)&gw_rom[gw_head.keyboard];

    if (gw_head.melody_size != 0)
        gw_melody = (u8 *)&gw_rom[gw_head.melody];

    return true;
}
