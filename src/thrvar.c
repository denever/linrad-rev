
#include "globdef.h"
#include "thrdef.h"
#include "rusage.h"


char thread_command_flag[THREAD_MAX];
char thread_status_flag[THREAD_MAX];
char mouse_thread_flag;
float thread_workload[THREAD_MAX];
double thread_tottim1[THREAD_MAX];
double thread_cputim1[THREAD_MAX];
double thread_tottim2[THREAD_MAX];
double thread_cputim2[THREAD_MAX];
#if RUSAGE_OLD != TRUE
int thread_pid[THREAD_MAX];
#endif


int rxin_local_workload_reset;
int rx_input_thread;
int threads_running;

int thread_waitsem[THREAD_MAX]={-1,            //0 = THREAD_RX_ADINPUT
                                -1,            //1 = THREAD_RX_RAW_NETINPUT
                                -1,            //2 = THREAD_RX_FFT1_NETINPUT
                                -1,            //3 = THREAD_RX_FILE_INPUT
                                -1,            //4 = THREAD_SDR14_INPUT
                                SEM_RX_DASIG,  //5 = THREAD_RX_OUTPUT
                                SEM_SCREEN,    //6 = THREAD_SCREEN
                                -1,            //7 = THREAD_TX_INPUT
                                SEM_TXINPUT,   //8 = THREAD_TX_OUTPUT
                                SEM_TIMF1,     //9 = THREAD_WIDEBAND_DSP
                                SEM_FFT1,      //10 = THREAD_NARROWBAND_DSP
                                SEM_KEYBOARD,  //11 = THREAD_USER_COMMAND
                                SEM_FFT1,      //12 = THREAD_TXTEST
                                SEM_FFT1,      //13 = THREAD_POWTIM
                                SEM_TIMF1,     //14 = THREAD_RX_ADTEST
                                SEM_FFT1,      //15 = THREAD_CAL_IQBALANCE
                                SEM_TIMF1,     //16 = THREAD_CAL_INTERVAL
                                SEM_FFT1,      //17 = THREAD_CAL_FILTERCORR
		                SEM_TIMF1,     //18 = THREAD_TUNE
                                -1,            //19 = THREAD_LIR_SERVER 
                                -1,            //20 = THREAD_PERSEUS_INPUT
                                SEM_FFT1,      //21 = THREAD_RADAR             
                                SEM_FFT2,      //22 = THREAD_SECOND_FFT
                                SEM_TIMF2      //23 = THREAD_TIMF2
		                };


char *thread_names[THREAD_MAX]={"RxAD",  //0 = THREAD_RX_ADINPUT
                                "Rxrn",  //1 = THREAD_RX_RAW_NETINPUT
                                "Rxfn",  //2 = THREAD_RX_FFT1_NETINPUT
                                "Rxfi",  //3 = THREAD_RX_FILE_INPUT
                                "SR14",  //4 = THREAD_SDR14_INPUT
                                "RxDA",  //5 = THREAD_RX_OUTPUT
                                "Scre",  //6 = THREAD_SCREEN
                                "TxAD",  //7 = THREAD_TX_INPUT
                                "TxDA",  //8 = THREAD_TX_OUTPUT
                                "Wdsp",  //9 = THREAD_WIDEBAND_DSP
                                "Ndsp",  //10 = THREAD_NARROWBAND_DSP
                                "Cmds",  //11 = THREAD_USER_COMMAND
                                "TxTe",  //12 = THREAD_TXTEST
                                "PowT",  //13 = THREAD_POWTIM
                                "ADte",  //14 = THREAD_RX_ADTEST
                                "C_IQ",  //15 = THREAD_CAL_IQBALANCE
                                "C_in",  //16 = THREAD_CAL_INTERVAL
                                "C_fi",  //17 = THREAD_CAL_FILTERCOR
                                "Tune",  //18 = THREAD_TUNE
                                "Serv",  //19 = THREAD_LIR_SERVER 
                                "Pers",  //20 = THREAD_PERSEUS_INPUT
                                "Radr",  //21 = THREAD_RADAR
                                "fft2",  //22 = THREAD_SECOND_FFT
                                "Tf2 "   //23 = THREAD_TIMF2
                                };

