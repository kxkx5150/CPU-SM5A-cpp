#ifndef _GW_SYSTEM_H_
#define _GW_SYSTEM_H_
#include "gw_def.h"


typedef struct gw_time_s
{
    u8 hours;
    u8 minutes;
    u8 seconds;
} gw_time_t;

int  gw_init(int argc, char **argv);
bool gw_system_config();
void gw_system_start();
void gw_system_reset();
int  gw_system_run(int clock_cycles);
void gw_system_sound_init();
bool gw_system_romload();
u32  gw_get_buttons();

void      gw_system_set_time(gw_time_t time);
gw_time_t gw_system_get_time();
void      gw_writeR(u8 data);
u8        gw_readK(u8 S);
u8        gw_readBA();
u8        gw_readB();


#endif
