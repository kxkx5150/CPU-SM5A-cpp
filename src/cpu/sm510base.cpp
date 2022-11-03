#include "../gw/gw_def.h"
#include "sm510.h"
#include "gw_romloader.h"
#include "gw_system.h"


int m_prgwidth;
int m_datawidth;
int m_prgmask;
int m_datamask;

un8 gw_ram[128];
un8 gw_ram_state[128];

u16  m_pc, m_prev_pc;
u16  m_op, m_prev_op;
u8   m_param;
int  m_stack_levels = 2;
u16  m_stack[4];
int  m_icount;
u8   m_acc;
u8   m_bl;
u8   m_bm;
bool m_sbm;
bool m_sbl;
u8   m_c;
bool m_skip;
u8   m_w, m_s_out;
u8   m_r, m_r_out;
int  m_r_mask_option;
bool m_k_active;
bool m_halt;
int  m_clk_div;
u8   m_melody_rd;
u8   m_melody_step_count;
u8   m_melody_duty_count;
u8   m_melody_duty_index;
u8   m_melody_address;
u16  m_div;
bool m_1s;
u8   flag_lcd_deflicker_level = 2;
u8   m_l, m_x;
u8   m_y;
u8   m_bp;
bool m_bc;
int  m_o_pins;
u8   m_ox[9];
u8   m_o[9];
u8   m_ox_state[9];
u8   m_o_state[9];
u8   m_cn;
u8   m_mx;
u8   trs_field;
u8   m_cb;
u8   m_s;

bool m_rsub;


void update_w_latch()
{
    m_write_s(m_w);
}
u8 ram_r()
{
    int blh     = (m_sbl) ? 8 : 0;
    int bmh     = (m_sbm) ? (1 << (m_datawidth - 1)) : 0;
    u8  address = (bmh | blh | m_bm << 4 | m_bl) & m_datamask;
    if ((m_stack_levels == 1) & (address > 0x4f))
        address &= 0x4f;
    return readb(address) & 0xf;
}
void ram_w(u8 data)
{
    int blh     = (m_sbl) ? 8 : 0;
    int bmh     = (m_sbm) ? (1 << (m_datawidth - 1)) : 0;
    u8  address = (bmh | blh | m_bm << 4 | m_bl) & m_datamask;
    if ((m_stack_levels == 1) & (address > 0x4f))
        address &= 0x4f;
    writeb(address, data & 0xf);
}
void pop_stack()
{
    m_pc = m_stack[0] & m_prgmask;
    for (int i = 0; i < m_stack_levels - 1; i++)
        m_stack[i] = m_stack[i + 1];
}
void push_stack()
{
    for (int i = m_stack_levels - 1; i >= 1; i--)
        m_stack[i] = m_stack[i - 1];
    m_stack[0] = m_pc;
}
void do_branch(u8 pu, u8 pm, u8 pl)
{
    m_pc = ((pu << 10) | (pm << 6 & 0x3c0) | (pl & 0x03f)) & m_prgmask;
}
u8 bitmask(u16 param)
{
    return 1 << (param & 3);
}
un8 read_byte_program(un16 rom_address)
{
    return *(gw_program + rom_address);
}
un8 readb(un8 ram_address)
{
    return gw_ram[ram_address];
}
void writeb(un8 ram_address, u8 ram_data)
{
    gw_ram[ram_address] = ram_data;
}
un8 m_read_k()
{
    return gw_readK(m_s_out);
}
un8 m_read_ba()
{
    return gw_readBA();
}
un8 m_read_b()
{
    return gw_readB();
}
void m_write_s(un8 data)
{
    m_s_out = data;
}
void m_write_r(un8 data)
{
    gw_writeR(data);
}
void increment_pc()
{
    int feed = ((m_pc >> 1 ^ m_pc) & 1) ? 0 : 0x20;
    m_pc     = feed | (m_pc >> 1 & 0x1f) | (m_pc & ~0x3f);
}
