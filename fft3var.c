
#include "globdef.h"
#include "fft3def.h"

int yieldflag_ndsp_fft3;
int yieldflag_ndsp_mix2;
char *fft3_handle;
float *fft3;
COSIN_TABLE *fft3_tab;
unsigned short int *fft3_permute;
float *fft3_window;
float *fft3_tmp;
float *fft3_power;
float *fft3_fqwin_inv;
float *fft3_slowsum;
float *bg_filterfunc;
float *bg_carrfilter;
short int *fft3_spectrum;
short int *bg_filterfunc_y;
short int *bg_carrfilter_y;
float *timf3_float;
int *timf3_int;
short int *timf3_graph;
int fft3_n;
int fft3_size;
int fft3_block;
int fft3_totsiz;
int fft3_mask;
float fft3_blocktime;
int fft3_pa;
int fft3_px;
int fft3_show_size;
int fft3_slowsum_recalc;
int fft3_slowsum_cnt;
int bg_show_pa;

float fft3_interleave_ratio;
int fft3_interleave_points;
int fft3_new_points;
float *mix2_window;
float *mix2_cos2win;
float *mix2_sin2win;
int mix2_interleave_points;
int mix2_new_points;
int mix2_crossover_points;
float *basebraw_fir;
int basebraw_fir_pts;
float *basebwbraw_fir;
int basebwbraw_fir_pts;
float *basebcarr_fir;
int basebcarr_fir_pts;


int timf3_size;
int timf3_mask;
int timf3_y0[8];
float timf3_wttim;
float fft3_wttim;
float timf3_sampling_speed;
int timf3_osc_interval;
int timf3_block;
int timf3_pa;
int timf3_px;
int timf3_py;
int timf3_ps;
int timf3_wts;
float fft3_wtb;

