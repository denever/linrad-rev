
#include "globdef.h"
#include "caldef.h"

short int *cal_graph;
unsigned short int *cal_permute;
COSIN_TABLE *cal_table; 
float *cal_win;
float *cal_buf;
float *cal_buf2;
float *cal_buf3;
float *cal_buf4;
float *cal_buf5;
float *cal_buf6;
float *cal_buf7;
float *cal_tmp;
float *cal_fft1_desired;
float *cal_fft1_filtercorr;

int *bal_flag;
int *bal_pos;
float *bal_phsum;
float *bal_amprat;
float *contracted_iq_foldcorr;
MEM_INF calmem[MAX_CAL_ARRAYS];
char *calmem_handle;
int bal_updflag;
int bal_segments;
int bal_screen;
float cal_ymax;
float cal_yzer;
float cal_xgain;
float cal_ygain;
int cal_lowedge;
int cal_midlim;
int cal_domain;
float cal_interval;
float cal_signal_level;
int cal_fft1_n;
int cal_fft1_size;
int caliq_clear_flag;
