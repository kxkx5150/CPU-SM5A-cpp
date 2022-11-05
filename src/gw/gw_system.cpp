#include <SDL2/SDL_keycode.h>
#include <cstdio>
#include <time.h>
#include <SDL2/SDL.h>
#include "gw_romloader.h"
#include "gw_system.h"
#include "gw_graphic.h"
#include "../cpu/sm510.h"
#include "../cpu/sm500.h"
#include "../data/o.h"


void (*device_reset)();
void (*device_start)();
void (*device_run)();
void (*device_blit)(u16 *_fb);

u32 gw_time_sync = 0;

// button
bool softkey_time_pressed  = 0;
bool softkey_alarm_pressed = 0;
bool softkey_A_pressed     = 0;
bool softkey_only          = 0;
bool set_watch             = false;

short ashBotones[1 << 12];

// sound
dma_trans_state dma_state;
unsigned char   mspeaker_data       = 0;
int             gw_audio_buffer_idx = 0;
bool            gw_audio_buffer_copied;

unsigned char gw_audio_buffer[GW_AUDIO_BUFFER_LENGTH * 2] = {0};
int16_t       audiobuffer_dma[0x800000]                   = {0};

uint32        LeftFIFOHeadPtr = 0, LeftFIFOTailPtr = 0, RightFIFOHeadPtr = 0, RightFIFOTailPtr = 0;
bool          bAudioYaInicializado = 0;
uint32_t      audio_mute;
SDL_AudioSpec desired;
uint16       *DACBuffer;

const uint8_t volume_tbl[GW_AUDIO_VOLUME_MAX + 1] = {
    (uint8_t)(UINT8_MAX * 0.00f),  (uint8_t)(UINT8_MAX * 0.06f), (uint8_t)(UINT8_MAX * 0.125f),
    (uint8_t)(UINT8_MAX * 0.187f), (uint8_t)(UINT8_MAX * 0.25f), (uint8_t)(UINT8_MAX * 0.35f),
    (uint8_t)(UINT8_MAX * 0.42f),  (uint8_t)(UINT8_MAX * 0.60f), (uint8_t)(UINT8_MAX * 0.80f),
    (uint8_t)(UINT8_MAX * 1.00f),
};


bool gw_system_config()
{
    gw_gfx_init();
    if (strncmp(gw_head.cpu_name, ROM_CPU_SM500, 5) == 0) {
        device_start = sm500_device_start;
        device_reset = sm500_device_reset;
        device_run   = sm500_execute_run;
        device_blit  = gw_gfx_sm500_rendering;
        return true;
    }
    if (strncmp(gw_head.cpu_name, ROM_CPU_SM5A, 5) == 0) {
        device_start = sm5a_device_start;
        device_reset = sm5a_device_reset;
        device_run   = sm5a_execute_run;
        device_blit  = gw_gfx_sm500_rendering;
        return true;
    }
    if (strncmp(gw_head.cpu_name, ROM_CPU_SM510, 5) == 0) {
        device_start = sm510_device_start;
        device_reset = sm510_device_reset;
        device_run   = sm510_execute_run;
        device_blit  = gw_gfx_sm510_rendering;
        return true;
    }
    if (strncmp(gw_head.cpu_name, ROM_CPU_SM511, 5) == 0) {
        device_start = sm510_device_start;
        device_reset = sm511_device_reset;
        device_run   = sm511_execute_run;
        device_blit  = gw_gfx_sm510_rendering;
        return (sm511_init_melody(gw_melody));
    }
    if (strncmp(gw_head.cpu_name, ROM_CPU_SM512, 5) == 0) {
        device_start = sm510_device_start;
        device_reset = sm511_device_reset;
        device_run   = sm511_execute_run;
        device_blit  = gw_gfx_sm510_rendering;
        return (sm511_init_melody(gw_melody));
    }
    return false;
}
void gw_system_reset()
{
    device_reset();
}
void gw_system_start()
{
    device_start();
}
void SDLSoundCallback(void *userdata, Uint8 *buffer, int length)
{
    int iCopyAux, iCopyTotal;
    if (LeftFIFOHeadPtr != LeftFIFOTailPtr) {

        int numLeftSamplesReady =
            (LeftFIFOTailPtr + (LeftFIFOTailPtr < LeftFIFOHeadPtr ? BUFFER_SIZE : 0)) - LeftFIFOHeadPtr;

        int numSamplesReady = numLeftSamplesReady;

        if (numSamplesReady > length / 2)
            numSamplesReady = length / 2;


        iCopyAux = BUFFER_SIZE - (LeftFIFOHeadPtr + numSamplesReady);
        if (iCopyAux >= 0) {
            memcpy(buffer, &DACBuffer[LeftFIFOHeadPtr], numSamplesReady * sizeof(uint16));
        } else {
            iCopyTotal = numSamplesReady + iCopyAux;
            memcpy(buffer, &DACBuffer[LeftFIFOHeadPtr], iCopyTotal * sizeof(uint16));
            memcpy(&buffer[iCopyTotal * sizeof(uint16)], &DACBuffer[0],
                   (numSamplesReady - iCopyTotal) * sizeof(uint16));
        }

        LeftFIFOHeadPtr = (LeftFIFOHeadPtr + numSamplesReady) % BUFFER_SIZE;
    }
}
void DACReset(void)
{
    LeftFIFOHeadPtr = LeftFIFOTailPtr = 0, RightFIFOHeadPtr = RightFIFOTailPtr = 1;
}
void DACInit(void)
{
    DACBuffer        = (uint16 *)malloc(BUFFER_SIZE * sizeof(uint16));
    desired.freq     = 11025;
    desired.format   = AUDIO_S16SYS;
    desired.channels = 1;
    desired.samples  = (11025 / 90);
    desired.callback = SDLSoundCallback;

    if (bAudioYaInicializado == 0) {
        if (SDL_OpenAudio(&desired, NULL) < 0) {
            exit(1);
        }
        bAudioYaInicializado = 1;
    }

    DACReset();
    SDL_PauseAudio(0);
}
void DACDone(void)
{
    SDL_PauseAudio(1);
    SDL_CloseAudio();
}
void gw_system_sound_init()
{
    memset(gw_audio_buffer, 0, sizeof(gw_audio_buffer));
    gw_audio_buffer_copied = false;
    gw_audio_buffer_idx    = 0;
    mspeaker_data          = 0;
}
void gw_sound_melody(u8 data)
{
    if (gw_audio_buffer_copied) {
        gw_audio_buffer_copied = false;
        gw_audio_buffer_idx    = gw_audio_buffer_idx - GW_AUDIO_BUFFER_LENGTH;

        if (gw_audio_buffer_idx < 0)
            gw_audio_buffer_idx = 0;

        if (gw_audio_buffer_idx != 0) {
            for (int i = 0; i < gw_audio_buffer_idx; i++)
                gw_audio_buffer[i] = gw_audio_buffer[i + GW_AUDIO_BUFFER_LENGTH];
        }
    }

    if (gw_melody != 0)
        mspeaker_data = data;
    else {
        switch (gw_head.flags & FLAG_SOUND_MASK) {
            case FLAG_SOUND_R1_PIEZO:
                mspeaker_data = data & 1;
                break;
            case FLAG_SOUND_R2_PIEZO:
                mspeaker_data = data >> 1 & 1;
                break;
            case FLAG_SOUND_R1R2_PIEZO:
                mspeaker_data = data & 3;
                break;
            case FLAG_SOUND_R1S1_PIEZO:
                mspeaker_data = (m_s_out & ~1) | (data & 1);
                break;
            case FLAG_SOUND_S1R1_PIEZO:
                mspeaker_data = (m_s_out & ~2) | (data << 1 & 2);
                break;
            default:
                mspeaker_data = data & 1;
        }
    }

    gw_audio_buffer[gw_audio_buffer_idx] = mspeaker_data;
    gw_audio_buffer_idx++;
}
void gw_writeR(u8 data)
{
    gw_sound_melody(data);
};
void vdXboxPlaySound(uint8_t *pucBuffer)
{
    memcpy(&DACBuffer[LeftFIFOHeadPtr], audiobuffer_dma, GW_AUDIO_BUFFER_LENGTH);
    LeftFIFOTailPtr = (LeftFIFOTailPtr + GW_AUDIO_BUFFER_LENGTH / 2) % BUFFER_SIZE;
}
void gw_sound_init()
{
    gw_system_sound_init();
    memset(audiobuffer_dma, 0, sizeof(audiobuffer_dma));
}
void gw_sound_submit()
{
    uint8_t volume = 7;
    int16_t factor = volume_tbl[volume];
    size_t  offset = (dma_state == DMA_TRANSFER_STATE_HF) ? 0 : GW_AUDIO_BUFFER_LENGTH;

    if (audio_mute || volume == ODROID_AUDIO_VOLUME_MIN) {
        for (int i = 0; i < GW_AUDIO_BUFFER_LENGTH; i++) {
            audiobuffer_dma[i + offset] = 0;
        }
    } else {
        for (int i = 0; i < GW_AUDIO_BUFFER_LENGTH; i++) {
            audiobuffer_dma[i + offset] = (factor) * (gw_audio_buffer[i] << 4);
        }
    }

    gw_audio_buffer_copied = true;
    vdXboxPlaySound((uint8_t *)audiobuffer_dma);
}
void gw_input_keyboard(SDL_Event *event, bool keyup)
{
    set_watch = false;

    switch (event->key.keysym.sym) {
        case SDLK_LEFT:
            if (keyup)
                ashBotones[GW_BUTTON_LEFT] = 0;
            else
                ashBotones[GW_BUTTON_LEFT] = 1;
            break;
        case SDLK_RIGHT:
            if (keyup)
                ashBotones[GW_BUTTON_RIGHT] = 0;
            else
                ashBotones[GW_BUTTON_RIGHT] = 1;
            break;
        case SDLK_UP:
            if (keyup)
                ashBotones[GW_BUTTON_UP] = 0;
            else
                ashBotones[GW_BUTTON_UP] = 1;
            break;
        case SDLK_DOWN:
            if (keyup)
                ashBotones[GW_BUTTON_DOWN] = 0;
            else
                ashBotones[GW_BUTTON_DOWN] = 1;
            break;
        case SDLK_q:
            if (keyup)
                ashBotones[GW_BUTTON_PAUSE] = 0;
            else
                ashBotones[GW_BUTTON_PAUSE] = 1;
            break;
        case SDLK_w:
            if (keyup)
                ashBotones[GW_BUTTON_POWER] = 0;
            else
                ashBotones[GW_BUTTON_POWER] = 1;
            break;
        case SDLK_z:
            if (keyup)
                ashBotones[GW_BUTTON_GAME] = 0;
            else
                ashBotones[GW_BUTTON_GAME] = 1;
            break;
        case SDLK_x:
            if (keyup)
                ashBotones[GW_BUTTON_TIME] = 0;
            else
                ashBotones[GW_BUTTON_TIME] = 1;
            break;
        case SDLK_a:
            if (keyup)
                ashBotones[GW_BUTTON_A] = 0;
            else
                ashBotones[GW_BUTTON_A] = 1;
            break;
        case SDLK_s:
            if (keyup)
                ashBotones[GW_BUTTON_B] = 0;
            else
                ashBotones[GW_BUTTON_B] = 1;
            break;
        case SDLK_c:
            if (keyup)
                ashBotones[GW_BUTTON_STIME] = 0;
            else
                ashBotones[GW_BUTTON_STIME] = 1;
            break;
        case SDLK_v:
            if (keyup)
                ashBotones[GW_BUTTON_SALARM] = 0;
            else
                ashBotones[GW_BUTTON_SALARM] = 1;
            break;


        case SDLK_t:
            set_watch = true;
            break;
        default:
            break;
    }
}
u32 gw_get_buttons()
{
    u32 hw_buttons = 0;
    if (!softkey_only) {
        if (gw_rotate) {
            hw_buttons |= ashBotones[GW_BUTTON_DOWN];
            hw_buttons |= ashBotones[GW_BUTTON_LEFT] << 1;
            hw_buttons |= ashBotones[GW_BUTTON_UP] << 2;
            hw_buttons |= ashBotones[GW_BUTTON_RIGHT] << 3;
        } else {
            hw_buttons |= ashBotones[GW_BUTTON_LEFT];
            hw_buttons |= ashBotones[GW_BUTTON_UP] << 1;
            hw_buttons |= ashBotones[GW_BUTTON_RIGHT] << 2;
            hw_buttons |= ashBotones[GW_BUTTON_DOWN] << 3;
        }


        hw_buttons |= ashBotones[GW_BUTTON_A] << 4;
        hw_buttons |= ashBotones[GW_BUTTON_B] << 5;

        hw_buttons |= ashBotones[GW_BUTTON_TIME] << 6;
        hw_buttons |= ashBotones[GW_BUTTON_GAME] << 7;

        hw_buttons |= ashBotones[GW_BUTTON_PAUSE] << 8;
        hw_buttons |= ashBotones[GW_BUTTON_POWER] << 9;

        hw_buttons |= ashBotones[GW_BUTTON_STIME] << 10;
        hw_buttons |= ashBotones[GW_BUTTON_SALARM] << 11;
    }
    // software keys
    hw_buttons |= ((unsigned int)softkey_A_pressed) << 4;
    hw_buttons |= ((unsigned int)softkey_time_pressed) << 10;
    hw_buttons |= ((unsigned int)softkey_alarm_pressed) << 11;

    return hw_buttons;
}
u8 gw_readB()
{
    unsigned int keys_pressed = gw_get_buttons() & 0xff;
    if (keys_pressed == 0)
        return 1;

    if (gw_keyboard[9] & keys_pressed)
        return 0;

    return 1;
}
u8 gw_readBA()
{
    unsigned int keys_pressed = gw_get_buttons() & 0xff;
    if (keys_pressed == 0)
        return 1;

    if (gw_keyboard[8] & keys_pressed)
        return 0;

    return 1;
}
u8 gw_readK(u8 io_S)
{
    u8  io_K           = 0;
    u32 key_soft_value = 0;
    u32 keys_pressed   = gw_get_buttons() & 0xffff;
    if (keys_pressed == 0)
        return set_watch ? 1 : 0;

    const unsigned int key_soft_time  = GW_BUTTON_B + GW_BUTTON_TIME;
    const unsigned int key_soft_alarm = GW_BUTTON_B + GW_BUTTON_GAME;

    if ((keys_pressed >> 10) & 1) {
        keys_pressed   = 0;
        key_soft_value = key_soft_time;
    }
    if ((keys_pressed >> 11) & 1) {
        keys_pressed   = 0;
        key_soft_value = key_soft_alarm;
    }

    for (int Sx = 0; Sx < 8; Sx++) {
        if (io_S == 0)
            io_S = 2;

        if (((io_S >> Sx) & 0x1) != 0) {
            if (key_soft_value != 0) {
                if (((gw_keyboard[Sx] & GW_MASK_K1) == (key_soft_value)))
                    io_K |= 0x1;
                if (((gw_keyboard[Sx] & GW_MASK_K2) == (key_soft_value << 8)))
                    io_K |= 0x2;
                if (((gw_keyboard[Sx] & GW_MASK_K3) == (key_soft_value << 16)))
                    io_K |= 0x4;
                if (((gw_keyboard[Sx] & GW_MASK_K4) == (key_soft_value << 24)))
                    io_K |= 0x8;
            } else {
                if (((gw_keyboard[Sx] & GW_MASK_K1) & (keys_pressed)) != 0)
                    if (((gw_keyboard[Sx] & GW_MASK_K1) != (key_soft_alarm)) &
                        ((gw_keyboard[Sx] & GW_MASK_K1) != (key_soft_time)))
                        io_K |= 0x1;

                if (((gw_keyboard[Sx] & GW_MASK_K2) & (keys_pressed << 8)) != 0)
                    if (((gw_keyboard[Sx] & GW_MASK_K2) != (key_soft_alarm << 8)) &
                        ((gw_keyboard[Sx] & GW_MASK_K2) != (key_soft_time << 8)))

                        io_K |= 0x2;
                if (((gw_keyboard[Sx] & GW_MASK_K3) & (keys_pressed << 16)) != 0)
                    if (((gw_keyboard[Sx] & GW_MASK_K3) != (key_soft_alarm << 16)) &
                        ((gw_keyboard[Sx] & GW_MASK_K3) != (key_soft_time << 16)))

                        io_K |= 0x4;
                if (((gw_keyboard[Sx] & GW_MASK_K4) & (keys_pressed << 24)) != 0)
                    if (((gw_keyboard[Sx] & GW_MASK_K4) != (key_soft_alarm << 24)) &
                        ((gw_keyboard[Sx] & GW_MASK_K4) != (key_soft_time << 24)))

                        io_K |= 0x8;
            }
        }
    }
    return io_K & 0xf;
}
gw_time_t gw_system_get_time()
{
    gw_time_t time = {0};
    if ((gw_head.time_hour_address_msb == 0) && (gw_head.time_hour_address_lsb == 0)) {
        time.hours = 127;
        return time;
    }

    if (gw_head.time_hour_msb_pm == 0) {
        time.hours = 127;
        return time;
    }

    u32 hour_msb = gw_ram[gw_head.time_hour_address_msb];
    u32 hour_lsb = gw_ram[gw_head.time_hour_address_lsb];
    u32 pm_flag  = gw_head.time_hour_msb_pm;

    time.minutes = (gw_ram[gw_head.time_min_address_msb] * 10) + gw_ram[gw_head.time_min_address_lsb];
    time.seconds = (gw_ram[gw_head.time_sec_address_msb] * 10) + gw_ram[gw_head.time_sec_address_lsb];

    if (hour_msb & pm_flag) {
        hour_msb = hour_msb & ~pm_flag;
        if ((hour_msb == 1) && (hour_lsb == 2))
            time.hours = 12;
        else
            time.hours = (10 * hour_msb) + 12 + hour_lsb;
    } else {
        if ((hour_msb == 1) && (hour_lsb == 2))
            time.hours = 0;
        else
            time.hours = (10 * hour_msb) + hour_lsb;
    }
    return time;
}
void gw_system_set_time(gw_time_t time)
{
    if ((gw_head.time_hour_address_msb == 0) & (gw_head.time_hour_address_lsb == 0))
        return;

    if ((time.hours > 0) && (time.hours < 12)) {
        gw_ram[gw_head.time_hour_address_msb] = time.hours / 10;
        gw_ram[gw_head.time_hour_address_lsb] = time.hours % 10;
    }

    if (time.hours == 0) {
        gw_ram[gw_head.time_hour_address_msb] = 1;
        gw_ram[gw_head.time_hour_address_lsb] = 2;
    }

    if (time.hours == 12) {
        gw_ram[gw_head.time_hour_address_msb] = gw_head.time_hour_msb_pm + 1;
        gw_ram[gw_head.time_hour_address_lsb] = 2;
    }

    if (time.hours > 12) {
        gw_ram[gw_head.time_hour_address_msb] = gw_head.time_hour_msb_pm + ((time.hours - 12) / 10);
        gw_ram[gw_head.time_hour_address_lsb] = (time.hours - 12) % 10;
    }

    gw_ram[gw_head.time_min_address_msb] = time.minutes / 10;
    gw_ram[gw_head.time_min_address_lsb] = time.minutes % 10;
    gw_ram[gw_head.time_sec_address_msb] = time.seconds / 10;
    gw_ram[gw_head.time_sec_address_lsb] = time.seconds % 10;
}
void gw_set_time()
{
    gw_time_t  t;
    time_t     timeValue;
    struct tm *tobj;
    time(&timeValue);
    tobj = localtime(&timeValue);

    t.hours   = tobj->tm_hour;
    t.minutes = tobj->tm_min;
    t.seconds = tobj->tm_sec;

    gw_system_set_time(t);
    printf("set time\n");
}
void gw_check_time()
{
    gw_time_t  t = {0};
    time_t     timeValue;
    struct tm *tobj;
    time(&timeValue);

    tobj      = localtime(&timeValue);
    t.hours   = tobj->tm_hour;
    t.minutes = tobj->tm_min;
    t.seconds = tobj->tm_sec;

    if ((t.seconds == 30) || (gw_time_sync == 0)) {
        gw_time_sync = 1;
        gw_system_set_time(t);
    }
}
int gw_system_run(int clock_cycles)
{
    m_k_active = (gw_get_buttons() != 0);
    if (m_clk_div == 2)
        m_icount += (clock_cycles / 2);

    if (m_clk_div == 4)
        m_icount += (clock_cycles / 4);

    device_run();
    return m_icount * m_clk_div;
}
void gw_mainloop(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *tex)
{

    softkey_time_pressed  = 0;
    softkey_alarm_pressed = 0;
    softkey_A_pressed     = 0;

    gw_sound_init();
    gw_system_config();
    gw_system_start();
    gw_system_reset();
    gw_check_time();
    gw_set_time();
    DACInit();

    printf("loop start\n");
    uint16_t  fb[GW_LCD_WIDTH * GW_LCD_WIDTH];
    uint32_t  last_tic = SDL_GetTicks();
    bool      Running  = true;
    uint64_t  count    = 0;
    SDL_Event Event;

    while (Running) {
        if ((SDL_GetTicks() - last_tic) >= 1000.0 / 160.0) {
            if (count % 2 == 0)
                gw_check_time();

            gw_system_run(GW_SYSTEM_CYCLES);
            gw_sound_submit();

            device_blit(fb);
            SDL_UpdateTexture(tex, NULL, fb, FRAME_PITCH);

            if (gw_rotate) {
                SDL_Rect srcrect;
                srcrect.x = 0;
                srcrect.y = 0;
                srcrect.w = GW_LCD_WIDTH;
                srcrect.h = GW_LCD_HEIGHT;
                SDL_RenderCopyEx(renderer, tex, &srcrect, NULL, 270, 0, SDL_FLIP_NONE);
            } else
                SDL_RenderCopy(renderer, tex, NULL, NULL);

            SDL_RenderPresent(renderer);
            last_tic = SDL_GetTicks();
        }

        while (SDL_PollEvent(&Event)) {
            switch (Event.type) {
                case SDL_QUIT:
                    Running = 0;
                    break;
                case SDL_KEYUP:
                    gw_input_keyboard(&Event, true);
                    break;
                case SDL_KEYDOWN: {
                    gw_input_keyboard(&Event, false);
                } break;
            }
        }
    }
}
int gw_init(int argc, char **argv)
{
    u8  *rom    = nullptr;
    bool romflg = false;
    int  size   = 0;

    if (argv[1]) {
        FILE *f = fopen(argv[1], "rb");
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        fseek(f, 0, SEEK_SET);
        rom   = new u8[size];
        int _ = fread(rom, size, 1, f);
        fclose(f);
        romflg = gw_romloader(rom, size);
        delete[] rom;
        if (!romflg)
            return 1;
    } else {
        size   = 58121;
        romflg = gw_romloader(gw_o_data, size);
    }

    if (argv[2]) {
        gw_rotate = true;
    }

    if (!romflg)
        return 1;

    int width  = GW_LCD_WIDTH;
    int height = GW_LCD_HEIGHT;
    if (gw_rotate) {
        width  = GW_LCD_WIDTH;
        height = GW_LCD_WIDTH;
    }

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window   *window   = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width * SCALE,
                                              height * SCALE, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture  *tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_TARGET, width, height);
    SDL_RenderSetScale(renderer, SCALE, SCALE);
    SDL_SetRenderTarget(renderer, NULL);

    gw_mainloop(window, renderer, tex);

    printf("finish\n");
    return 0;
}
