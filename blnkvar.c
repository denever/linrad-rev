#include "globdef.h"
#include "blnkdef.h"

unsigned char chan_color[2]={2,4};

int *blanker_handle;
float *blanker_refpulse;
float *blanker_phasefunc;
float *blanker_input;
int *blanker_pulindex;
char *blanker_flag;


BLANKER_CONTROL_INFO bln[BLN_INFO_SIZE];
float blanker_pol_c1;
float blanker_pol_c2;
float blanker_pol_c3;

int largest_blnfit;
int blnclear_range;
int blnfit_range;

int timf2_show_pointer;
int blanker_pulsewidth;
int refpul_n;
int refpul_size;
int timf2_noise_floor_avgnum;
int timf2_noise_floor;

float timf2_despiked_pwr[2];
float timf2_despiked_pwrinc[2];
int clever_blanker_rate;
int stupid_blanker_rate;
int blanker_info_update_counter;
int blanker_info_update_interval;
float blanker_phaserot;

