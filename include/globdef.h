#ifndef GLOBDEF_H
#define GLOBDEF_H

#include <errno.h>

#define TRUE 1
#define FALSE 0
#define YELLOW 14
#define LIGHT_GREEN 10
#define LIGHT_RED 12

#define NO_REDRAW_BLOCKS 16
// *******************************************
// Global numerical values
#define PI_L 3.1415926535897932
#define BIG 300000000000000000000000000000000000000.
// values for lir_status
// Memory errors are larger than LIR_OK.
// Other abnormal conditions smaller than LIR_OK.
#define LIR_OK 0
#define LIR_TUNEERR 1
#define LIR_FFT1ERR 2
#define LIR_MEMERR 4
#define LIR_NEW_SETTINGS -1
#define LIR_SPURERR -2
#define LIR_PARERR -3
#define LIR_NEW_POL -4
#define LIR_TUNEERR2 -5
#define LIR_POWTIM -6
// *******************************************
// Fixed array dimensions depend on these defines:

#define MODE_WCW 0
#define MODE_NCW 1
#define MODE_HSMS 2
#define MODE_SSB 3
#define MODE_FM 4
#define MODE_AM 5
#define MODE_QRSS 6
// Note that modes below MODE_TXTEST are allowed to call 
// users tx routines, but that MODE_TXTEST and higher may 
// call users_init_mode, but users_hwaredriver is responsible
// for checking rx_mode < MODE_TXTEST and not do any tx
// activities or screen writes.
#define MODE_TXTEST 7
#define MODE_RX_ADTEST 8
#define MODE_TUNE 9
#define MODE_RADAR 10
#define MAX_RX_MODE 11

#define TXMODE_OFF 0
#define TXMODE_CW 1
#define TXMODE_SSB 2

#define MAX_COLOR_SCALE 23
#define MAX_SCRO 50
#define LLSQ_MAXPAR 10
#define MAX_ADCHAN 4
#define MAX_MIX1 8           // No of different signals to process simultaneously
#define MAX_AD_CHANNELS 4
#define MAX_RX_CHANNELS 2
#define DAOUT_MAXTIME 1.
#define SPUR_N 3
#define SPUR_SIZE (1<<SPUR_N)
#define NO_OF_SPUR_SPECTRA 256
#define MAX_FFT1_ARRAYS 85
#define MAX_FFT3_ARRAYS 30
#define MAX_BASEB_ARRAYS 40
#define CG_MAXTRACE 24
#define MAX_HIRES_ARRAYS  8
#define MAX_AFC_ARRAYS 20
#define MAX_VOICELAB_ARRAYS 16
#define MAX_VOICELAB_ARRAYS 16
#define MAX_TXMEM_ARRAYS 30
#define MAX_BLANKER_ARRAYS 15
#define MAX_RADAR_ARRAYS 15

#define USR_NORMAL_RX 0
#define USR_TXTEST 1
#define USR_POWTIM 2
#define USR_ADTEST 3
#define USR_IQ_BALANCE 4
#define USR_CAL_INTERVAL 5
#define USR_CAL_FILTERCORR 6
#define USR_TUNE 7

#define SPECIFIC_DEVICE_CODES 9999
#define SDR14_DEVICE_CODE (SPECIFIC_DEVICE_CODES+1)
#define SDRIQ_DEVICE_CODE (SPECIFIC_DEVICE_CODES+2)
#define PERSEUS_DEVICE_CODE (SPECIFIC_DEVICE_CODES+3)

// Always include stdio.h, math.h and stdlib.h
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

// Positions for runtime error messages in the main spectrum.
#define WGERR_DASYNC 0
#define WGERR_ADSPEED 1
#define WGERR_HWARE 2
#define WGERR_MEMORY 2
#define WGERR_SCREEN 2
#define WGERR_RXIN 3
#define WGERR_RXOUT 4
#define WGERR_TXIN 5
#define WGERR_TXOUT 6
#define WGERR_NETWR 7



#define NET_RXOUT_RAW16 1
#define NET_RXOUT_RAW18 2
#define NET_RXOUT_RAW24 4
#define NET_RXOUT_FFT1  8
#define NET_RXOUT_TIMF2 16
#define NET_RXOUT_FFT2  32
#define NET_RXOUT_BASEB 64
#define NET_RX_OUTPUT  0x0000ffff
#define NET_RX_INPUT   0xffff0000
#define NET_RXIN_RAW16 0x00010000
#define NET_RXIN_RAW18 0x00020000
#define NET_RXIN_RAW24 0x00040000
#define NET_RXIN_FFT1  0x00080000
#define NET_RXIN_BASEB 0x00100000

#define NETMSG_OWN_FREQ 41651
#define NETMSG_SUBSLAVE_FREQ 41652
#define NETMSG_CAL_REQUEST 41653
#define NETMSG_CALIQ_REQUEST 41654
#define NETMSG_FFT1INFO_REQUEST 41655
#define NETMSG_MODE_REQUEST 41656
#define NETMSG_REMOVE_SUBSLAVE 41567
#define NETMSG_BASEBMODE_REQUEST 41658

typedef void (*ROUTINE) (void);
#define PERMDEB fprintf(dmp,
#define DEB if(dmp != NULL)PERMDEB
#define XZ xz
// *******************************************
// Audio board parameter definitions
#define DWORD_INPUT 1
#define TWO_CHANNELS 2
#define IQ_DATA 4
#define BYTE_INPUT 8
#define NO_DUPLEX 16
#define DIGITAL_IQ 32
#define MODEPARM_MAX 64
// *******************************************
// Processing parameter definitions
#define FIRST_FFT_BANDWIDTH 0
#define FIRST_FFT_SINPOW 1
#define FIRST_FFT_VERNR 2
#define FFT1_STORAGE_TIME 3 
#define FIRST_FFT_GAIN 4 
#define WG_WATERF_BLANKED_PERCENT 5
#define SECOND_FFT_ENABLE 6

#define FIRST_BCKFFT_VERNR 7
#define SELLIM_MAXLEVEL 8
#define FIRST_BCKFFT_ATT_N 9
#define SECOND_FFT_NINC 10
#define SECOND_FFT_SINPOW 11
#define SECOND_FFT_VERNR 12
#define SECOND_FFT_ATT_N 13
#define FFT2_STORAGE_TIME 14

#define AFC_ENABLE 15
#define AFC_LOCK_RANGE 16
#define AFC_MAX_DRIFT 17
#define CW_DECODE_ENABLE 18
#define MAX_NO_OF_SPURS 19
#define SPUR_TIMECONSTANT 20

#define MIX1_BANDWIDTH_REDUCTION_N 21
#define MIX1_NO_OF_CHANNELS 22
#define THIRD_FFT_SINPOW 23
#define BASEBAND_STORAGE_TIME 24
#define OUTPUT_DELAY_MARGIN 25
#define DA_OUTPUT_SPEED 26

#define OUTPUT_MODE 27

#define AMPLITUDE_EXPAND_EXPONENT 28
#define MAX_GENPARM 29

// *******************************************************
// Defines for state machine in fft2.c
#define FFT2_NOT_ACTIVE 0
#define FFT2_B 10
#define FFT2_C 11
#define FFT2_MMXB 20
#define FFT2_MMXC 21
#define FFT2_ELIMINATE_SPURS 104
#define FFT2_WATERFALL_LINE 1001
#define FFT2_TEST_FINISH 9999
#define FFT2_COMPLETE -1
// **********************************************
#define CALIQ 2
#define CALAMP 1

// **********************************************
#define OS_FLAG_LINUX 1
#define OS_FLAG_WINDOWS 2
#define OS_FLAG_X 4
// *******************************************

#define FD int

// Structure for transmitter parameters in ssb mode
typedef struct {
int minfreq;
int maxfreq;
int slope;
int bass;
int treble;
int mic_gain;
int mic_out_gain;
int mic_agc_time;
int mic_f_threshold;
int mic_t_threshold;
int rf1_gain;
int check;
}SSBPROC_PARM;
#define MAX_SSBPROC_PARM 12
extern char *ssbprocparm_text[MAX_SSBPROC_PARM];

// Structure for transmitter parameters in CW modes
typedef struct {
int rise_time;
int enable_hand_key;
int enable_tone_key;
int enable_ascii;
int radar_interval;
int radar_pulse;
int check;
}CWPROC_PARM;
#define MAX_CWPROC_PARM 7
extern char *cwprocparm_text[MAX_CWPROC_PARM];

// *******************************************
// Structure for soundcard read/write size variables
typedef struct {
unsigned int frame;
unsigned int block_bytes;
unsigned int block_frames;
unsigned int buffer_frames;
unsigned int buffer_bytes;
float interrupt_rate;
}SOUNDCARD_SIZES;

// *******************************************
// Structure for printing time stamps on waterfall graphs
typedef struct {
int line;
char text[10];
}WATERF_TIMES;

// *******************************************
// Structure for user interface parameters selected by operator
typedef struct {
int vga_mode;
int shm_mode;
int screen_width_factor;
int screen_height_factor;
int font_scale;
int mouse_speed;
int max_dma_rate;
int process_priority;
int use_alsa;
int rx_input_mode;
int rx_rf_channels;
int rx_ad_channels;
int rx_ad_speed;
int rx_addev_no;
int rx_admode;
int rx_damode;
int rx_dadev_no;
int rx_min_da_speed;
int rx_max_da_speed;
int rx_max_da_channels;
int rx_max_da_bytes;
int rx_min_da_channels;
int rx_min_da_bytes;
int parport;
int parport_pin;
int network_flag;
int tx_ad_speed;
int tx_da_speed;
int tx_addev_no;
int tx_dadev_no;
int tx_da_channels;
int tx_ad_channels;
int tx_da_bytes;
int tx_ad_bytes;
int tx_enable;
int tx_pilot_tone_db;
int tx_pilot_tone_prestart;
int newcomer_mode;
int max_blocked_cpus;
int check;
}USERINT_PARM;
#define MAX_UIPARM 40
extern char *uiparm_text[MAX_UIPARM];


// *******************************************
// Screen object parameters for mouse and screen drawing.
// Each screen object is defined by it's number, scro[].no
// For each type scro[].type the number scro[].no is unique and
// defines what to do in the event of a mouse click.

// Definitions for type = GRAPH
#define GRAPH 0
#define WIDE_GRAPH 1
#define HIRES_GRAPH 2
#define TRANSMIT_GRAPH 3
#define FREQ_GRAPH 4
#define RADAR_GRAPH 5
#define MAX_WIDEBAND_GRAPHS 10
#define AFC_GRAPH 11
#define BASEBAND_GRAPH 12
#define POL_GRAPH 13
#define COH_GRAPH 14
#define EME_GRAPH 15
#define METER_GRAPH 16
#define GRAPH_RIGHTPRESSED 128
#define GRAPH_MASK 0x8000007f

// Definitions for parameter input routines under mouse control
#define FIXED_INT_PARM 3
#define TEXT_PARM 4
#define FIXED_FLOAT_PARM 5
#define DATA_READY_PARM 128
#define MAX_TEXTPAR_CHARS 32
#define EG_DX_CHARS 12
#define EG_LOC_CHARS 6 

// Definitions for type WIDE_GRAPH
#define WG_TOP 0
#define WG_BOTTOM 1
#define WG_LEFT 2
#define WG_RIGHT 3
#define WG_BORDER 4
#define WG_YSCALE_EXPAND 5
#define WG_YSCALE_CONTRACT 6
#define WG_YZERO_DECREASE 7
#define WG_YZERO_INCREASE 8
#define WG_FQMIN_DECREASE 9
#define WG_FQMIN_INCREASE 10
#define WG_FQMAX_DECREASE 11
#define WG_FQMAX_INCREASE 12
#define WG_AVG1NUM 13
#define WG_FFT1_AVGNUM 14
#define WG_WATERF_AVGNUM 15
#define WG_WATERF_ZERO 16
#define WG_WATERF_GAIN 17
#define WG_SPUR_TOGGLE 18
#define WG_FREQ_ADJUSTMENT_MODE 19
#define WG_LOWEST_FREQ 20
#define WG_HIGHEST_FREQ 21
#define MAX_WGBUTT 22

// Definitions for type HIRES_GRAPH
#define HG_TOP 0
#define HG_BOTTOM 1
#define HG_LEFT 2
#define HG_RIGHT 3
#define HG_BLN_STUPID 4
#define HG_BLN_CLEVER 5
#define HG_TIMF2_STATUS 6
#define HG_TIMF2_WK_INC 7
#define HG_TIMF2_WK_DEC 8
#define HG_TIMF2_ST_INC 9
#define HG_TIMF2_ST_DEC 10
#define HG_TIMF2_LINES 11
#define HG_TIMF2_HOLD 12
#define HG_FFT2_AVGNUM 13
#define HG_SPECTRUM_ZERO 14
#define HG_SPECTRUM_GAIN 15
#define MAX_HGBUTT 16

// Definitions for type BASEBAND_GRAPH
#define BG_TOP 0
#define BG_BOTTOM 1
#define BG_LEFT 2
#define BG_RIGHT 3
#define BG_YSCALE_EXPAND 4
#define BG_YSCALE_CONTRACT 5
#define BG_YZERO_DECREASE 6
#define BG_YZERO_INCREASE 7
#define BG_RESOLUTION_DECREASE 8
#define BG_RESOLUTION_INCREASE 9
#define BG_OSCILLOSCOPE 10
#define BG_OSC_INCREASE 11
#define BG_OSC_DECREASE 12
#define BG_PIX_PER_PNT_INC 13
#define BG_PIX_PER_PNT_DEC 14
#define BG_TOGGLE_EXPANDER 15
#define BG_TOGGLE_COHERENT 16
#define BG_TOGGLE_PHASING 17
#define BG_TOGGLE_CHANNELS 18
#define BG_TOGGLE_BYTES 19
#define BG_TOGGLE_TWOPOL 20
#define BG_SEL_COHFAC 21
#define BG_SEL_DELPNTS 22
#define BG_SEL_FFT3AVGNUM 23
#define BG_TOGGLE_AGC 24
#define BG_SEL_AGC_ATTACK 25
#define BG_SEL_AGC_RELEASE 26
#define BG_YBORDER 27
#define BG_WATERF_ZERO 28
#define BG_WATERF_GAIN 29
#define BG_WATERF_AVGNUM 30
#define BG_HORIZ_ARROW_MODE 31
#define BG_MIXER_MODE 32
#define BG_FILTER_SHIFT 33
#define BG_NOTCH_NO 34
#define BG_NOTCH_POS 35
#define BG_NOTCH_WIDTH 36
#define MAX_BGBUTT 37

#define MAX_BG_NOTCHES 9

// Definitions for type AFC_GRAPH
#define AG_TOP 0
#define AG_BOTTOM 1
#define AG_LEFT 2
#define AG_RIGHT 3
#define AG_FQSCALE_EXPAND 4
#define AG_FQSCALE_CONTRACT 5
#define AG_MANAUTO 6
#define AG_WINTOGGLE 7
#define AG_SEL_AVGNUM 8
#define AG_SEL_FIT 9
#define AG_SEL_DELAY 10
#define MAX_AGBUTT 11

// Definitions for type APOL_GRAPH
#define PG_TOP 0
#define PG_BOTTOM 1
#define PG_LEFT 2
#define PG_RIGHT 3
#define PG_ANGLE 4
#define PG_CIRC 5
#define PG_AUTO 6
#define PG_AVGNUM 7
#define MAX_PGBUTT 8

// Definitions for type ACOH_GRAPH
#define CG_TOP 0
#define CG_BOTTOM 1
#define CG_LEFT 2
#define CG_RIGHT 3
#define CG_OSCILLOSCOPE 4
#define CG_METER_GRAPH 5
#define MAX_CGBUTT 6

// Definitions for type AEME_GRAPH
#define EG_TOP 0
#define EG_BOTTOM 1
#define EG_LEFT 2
#define EG_RIGHT 3
#define EG_MINIMISE 4
#define EG_LOC 5
#define EG_DX 6
#define MAX_EGBUTT 7

// Definitions for type FREQ_GRAPH
#define FG_TOP 0
#define FG_BOTTOM 1
#define FG_LEFT 2
#define FG_RIGHT 3
#define FG_INCREASE_FQ 4
#define FG_DECREASE_FQ 5
#define FG_INCREASE_GAIN 6
#define FG_DECREASE_GAIN 7
#define MAX_FGBUTT 8

// Definitions for type (S)METER_GRAPH
#define MG_TOP 0
#define MG_BOTTOM 1
#define MG_LEFT 2
#define MG_RIGHT 3
#define MG_INCREASE_AVGN 4
#define MG_DECREASE_AVGN 5
#define MG_INCREASE_GAIN 6
#define MG_DECREASE_GAIN 7
#define MG_INCREASE_YREF 8
#define MG_DECREASE_YREF 9
#define MG_CHANGE_CAL 10
#define MG_CHANGE_TYPE 11
#define MG_CHANGE_TRACKS 12
#define MAX_MGBUTT 13

// Definitions for type TRANSMIT_GRAPH

#define TG_TOP 0
#define TG_BOTTOM 1
#define TG_LEFT 2
#define TG_RIGHT 3
#define TG_INCREASE_FQ 4
#define TG_DECREASE_FQ 5
#define TG_CHANGE_SSBPROC_NO 6
#define TG_NEW_TX_FREQUENCY 7
#define TG_SET_SIGNAL_LEVEL 8
#define TG_ONOFF 9
#define TG_RADAR_INTERVAL 10
#define MAX_TGBUTT 11

// Definitions for type RADAR_GRAPH

#define RG_TOP 0
#define RG_BOTTOM 1
#define RG_LEFT 2
#define RG_RIGHT 3
#define RG_TIME 4
#define RG_ZERO 5
#define RG_GAIN 6
#define MAX_RGBUTT 7



// Structure for the eme database.
#define CALLSIGN_CHARS 12
typedef struct{
char call[CALLSIGN_CHARS];
float lon;
float lat;
}DXDATA;

// Structure for mouse buttons
typedef struct {
int x1;
int x2;
int y1;
int y2;
}BUTTONS;

// Structure to remember screen positions
// for processing parameter change boxes
typedef struct{
int no;
int type;
int x;
int y;
}SAVPARM;

// Structure for a screen object on which the mouse can operate.
typedef struct {
int no;
int x1;
int x2;
int y1;
int y2;
}SCREEN_OBJECT;

// Structure for the AFC graph
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int mode_control;
int avgnum;
int window;
int fit_points;
int delay;
int check;
float minston;
float search_range;
float lock_range;
float frange;
} AG_PARMS;
#define MAX_AG_INTPAR 10
extern char *ag_intpar_text[MAX_AG_INTPAR];
#define MAX_AG_FLOATPAR 4
extern char *ag_floatpar_text[MAX_AG_FLOATPAR];

// Structure for the wide graph.
// Waterfall and full dynamic range spectrum
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int yborder;
int xpoints_per_pixel;
int pixels_per_xpoint;
int first_xpoint;
int xpoints;
int fft_avg1num;
int spek_avgnum;
int waterfall_avgnum;
int spur_inhibit;
int check;
float yzero;
float yrange;
float waterfall_db_zero;
float waterfall_db_gain;
} WG_PARMS;
#define MAX_WG_INTPAR 14
extern char *wg_intpar_text[MAX_WG_INTPAR];
#define MAX_WG_FLOATPAR 4
extern char *wg_floatpar_text[MAX_WG_FLOATPAR];

// Structure for the high resolution and blanker control graph.
// Waterfall and full dynamic range spectrum
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int stupid_bln_mode;
int clever_bln_mode;
int timf2_display;
int timf2_display_lines;
int timf2_display_hold;
int spek_avgnum;
unsigned int stupid_bln_limit;
unsigned int clever_bln_limit;
int spek_zero;
int spek_gain;
int check;
float stupid_bln_factor;
float clever_bln_factor;
float blanker_ston_fft1;
float blanker_ston_fft2;
float timf2_display_wk_gain;
float timf2_display_st_gain;
} HG_PARMS;
#define MAX_HG_INTPAR 15
extern char *hg_intpar_text[MAX_HG_INTPAR];
#define MAX_HG_FLOATPAR 6
extern char *hg_floatpar_text[MAX_HG_FLOATPAR];


// Structure for the baseband graph
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int yborder;
int fft_avgnum;
int pixels_per_point;
int coh_factor;
int delay_points;
int agc_flag;
int agc_attack;
int agc_release;
int waterfall_avgnum;
int wheel_stepn;
int oscill_on;
int horiz_arrow_mode;
int mixer_mode;
int filter_shift;
int check;
float filter_flat;
float filter_curv;
float yzero;
float yrange;
float db_per_pixel;
float yfac_power;
float yfac_log;
float bandwidth;
float first_frequency;
float bfo_freq;
float output_gain;
float waterfall_gain;
float waterfall_zero;
float oscill_gain;
} BG_PARMS;
#define MAX_BG_INTPAR 19
extern char *bg_intpar_text[MAX_BG_INTPAR];
#define MAX_BG_FLOATPAR 14
extern char *bg_floatpar_text[MAX_BG_FLOATPAR];

// Structure for the polarization graph
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int adapt;
int avg;
int check;
float angle;
float c1;
float c2;
float c3;
} PG_PARMS;
#define MAX_PG_INTPAR 7
extern char *pg_intpar_text[MAX_PG_INTPAR];
#define MAX_PG_FLOATPAR 4
extern char *pg_floatpar_text[MAX_PG_FLOATPAR];

// Structure for the coherent processing graph
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int meter_graph_on;
int oscill_on;
} CG_PARMS;
#define MAX_CG_INTPAR 6
extern char *cg_intpar_text[MAX_CG_INTPAR];
#define MAX_CG_FLOATPAR 0
extern char *cg_floatpar_text[MAX_CG_FLOATPAR];


// Structure for the S-meter graph
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int scale_type;
int avgnum;
int tracks;
int check;
float ygain;
float yzero;
float cal_dbm;
float cal_s_units;
} MG_PARMS;
#define MAX_MG_INTPAR 8
extern char *mg_intpar_text[MAX_MG_INTPAR];
#define MAX_MG_FLOATPAR 4
extern char *mg_floatpar_text[MAX_MG_FLOATPAR];


// Structure for the EME graph.
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int minimise;
} EG_PARMS;
#define MAX_EG_INTPAR 5
extern char *eg_intpar_text[MAX_EG_INTPAR];
#define MAX_EG_FLOATPAR 0
extern char *eg_floatpar_text[MAX_EG_FLOATPAR];

// Structure for the frequency control box.
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int yborder;
int passband_direction;
int gain;
int gain_increment;
double passband_increment;
double passband_center;
} FG_PARMS;
#define MAX_FG_INTPAR 8
extern char *fg_intpar_text[MAX_FG_INTPAR];
#define MAX_FG_FLOATPAR 2
extern char *fg_floatpar_text[MAX_FG_FLOATPAR];

// Structure for the Tx control box.
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int spproc_no;
int band_direction;
double level_db;
double freq;
double band_increment;
double band_center;
} TG_PARMS;
#define MAX_TG_INTPAR 6
extern char *tg_intpar_text[MAX_TG_INTPAR];
#define MAX_TG_FLOATPAR 4
extern char *tg_floatpar_text[MAX_TG_FLOATPAR];

// Structure for the radar graph.
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
float time;
float zero;
float gain;
} RG_PARMS;
#define MAX_RG_INTPAR 4
extern char *rg_intpar_text[MAX_RG_INTPAR];
#define MAX_RG_FLOATPAR 3
extern char *rg_floatpar_text[MAX_RG_FLOATPAR];



// Structure for network parameters
typedef struct {
int send_group;
int rec_group;
int port;
int send_raw;
int send_fft1;
int send_fft2;
int send_timf2;
int send_baseb;
int receive_raw;
int receive_fft1;
int receive_baseb;
int check;
} NET_PARMS;
#define MAX_NET_INTPAR 12
extern char *net_intpar_text[MAX_NET_INTPAR];

#define MAX_NETSLAVES 16
// Structure for file descriptors used by the network
typedef struct {
FD send_rx_raw16;
FD send_rx_raw18;
FD send_rx_raw24;
FD send_rx_fft1;
FD send_rx_fft2;
FD send_rx_timf2;
FD send_rx_baseb;
FD rec_rx;
FD master;
FD any_slave;
FD slaves[MAX_NETSLAVES];
} NET_FD;
#define MAX_NET_FD (9+MAX_NETSLAVES)
extern NET_FD netfd;
#define MAX_FREQLIST (2*MAX_NETSLAVES)

// Structure for multicasting receive data on the network.
#define NET_MULTICAST_PAYLOAD 1392 // This number must be a multiple of 48
typedef struct {

double passband_center;        //  8
int time;                      //  4
float userx_freq;              //  4
int ptr;                       //  4
unsigned short int block_no;   //  2
char userx_no;                 //  1
char passband_direction;       //  1
char buf[NET_MULTICAST_PAYLOAD];
} NET_RX_STRUCT;
extern NET_RX_STRUCT net_rxdata_16;
extern NET_RX_STRUCT net_rxdata_18;
extern NET_RX_STRUCT net_rxdata_24;
extern NET_RX_STRUCT net_rxdata_fft1;
extern NET_RX_STRUCT net_rxdata_timf2;
extern NET_RX_STRUCT net_rxdata_fft2;
extern NET_RX_STRUCT net_rxdata_baseb;

// Structure for messages from slaves.
typedef struct {
int type;
int frequency;
} SLAVE_MESSAGE;
extern SLAVE_MESSAGE slave_msg;




// ***********************************************
// Structure used by float fft routines
typedef struct {
float sin;
float cos;
}COSIN_TABLE;

// ***********************************************
// Structure used by d_float fft routines (double)
typedef struct {
double sin;
double cos;
}D_COSIN_TABLE;

// Structure used by MMX fft routines
typedef struct {
short int c1p;
short int s2p;
short int c3p;
short int s4m;
}MMX_COSIN_TABLE;

// Structure for mixer/decimater using backwards FFTs
typedef struct {
float *window;
float *cos2win;
float *sin2win;
unsigned short int *permute;
COSIN_TABLE *table;
int interleave_points;
int new_points;
int crossover_points;
int size;
int n;
}MIXER_VARIABLES;

// Define setup info for each fft version.
typedef struct {
unsigned char window;
unsigned char permute;
unsigned char max_n;
unsigned char mmx;
unsigned char simd;
char *text;
} FFT_SETUP_INFO;

typedef struct {
void *pointer;
int size;
int scratch_size;
int num;
} MEM_INF;

typedef struct {
float x2;
float y2;
float im_xy;
float re_xy;
}TWOCHAN_POWER;

# define MAX_FFT_VERSIONS 18
# define MAX_FFT1_VERNR 8
# define MAX_FFT1_BCKVERNR 4
# define MAX_FFT2_VERNR 4
#endif
