#ifndef LDEF_H
#define LDEF_H

#include <linux/soundcard.h>

extern pthread_mutex_t parport_lock;
extern char uninit_mem_end;
extern int uninit_mem_begin;
extern audio_buf_info rx_da_info;
extern audio_buf_info tx_da_info;
extern audio_buf_info tx_ad_info;
extern pthread_t thread_identifier_kill_all;
extern pthread_t thread_identifier_keyboard;
extern pthread_t thread_identifier_mouse;
extern pthread_t thread_identifier_main_menu;
extern char *behind_mouse;
extern pthread_t thread_identifier[THREAD_MAX];
extern sem_t lirsem[MAX_LIRSEM];
extern char lirsem_flag[MAX_LIRSEM];
extern sem_t sem_kill_all;
extern ROUTINE thread_routine[THREAD_MAX];
extern int serport;

void mmxerr(void);
void graphics_init(void);
void remove_keyboard_and_mouse(void);
void thread_main_menu(void);
void thread_rx_adinput(void);
void thread_rx_raw_netinput(void);
void thread_rx_fft1_netinput(void);
void thread_rx_file_input(void);
void thread_rx_output(void);
void thread_screen(void);
void thread_tx_input(void);
void thread_tx_output(void);
void thread_wideband_dsp(void);
void thread_second_fft(void);
void thread_timf2(void);
void thread_narrowband_dsp(void);
void thread_user_command(void);
void thread_txtest(void);
void thread_powtim(void);
void thread_rx_adtest(void);
void thread_cal_interval(void);
void thread_cal_filtercorr(void);
void thread_sdr14_input(void);
void thread_tune(void);
void thread_kill_all(void);
void thread_keyboard(void);
void thread_mouse(void);
void thread_lir_server(void);
void thread_perseus_input(void);
void thread_radar(void);
int investigate_cpu(void);
void print_procerr(int xxprint);

#endif
