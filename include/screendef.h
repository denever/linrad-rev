#ifndef SCREENDEF_H
#define SCREENDEF_H

// Writing to the screen from different threads may cause 
// incorrect colours and under X11 or Windows the variables
// first_mempix and last_mempix may get incorrect values
// causing incomplete screen updates. During normal processing
// all screen writes are made from the screen thread which
// looks for changed values in sc[]. The screen thread updates
// sd[] so it knows whether the write has been done.
// Defines for sc and sd:
#define SC_WG_WATERF_INIT 0
#define SC_BG_WATERF_INIT 1
#define SC_SHOW_FFT1 2
#define SC_SHOW_FFT2 3
#define SC_SHOW_FFT3 4
#define SC_WG_WATERF_REDRAW 5
#define SC_BG_WATERF_REDRAW 6
#define SC_CG_REDRAW 7
#define SC_WG_BUTTONS 8
#define SC_FILL_AFC 9
#define SC_SHOW_AFC 10
#define SC_TIMF2_OSCILLOSCOPE 11
#define SC_BLANKER_INFO 12
#define SC_HG_FQ_SCALE 13
#define SC_HG_Y_SCALE 14
#define SC_BG_FQ_SCALE 15
#define SC_WG_FQ_SCALE 16
#define SC_BG_BUTTONS 17
#define SC_SHOW_POL 18
#define SC_FREQ_READOUT 19
#define SC_SHOW_CENTER_FQ 20
#define SC_SHOW_COHERENT 21
#define SC_AFC_CURSOR 22
#define SC_MG_REDRAW 23
#define SC_UPDATE_METER_GRAPH 24
#define SC_SHOW_KEYING_SPECTRUM 25
#define SC_COMPUTE_EME_DATA 26
#define SC_SHOW_TX_FQ 27
#define SC_SHOW_WHEEL 28
#define SC_HG_STONBARS_REDRAW 29
#define SC_RG_REDRAW 30
#define MAX_SC 31

extern unsigned int sc[MAX_SC];
extern unsigned int sd[MAX_SC];
#define MG_SCALE_SUNITS 0
#define MG_SCALE_DB 1
#define MG_SCALE_DBM 2
#define MG_SCALE_MAX 3




#define WG_DBSCALE_COLOR 1
#define AG_DBSCALE_COLOR 31
#define AG_SRC_RANGE_COLOR 16
#define AG_STON_RANGE_COLOR 34
#define AG_LOCK_RANGE_COLOR 35
#define HG_STON1_RANGE_COLOR 34
#define HG_STON2_RANGE_COLOR 35
#define HG_DBSCALE_COLOR 4

#define BG_GAIN_COLOR 35
#define BG_DBSCALE_COLOR 2
#define BG_INACTIVE_BUTTON_COLOR 1
#define BG_ACTIVE_BUTTON_COLOR 56
#define PG_BACKGROUND_COLOR 38
#define PC_CONTROL_COLOR 43
#define MG_BAR_COLOR 52


#define MIX1_MAINCUR_COLOR 32
#define MIX1_SUBCUR_COLOR 1
#define MIX1_NETCUR_COLOR 6
#define ADTEST_BACKGROUND_COLOR 33 

#define TXTEST_NARROW_COLOR 15
#define TXTEST_PEAK_DECAY_COLOR 14
#define TXTEST_PEAK_POWER_COLOR 10
#define TXTEST_WIDE_AVERAGE_COLOR 12



#define MAX_SVGA_PALETTE 59


// Note that these definitions give the order by which data is 
// stored into the graphtype arrays in screenvar.c
#define GRAPHTYPE_AG 0
#define GRAPHTYPE_WG 1
#define GRAPHTYPE_BG 2
#define GRAPHTYPE_HG 3
#define GRAPHTYPE_PG 4
#define GRAPHTYPE_CG 5
#define GRAPHTYPE_EG 6
#define GRAPHTYPE_FG 7
#define GRAPHTYPE_MG 8
#define GRAPHTYPE_TG 9
#define GRAPHTYPE_RG 10
#define GRAPHTYPE_NET 11
#define MAX_GRAPHTYPES 12

extern char *graphtype_names[MAX_GRAPHTYPES];
extern char *graphtype_parptr[MAX_GRAPHTYPES];
extern char **graphtype_partexts_int[MAX_GRAPHTYPES];
extern char **graphtype_partexts_float[MAX_GRAPHTYPES];
extern int graphtype_max_intpar[MAX_GRAPHTYPES];
extern int graphtype_max_floatpar[MAX_GRAPHTYPES];
extern int screen_last_line;
extern int waterfall_yield_interval;
extern int screen_loop_counter;
extern int screen_loop_counter_max;



extern int numinput_flag;
extern int numinput_xdiff;
extern int numinput_ydiff;
extern int numinput_xpix;
extern int numinput_ypix;
extern int numinput_curpos;
extern int numinput_int_data;
extern int numinput_chars;
extern float moon_az;
extern float moon_el;

extern float numinput_float_data;

extern char numinput_txt[MAX_TEXTPAR_CHARS+1];
extern int wg_waterf_sum_counter;
extern float *wg_waterf_sum;
extern int wg_waterf_ymove_ptr;
extern int wg_waterf_ymove_start;
extern int wg_waterf_yinc;
extern int wg_waterf_y1;
extern int wg_waterf_y2;
extern int wg_waterf_y;
extern float wg_waterf_cfac;
extern float wg_waterf_czer;
extern int wg_waterf_block;
extern int wg_waterf_lines;

extern short int *bg_waterf;
extern int max_bg_waterf_times;
extern WATERF_TIMES *bg_waterf_times;
extern int bg_waterf_ptr;
extern int bg_waterf_ptr2;
extern int bg_waterf_size;
extern int local_bg_waterf_ptr;
extern int local_bg_yborder;

extern int bg_waterf_sum_counter;
extern float *bg_waterf_sum;
extern int bg_waterf_ymove_ptr;
extern int bg_waterf_ymove_start;
extern int bg_waterf_yinc;
extern int bg_waterf_y1;
extern int bg_waterf_y2;
extern int bg_waterf_y;
extern float bg_waterf_cfac;
extern float bg_waterf_czer;
extern int bg_waterf_block;
extern int bg_waterf_lines;
extern float bg_waterf_yfac;



extern char *wg_background;
extern int wg_first_xpixel;
extern int wg_last_xpixel;
extern int wg_xpixels;
extern float wg_db_per_pixel;
extern float wg_yfac_power;
extern float wg_yfac_log;
extern float wg_hz_per_pixel;
extern float wg_first_frequency;
extern int wg_timestamp_counter;
extern double frequency_scale_offset;

extern char *bg_background;
extern int bg_flatpoints;
extern int bg_curvpoints;
extern int bg_6db_points;
extern int bg_60db_points;
extern int bg_120db_points;
extern int bg_first_xpixel;
extern int bg_last_xpixel;
extern int bg_first_xpoint;
extern int bg_xpoints;
extern int bg_xpixels;
extern int bg_timestamp_counter;
extern int bg_ymax;
extern int bg_y0;
extern int bg_avg_counter;
extern int bg_y1;
extern int bg_y2;
extern int bg_y3;
extern int bg_y4;

extern int bg_yborder_max;
extern int bg_yborder_min;
extern float bg_hz_per_pixel;
extern int bg_vol_x1;
extern int bg_vol_x2;

extern int *hires_handle;
extern char *hg_background;
extern char *hg_stonbuf;
extern short int *hg_spectrum;
extern int hg_size;
extern int hg_first_point;
extern int hg_last_point;
extern int hg_first_xpixel;
extern int hg_last_xpixel;
extern int hg_xpix1;
extern int hg_first_fq;
extern int hg_curx;
extern int hg_center;
extern int hg_cury0;
extern int hg_cury1;
extern int hg_cury2;
extern int hg_stonbars_ytop;
extern int hg_y0;
extern int hg_redraw_counter;
extern int hg_ymax;
extern float hg_db_per_pixel;
extern float hg_hz_per_pixel;
extern float hg_yfac_log;
extern float hg_yfac_power;
extern int hg_powersum_recalc;
extern int hg_recalc_pointer;
extern int hg_ston1_y;
extern int hg_ston2_y;
extern int hg_ston1_yold;
extern int hg_ston2_yold;
extern int hg_ston_y0;
extern int hg_ston1_x1;
extern int hg_ston1_x2;
extern int hg_ston2_x1;
extern int hg_ston2_x2;

extern int pg_x0;
extern int pg_y0;
extern int pg_oldx;
extern int pg_oldy;
extern float pg_pol_angle;
extern float pg_b;


extern AG_PARMS ag;
extern WG_PARMS wg;
extern HG_PARMS hg;
extern BG_PARMS bg;
extern PG_PARMS pg;
extern PG_PARMS dpg;
extern CG_PARMS cg;
extern MG_PARMS mg;
extern EG_PARMS eg;
extern FG_PARMS fg;
extern TG_PARMS tg;
extern RG_PARMS rg;

extern NET_PARMS net;


extern BUTTONS wgbutt[MAX_WGBUTT];
extern BUTTONS hgbutt[MAX_HGBUTT];
extern BUTTONS bgbutt[MAX_BGBUTT];
extern BUTTONS agbutt[MAX_AGBUTT];
extern BUTTONS pgbutt[MAX_PGBUTT];
extern BUTTONS cgbutt[MAX_CGBUTT];
extern BUTTONS egbutt[MAX_EGBUTT];
extern BUTTONS fgbutt[MAX_FGBUTT];
extern BUTTONS mgbutt[MAX_MGBUTT];
extern BUTTONS tgbutt[MAX_TGBUTT];
extern BUTTONS rgbutt[MAX_RGBUTT];
extern int no_of_scro;
extern unsigned char button_color;
extern SCREEN_OBJECT scro[];

extern int wg_flag;
extern int hg_flag;
extern int ag_flag;
extern int bg_flag;
extern int pg_flag;
extern int cg_flag;
extern int mg_flag;
extern int eg_flag;
extern int fg_flag;
extern int tg_flag;
extern int rg_flag;
extern int wg_fft_avg2num;

extern int cg_x0;
extern int cg_y0;
extern int cg_y1;
extern int cg_chirpx;
extern int cg_chirpy;

extern int cg_oldx;
extern int cg_oldy;


extern int eg_hsiz;
extern int eg_vsiz;
extern int eg_oldx;
extern int eg_oldy;
extern int eme_active_flag;

extern int fg_oldx;
extern int fg_oldy;
extern int fg_yborder;

extern int tg_oldx;
extern int tg_oldy;
extern int tg_yborder;
extern int tg_old_band;

extern int afc_curx;
extern int fftx_totmem;
extern int baseband_totmem;
extern int afc_totmem;
extern int hires_totmem;
extern int fft3_totmem;
extern int radar_totmem;
extern int s_meter_avg_filled_flag;



extern ROUTINE current_mouse_activity;
extern int mouse_active_flag;
extern unsigned char svga_palette[];
extern unsigned char color_scale[];
void make_button(int x, int y, BUTTONS *butt, int m, char chr);
void wg_error(char *txt, int line);
void new_fft1_averages(int ia, int ib);
void update_fft1_averages(void);
void mouse_on_wide_graph(void);
void mouse_on_hires_graph(void);
void mouse_on_afc_graph(void);
void mouse_on_baseband_graph(void);
void mouse_on_pol_graph(void);
void mouse_on_coh_graph(void);
void mouse_on_eme_graph(void);
void mouse_on_freq_graph(void);
void mouse_on_meter_graph(void);
void mouse_on_tx_graph(void);
void mouse_on_radar_graph(void);
void help_on_wide_graph(void);
void help_on_hires_graph(void);
void help_on_afc_graph(void);
void help_on_baseband_graph(void);
void help_on_pol_graph(void);
void help_on_coherent_graph(void);
void help_on_eme_graph(void);
void help_on_freq_graph(void);
void help_on_meter_graph(void);
void help_on_tx_graph(void);
void help_on_radar_graph(void);
void update_wg_spectrum(void);
void init_wide_graph(void);
void init_hires_graph(void);
void init_afc_graph(void);
void init_pol_graph(void);
void init_baseband_graph(void);
void init_coherent_graph(void);
void init_radar_graph(void);
void check_graph_placement(WG_PARMS *a);
void set_graph_minwidth(WG_PARMS *a);
void decrease_wg_pixels_per_points(void);
void decrease_hg_pixels_per_points(void);
void decrease_bg_pixels_per_points(void);
void increase_wg_pixels_per_points(void);
void get_numinput_chars(void);
void show_pol(void);
void make_wide_graph(int clear_old);
void show_coherent(void);

#endif
