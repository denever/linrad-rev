// *********************************************************************
// *  This software uses the Perseus API which is defined through the  *
// *  following four files of the Microtelecom Software Defined Radio  *
// *  Developer Kit (SDRK):                                            *
// *  main.cpp, perseusdll.cpp, perseusdefs.h, perseusdll.h            *
// *  To use the Perseus hardware you need the perseususb.dll file     *
// *  You also need at least one of the following files:               *
// *  perseus125k24v21.sbs                                             *
// *  perseus250k24v21.sbs                                             *
// *  perseus500k24v21.sbs                                             *
// *  perseus1m24v21.sbs                                               *
// *  perseus2m24v21.sbs                                               *
// *  The dll and the sbs files are copyright protected and only       *
// *  available at the official Microtelecom Internet site:            *
// *  http://www.microtelecom.it                                       *
// *********************************************************************

#include <string.h>
#include "globdef.h"
#include "uidef.h"
#include "vernr.h"
#include "perseusdef.h"

#include <thrdef.h>
#include <sdrdef.h>
#include <fft1def.h>
#include <screendef.h>
#include <hwaredef.h>
#include <rusage.h>
#include <unistd.h>

extern int perseus_att_counter;
extern int perseus_nco_counter;

Key48 perseus95k24v31_signature= {{0x22, 0x33, 0xB8, 0xEA, 0x2C, 0xBC }};
Key48 perseus125k24v21_signature={{0x45, 0x7C, 0xD4, 0x3A, 0x0E, 0xD4 }};
Key48 perseus250k24v21_signature={{0xCF, 0x4B, 0x9F, 0x02, 0xB2, 0x1E }};
Key48 perseus500k24v21_signature={{0xF2, 0x9E, 0x67, 0x49, 0x51, 0x6B }};
Key48 perseus1m24v21_signature=  {{0xC5, 0xC1, 0x96, 0x4D, 0xA8, 0x05 }};
Key48 perseus2m24v21_signature=  {{0xDE, 0xEA, 0x57, 0x52, 0x14, 0x35 }};

void open_perseus(void);
char *perseus_name(void);
char *perseus_frname(void);
int perseus_eeprom_read(unsigned char *buf, int addr, unsigned char count);
void perseus_store_att(unsigned char ch);
void perseus_store_presel(unsigned char ch);
void perseus_store_sio(void);
void lir_perseus_read(void);
void start_perseus_read(void);
void perseus_stop(void);
int perseus_load_proprietary_code(int rate_no);

// Structure for Perseus parameters
typedef struct {
int rate_no;
int clock_adjust;
int presel;
int preamp;
int dither;
int check;
}P_PERSEUS;
#define MAX_PERSEUS_PARM 6 
P_PERSEUS pers;

char *perseus_parm_text[MAX_PERSEUS_PARM]={"Rate no",           //0
                                           "Sampl. clk adjust", //1
                                           "Preselector",       //2
                                           "Preamp",            //3
                                           "Dither",            //4
                                           "Check"};            //5
int rate_factor[MAX_PERSEUS_RATES]={840,640,320,160,80,40};

char *perseus_bitstream_names[MAX_PERSEUS_RATES]={
                 "perseus95k24v31",
                 "perseus125k24v21",
                 "perseus250k24v21",
                 "perseus500k24v21",
                 "perseus1m24v21",
                 "perseus2m24v21"};

Key48 *perseus_bitstream_signatures[MAX_PERSEUS_RATES]={
                               &perseus95k24v31_signature,
                               &perseus125k24v21_signature,
                               &perseus250k24v21_signature,
                               &perseus500k24v21_signature,
                               &perseus1m24v21_signature,
                               &perseus2m24v21_signature};
float perseus_filter_cutoff[MAX_PERSEUS_FILTER+1]={
                  1.7,
                  2.1,
                  3.0,
                  4.2,
                  6.0,
                  8.4,
                 12.0,
                 17.0,
                 24.0,
                 32.0,
                 BIG}; 

// Data structure which holds controls for the receiver ADC and DDC
SIOCTL sioctl;

// Perseus EEPROM Definitions and data structures
#define ADDR_EEPROM_PRODID 8
#pragma pack(1)
typedef struct {
unsigned short int sn;       // Receiver Serial Number
unsigned short int prodcode; // Microtelecom Product Code
unsigned char hwrel;         // Product release
unsigned char hwver;         // Product version
unsigned char signature[6];  // Microtelecom Original Product Signature
} eeprom_prodid;
#pragma pack()
char *rate_names[MAX_PERSEUS_RATES]={"95.2k","125k","250k","500k","1M","2M"};

int init_perseus(void)
{
int i, k;
char s[80];
char *ss;
int *sdr_pi;
int line;
char *looking;
eeprom_prodid prodid;
FILE *perseus_file;
looking="Looking for a Perseus on the USB port.";
// Set device_no to appropriate values if some dedicated hardware
// is selected by the user.
//  ------------------------------------------------------------
// See if there is a Perseus on the system.
settextcolor(12);
lir_text(10,10,looking);
SNDLOG"\n%s",looking);
lir_text(10,11,"Reset CPU, USB and SDR hardware if system hangs here.");
lir_refresh_screen();
settextcolor(14);
open_perseus();
settextcolor(7);
clear_lines(10,11);
lir_refresh_screen();
line=0;
if(sdr != -1)
  {
  lir_text(5,5,"A Perseus is detected on the USB port.");
  lir_text(5,6,"Do you want to use it for RX input (Y/N)?");
qpers:;
  await_processed_keyboard();
  if(kill_all_flag) goto pers_errexit;
  if(lir_inkey == 'Y')
    {
    ss=perseus_frname();
    if(ss == NULL)goto pers_errexit;
    sprintf(s,"USB Friendly device Name is: %s",ss);
    lir_text(2,8,s);
    SNDLOG"%s\n",s);
    if( strcmp(perseus_name_string,ss) == 0)
      {
      ui.rx_addev_no=PERSEUS_DEVICE_CODE;
      }
    if(ui.rx_addev_no == -1)  
      {
      lir_text(5,12,"Unknown hardware, can not use.");
      lir_text(5,13,press_any_key);
      await_keyboard();
      if(kill_all_flag) goto pers_errexit;
      clear_screen();
      }
    else
      {
      ss=perseus_name();
      if(ss == NULL)goto pers_errexit;
      sprintf(s,"USB Device Name is: %s",ss);
      lir_text(2,9,s);
      SNDLOG"%s\n",s);
      ui.rx_input_mode=IQ_DATA+DIGITAL_IQ+DWORD_INPUT;
      ui.rx_rf_channels=1;
      ui.rx_ad_channels=2;
      ui.rx_admode=0;
      perseus_eeprom_read((unsigned char*)&prodid, 
                                        ADDR_EEPROM_PRODID, sizeof(prodid));
      sprintf(s,"Microtelecom product code: 0x%04X", prodid.prodcode);
      lir_text(2,10,s);
      SNDLOG"%s\n",s);
      sprintf(s,"Serial Number: %05hd-%02hX%02hX-%02hX%02hX-%02hX%02hX",
				(unsigned short)prodid.sn,
				(unsigned short)prodid.signature[5],
				(unsigned short)prodid.signature[4],
				(unsigned short)prodid.signature[3],
				(unsigned short)prodid.signature[2],
				(unsigned short)prodid.signature[1],
				(unsigned short)prodid.signature[0]);
      lir_text(2,11,s);
      SNDLOG"%s\n",s);
      sprintf(s,"Hardware version/revision: %d.%d",
                         (unsigned)prodid.hwver, (unsigned)prodid.hwrel);
      lir_text(2,12,s);
      SNDLOG"%s\n",s);
      lir_text(10,16,"PRESS ANY KEY");
      await_processed_keyboard();
      if(kill_all_flag) goto pers_errexit;
      clear_screen();
      sprintf(s,"%s selected for input",ss);
      lir_text(10,line,s);
      line+=2;
      k=MAX_PERSEUS_RATES-1;
      if(ui.newcomer_mode != 0)k-=2;
      for(i=0; i<MAX_PERSEUS_RATES; i++)
        {
        if(i>k)settextcolor(8);
        sprintf(s,"%d   %sHz",i,rate_names[i]);
        lir_text(5,line,s);
        line++;
        }
      settextcolor(7);  
      line++;  
      sprintf(s,"Set output sampling rate (by number 0 to %d)",
                                                        MAX_PERSEUS_RATES-1);
      lir_text(5,line,s);
      pers.rate_no=lir_get_integer(50, line, 2, 0, k);
      if(kill_all_flag)goto pers_errexit;
      line++;
      if(ui.newcomer_mode != 0)
        {
        pers.clock_adjust=0;
        }
      else
        {  
        sprintf(s,"Set sampling clock shift (Hz))");
        lir_text(5,line,s);
        pers.clock_adjust=lir_get_integer(50, line, 5,-10000,10000);
        if(kill_all_flag)goto pers_errexit;
        line++;
        }
      adjusted_sdr_clock=PERSEUS_SAMPLING_CLOCK+pers.clock_adjust;
      ui.rx_ad_speed=adjusted_sdr_clock/rate_factor[pers.rate_no];
      if(ui.newcomer_mode != 0)
        {
        pers.presel=1;
        pers.preamp=1;
        pers.dither=1;
        }
      else
        {  
        lir_text(5,line,"Enable preselector ? (0/1)");
        pers.presel=lir_get_integer(32, line, 1, 0, 1);
        if(kill_all_flag)goto pers_errexit;
        line++;
        lir_text(5,line,"Enable preamp ? (0/1)");
        pers.preamp=lir_get_integer(27, line, 1, 0, 1);
        if(kill_all_flag)goto pers_errexit;
        line++;
        lir_text(5,line,"Enable dither ? (0/1)");
        pers.dither=lir_get_integer(27, line, 1, 0, 1);
        if(kill_all_flag)goto pers_errexit;
        line++;
        }
      pers.check=PERSEUS_PAR_VERNR;
      perseus_file=fopen("par_perseus","w");
      if(perseus_file == NULL)
        {
pers_errexit:;
        close_perseus();
        clear_screen(); 
        return 0;
        }
      sdr_pi=(void*)(&pers);
      for(i=0; i<MAX_PERSEUS_PARM; i++)
        {
        fprintf(perseus_file,"%s [%d]\n",perseus_parm_text[i],sdr_pi[i]);
        }
      parfile_end(perseus_file);
      }
    }
  else
    {
    if(lir_inkey != 'N')goto qpers;
    clear_screen();
    }
  close_perseus();
  sdr=-1;
  }
else
  {
  SNDLOG"open_perseus failed.\n");
  }
return line;
}

void set_perseus_att(void)
{
int i;
i=-fg.gain/10;
perseus_store_att(i);
}

void set_perseus_frequency(void)
{
unsigned char sl;
double dt1;
sl=PERSEUS_FLT_WB;
if(pers.presel !=0 )
  {
  sl=0;
// select filter according to frequency.
  while(perseus_filter_cutoff[sl]<fg.passband_center)sl++;
  }  
perseus_store_presel(sl);
if(pers.preamp != 0)sioctl.ctl|=PERSEUS_SIO_GAINHIGH;
if(pers.dither != 0)sioctl.ctl|=PERSEUS_SIO_DITHER;
dt1=fg.passband_center*1000000+0.5;
if(dt1 < 0 || dt1 > adjusted_sdr_clock/2)
  {
  fg.passband_center=7.05;
  dt1=fg.passband_center*1000000;
  }
sioctl.freg=4.294967296E9*dt1/adjusted_sdr_clock;
perseus_store_sio();
}

void perseus_input(void)
{
#if RUSAGE_OLD == TRUE
int local_workload_counter;
#endif
int *sdr_pi;
char *testbuff;
FILE *perseus_file;
char s[80];
char *ss;
int i, j, k, errcod;
double dt1, read_start_time, total_reads;
short int *isho;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
float t1;
int local_perseus_att_counter;
int local_perseus_nco_counter;
clear_thread_times(THREAD_PERSEUS_INPUT);
local_perseus_att_counter=perseus_att_counter;
local_perseus_nco_counter=perseus_nco_counter;
errcod=0;
j=0;
dt1=current_time();
while(sdr == -1)
  {
  open_perseus();
  lir_sleep(30000);
  if(sdr == -1)
    {
    sprintf(s,"Waiting %.2f", current_time()-dt1);
    lir_text(0,screen_last_line,s);
    lir_refresh_screen();
    if(kill_all_flag)goto perseus_init_error_exit;
    }
  }                                                
ss=perseus_frname();
errcod=1100;
if(ss == NULL)goto perseus_error_exit;
if( strcmp(perseus_name_string,ss) != 0)goto perseus_error_exit;
// ****************************************************
// We have a connection to the Perseus.
perseus_file=fopen("par_perseus","r");
// Read control parameters from par_xxx_perseus
if(perseus_file == NULL)goto perseus_error_exit;
errcod=1080;
sdr_pi=(void*)(&pers);
testbuff = malloc(4096);
if(testbuff == NULL)
  {
  lirerr(1003);
  goto perseus_error_exit;
  }
for(i=0; i<4096; i++)testbuff[i]=0;
i=fread(testbuff,1,4095,perseus_file);
fclose(perseus_file);
if(i >= 4095)goto perseus_error_exit;
k=0;
for(i=0; i<MAX_PERSEUS_PARM; i++)
  {
  while( (testbuff[k]==' ' || testbuff[k]== '\n' ) && k<4095)k++;
  j=0;
  while(testbuff[k]== perseus_parm_text[i][j] && k<4095)
    {
    k++;
    j++;
    } 
  if(perseus_parm_text[i][j] != 0)goto perseus_error_exit;    
  while(testbuff[k]!='[' && k<4095)k++;
  if(k>=4095)goto perseus_error_exit;
  sscanf(&testbuff[k],"[%d]",&sdr_pi[i]);
  while(testbuff[k]!='\n' && k<4095)k++;
  if(k>=4095)goto perseus_error_exit;
  }
if( pers.check != PERSEUS_PAR_VERNR ||
    pers.rate_no < 0 || 
    abs(pers.clock_adjust) > 10000 ||
    pers.rate_no  >= MAX_PERSEUS_RATES)goto perseus_error_exit;
sioctl.ctl=0;
adjusted_sdr_clock=PERSEUS_SAMPLING_CLOCK+pers.clock_adjust;
t1=ui.rx_ad_speed-adjusted_sdr_clock/rate_factor[pers.rate_no];
if(fabs(t1/ui.rx_ad_speed) > 0.0001)
  {
  errcod=1091;
  goto perseus_error_exit; 
  }
errcod=perseus_load_proprietary_code(pers.rate_no);
if(errcod != 0)goto perseus_error_exit;
ui.rx_ad_speed=adjusted_sdr_clock/rate_factor[pers.rate_no];
set_perseus_att();
set_perseus_frequency();
fft1_block_timing();
fft1_hz_per_point=(float)ui.rx_ad_speed/fft1_size;
rxin_local_workload_reset=workload_reset_flag;
timing_loop_counter_max=interrupt_rate;
timing_loop_counter=2.5*timing_loop_counter_max;
screen_loop_counter_max=0.1*interrupt_rate;
if(screen_loop_counter_max==0)screen_loop_counter_max=1;
screen_loop_counter=screen_loop_counter_max;
read_start_time=current_time();
total_reads=0;
initial_skip_flag=2;
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_PERSEUS_INPUT] == 
                                           THRFLAG_KILL)goto perseus_error_exit;
    lir_sleep(10000);
    }
  }  
start_perseus_read();
if(sdr != 0) goto perseus_error_exit;
thread_status_flag[THREAD_PERSEUS_INPUT]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_PERSEUS_INPUT] == THRFLAG_ACTIVE)
  {
#if RUSAGE_OLD == TRUE
  if(local_workload_counter != workload_counter)
    {
    local_workload_counter=workload_counter;
    make_thread_times(THREAD_PERSEUS_INPUT);
    }
#endif
  if(local_perseus_att_counter != perseus_att_counter)
    {
    local_perseus_att_counter=perseus_att_counter;
    set_perseus_att();
    }
  if(local_perseus_nco_counter != perseus_nco_counter)
    {
    local_perseus_nco_counter=perseus_nco_counter;
    set_perseus_frequency();
    }
  timing_loop_counter--;
  if(timing_loop_counter == 0)
    {
    timing_loop_counter=timing_loop_counter_max;
    if(initial_skip_flag != 0)
      {
      read_start_time=current_time();
      total_reads=0;
      initial_skip_flag--;
      }
    else
      {
      total_reads+=timing_loop_counter_max;
      dt1=current_time()-read_start_time;
      measured_ad_speed=total_reads*rxad.block_frames/dt1;
      }
    }
  isho=(void*)(&timf1_char[timf1p_pa]);
  timf1p_pnb=timf1p_pa;
  lir_perseus_read();
  if(kill_all_flag)goto perseus_exit;
  finish_rx_read(isho);
  if(kill_all_flag) goto perseus_exit;
  }
perseus_exit:;
perseus_stop();
perseus_error_exit:;
if(errcod != 0)lirerr(errcod);
close_perseus();
perseus_init_error_exit:;
thread_status_flag[THREAD_PERSEUS_INPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_PERSEUS_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

