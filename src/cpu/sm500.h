#ifndef _SM500_H_
#define _SM500_H_
#include "../gw/gw_def.h"


void sm500_device_start();
void sm500_device_reset();
void sm500_execute_run();
void sm500_get_opcode_param();
void sm500_div_timer(int nb_inst);
void sm500_update_segments_state();
void sm500_reset_vector();
void sm500_wakeup_vector();
bool sm500_wake_me_up();

void shift_w();
u8   get_digit();
void set_su(u8 su);
u8   get_su();
int  get_trs_field();

void sm5a_device_start();
void sm5a_device_reset();
void sm5a_execute_run();

void sm500_op_lax();
void sm500_op_adx();
void sm500_op_rm();
void sm500_op_sm();
void sm500_op_exc();
void sm500_op_exci();
void sm500_op_lda();
void sm500_op_excd();
void sm500_op_tmi();
void sm500_op_skip();
void sm500_op_atr();
void sm500_op_add();
void sm500_op_add11();
void sm500_op_coma();
void sm500_op_exbla();
void sm500_op_tal();
void sm500_op_tb();
void sm500_op_tc();
void sm500_op_tam();
void sm500_op_tis();
void sm500_op_ta0();
void sm500_op_tabl();
void sm500_op_lbl();
void sm500_op_rc();
void sm500_op_sc();
void sm500_op_kta();
void sm500_op_decb();
void sm500_op_rtn1();
void sm500_op_cend();
void sm500_op_dta();
void sm500_op_illegal();
void sm500_op_lb();
void sm500_op_incb();
void sm500_op_sbm();
void sm500_op_rbm();
void sm500_op_comcb();
void sm500_op_rtn0();
void sm500_op_ssr();
void sm500_op_tr();
void sm500_op_trs();
void sm500_op_atbp();
void sm500_op_ptw();
void sm500_op_tw();
void sm500_op_pdtw();
void sm500_op_dtw();
void sm500_op_wr();
void sm500_op_ws();
void sm500_op_ats();
void sm500_op_exksa();
void sm500_op_exkfa();
void sm500_op_idiv();
void sm500_op_rmf();
void sm500_op_smf();
void sm500_op_comcn();
#endif
