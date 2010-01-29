
#include "globdef.h"
#include "uidef.h"

int sdr;

MIXER_VARIABLES mix1;
MIXER_VARIABLES mix2;

double wg_lowest_freq;
double wg_highest_freq;
int wg_freq_adjustment_mode;
int wg_freq_x1;
int wg_freq_x2;
double expose_time;
int no_of_processors;
long tickspersec;
int freq_from_file;
float workload;
int workload_counter;
int mouse_inhibit;
double mouse_time_wide;
double mouse_time_narrow;
double time_info_time;
double accumulated_netwait_time;
double netstart_time;
ROUTINE par_from_keyboard_routine;
int baseb_reset_counter;
int baseb_control_flag;
int all_threads_started; 
int write_log;
int spurcancel_flag;
int spurinit_flag;
int autospur_point;
double recent_time;
int internal_generator_flag;
double internal_generator_phase1;
double internal_generator_phase2;
double internal_generator_shift;
int internal_generator_key;
int internal_generator_noise;
int truncate_flag;
int hand_key;
int kill_all_flag;
char lbutton_state;
char rbutton_state;
char new_lbutton_state;
char new_rbutton_state;
int no_of_netslaves;
int netfft1_blknum;
int netraw16_blknum;
int netraw18_blknum;
int netraw24_blknum;
int nettimf2_blknum;
int netfft2_blknum;
int netbaseb_blknum;
int basebnet_block_bytes;
unsigned int next_blkptr_16;
unsigned int next_blkptr_18;
unsigned int next_blkptr_24;
unsigned int next_blkptr_fft1;
unsigned int next_blkptr_timf2;
unsigned int next_blkptr_fft2;
unsigned int next_blkptr_baseb;
int netsend_ptr_16;
int netsend_ptr_18;
int netsend_ptr_24;
int netsend_ptr_fft1;
int netsend_ptr_timf2;
int netsend_ptr_fft2;
int netsend_ptr_fft2;
int netsend_ptr_baseb;
int latest_listsend_time;
int net_no_of_errors;
int rxin_block_counter;
int rx_hware_fqshift;
char *msg_filename;

int lir_errcod;


int font_xsep;
int font_ysep;
int font_w;
int font_h;
int font_size;

int mouse_task;
float min_delay_time;
int savefile_repeat_flag;
int os_flag;
int qz;
float tune_yzer;
int usercontrol_mode;
int workload_reset_flag;
int fft2_to_fft1_ratio;
float baseband_bw_hz;
int baseband_bw_fftxpts;
int lir_inkey;
char *vga_font;
int text_width;
int text_height;
int screen_last_xpixel;
int screen_last_col;
int screen_width;
int screen_height;
int screen_totpix;
int uiparm_change_flag;
USERINT_PARM ui;
SSBPROC_PARM txssb;
CWPROC_PARM txcw;
int mouse_flag;
int mouse_hide_flag;
int mouse_x;
int new_mouse_x;
int mouse_xmax;
int mouse_xmin;
int mouse_y;
int new_mouse_y;
int mouse_ymax;
int mouse_ymin;
int mouse_cursize;
int leftpressed;
int rightpressed;
int mouse_lbutton_x;
int mouse_rbutton_x;
int mouse_lbutton_y;
int mouse_rbutton_y;
int mmx_present;
int simd_present;
int rx_audio_in;
int rx_audio_in2;
int rx_audio_out;

SOUNDCARD_SIZES rxda;
SOUNDCARD_SIZES rxad;
int rx_read_bytes;
int mixer; 
int rx_channels;
int twice_rxchan;
FILE *dmp;
FILE *sndlog;
FILE *wav_file;
char *rx_da_wrbuf;
float total_wttim;
int rx_daout_bytes;
int rx_daout_channels;
int timinfo_flag;
int ampinfo_flag;
float measured_da_speed;
float measured_ad_speed;
int rx_mode;
int use_bfo;
double diskread_time;
double eme_time;
int dasync_counter;
float dasync_sum;
float dasync_avg1;
float dasync_avg2;
float dasync_avg3;
double dasync_time;
double dasync_avgtime;
double daspeed_time;
double da_start_time;
int overrun_count;
int da_start_samps;
int wav_write_flag;
int wav_read_flag;
int wavfile_bytes;
int audio_dump_flag;
int diskread_pause_flag;
int current_graph_minh;
int current_graph_minw;
int calibrate_flag;
double old_passband_center;
int swmmx_fft1;
int swmmx_fft2;
int swfloat;
int sw_onechan;
int memalloc_no;
int memalloc_max;
MEM_INF *memalloc_mem;
int eme_flag;
int timf3_oscilloscope_limit;
int lir_status;
int allow_parport;
float rx_output_blockrate;
float tx_output_blockrate;
int no_of_rx_overrun_errors;
int no_of_rx_underrun_errors;
int no_of_tx_overrun_errors;
int no_of_tx_underrun_errors;
int count_rx_underrun_flag;

DXDATA *dx;
MEM_INF fft1mem[MAX_FFT1_ARRAYS];
MEM_INF fft3mem[MAX_FFT3_ARRAYS];
MEM_INF basebmem[MAX_BASEB_ARRAYS];
MEM_INF hiresmem[MAX_HIRES_ARRAYS];
MEM_INF afcmem[MAX_AFC_ARRAYS];
MEM_INF voicelabmem[MAX_VOICELAB_ARRAYS];
MEM_INF txmem[MAX_TXMEM_ARRAYS];
MEM_INF blankermem[MAX_BLANKER_ARRAYS];
MEM_INF radarmem[MAX_RADAR_ARRAYS];
double flowcnt[MAX_FLOWCNT];
double flowtime;
double old_flowtime;
int mailbox[64];

NET_RX_STRUCT net_rxdata_16;
NET_RX_STRUCT net_rxdata_18;
NET_RX_STRUCT net_rxdata_24;
NET_RX_STRUCT net_rxdata_fft1;
NET_RX_STRUCT net_rxdata_timf2;
NET_RX_STRUCT net_rxdata_fft2;
NET_RX_STRUCT net_rxdata_baseb;

SLAVE_MESSAGE slave_msg;




char savefile_parname[44];  //40chars +  _ag  + ending zero.
char savefile_name[41];

NET_FD netfd;

char userint_filename[]="par_userint";
char network_filename[]="par_network";
char *rxpar_filenames[MAX_RX_MODE]={"par_wcw",
                         "par_cw",
                         "par_hsms",
                         "par_ssb",
                         "par_fm",
                         "par_am",
                         "par_qrss",
                         "par_txtest",
                         "par_test",
                         "par_tune",
                         "par_radar"};
char newcomer_rx_modes[MAX_RX_MODE]={0,1,0,1,1,10,0,0,0,0};

char *rxmodes[MAX_RX_MODE]={"Weak signal CW",
                 "Normal CW",
                 "Meteor scatter CW",
                 "SSB",
                 "FM",
                 "AM",
                 "QRSS CW",
                 "TX TEST",
                 "SOUNDCARD TEST MODE",
                 "ANALOG HARDWARE TUNE",
                 "RADAR"};

#define A MAX_MIX1
#define B MAX_FFT1_VERNR
#define C MAX_FFT1_BCKVERNR
#define G MAX_FFT2_VERNR
#define K 6000

#define D 1000
#define E 1000000
#define F 11025
#define H 200
#define M 8000
#define N 10000
#define P 24000
#define A1 100
#define B5 5000
int genparm[MAX_GENPARM+2];
//                w        b  b           e      A e   s    m        D    d e
//      f f  s    t 2 b l  a  w f f  f  s n   A  F n m p  m i f  b   A D  e x 
//    f f f  t    r e a i  c  f f f  w  t a   F  C . a u  i x f  a   m A  f p
//    f t t  1    f n c m  k  a t t  d  2 b   C  d M x r  x 1 t  s   a s  . a
//    t 1 1  t a  . a k .  a  c 2 2  a  t l   l  r o s t  1 c 3  t   r p  m n
//    1 w v  i m  l b v m  t  t w v  t  i A   o  i r p i  r h w  i   g e  o d
//    b i e  m p  i l e a  t  o i e  t  m F   c  f s u m  e a i  m   i e  d e
//    w n r  e l  m e r x  N  r n r  N  e C   k  t e r e  d n n  e   n d  e r
//                            1 1 1  1  1 1   1  1 1 1 2  2 2 2  2   2 2  2 2
//    0 1 2  3 4  5 6 7 8  9  0 1 2  3  4 5   6  7 8 9 0  1 2 3  4   5 6  7 8
int genparm_min[MAX_GENPARM]=
   {  0,0,0, 0,1, 0,0,0,H, 2, 0,0,0, 2, 0,0,  1, 0,0,0,1, 1,1,1, 2,  0,H, 0,0};
int genparm_max[MAX_GENPARM]=
   {  K,9,B,60,E,99,1,C,E,99,10,4,G,16, D,2,800,B5,1,E,H,10,A,9, D,  N,E, H,9};
int genparm_default[MAX_RX_MODE][MAX_GENPARM]={
// Weak signal CW
   { 25,2,0, 4,D, 0,0,0,K, 6, 2,2,0, 8,20,1,150,A1,0,0,5, 6,1,2,20,200,F, 1,3},
// Normal CW
   {100,2,0, 1,D, 0,0,0,K, 6, 2,2,0, 7, 5,0,150,A1,0,0,5, 4,1,2, 5,100,F, 1,3},
// Meteor scatter CW
   {300,2,0, 1,D, 0,0,0,K, 6, 2,2,0, 7, 5,0,150,A1,0,0,5, 4,1,2, 5,100,F, 1,3},
// SSB
   {100,2,0, 1,D, 0,0,0,K, 6, 2,2,0, 7, 5,0,150,A1,0,0,5, 4,1,2, 2,100,F, 1,3},
// FM
   {100,2,0, 1,D, 0,0,0,K, 6, 2,2,0, 7, 5,0,150,A1,0,0,5, 2,1,2, 2,200,P, 1,3},
// AM
   { 50,2,0, 8,D, 0,0,0,K, 6, 0,2,0, 7, 5,0,350,40,0,0,5, 2,1,2, 2,200,P, 1,3},
// QRSS CW
   { 25,2,0, 1,D, 0,0,0,K, 6, 2,2,0, 7, 5,0,150,A1,0,0,5, 5,1,2, 2,500,F, 1,3},
// Tx test
   { 50,2,0, 1,D, 0,0,0,K, 6, 2,2,0, 7, 5,0,150,A1,0,0,5, 4,1,2, 2,500,F, 1,3},
// Soundcard test
   { 83,2,0, 1,D, 0,0,0,K, 6, 2,2,0, 7, 5,0,150,A1,0,0,5, 4,1,2, 2,500,F, 1,3},
// Analog hardware tune
   { 85,2,0, 1,D, 0,0,0,K, 6, 2,2,0, 7, 5,0,150,A1,0,0,5, 4,1,2, 2,500,F, 1,3},
// Radar
   {200,2,0, 4,D, 0,0,0,K, 6, 2,2,0, 8, 5,0,150,A1,0,0,5, 4,1,2, 2,500,F, 1,3}};

char *genparm_text[MAX_GENPARM+2]={"First FFT bandwidth (Hz)",          //0
                      "First FFT window (power of sin)",                //1
                      "First forward FFT version",                      //2
                      "First FFT storage time (s)",                     //3
                      "First FFT amplitude",                            //4
                      "Main waterfall saturate limit",                  //5
                      "Enable second FFT",                              //6
                      "First backward FFT version",                     //7
                      "Sellim maxlevel",                                //8
                      "First backward FFT att. N",                      //9
                      "Second FFT bandwidth factor in powers of 2",     //10
                      "Second FFT window (power of sin)",               //11
                      "Second forward FFT version",                     //12
                      "Second forward FFT att. N",                      //13
                      "Second FFT storage time (s)",                    //14
                      "Enable AFC/SPUR/DECODE (2=auto spur)",           //15
                      "AFC lock range Hz",                              //16
                      "AFC max drift Hz/minute",                        //17
                      "Enable Morse decoding",                          //18
                      "Max no of spurs to cancel",                      //19
                      "Spur timeconstant (0.1sek)",                     //20
                      "First mixer bandwidth reduction in powers of 2", //21
                      "First mixer no of channels",                     //22
                      "Third FFT window (power of sin)",                //23
                      "Baseband storage time (s)",                      //24
                      "Output delay margin (ms)",                       //25
                      "Output sampling speed (Hz)",                     //26
                      "Default output mode",                            //27
                      "Audio expander exponent",                        //28
                      "A/D speed",                                      //29
                      "Check"}; 

char newco_genparm[MAX_GENPARM]={1, //First FFT bandwidth (Hz)            0
                                 0, //First FFT window (power of sin)     1
                                 0, //First forward FFT version           2
                                 1, //First FFT storage time (s)          3
                                 1, //First FFT amplitude                 4
                                 0, //Main waterfall saturate limit       5
                                 0, //Enable second FFT                   6
                                 0, //First backward FFT version          7
                                 0, //Sellim maxlevel                     8
                                 0, //First backward FFT att. N           9
                                 0, //Second FFT bandwidth factor...     10
                                 0, //Second FFT window (power of sin)   11
                                 0, //Second forward FFT version         12
                                 0, //Second forward FFT att. N          13
                                 0, //Second FFT storage time (s)        14
                                 0, //Enable AFC/SPUR/DECODE (2=auto     15
                                 0, //AFC lock range Hz                  16
                                 0, //AFC max drift Hz/minute            17
                                 0, //Enable Morse decoding              18
                                 0, //Max no of spurs to cancel          19
                                 0, //Spur timeconstant (0.1sek)         20
                                 1, //First mixer bandwidth reduction..  21
                                 0, //First mixer no of channels         22
                                 0, //Third FFT window (power of sin)    23
                                 1, //Baseband storage time (s)          24
                                 0, //Output delay margin (ms)           25
                                 1, //Output sampling speed (Hz)         26
                                 0, //Default output mode                27
                                 0};//Audio expander exponent            28

char *ssbprocparm_text[MAX_SSBPROC_PARM]={"Lowest freq",        //1
                                          "Highest freq",       //2           
                                          "Slope",              //3
                                          "Bass",               //4
                                          "Treble",             //5
                                          "Mic gain",           //6
                                          "Mic out gain",       //7
                                          "Mic AGC time",       //8
                                          "Mic F threshold",    //9 
                                          "Mic T threshold",    //10
                                          "RF1 gain",           //11
                                          "Check"};             //12

char *cwprocparm_text[MAX_CWPROC_PARM]={"Rise time",        //1
                                        "Hand key",         //2
                                        "Tone key",         //3
                                        "ASCII input",      //4
                                        "Radar interval",   //5
                                        "Radar pulse",      //6
                                        "Check"};           //7  

char *uiparm_text[MAX_UIPARM]={"vga mode",           //1
                               "shm mode",           //2
                               "Screen width (%)",   //3
                               "Screen height (%)",  //4
                               "font scale",         //5
                               "mouse speed",        //6
                               "Max DMA rate",       //7
                               "Process priority",   //8  
                               "Native ALSA",        //9
                               "Rx input mode",      //10
                               "Rx rf channels",     //11
                               "Rx ad channels",     //12
                               "Rx ad speed",        //13
                               "Rx ad device no",    //14
                               "Rx ad mode",         //15
                               "Rx da mode",         //16
                               "Rx da device no",    //17
                               "Rx min da speed",    //18  
                               "Rx max da speed",    //19
                               "Rx max da channels", //20
                               "Rx max da bytes",    //21
                               "Rx min da channels", //22
                               "Rx min da bytes",    //23
                               "parport",            //24
                               "parport read pin",   //25
                               "network flag",       //26
                               "Tx ad speed",        //27
                               "Tx da speed",        //28
                               "Tx ad device no",    //29
                               "Tx da device no",    //30
                               "Tx da channels",     //31
                               "Tx ad channels",     //32
                               "Tx da bytes",        //33
                               "Tx ad bytes",        //34
                               "Tx enable",          //35
                               "Tx pilot tone dB",   //36
                               "Tx pilot microsec.", //37 
                               "Newcomer mode",      //38 
                               "Max blocked CPUs",   //39
                               "check"};             //40


char *ag_intpar_text[MAX_AG_INTPAR]={"ytop",               //1
                                     "ybottom",            //2
                                     "xleft",              //3
                                     "xright",             //4
                                     "mode",               //5
                                     "avgnum",             //6
                                     "window",             //7 
                                     "fit points",         //8
                                     "delay",              //9
                                     "check"};             //10

char *ag_floatpar_text[MAX_AG_FLOATPAR]={"min S/N",        //1
                                         "search range",   //2
                                         "lock_range",     //3
                                         "freq range"};    //4    




char *wg_intpar_text[MAX_WG_INTPAR]={"ytop",               //1
                                     "ybottom",            //2
                                     "xleft",              //3
                                     "xright",             //4
                                     "yborder",            //5
                                     "xpoints per pixel",  //6
                                     "pixels per xpoint",  //7
                                     "first xpoint",       //8
                                     "xpoints",            //9
                                     "avg1num",            //10 
                                     "spek avgnum",        //11  
                                     "waterfall avgnum",   //12
                                     "spur_inhibit",       //13 
                                     "check"};             //14               

char *wg_floatpar_text[MAX_WG_FLOATPAR]={"yzero",          //1
                                         "yrange",         //2 
                                         "wat. db zero",   //3
                                         "wat. db gain"};  //4

char *hg_intpar_text[MAX_HG_INTPAR]={"ytop",                //1
                                     "ybottom",             //2
                                     "xleft",               //3
                                     "xright",              //4
                                     "mode (dumb)",         //5
                                     "mode (clever)",       //6
                                     "timf2 display",       //7 
                                     "timf2 display lines", //8
                                     "timf2 display hold",  //9
                                     "spek avgnum",         //10
                                     "limit (dumb)",        //11
                                     "limit (clever)",      //12
                                     "spek gain",           //13
                                     "spek zero",           //14
                                     "check"};              //15

char *hg_floatpar_text[MAX_HG_FLOATPAR]={"factor (dumb)",          //1
                                         "factor (clever)",        //2
                                         "sellim fft1 S/N",        //3
                                         "sellim fft2 S/N",        //4    
                                         "timf2 display gain wk",  //5 
                                         "timf2 display gain st"}; //6 

char *bg_intpar_text[MAX_BG_INTPAR]={"ytop",                //1
                                     "ybottom",             //2
                                     "xleft",               //3
                                     "xright",              //4
                                     "yborder",             //5
                                     "fft3 avgnum",         //6
                                     "pixels/point",        //7
                                     "coh factor",          //8 
                                     "delay points",        //9
                                     "AGC flag",            //10
                                     "AGC attack",          //11
                                     "AGC release",         //12
                                     "Waterfall avgnum",    //13
                                     "Mouse wheel step",    //14
                                     "Oscill ON",           //15
                                     "Arrow mode",          //16
                                     "Filter FIR/FFT",      //17
                                     "Filter shift",        //18
                                     "check"};              //19

char *bg_floatpar_text[MAX_BG_FLOATPAR]={"filter flat",          //1
                                         "filter curved",        //2
                                         "yzero",                //3
                                         "yrange",               //4    
                                         "dB/pixel",             //5 
                                         "yfac pwr",             //6
                                         "yfac log",             //7
                                         "bandwidth",            //8
                                         "first freq",           //9
                                         "BFO freq",             //10
                                         "Output gain",          //11
                                         "Waterfall gain",       //12
                                         "Waterfall zero",       //13
                                         "Oscill gain"};         //14

char *pg_intpar_text[MAX_PG_INTPAR]={"ytop",            //1
                                     "ybottom",         //2
                                     "xleft",           //3
                                     "xright",          //4
                                     "adapt",           //5
                                     "avg",             //6
                                     "check"};          //7

char *pg_floatpar_text[MAX_PG_FLOATPAR]={"angle",     //1
                                         "c1",        //2
                                         "c2",        //3
                                         "c3"};       //4    



char *cg_intpar_text[MAX_CG_INTPAR]={"ytop",             //1
                                     "ybottom",          //2
                                     "xleft",            //3
                                     "xright",           //4
                                     "Meter graph on",   //5
                                     "Oscill on"};       //6

char *cg_floatpar_text[MAX_CG_FLOATPAR];

char *mg_intpar_text[MAX_MG_INTPAR]={"ytop",             //1
                                     "ybottom",          //2
                                     "xleft",            //3
                                     "xright",           //4
                                     "Scale type",       //5
                                     "Avgnum",           //6
                                     "Tracks",           //7
                                     "Check"};           //8     
															
char *mg_floatpar_text[MAX_MG_FLOATPAR]={"ygain",         //1
                                         "yzero",         //2
                                         "Cal dBm",       //3
                                         "Cal S-units"};  //4  
char *eg_intpar_text[MAX_EG_INTPAR]={"ytop",            //1
                                     "ybottom",         //2
                                     "xleft",           //3
                                     "xright",          //4
                                     "minim"};          //5

char *eg_floatpar_text[MAX_EG_FLOATPAR];


char *fg_intpar_text[MAX_FG_INTPAR]={"ytop",            //1
                                     "ybottom",         //2
                                     "xleft",           //3
                                     "xright",          //4
                                     "yborder",         //5
                                     "direction",       //6
                                     "gain",            //7
                                     "gain_inc"};       //8

char *fg_floatpar_text[MAX_FG_FLOATPAR]={"fq inc",      //1
                                          "freq"};      //2

char *tg_intpar_text[MAX_TG_INTPAR]={"ytop",            //1
                                     "ybottom",         //2
                                     "xleft",           //3
                                     "xright",          //4
                                     "ssbproc no",      //5
                                     "direction"};      //6
                                     
char *tg_floatpar_text[MAX_TG_FLOATPAR]={"level dB",    //1     
                                         "freq",        //2
                                         "fq inc",      //3
                                         "band fq"};    //4

char *net_intpar_text[MAX_NET_INTPAR]={"send group",       //1
                                       "receive group",    //2
                                       "port",             //3
                                       "send_raw",         //4
                                       "send_fft1",        //5
                                       "send_fft2",        //6
                                       "send_timf2",       //7
                                       "send_baseb",       //8
                                       "receive_raw",      //9
                                       "receive_fft1",     //10
                                       "receive_baseb",    //10
                                       "check"};           //11

char *rg_intpar_text[MAX_RG_INTPAR]={"ytop",            //1
                                     "ybottom",         //2
                                     "xleft",           //3
                                     "xright"           //4
                                     };
                                     
char *rg_floatpar_text[MAX_RG_FLOATPAR]={"Time",        //1
                                         "Zero",        //2
                                         "Gain"         //3
                                         };

char modes_man_auto[]={'-','A','M'};
char arrow_mode_char[3]={'T', 'P','B'};
char *press_any_key={"Press any key"};
char *press_enter={"Press ENTER"};
char *audiomode_text[4]={
         "One RF, one audio channel (normal audio)",
         "One RF, two audio channels (direct conversion)",
 "Two RF, two audio channels (normal audio, adaptive polarization)",
 "Two RF, four audio channels (direct conversion, adaptive polarization)"};
char remind_parsave[]="(Do not forget to save with ""W"" on the main menu)";
char overrun_error_msg[]=" OVERRUN_ERROR ";
char underrun_error_msg[]=" UNDERRUN_ERROR ";

