#ifndef NLNKDEF_H
#define NLNKDEF_H

typedef struct {
int n;
int size;
float rest;
float avgpwr;
float avgmax;
} BLANKER_CONTROL_INFO;
#define BLN_INFO_SIZE 7 //6
#define MAX_REFPULSES 256 //128
extern unsigned char chan_color[2];
extern BLANKER_CONTROL_INFO bln[];
extern int largest_blnfit;
extern int *blanker_handle;
extern int timf2_noise_floor;
extern float timf2_despiked_pwr[];
extern float timf2_despiked_pwrinc[2];
extern int blanker_pulsewidth;
extern int refpul_n;
extern int refpul_size;
extern int timf2_noise_floor_avgnum;
extern int blanker_info_update_counter;
extern int blanker_info_update_interval;
extern int blnfit_range;
extern int blnclear_range;
extern int clever_blanker_rate;
extern int stupid_blanker_rate;

extern float blanker_pol_c1,blanker_pol_c2,blanker_pol_c3;
extern int timf2_show_pointer;
extern float blanker_phaserot;
extern float *blanker_refpulse;
extern float *blanker_phasefunc;
extern float *blanker_input;
extern int *blanker_pulindex;
extern char *blanker_flag;

#endif
