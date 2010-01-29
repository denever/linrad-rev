#ifndef FFT3DEF_H
#define FFT3DEF_H

extern int yieldflag_ndsp_fft3;
extern int yieldflag_ndsp_mix2;
extern int fft3_n;
extern int fft3_size;
extern int fft3_totsiz;
extern int fft3_mask;
extern int fft3_block;
extern float fft3_blocktime;
extern char *fft3_handle;
extern float *fft3;
extern float *fft3_tmp;
extern COSIN_TABLE *fft3_tab;
extern unsigned short int *fft3_permute;
extern short int *fft3_spectrum;
extern float *fft3_window;
extern float *fft3_power;
extern float *fft3_slowsum;
extern float *fft3_fqwin_inv;
extern float *bg_filterfunc;
extern float *bg_carrfilter;
extern short int *bg_filterfunc_y;
extern short int *bg_carrfilter_y;
extern int fft3_pa;
extern int fft3_px;
extern int fft3_show_size;
extern int fft3_slowsum_recalc;
extern int fft3_slowsum_cnt;
extern int bg_show_pa;

extern float fft3_interleave_ratio;
extern int fft3_interleave_points;
extern int fft3_new_points;
extern float *mix2_window;
extern float *mix2_cos2win;
extern float *mix2_sin2win;
extern int mix2_interleave_points;
extern int mix2_new_points;
extern int mix2_crossover_points;
extern float *basebraw_fir;
extern int basebraw_fir_pts;
extern float *basebwbraw_fir;
extern int basebwbraw_fir_pts;
extern float *basebcarr_fir;
extern int basebcarr_fir_pts;



extern int timf3_size;
extern int timf3_mask;
extern float *timf3_float;
extern int *timf3_int;
extern short int *timf3_graph;
extern int timf3_y0[];
extern float timf3_sampling_speed;
extern int timf3_osc_interval;
extern int timf3_block;
extern int timf3_pa;
extern int timf3_px;
extern int timf3_py;
extern int timf3_ps;
extern int timf3_wts;
extern float fft3_wtb;
extern float timf3_wttim;
extern float fft3_wttim;


#endif
