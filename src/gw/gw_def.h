#ifndef __GW_TYPE_DEFS_H__
#define __GW_TYPE_DEFS_H__
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>


#define GW_LCD_WIDTH     320
#define GW_LCD_HEIGHT    240
#define GW_SCREEN_WIDTH  320
#define GW_SCREEN_HEIGHT 240
#define GW_REFRESH_RATE  128
#define SCALE            2
#define FRAME_PITCH      (GW_LCD_WIDTH * sizeof(uint16_t))

#define GW_SYS_FREQ            32768U
#define GW_AUDIO_FREQ          GW_SYS_FREQ
#define GW_AUDIO_BUFFER_LENGTH (GW_AUDIO_FREQ / GW_REFRESH_RATE)
#define GW_SYSTEM_CYCLES       (GW_AUDIO_FREQ / GW_REFRESH_RATE)

#define GW_BUTTON_LEFT  1
#define GW_BUTTON_UP    (1 << 1)
#define GW_BUTTON_RIGHT (1 << 2)
#define GW_BUTTON_DOWN  (1 << 3)
#define GW_BUTTON_A     (1 << 4)
#define GW_BUTTON_B     (1 << 5)
#define GW_BUTTON_TIME  (1 << 6)
#define GW_BUTTON_GAME  (1 << 7)
#define GW_BUTTON_PAUSE (1 << 8)
#define GW_BUTTON_POWER (1 << 9)

#define GW_MASK_K1    0x000000ff
#define GW_MASK_K2    0x0000ff00
#define GW_MASK_K3    0x00ff0000
#define GW_MASK_K4    0xff000000
#define GW_MAGIC_WORD "G&WS\x00\x00\x00\x05"

// rom
#define ZLIB_MAGIC "ZLIB"
#define LZMA_MAGIC "LZMA"

#define ROM_CPU_SM5A  "SM5A_"
#define ROM_CPU_SM500 "SM500"
#define ROM_CPU_SM510 "SM510"
#define ROM_CPU_SM511 "SM511"
#define ROM_CPU_SM512 "SM512"
#define ROM_CPU_SM520 "SM520"
#define ROM_CPU_SM530 "SM530"

#define FLAG_RENDERING_LCD_INVERTED 0x01
#define FLAG_SEGMENTS_4BITS         0x10
#define FLAG_SEGMENTS_2BITS         0x100
#define FLAG_BACKGROUND_JPEG        0x20
#define FLAG_SOUND_MASK             0xE

#define FLAG_SOUND_R1_PIEZO   1 << 1
#define FLAG_SOUND_R2_PIEZO   2 << 1
#define FLAG_SOUND_R1R2_PIEZO 3 << 1
#define FLAG_SOUND_R1S1_PIEZO 4 << 1
#define FLAG_SOUND_S1R1_PIEZO 5 << 1

#define FLAG_LCD_DEFLICKER_MASK 0xC0
#define FLAG_LCD_DEFLICKER_OFF  0x00
#define FLAG_LCD_DEFLICKER_1    0x40
#define FLAG_LCD_DEFLICKER_2    0x80
#define FLAG_LCD_DEFLICKER_3    0xC0

#define NB_SEGS 1000
#define GW_ROM_LZ4_SUPPORT
#define GW_ROM_SIZE_MAX (uint32_t)(400000)

#define GW_AUDIO_VOLUME_MAX     100
#define ODROID_AUDIO_VOLUME_MIN 0
#define BUFFER_SIZE             0x800000

typedef enum
{
    DMA_TRANSFER_STATE_HF = 0x00,
    DMA_TRANSFER_STATE_TC = 0x01,
} dma_trans_state;

typedef unsigned char  byte;
typedef unsigned char  un8, u8, uint8;
typedef unsigned short un16, u16, uint16;
typedef unsigned int   un32, u32, uint32;
typedef signed char    n8;
typedef signed short   n16;
typedef signed int     n32;
typedef un16           word;
typedef word           addr;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#endif
