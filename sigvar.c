
#include "globdef.h"
#include "sigdef.h"

// Repeat defines so we may notice mistakes.
#define CW_WORDSEP 255
#define CW_SPACE 254
#define CW_DOT 253
#define CW_DASH 252

int cw_item_len[256-CW_DASH]={4,2,2,4};
char *cw_item_text[256-CW_DASH]={"WORDSEP","SPACE","DOT","DASH"};

// Store character values. 
// 243 means an error. The code is not used and no used code can
// be made by adding dots or dashes.
// 242 is an incomplete code. At least one dot or dash must be
// appended to get a valid code.

unsigned char morsascii1[2]={
 'E',//.
'T'};//_
unsigned char morsascii2[4]={
 'I',//..
 'A',//._
 'N',//_.
'M'};//__
unsigned char morsascii3[8]={
 'S',//...
 'U',//.._
 'R',//._.
 'W',//.__
 'D',//_..
 'K',//_._
 'G',//__.
'O'};//___
unsigned char morsascii4[16]={
 'H',//....
 'V',//..._
 'F',//.._.
 198,//..__
 'L',//._..
 196,//._._
 'P',//.__.
 'J',//.___
 'B',//_...
 'X',//_.._
 'C',//_._.
 'Y',//_.__
 'Z',//__..
 'Q',//__._
 246,//___.
245};//____
unsigned char morsascii5[32]={
 '5',//.....
 '4',//...._
 247,//..._.
 '3',//...__
 243,//.._..
 243,//.._._
 '!',//..__.
 '2',//..___
 243,//._...
 242,//._.._
 199,//._._.
 243,//._.__
 243,//.__..
 197,//.__._
 243,//.___.
 '1',//.____
 '6',//_....
 '=',//_..._
 '/',//_.._.
 243,//_..__
 243,//_._..
 242,//_._._
 '(',//_.__.
 243,//_.___
 '7',//__...
 242,//__.._
 243,//__._.
 243,//__.__
 '8',//___..
 '9',//____.
'0'};//_____
unsigned char morsascii6[64]={
 243,//......
 243,//....._
 243,//...._.
 243,//....__
 243,//..._..
 200,//..._._
 243,//...__.
 243,//...___
 243,//.._...
 243,//.._.._
 243,//.._._.
 243,//.._.__
 '?',//..__..
 243,//..__._
 243,//..___.
 243,//..____
 243,//._....
 243,//._..._
 '"',//._.._.
 243,//._..__
 243,//._._..
 '.',//._._._
 243,//._.__.
 243,//._.___
 243,//.__...
 243,//.__.._
 243,//.__._.
 243,//.__.__
 243,//.___..
 243,//.____.
 243,//._____
 243,//_.....
 '-',//_...._
 243,//_..._.
 243,//_...__
 243,//_.._..
 243,//_.._._
 243,//_..__.
 243,//_..___
 243,//_._...
 243,//_._.._
 ';',//_._._.
 243,//_._.__
 243,//_.__..
 ')',//_.__._
 243,//_.___.
 243,//_.____
 243,//__....
 243,//__..._
 243,//__.._.
 ',',//__..__
 243,//__._..
 243,//__._._
 243,//__.__.
 243,//__.___
 ':',//___...
 243,//___.._
 243,//___._.
 243,//___.__
 243,//____..
 243,//_____.
243};//______

unsigned char *morsascii[6]={morsascii1, morsascii2, morsascii3, 
               morsascii4, morsascii5, morsascii6};               

float keying_spectrum_pos[KEYING_SPECTRUM_MAX];
float keying_spectrum_ampl[KEYING_SPECTRUM_MAX];
int keying_spectrum_ptr;
int keying_spectrum_max;
int keying_spectrum_cnt;
float keying_spectrum_f1;
int bg_no_of_notches;
int bg_current_notch;
int bg_notch_pos[MAX_BG_NOTCHES];
int bg_notch_width[MAX_BG_NOTCHES];

int cg_oscw;
int cg_osc_points;

int *baseband_handle;
float *baseb_out;
float *baseb;
float *baseb_raw;
float *baseb_wb_raw;
float *baseb_raw_orthog;
float *baseb_carrier;
float *baseb_carrier_ampl;
float *baseb_totpwr;
float *baseb_envelope;
float *baseb_upthreshold;
float *baseb_threshold;
float *baseb_fit;
float *baseb_tmp;
float *baseb_agc_level;
short int *baseb_ramp;
float *baseb_sho1;
float *baseb_sho2;
float carrfil_weight;
float bgfil_weight;
float *baseb_clock;

float reg_dot_power[5];
float reg_dot_re[5];
float reg_dot_im[5];
float reg_dash_power;
float reg_dash_re;
float reg_dash_im;
int dot_siz, dot_sizhalf, dash_siz, dash_sizhalf;
int cg_old_y1;
int cg_old_y2;
int cg_old_x1;
int cg_old_x2;



int keying_spectrum_size;
float *keying_spectrum;
char *mg_behind_meter;
short int *mg_rms_ypix;
short int *mg_peak_ypix;
float *mg_rms_meter;
float *mg_peak_meter;
short int mg_first_xpixel;
short int mg_last_xpixel;
short int mg_pa;
short int mg_px;
short int mg_size;
short int mg_mask;
short int mgw_p;
short int mg_ymin;
short int mg_ymax;
char mg_clear_flag;
double mg_dbscale_start;
double mg_scale_offset;
double mg_dbstep;
float mg_midscale;
int mg_valid;
int mg_bar;
char *mg_barbuf;
short int mg_bar_x1;
short int mg_bar_x2;
short int mg_y0;
short int mg_bar_y;

int cg_wave_start;
float cg_wave_midpoint;
float cg_wave_fit;
float cg_wave_coh_re;
float cg_wave_coh_im;
float cg_wave_raw_re;
float cg_wave_raw_im;
float cg_wave_dat;
float cg_wave_err;
float *basblock_maxpower;
float *basblock_avgpower;
char *daout;
float *cg_map;
short int *cg_traces;
float *mix2_tmp;
float *mix2_pwr;
float *fftn_tmp;
MORSE_DECODE_DATA *cw;
float *cw_carrier_window;
char *bg_volbuf;
float *bg_binshape;
float *bg_ytmp;
float *dash_waveform;

float *dash_wb_waveform;
float *dot_wb_waveform;
int dash_wb_ia;
int dash_wb_ib;
int dot_wb_ia;
int dot_wb_ib;


int cg_yzer[CG_MAXTRACE];
int cw_ptr;
int cw_detect_flag;
int max_cwdat;
int no_of_cwdat;
int correlate_no_of_cwdat;
int cw_decoded_chars;
float baseband_noise_level;
float carrier_noise_level;



double da_block_counter;
int poleval_pointer;
float baseband_sampling_speed;
int cg_no_of_traces;
int cg_lsep;
unsigned char cg_color[CG_MAXTRACE];
int cg_osc_ptr;
int cg_osc_offset;
int cg_osc_offset_inc;
int cg_osc_shift_flag;
int cg_max_trlevel;
float cg_code_unit;
float cg_decay_factor;
int cw_waveform_max;
int cw_avg_points;
int cg_update_interval;
int cg_update_count;
float cw_stoninv;

int baseband_size;
int baseband_mask;
int baseband_neg;
int baseband_sizhalf;
int baseb_pa;
int baseb_pb;
int baseb_pc;
int baseb_pd;
int baseb_pe;
int baseb_pf;
int baseb_ps;
int baseb_pm;
int baseb_px;
int baseb_py;
float baseb_fx;
int daout_size;
int new_baseb_flag;
float da_resample_ratio;
float new_da_resample_ratio;
int new_daout_upsamp;
int daout_pa;
int daout_pb;
int daout_px;
int rx_daout_block;
int tx_daout_block;
int daout_samps;
int daout_bufmask;
int flat_xpixel;
int curv_xpixel;
int bfo_xpixel;
int bfo10_xpixel;
int bfo100_xpixel;
double rx_daout_cos;
double rx_daout_sin;
double rx_daout_phstep_sin;
double rx_daout_phstep_cos;

float daout_gain;
int daout_gain_y;
int bfo_flag;
int da_wts;
int db_wts;
int rx_da_maxbytes;
int baseb_channels;
float baseb_wts;
float da_wttim;  
float baseb_wttim;
float db_wttim;
float da_wait_time;
POLCOFF poleval_data[POLEVAL_SIZE];
int bg_filtershift_points;
int bg_expand;
int bg_coherent;
int bg_delay;
int bg_da_channels;
int bg_da_bytes;
int bg_twopol;
float bg_expand_a;
float bg_expand_b;
float bg_amplimit;
float bg_agc_amplimit;
float bg_maxamp;
int bg_amp_indicator_x;
int bg_amp_indicator_y;
int da_ch2_sign;
int cg_size;
int dash_pts;
float dash_sumsq;
float cwbit_pts;
float refpwr;
int basblock_size;
int basblock_mask;
int basblock_hold_points;
int basblock_pa;
float s_meter_peak_hold;
float s_meter_fast_attack_slow_decay;
float s_meter_average_power;
float cg_maxavg;
float baseband_pwrfac;
float rx_agc_factor1;
float rx_agc_factor2;
float rx_agc_sumpow1;
float rx_agc_sumpow2;
float rx_agc_sumpow3;
float rx_agc_sumpow4;
float agc_attack_factor1;
float agc_attack_factor2;
float agc_release_factor;
float am_dclevel1;
float am_dclevel2;
float am_dclevel_factor1;
float am_dclevel_factor2;
int s_meter_avgnum;
float s_meter_avg;
