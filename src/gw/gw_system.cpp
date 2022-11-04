#include <SDL2/SDL_keycode.h>
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

u32   gw_time_sync = 0;
short ashBotones[256];


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
void gw_system_sound_init()
{
}
void gw_sound_melody(u8 data)
{
}
void gw_writeR(u8 data)
{
    gw_sound_melody(data);
};
u32 gw_get_buttons()
{
    unsigned int hw_buttons = 0;
    hw_buttons |= ashBotones[GW_BUTTON_LEFT];
    hw_buttons |= ashBotones[GW_BUTTON_UP] << 1;
    hw_buttons |= ashBotones[GW_BUTTON_RIGHT] << 2;
    hw_buttons |= ashBotones[GW_BUTTON_DOWN] << 3;

    hw_buttons |= ashBotones[GW_BUTTON_A] << 4;
    hw_buttons |= ashBotones[GW_BUTTON_B] << 5;

    hw_buttons |= ashBotones[GW_BUTTON_TIME] << 6;
    hw_buttons |= ashBotones[GW_BUTTON_GAME] << 7;

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
    unsigned char io_K         = 0;
    unsigned int  keys_pressed = gw_get_buttons() & 0xff;
    if (keys_pressed == 0)
        return 0;

    for (int Sx = 0; Sx < 8; Sx++) {
        if (((io_S >> Sx) & 0x1) != 0) {
            if (((gw_keyboard[Sx] & GW_MASK_K1) & (keys_pressed)) != 0)
                io_K |= 0x1;
            if (((gw_keyboard[Sx] & GW_MASK_K2) & (keys_pressed << 8)) != 0)
                io_K |= 0x2;
            if (((gw_keyboard[Sx] & GW_MASK_K3) & (keys_pressed << 16)) != 0)
                io_K |= 0x4;
            if (((gw_keyboard[Sx] & GW_MASK_K4) & (keys_pressed << 24)) != 0)
                io_K |= 0x8;
        } else if (io_S == 0) {
            if (((gw_keyboard[1] & GW_MASK_K1) & (keys_pressed)) != 0)
                io_K |= 0x1;
            if (((gw_keyboard[1] & GW_MASK_K2) & (keys_pressed << 8)) != 0)
                io_K |= 0x2;
            if (((gw_keyboard[1] & GW_MASK_K3) & (keys_pressed << 16)) != 0)
                io_K |= 0x4;
            if (((gw_keyboard[1] & GW_MASK_K4) & (keys_pressed << 24)) != 0)
                io_K |= 0x8;
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
void gw_input_keyboard(SDL_Event *event, bool keyup)
{
    switch (event->key.keysym.sym) {
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




        default:
            break;
    }
}
void gw_mainloop(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *tex)
{
    gw_system_config();
    gw_system_start();
    gw_system_reset();
    gw_check_time();
    gw_set_time();

    uint16_t  fb[GW_LCD_WIDTH * GW_LCD_HEIGHT];
    uint32_t  last_tic = SDL_GetTicks();
    bool      Running  = true;
    uint64_t  count    = 0;
    SDL_Event Event;

    while (Running) {
        if ((SDL_GetTicks() - last_tic) >= 1000.0 / 160.0) {
            if (count % 2 == 0)
                gw_check_time();

            gw_system_run(GW_SYSTEM_CYCLES);

            device_blit(fb);
            SDL_UpdateTexture(tex, NULL, fb, FRAME_PITCH);
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
    int  size   = 0;
    bool romflg = false;

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

    if (!romflg)
        return 1;

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window   *window   = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, GW_LCD_WIDTH * SCALE,
                                              GW_LCD_HEIGHT * SCALE, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture  *tex =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_TARGET, GW_LCD_WIDTH, GW_LCD_HEIGHT);
    SDL_RenderSetScale(renderer, SCALE, SCALE);
    SDL_SetRenderTarget(renderer, NULL);

    gw_mainloop(window, renderer, tex);

    printf("finish\n");
    return 0;
}
