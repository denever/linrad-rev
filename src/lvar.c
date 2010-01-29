
#include <pthread.h>
#include <semaphore.h>
#include "globdef.h"
#include "lconf.h"
#include "thrdef.h"
#include "caldef.h"
#include "ldef.h"

pthread_mutex_t parport_lock;

audio_buf_info tx_da_info;
audio_buf_info tx_ad_info;
audio_buf_info rx_da_info;
sem_t sem_kill_all;
pthread_t thread_identifier_kill_all;
pthread_t thread_identifier_keyboard;
pthread_t thread_identifier_mouse;
pthread_t thread_identifier_main_menu;
char *behind_mouse;
pthread_t thread_identifier[THREAD_MAX];
sem_t lirsem[MAX_LIRSEM];
char lirsem_flag[MAX_LIRSEM];
int serport;

ROUTINE thread_routine[THREAD_MAX]={thread_rx_adinput,          //0
                                    thread_rx_raw_netinput,     //1
                                    thread_rx_fft1_netinput,    //2
                                    thread_rx_file_input,       //3
                                    thread_sdr14_input,         //4
                                    thread_rx_output,           //5
                                    thread_screen,              //6
                                    thread_tx_input,            //7
                                    thread_tx_output,           //8
                                    thread_wideband_dsp,        //9
                                    thread_narrowband_dsp,      //10
                                    thread_user_command,        //11
                                    thread_txtest,              //12
                                    thread_powtim,              //13
                                    thread_rx_adtest,           //14
                                    thread_cal_iqbalance,       //15
                                    thread_cal_interval,        //16
                                    thread_cal_filtercorr,      //17
                                    thread_tune,                //18
                                    thread_lir_server,          //19
                                    thread_perseus_input,       //20
                                    thread_radar,               //21
                                    thread_second_fft,          //22
                                    thread_timf2                //23
                                    };
char *eme_own_info_filename={"/home/emedir/own_info"};
char *eme_allcalls_filename={"/home/emedir/allcalls.dta"};
char *eme_emedta_filename={"/home/emedir/eme.dta"};
char *eme_dirskd_filename={"/home/emedir/dir.skd"};
char *eme_dxdata_filename={"/home/emedir/linrad_dxdata"};
char *eme_call3_filename={"/home/emedir/CALL3.TXT"};
char *eme_error_report_file={"/home/emedir/location_errors.txt"};
