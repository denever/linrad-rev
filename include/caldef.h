#ifndef CALDEF_H
#define CALDEF_H

#define FOLDCORR_VERNR 0x3b5f0001
#define START_PULSES 4  
#define STON_LIMIT 20    
#define INIT_PULSENUM 8
#define BAL_MAX_SEG 256
#define BAL_AVGNUM 15
#define MAX_CAL_ARRAYS 25

extern int cal_update_ram(void);
extern void cal_initscreen(void);
extern void write_filcorr(int type);
extern void thread_cal_iqbalance(void);
extern int remove_iq_notch(void);
extern void write_iq_foldcorr(void);
extern void expand_foldcorr(float *x, float *tmp);
extern MEM_INF calmem[MAX_CAL_ARRAYS];
extern char *calmem_handle;
extern int caliq_clear_flag;

extern short int *cal_graph;
extern unsigned short int *cal_permute;
extern COSIN_TABLE *cal_table; 
extern float cal_ymax,cal_yzer;
extern float *cal_tmp;
extern float *cal_buf;
extern float *cal_buf2;
extern float *cal_buf3;
extern float *cal_buf4;
extern float *cal_buf5;
extern float *cal_buf6;
extern float *cal_buf7;
extern float *cal_fft1_desired;
extern float *cal_fft1_filtercorr;



extern float *cal_win;
extern int *bal_flag,*bal_pos;
extern float *bal_phsum, *bal_amprat;
extern int bal_updflag;
extern int bal_segments;
extern float *contracted_iq_foldcorr;
extern float cal_interval;
extern int bal_screen;
extern float cal_signal_level;
extern int cal_fft1_n;
extern int cal_fft1_size;


extern float cal_xgain;
extern float cal_ygain;
extern int cal_lowedge;
extern int cal_midlim;
extern int cal_domain;
extern void final_filtercorr_init(void);

#endif
