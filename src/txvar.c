#include "globdef.h"
#include "txdef.h"

int tx_onoff_flag;
double tg_basebfreq;
int mic_key_pa;
int mic_key_px;
int screen_count;
int tx_lowest_bin;
int tx_highest_bin;
char *clipfft_mute;
char *cliptimf_mute;
char *alctimf_mute;
float *tx_mic_key;
float micfft_winfac;
int tg_old_y1;
int tg_old_y2;
int tg_old_x1;
int tg_old_x2;
float tx_ssbproc_fqshift;
float tx_hware_fqshift;
int mic_key_bufsiz;
int mic_key_block;
int mic_key_mask;
int tx_indicator_first_pixel;
int old_cw_rise_time;
double tx_daout_cos;
double tx_daout_sin;
double tx_daout_phstep_sin;
double tx_daout_phstep_cos;
int mic_key_indicator_block;
float rg_old_time;
float rg_old_gain;
float rg_old_zero;


int tx_flag;
int tx_graph_scro;
int txmem_size;
int tx_ad_maxamp;
int tx_input_mode;
float tx_ideal_delay;
float tx_ssb_resamp_block_factor;

SOUNDCARD_SIZES txda;
SOUNDCARD_SIZES txad;

float tx_resamp_ampfac1;
float tx_resamp_ampfac2;
int *txmem_handle;
int tx_audio_in;
int tx_audio_out;
short int *txtrace;
float txad_hz_per_bin;
float txtrace_gain;
int txtrace_height;
float micfft_bin_minpower_ref;
float micfft_bin_minpower;
float micfft_minpower_ref;
float micfft_minpower;
float *mic_filter;
int *tx_out_int;

short int *tx_out_shi;
float tx_agc_factor;
float tx_agc_decay;
float txpwr_decay;
float tx_forwardpwr;
float tx_backpwr;
int tx_output_flag;
int tx_ph1;
float tx_resample_ratio;
float new_tx_resample_ratio;
float tx_output_upsample;
int tx_resamp_pa;
int tx_resamp_px;
int tx_filter_ia1;
int tx_filter_ia2;
int tx_filter_ia3;
int tx_filter_ib1;
int tx_filter_ib2;
int tx_filter_ib3;
char display_color[MAX_DISPLAY_TYPES]={0,10,11};
int tx_pre_resample;
int tx_wrblock_no;
int tx_output_mode;
int tx_show_siz;
int tx_wsamps;
float tx_wttim;
float tx_wttim_sum;
float tx_wttim_ref;
float tx_wttim_diff;
int tx_setup_flag;

float tx_pilot_tone;
float tx_output_amplitude;
int txout_pa;
float mic_key_fx;
float *txout_waveform;
int max_tx_cw_waveform_points;
int tx_cw_waveform_points;
int tone_key, tot_key;
int tx_waveform_pointer;
int old_rxtx_state;
int rxtx_state;
int rxtx_wait;
int tx_mode;

float *radar_pulse_ston;
float *radar_pulse_noise_floor;
int *radar_pulse_bin;
int *radar_pulse_pointer;
int *radar_handle;
float *radar_average;
int radar_graph_scro;
int rg_old_x1;
int rg_old_x2;
int rg_old_y1;
int rg_old_y2;
int radar_bins;
int radar_maxlines;
int radar_lines;
int pulse_sep;
int pulse_bin;
float radar_decayfac;
int radar_redraw_count;
int radar_update_cnt;
float rg_cfac;
float rg_czer;
int radar_first_bin;
int radar_last_bin;
int radar_show_bins;







// ***************************************************
//      Transform sizes and associated variables.
//
// ***************************************************
// The first time function, data from the soundcard. The transform
// size is set for the data to span a suitable time in order to
// get a frequency resolution and an associated time delay that
// is independent of the microphone input sampling speed.
// Size is micsize.
short int *mictimf_shi;
int *mictimf_int;
int micsize;
int micn;
int mictimf_pa;
int mictimf_px;
int mictimf_block;
int mictimf_bufsiz;
int mictimf_mask;
float *mic_win;
COSIN_TABLE *mic_table;
unsigned short int *mic_permute;
float *mic_tmp;
// ***************************************************
// The first FFT. The transforms are likely to span a far too large
// frequency range. Only the minimum required to represent the passband
// of interest is used for the output of mic_fft     
// Size is mic_fftsize.
float *micfft;
int mic_fftsize; 
int mic_fftn;
int mic_fftsize_start;
int mic_fftn_start;
int mic_sizhalf;
int micfft_pa;
int micfft_px;
int micfft_block;
int micfft_bufsiz;
int micfft_mask;
COSIN_TABLE *micfft_table;
unsigned short int *micfft_permute;
float *micfft_win;
int micfft_indicator_block;
// ***************************************************
// The second time function. Here we will do clipping.
// Size is mic_fftsize.
float *cliptimf;
int cliptimf_pa;
int cliptimf_px;
// ***************************************************
// The second FFT. Used to clean up the spectrum.
// Size is mic_fftsize.
float *clipfft;
int clipfft_pa;
int clipfft_px;
// ***************************************************
// The third time function. Used for ALC.
// We will use this time function for resampling to a new sampling speed
// that will be equal to the output speed or a power of two faster or
// slower.
// The fractional resampler needs an oversampled input and therefore
// we first upsample by a power of two here.
// Size is alc_fftsize.
int alc_fftsize;
int alc_fftn;
int alc_sizhalf;
float *alctimf;
float *alctimf_pwrf;
float *alctimf_pwrd;
int alctimf_pa;
int alctimf_pb;
float alctimf_fx;
int alc_block;
int alc_bufsiz;
int alc_mask;
COSIN_TABLE *alc_table;
unsigned short int *alc_permute;
float *alc_win;
// ***************************************************
// The fourth time function, the output from the fractional resampler.
// Size is alc_fftsize.
// We use transforms one by one so no buffer pointer is needed.
// The third FFT is computed in place and the size of the transform
// is changed in place to txout_fftsize.
float *tx_resamp;
float *resamp_tmp;
// ***************************************************
// The fifth time function. 
// Size is txout_fftsize.
int txout_fftsize;
int txout_fftn;
int txout_sizhalf;
float *txout;
float *txout_tmp;
COSIN_TABLE *txout_table;
unsigned short int *txout_permute;
// ****************************************************


char ssbproc_filename[14]="par_ssbproc00";
char cwproc_filename[13]="par_cwproc00";
char *txproc_filename;
