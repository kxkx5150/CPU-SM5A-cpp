#include "../gw/gw_def.h"
#include "sm510.h"


void sm510_reset_vector()
{
    do_branch(3, 7, 0);
}
void sm510_wakeup_vector()
{
    do_branch(1, 0, 0);
}
void sm510_update_segments_state()
{
    for (int address = 0; address < 128; address++)
        gw_ram_state[address] = gw_ram[address];
};
void sm510_op_lb()
{
    m_bm = (m_bm & 4) | (m_op & 3);
    m_bl = (m_op >> 2 & 3) | ((m_op & 0xc) ? 0xc : 0);
}
void sm510_op_lbl()
{
    m_bl = m_param & 0xf;
    m_bm = (m_param & m_datamask) >> 4;
}
void sm510_op_sbl()
{
}
void sm510_op_sbm()
{
}
void sm510_op_exbla()
{
    u8 a  = m_acc;
    m_acc = m_bl;
    m_bl  = a;
}
void sm510_op_incb()
{
    m_bl   = (m_bl + 1) & 0xf;
    m_skip = (m_bl == 0);
}
void sm510_op_decb()
{
    m_bl   = (m_bl - 1) & 0xf;
    m_skip = (m_bl == 0xf);
}
void sm510_op_atpl()
{
    m_pc = (m_prev_pc & ~0xf) | m_acc;
}
void sm510_op_rtn0()
{
    if (flag_lcd_deflicker_level < 2)
        sm510_update_segments_state();
    pop_stack();
}
void sm510_op_rtn1()
{
    sm510_op_rtn0();
    m_skip = true;
}
void sm510_op_t()
{
    m_pc = (m_pc & ~0x3f) | (m_op & 0x3f);
}
void sm510_op_tl()
{
    do_branch(m_param >> 6 & 3, m_op & 0xf, m_param & 0x3f);
}
void sm510_op_tml()
{
    push_stack();
    do_branch(m_param >> 6 & 3, m_op & 3, m_param & 0x3f);
}
void sm510_op_tm()
{
    m_icount--;
    push_stack();
    u8 idx = read_byte_program(m_op & 0x3f);
    do_branch(idx >> 6 & 3, 4, idx & 0x3f);
}
void sm510_op_exc()
{
    u8 a  = m_acc;
    m_acc = ram_r();
    ram_w(a);
    m_bm ^= (m_op & 3);
}
void sm510_op_bdc()
{
    m_bc = (m_c != 0);
}
void sm510_op_exci()
{
    sm510_op_exc();
    sm510_op_incb();
}
void sm510_op_excd()
{
    sm510_op_exc();
    sm510_op_decb();
}
void sm510_op_lda()
{
    m_acc = ram_r();
    m_bm ^= (m_op & 3);
}
void sm510_op_lax()
{
    if ((m_op & ~0xf) != (m_prev_op & ~0xf))
        m_acc = m_op & 0xf;
}
void sm510_op_ptw()
{
    m_write_s(m_w);
}
void sm510_op_wr()
{
    m_w = m_w << 1 | 0;
    update_w_latch();
}
void sm510_op_ws()
{
    m_w = m_w << 1 | 1;
    update_w_latch();
}
void sm510_op_kta()
{
    sm510_update_segments_state();
    m_acc = m_read_k() & 0xf;
}
void sm510_op_atbp()
{
    m_bp = m_acc & 1;
}
void sm510_op_atx()
{
    m_x = m_acc;
}
void sm510_op_atl()
{
    m_l = m_acc;
}
void sm510_op_atfc()
{
    m_y = m_acc;
}
void sm510_op_atr()
{
    m_r = m_acc;
}
void sm510_op_add()
{
    m_acc = (m_acc + ram_r()) & 0xf;
}
void sm510_op_add11()
{
    m_acc += ram_r() + m_c;
    m_c    = m_acc >> 4 & 1;
    m_skip = (m_c == 1);
    m_acc &= 0xf;
}
void sm510_op_adx()
{
    m_acc += (m_op & 0xf);
    m_skip = ((m_op & 0xf) != 10 && (m_acc & 0x10) != 0);
    m_acc &= 0xf;
}
void sm510_op_coma()
{
    m_acc ^= 0xf;
}
void sm510_op_rot()
{
    u8 c  = m_acc & 1;
    m_acc = m_acc >> 1 | m_c << 3;
    m_c   = c;
}
void sm510_op_rc()
{
    m_c = 0;
}
void sm510_op_sc()
{
    m_c = 1;
}
void sm510_op_tb()
{
    sm510_update_segments_state();
    m_skip = (m_read_b() != 0);
}
void sm510_op_tc()
{
    m_skip = !m_c;
}
void sm510_op_tam()
{
    m_skip = (m_acc == ram_r());
}
void sm510_op_tmi()
{
    m_skip = ((ram_r() & bitmask(m_op)) != 0);
}
void sm510_op_ta0()
{
    m_skip = !m_acc;
}
void sm510_op_tabl()
{
    m_skip = (m_acc == m_bl);
}
void sm510_op_tis()
{
    m_skip = !m_1s;
    m_1s   = false;
}
void sm510_op_tal()
{
    sm510_update_segments_state();
    m_skip = (m_read_ba() != 0);
}
void sm510_op_tf1()
{
    m_skip = ((m_div & 0x4000) != 0);
}
void sm510_op_tf4()
{
    m_skip = ((m_div & 0x0800) != 0);
}
void sm510_op_rm()
{
    ram_w(ram_r() & ~bitmask(m_op));
}
void sm510_op_sm()
{
    ram_w(ram_r() | bitmask(m_op));
}
void sm510_op_pre()
{
    m_melody_address    = m_param;
    m_melody_step_count = 0;
}
void sm510_op_sme()
{
    m_melody_rd |= 1;
}
void sm510_op_rme()
{
    m_melody_rd &= ~1;
}
void sm510_op_tmel()
{
    m_skip = ((m_melody_rd & 2) != 0);
    m_melody_rd &= ~2;
}
void sm510_op_skip()
{
}
void sm510_op_cend()
{
    m_halt = true;
}
void sm510_op_idiv()
{
    m_div = 0;
}
void sm510_op_dr()
{
    m_div &= 0x7f;
}
void sm510_op_dta()
{
    m_acc = m_div >> 11 & 0xf;
}
void sm510_op_clklo()
{
    m_clk_div = 4;
}
void sm510_op_clkhi()
{
    m_clk_div = 2;
}
void sm510_op_illegal()
{
    printf("unknown opcode $%02X at $%04X\n", m_op, m_prev_pc);
}
