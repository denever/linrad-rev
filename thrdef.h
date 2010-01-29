
#include "rusage.h"

#define THRFLAG_NOT_ACTIVE 0
#define THRFLAG_INIT 1
#define THRFLAG_ACTIVE 2
#define THRFLAG_IDLE 4
#define THRFLAG_RESET 5
#define THRFLAG_SEM_WAIT 7
#define THRFLAG_AWAIT_INPUT 10
#define THRFLAG_SEMCLEAR 11
#define THRFLAG_RETURNED 12
#define THRFLAG_INPUT_WAIT 13

#define THRFLAG_KILL -1

#define THREAD_RX_ADINPUT 0
#define THREAD_RX_RAW_NETINPUT 1
#define THREAD_RX_FFT1_NETINPUT 2
#define THREAD_RX_FILE_INPUT 3
#define THREAD_SDR14_INPUT 4
#define THREAD_RX_OUTPUT 5
#define THREAD_SCREEN 6
#define THREAD_TX_INPUT 7
#define THREAD_TX_OUTPUT 8
#define THREAD_WIDEBAND_DSP 9
#define THREAD_NARROWBAND_DSP 10
#define THREAD_USER_COMMAND 11
#define THREAD_TXTEST 12
#define THREAD_POWTIM 13
#define THREAD_RX_ADTEST 14
#define THREAD_CAL_IQBALANCE 15
#define THREAD_CAL_INTERVAL 16
#define THREAD_CAL_FILTERCORR 17
#define THREAD_TUNE 18
#define THREAD_LIR_SERVER 19
#define THREAD_PERSEUS_INPUT 20
#define THREAD_RADAR 21
#define THREAD_SECOND_FFT 22
#define THREAD_TIMF2 23
#define THREAD_MAX 24

extern char thread_command_flag[THREAD_MAX];
extern char thread_status_flag[THREAD_MAX];
extern int thread_waitsem[THREAD_MAX];
extern char mouse_thread_flag;
extern float thread_workload[THREAD_MAX];
extern double thread_tottim1[THREAD_MAX];
extern double thread_cputim1[THREAD_MAX];
extern double thread_tottim2[THREAD_MAX];
extern double thread_cputim2[THREAD_MAX];
#if RUSAGE_OLD != TRUE
extern int thread_pid[THREAD_MAX];
#endif

extern char *thread_names[THREAD_MAX];
extern int rxin_local_workload_reset;
extern int rx_input_thread;
extern int threads_running;


#define SEM_TIMF1 0
#define SEM_FFT1 1
#define SEM_RX_DASIG 2
#define SEM_SCREEN 3
#define SEM_FFT2 4
#define SEM_TIMF2 5
// semaphores 0 to SEM_KEYBOARD-1 are initiated in lir_sem_init()
// The following ones have to be initiated individually.
#define SEM_KEYBOARD 6
#define SEM_TXINPUT 7
#define SEM_MOUSE 8
#define MAX_LIRSEM 9

void linrad_thread_create(int no);
void linrad_thread_stop_and_join(int no);
void init_semaphores(void);

#if RUSAGE_OLD == TRUE
double lir_get_thread_time(void);
#else
double lir_get_thread_time(int no);
#endif
double lir_get_cpu_time(void);
