

#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include "globdef.h"
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "sigdef.h"
#include "screendef.h"
#include "seldef.h"
#include "thrdef.h"
#include "txdef.h"
#include "vernr.h"
#include "hwaredef.h"
#include "options.h"
#include "keyboard_def.h"

#define TX_WRBLOCK_MAXCNT 200

#define TX_DA_MARGIN 0.97
#define PILOT_TONE_CONTROL 0.00000000001

#if BUFBARS == TRUE
#define TX_INDICATOR_MAXNUM 4
int tx_indicator_bufpos[TX_INDICATOR_MAXNUM];
#endif

int radar_cnt;
int radar_pulse_cnt;
int radar_cnt_max;



void tx_ssb_buftim(char cc)
{
int i1,i2,i3,i4,i5,i6,i7,j;
i1=2*lir_tx_input_samples();
fprintf( dmp,"\n%c in %d",cc,i1);
i2=2*(mictimf_pa-mictimf_px+mictimf_bufsiz)&mictimf_mask;
fprintf( dmp," m.timf %d",i2);
i3=((micfft_pa-micfft_px+micfft_bufsiz)&micfft_mask)/2;
fprintf( dmp," m.fft %d",i3);
i4=((cliptimf_pa-cliptimf_px+micfft_bufsiz)&micfft_mask);
fprintf( dmp," cl.timf %d",i4); 
i5=((clipfft_pa-clipfft_px+alc_bufsiz)&alc_mask)/(2*tx_pre_resample);
fprintf( dmp," cl.fft %d",i5);
i6=((alctimf_pa-((int)(alctimf_fx))+4+alc_bufsiz)&alc_mask)/tx_pre_resample;
fprintf( dmp," alctimf %d",i6);
i7=lir_tx_output_samples()/(tx_resample_ratio*tx_output_upsample);
fprintf( dmp," out %d",i7);
j=i1+i2+i3+i4+i5+i6+i7;
fprintf( dmp," tot %d tim %f",j,(float)(j)/ui.tx_ad_speed);
}

void tx_cw_buftim(char cc)
{
int i,j,k,n,m;
i=lir_tx_input_samples();
fprintf( dmp,"\n%c input %d",cc,i);
j=((mic_key_pa-mic_key_px+mic_key_bufsiz)&mic_key_mask)*tx_pre_resample;
fprintf( dmp," mic_key %d",j);
k=lir_tx_output_samples()/(tx_resample_ratio);
fprintf( dmp," output %d",k);
n=txout_pa/(tx_resample_ratio);
fprintf( dmp," txout %d",n);
m=i+j+k+n;
fprintf( dmp,"  tot %d  tim %f %f",m,(float)(m)/ui.tx_ad_speed,
                          current_time());
}

void do_cw_keying(void)
{
int i, k, nn, mm;
float amplitude, pilot_ampl;
float r1, r2;
double dt1;
pilot_ampl=0;
dt1=1/sqrt(tx_daout_sin*tx_daout_sin+tx_daout_cos*tx_daout_cos);
tx_daout_sin*=dt1;
tx_daout_cos*=dt1;
// Find out how many points we should place in the output buffer.
mm=(mic_key_pa-mic_key_px+mic_key_bufsiz)&mic_key_mask;
#if BUFBARS == TRUE
if(timinfo_flag!=0)
  {
  i=mm/mic_key_indicator_block;
  if(i != tx_indicator_bufpos[0])
    {
    tx_indicator_bufpos[0]=i;
    lir_hline(tx_indicator_first_pixel,indicator_ypix,
                                 tx_indicator_first_pixel+i-1,15);
    lir_hline(tx_indicator_first_pixel+i,indicator_ypix,
                                 tx_indicator_first_pixel+INDICATOR_SIZE-1,2);
    }
  }
#endif
repeat:;
r2=1.0000/(tx_resample_ratio*tx_pre_resample);
r1=(mm-mic_key_fx)/r2;
nn=r1;
// tx_cw_buftim('B');

for(i=0; i<nn; i++)
  {
  mic_key_fx+=r2;
  if(mic_key_fx > 1)
    {
    mic_key_fx-=1;
    mic_key_px=(mic_key_px+1)&mic_key_mask;
    if(tx_mic_key[mic_key_px] > 1)
      {
      tone_key=1;
      }
    else
      {
      tone_key=0;
      }
    }  
  if(tx_waveform_pointer==0 || 
                tx_waveform_pointer==tx_cw_waveform_points)
    {
    if(rx_mode == MODE_RADAR)
      {
      tot_key=tone_key;
      }
    else
      {  
      tot_key=(hand_key&txcw.enable_hand_key)|
             (tone_key&txcw.enable_tone_key);
      }
    }
  amplitude=txout_waveform[tx_waveform_pointer];
  tx_pilot_tone=-tx_pilot_tone;
  if(amplitude == 0)
    {
    pilot_ampl=0;
    }
  else
    {
    pilot_ampl=tx_pilot_tone;
    }
  if(tot_key)
    {
    if(tx_waveform_pointer == 0 && old_cw_rise_time != txcw.rise_time)
      {
      make_tx_cw_waveform();
      }
    if(tx_waveform_pointer < tx_cw_waveform_points)tx_waveform_pointer++;
    }
  else
    {
    if(tx_waveform_pointer > 0)
      {
      tx_waveform_pointer--;
      }
    if(amplitude < PILOT_TONE_CONTROL)pilot_ampl=0;
    }
  dt1=tx_daout_cos;
  tx_daout_cos= tx_daout_cos*tx_daout_phstep_cos+tx_daout_sin*tx_daout_phstep_sin;
  tx_daout_sin= tx_daout_sin*tx_daout_phstep_cos-dt1*tx_daout_phstep_sin;
  amplitude*=tx_output_amplitude*tx_onoff_flag;

  txout[2*txout_pa  ]=amplitude*tx_daout_cos+pilot_ampl;
  txout[2*txout_pa+1]=amplitude*tx_daout_sin-pilot_ampl;
  txout_pa++;
  if(txout_pa == txout_sizhalf)
    {
    if( (hand_key|tone_key) )
      {
      rxtx_state=TRUE;
      }
    else
      {
      rxtx_state=FALSE;
      }
    if(old_rxtx_state == FALSE && rxtx_state == TRUE)
      {
      hware_set_rxtx(TRUE);
      old_rxtx_state=TRUE;
      }
    if(rxtx_state == TRUE) rxtx_wait=16+lir_tx_output_samples()/txout_sizhalf;
    if(old_rxtx_state == TRUE && rxtx_state == FALSE)
      {
      rxtx_wait--;
      if(rxtx_wait == 0)
        {
        old_rxtx_state=FALSE;
        hware_set_rxtx(FALSE);
        }
      }
// Compute the total time from tx input to tx output and
// express it as samples of the tx input clock and store in tx_wsamps. 
    tx_wsamps=lir_tx_input_samples();
    k=(mic_key_pa-mic_key_px+mic_key_bufsiz)&mic_key_mask;
    tx_wsamps+=k*tx_pre_resample;
    tx_send_to_da();
    txout_pa=0;   
    goto new_resamp;
    }
  }  
new_resamp:;  
mm=(mic_key_pa-mic_key_px+mic_key_bufsiz)&mic_key_mask;
if(mm > 1 && lir_tx_output_samples() < txda.buffer_frames-
                                txda.block_frames)goto repeat;
// tx_cw_buftim('X');

}

void tx_send_to_da(void)
{
int i;
float t1, t2;

//tx_ssb_buftim('W');

switch(tx_output_mode)
  {
  case 0:
  for(i=0; i<txout_sizhalf; i++)
    {
    t1=txout[2*i  ];
    tx_out_shi[i]=TX_DA_MARGIN*0x7fff*t1;
    }
  lir_tx_dawrite((char*)tx_out_shi);
  break;
  
  case 1:
  for(i=0; i<txout_sizhalf; i++)
    {
    t1=txout[2*i  ];
    t2=txout[2*i+1];
    tx_out_shi[2*i]=TX_DA_MARGIN*0x7fff*t2;
    tx_out_shi[2*i+1]=TX_DA_MARGIN*0x7fff*t1;
    }
  lir_tx_dawrite((char*)tx_out_shi);
  break;

  case 2:
  for(i=0; i<txout_sizhalf; i++)
    {
    t1=txout[2*i  ];
    tx_out_int[i]=TX_DA_MARGIN*0x7fff0000*t1;
    }
  lir_tx_dawrite((char*)tx_out_int);
  break;
  
  case 3:
  for(i=0; i<txout_sizhalf; i++)
    {
    t1=txout[2*i  ];
    t2=txout[2*i+1];
    tx_out_int[2*i  ]=TX_DA_MARGIN*0x7fff0000*t2;
    tx_out_int[2*i+1]=TX_DA_MARGIN*0x7fff0000*t1;

    }
  lir_tx_dawrite((char*)tx_out_int);
  break;
  }
tx_wsamps+=(float)(lir_tx_output_samples())/
              (tx_resample_ratio*tx_output_upsample);
tx_wttim=(float)(tx_wsamps)/ui.tx_ad_speed;
tx_wttim_sum+=tx_wttim;
tx_wrblock_no++;

/*
char s[80];
sprintf(s,"wttim=%f   %f",tx_wttim,1/txda.interrupt_rate);
lir_text(0,0,s);
//fprintf( dmp,"\ntx_wttim %f  sumavg %f",tx_wttim,tx_wttim_sum/tx_wrblock_no);
*/
if(tx_wrblock_no == TX_WRBLOCK_MAXCNT)
  {
  tx_wrblock_no=0;
  tx_wttim_sum/=TX_WRBLOCK_MAXCNT;
  t1=tx_wttim_sum-tx_ideal_delay;

/*
sprintf(s,"wttim_sum=%f   t1=%f  (%f)",tx_wttim_sum,t1,tx_ideal_delay);
lir_text(0,1,s);
//fprintf( dmp,"\n%s",s);
*/

  t2=t1-tx_wttim_diff;
  tx_wttim_diff=t1;
// We have accumulated an error of t1 seconds while writing 
// TX_WRBLOCK_MAXCNT data blocks. Each block spans a time of
// txout_sizhalf/txssb.output speed so the relative error is
// X=t1/(TX_WRBLOCK_MAXCNT*txout_sizhalf/txssb.output speed);
// Therefore we should multiply the output 
// speed by (1-X)
// Use half the correction and add in a small part of t1,
// the deviation from the initial time delay.
  new_tx_resample_ratio=tx_resample_ratio*(1-0.5*(t2+0.2*t1)*
                           ui.tx_da_speed/(TX_WRBLOCK_MAXCNT*txout_sizhalf));    
  tx_resample_ratio=0.9*tx_resample_ratio+0.1*new_tx_resample_ratio;
  tx_wttim_sum=0;
  }
}



void run_tx_output(void)
{
int i, k;
float t1, t2, r2, prat;
tx_setup_flag=FALSE;
#if BUFBARS == TRUE
for(i=0; i<TX_INDICATOR_MAXNUM; i++)tx_indicator_bufpos[i]=-1;
#endif
tot_key=0;
thread_status_flag[THREAD_TX_OUTPUT]=THRFLAG_ACTIVE;
// Allow the receive side to use the CPU for a while.  
if(!kill_all_flag)lir_sem_wait(SEM_TXINPUT);
switch (rx_mode)
  {
  case MODE_SSB:
  micfft_px=micfft_pa;
  while(micfft_px == micfft_pa && !kill_all_flag)lir_sem_wait(SEM_TXINPUT);
  for(i=0;i<8; i++)
    {
    if(!kill_all_flag)lir_sem_wait(SEM_TXINPUT);
    }
  micfft_px=micfft_pa;
  while(!kill_all_flag && 
                    thread_command_flag[THREAD_TX_OUTPUT] == THRFLAG_ACTIVE)
    {
//    tx_ssb_buftim('A');
    lir_sem_wait(SEM_TXINPUT);
#if BUFBARS == TRUE
    if(timinfo_flag!=0)
      {
      i=(micfft_pa-micfft_px+micfft_bufsiz)&micfft_mask;
      i/=micfft_indicator_block;
      if(i != tx_indicator_bufpos[0])
        {
        tx_indicator_bufpos[0]=i;
        lir_hline(tx_indicator_first_pixel,indicator_ypix,
                                 tx_indicator_first_pixel+i-1,15);
        lir_hline(tx_indicator_first_pixel+i,indicator_ypix,
                                 tx_indicator_first_pixel+INDICATOR_SIZE-1,2);
        }
      }
#endif
    tx_agc_factor=tx_agc_decay*tx_agc_factor+(1-tx_agc_decay);
    micfft_bin_minpower=micfft_bin_minpower_ref*tx_agc_factor*tx_agc_factor;
    micfft_minpower=micfft_minpower_ref*tx_agc_factor*tx_agc_factor;
    k=tx_ssb_step2(&t1);
    if(k!=0)
      {
      tx_ssb_step3(&t1);
      }
// In case AGC has reduced the gain by more than 20 dB, set the
// AGC factor to -20 dB immediately because voice should never
// be as loud. This is to avoid the agc release time constant
// for impulse sounds caused by hitting the microphone etc.       
    if(tx_agc_factor < 0.1)tx_agc_factor=0.1;
    if(k!=0)
      {
      fftback(mic_fftsize, mic_fftn, &micfft[micfft_px], 
                             micfft_table, micfft_permute,FALSE);
      r2=0;
      for(i=0; i<mic_fftsize; i++)
        {
        t2=micfft[micfft_px+2*i  ]*micfft[micfft_px+2*i  ]+
           micfft[micfft_px+2*i+1]*micfft[micfft_px+2*i+1];
        if(t2>r2)r2=t2;
        }
      if(r2 > 0.01*MAX_DYNRANGE)
        {
        r2=1/sqrt(r2);
        if(r2>10/tx_agc_factor)r2=10/tx_agc_factor;
        }
      else
        {
        r2=0;
        }  
      }
    else
      {
      r2=0;
      }  
//    tx_ssb_buftim('B'); 
    tx_ssb_step4(r2,&t1,&prat);
//    tx_ssb_buftim('C'); 
    tx_ssb_step5();
//    tx_ssb_buftim('D'); 
    if(lir_tx_output_samples() < txda.buffer_frames-2*txda.block_frames)
      {    
      while(clipfft_px != clipfft_pa)
        {
        tx_ssb_step6(&prat);
        }
      while( ((alctimf_pa-alctimf_pb+alc_bufsiz)&alc_mask) >= 3*alc_fftsize)
        {
        tx_ssb_step7(&prat);
        }
      tx_ssb_step8();
      }
    }
  break;

  case MODE_WCW:
  case MODE_NCW:
  case MODE_HSMS:
  case MODE_QRSS:
  case MODE_RADAR:
  mic_key_px=mic_key_pa;
  while(mic_key_px == mic_key_pa && !kill_all_flag)lir_sem_wait(SEM_TXINPUT);
  for(i=0;i<8; i++)
    {
    if(!kill_all_flag)lir_sem_wait(SEM_TXINPUT);
    }
  mic_key_px=mic_key_pa;
// Fill one extra buffer before starting the output.
  lir_sem_wait(SEM_TXINPUT);
  while(!kill_all_flag && 
                    thread_command_flag[THREAD_TX_OUTPUT] == THRFLAG_ACTIVE)
    {
    lir_sem_wait(SEM_TXINPUT);
//    tx_cw_buftim('A');
    do_cw_keying();    
    }
  break;
  }
thread_status_flag[THREAD_TX_OUTPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_TX_OUTPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void cwproc_setup(void)
{
char color;
char *onoff[]={"Disabled", "Enabled"};
char s[256];
int i, k, pa, default_cwproc_no;
int old_tone_key, old_hand_key;
int isum;
float t1, t2, t3;
rx_mode=MODE_NCW;
tg.spproc_no=0;
set_hardware_tx_frequency();
default_cwproc_no=tg.spproc_no;
if(read_txpar_file()==FALSE)
  {
  set_default_cwproc_parms();
  }
if(txcw.rise_time < 1000*CW_MIN_TIME_CONSTANT)
                         txcw.rise_time=1000*CW_MIN_TIME_CONSTANT;
if(txcw.rise_time > 1000*CW_MAX_TIME_CONSTANT)
                         txcw.rise_time=1000*CW_MAX_TIME_CONSTANT;
if(ui.tx_pilot_tone_prestart > 1000*CW_MAX_TIME_CONSTANT)
  {
  lirerr(1070);
  return;
  }
tx_setup_flag=TRUE;
init_txmem_cwproc();
if(kill_all_flag)return;
lir_mutex_init();
lir_sleep(50000);
linrad_thread_create(THREAD_TX_INPUT);
restart:;
tx_waveform_pointer=0;
if(txcw.rise_time < 1000*CW_MIN_TIME_CONSTANT)
                         txcw.rise_time=1000*CW_MIN_TIME_CONSTANT;
if(txcw.rise_time > 1000*CW_MAX_TIME_CONSTANT)
                         txcw.rise_time=1000*CW_MAX_TIME_CONSTANT;
make_tx_cw_waveform();
old_tone_key=-1;
old_hand_key=-1;
tot_key=0;
clear_screen();
lir_text(20,0,"Setup for CW keying.");
lir_text(0,2,"There are three ways to produce Morse code in Linrad:");
if(txcw.enable_tone_key == 0)settextcolor(7); else settextcolor(14);
sprintf(s,"Tone into the microphone input. (Fast keying)  [%s]",
                    onoff[txcw.enable_tone_key]);
lir_text(0,4,s);
if(txcw.enable_hand_key == 0)settextcolor(7); else settextcolor(14);
sprintf(s,"Parallel port pin 13. (Hand keying)  [%s]",
                                          onoff[txcw.enable_hand_key]);
lir_text(0,5,s);
if(txcw.enable_ascii == 0)settextcolor(7); else settextcolor(14);
sprintf(s,"Computer generated from ascii on keyboard.  [%s]",
                                          onoff[txcw.enable_ascii]);
lir_text(0,6,s);
settextcolor(7);
lir_text(0,7,"Press 'T', 'H' or 'C' to toggle Enable/Disable");
sprintf(s,"Rise time = %.2f ms. ('A' to change)",txcw.rise_time*0.001);
lir_text(0,9,s);
make_txproc_filename();
sprintf(s,"Press 'S' to save %s, 'R' to read.  '+' or '-' to change file no.",
                                                 txproc_filename);
lir_text(0,10,s);

k=screen_last_col;
if(k>255)k=255;
for(i=0; i<k; i++)s[i]='-';
s[k]=0;
lir_text(0,MIC_KEY_LINE,s);
lir_text(0,MIC_KEY_LINE+8,s);
//                         0123456789012345678901234567890
lir_text(0,MIC_KEY_LINE+1,"Average       Max        Min");
lir_text(0,MIC_KEY_LINE+3,
"Set tone level and frequency for numbers well above 1.0 with key down.");
lir_text(0,MIC_KEY_LINE+4,
"Make sure the numbers are well below 1.0 with key up.");
lir_text(0,MIC_KEY_LINE+5,
"Typically Min(key down) > 10.0 and Max(key up) < 0.1.");
sprintf(s,
  "Your Nyquist frequency is %.1f kHz so you should use a keying tone",
                                                     0.0005*ui.tx_ad_speed);
lir_text(0,MIC_KEY_LINE+6,s);
lir_text(0,MIC_KEY_LINE+7,"that is a little lower than this.");
for(i=0; i<4; i++) lir_sem_wait(SEM_TXINPUT);
t1=0;
t2=0;
t3=BIG;
isum=0;
while(!kill_all_flag)
  {
  lir_refresh_screen();
  lir_sem_wait(SEM_TXINPUT);
  pa=mic_key_px;
  while(pa != mic_key_pa)
    {
    isum++;
    t1+=tx_mic_key[pa];
    if(t2 < tx_mic_key[pa])t2=tx_mic_key[pa];
    if(t3 > tx_mic_key[pa])t3=tx_mic_key[pa];
    pa=(pa+1)&mic_key_mask;
    }
  if(isum > 4000)
    {
    t1/=isum;  
    sprintf(s,"%.3f",t1);
    lir_text(1,MIC_KEY_LINE+2,s);
    sprintf(s,"%.3f",t2);
    lir_text(12,MIC_KEY_LINE+2,s);
    sprintf(s,"%.3f",t3);
    lir_text(23,MIC_KEY_LINE+2,s);
    if(t1 < 1)
      {
      tone_key=0;
      }
    else
      {
      tone_key=1;
      }  
    if(tone_key != old_tone_key)
      {
      old_tone_key=tone_key;
      if(tone_key == 0)
        {
        color=7;
        }
      else
        {
        color=12;
        }
      lir_fillbox(5*text_width, KEY_INDICATOR_LINE*text_height, 10*text_width,
                                                        3*text_height,color);
      lir_text(8,KEY_INDICATOR_LINE+1,"TONE");
      lir_sched_yield();    
      }
    t1=0;
    t2=0;
    t3=BIG;
    isum=0;
    }
  hware_hand_key();
  if(hand_key != old_hand_key)
    {
    old_hand_key=hand_key;
    if(hand_key == 0)
      {
      color=7;
      }
    else
      {
      color=12;
      }
    lir_fillbox(20*text_width, KEY_INDICATOR_LINE*text_height, 10*text_width,
                                                        3*text_height,color);
    lir_text(22,KEY_INDICATOR_LINE+1,"HAND");
    lir_sched_yield();    
    }
  do_cw_keying();
  test_keyboard();
  if(lir_inkey != 0)
    {
    process_current_lir_inkey();
    if(lir_inkey=='X')goto end_tx_setup;
    switch (lir_inkey)
      {
      case 'A':
      lir_text(0,TXCW_INPUT_LINE,"Rise time (ms):");
      t1=lir_get_float(15,TXCW_INPUT_LINE,6,
                                  CW_MIN_TIME_CONSTANT,CW_MAX_TIME_CONSTANT);
      txcw.rise_time=1000*t1;  
      break;

      case 'T':
      txcw.enable_tone_key^=1;
      txcw.enable_tone_key&=1;
      break;
      
      case 'H':
      txcw.enable_hand_key^=1;
      txcw.enable_hand_key&=1;
      break;

      case 'C':
      txcw.enable_ascii^=1;
      txcw.enable_ascii&=1;
      break;

      case 'S':
      clear_screen();
      save_tx_parms(TRUE);
      break;

      case 'R':
      clear_screen();
      read_txpar_file();
      break;

      case '+':
      tg.spproc_no++;
      if(tg.spproc_no > MAX_CWPROC_FILES)tg.spproc_no=MAX_CWPROC_FILES;
      break;

      case '-':
      tg.spproc_no--;
      if(tg.spproc_no < 0)tg.spproc_no=0;
      break;
      }
    goto restart;
    }
  }
end_tx_setup:;
close_tx_output();
linrad_thread_stop_and_join(THREAD_TX_INPUT);
close_tx_input();
if(txmem_handle != NULL)free(txmem_handle);
txmem_handle=NULL;
lir_sem_free(SEM_TXINPUT);
lir_mutex_destroy();
tg.spproc_no=default_cwproc_no;
}

void make_tx_phstep(void)
{
float t1;
t1=2*PI_L*(tg_basebfreq*1000000+tx_hware_fqshift+
                                            tx_ssbproc_fqshift)/ui.tx_da_speed;
tx_daout_phstep_cos=cos(t1);
tx_daout_phstep_sin=sin(t1);
}

void init_txmem_common(float micblock_time)
{
tx_pilot_tone=pow(10.0,-0.05*ui.tx_pilot_tone_db);
tx_output_amplitude=pow(10.0,-0.05*tg.level_db);
tx_indicator_first_pixel=indicator_first_pixel+INDICATOR_SIZE+2*text_width;
tx_daout_cos=1;
tx_daout_sin=0;
micsize=ui.tx_ad_speed*micblock_time;
make_power_of_two(&micsize);
txad.frame=ui.tx_ad_channels*ui.tx_ad_bytes;
txad.block_bytes=txad.frame*micsize;
lir_sem_init(SEM_TXINPUT);
lir_sleep(200000);
}

void set_txmem_sizes(void)
{
int i;
mictimf_block=txad.block_bytes/ui.tx_ad_bytes;
micsize=mictimf_block/ui.tx_ad_channels;
micn=0;
i=micsize;
while(i>1)
  {
  i>>=1;
  micn++;
  }
mic_sizhalf=micsize/2;
mictimf_bufsiz=16*mictimf_block;
mictimf_mask=mictimf_bufsiz-1;
}

void init_txmem_cwproc(void)
{
int i;
init_txmem_common(MICBLOCK_CWTIME);
open_tx_input();
if(kill_all_flag)return;
// Allow the input buffer to hold 80 milliseconds of data (or more)
// Keying with a tone into the microphone input should
// be done with a tone frequency near, but below the Nyquist frequency
// which is ui.tx_ad_speed/2.
// We do not want faster rise times than 1 ms for a dot length
// of 2 ms minimum or a dot rate of 250 Hz maximum.
// With at least 4 points on the rise time we need a sampling
// rate of 4kHz.
set_txmem_sizes();
tx_pre_resample=1;
i=1.05*ui.tx_ad_speed/2;
mic_key_block=mictimf_block;
if(rx_mode == MODE_RADAR)i/=2;
while(i > 4000)
  {
  i/=2;
  mic_key_block/=2;
  tx_pre_resample*=2;
  }
if(tx_pre_resample < 4 && rx_mode != MODE_RADAR)lirerr(1147);
tx_resample_ratio=(float)(ui.tx_da_speed)/ui.tx_ad_speed;
new_tx_resample_ratio=tx_resample_ratio;
mic_key_bufsiz=16*mic_key_block;
mic_key_mask=mic_key_bufsiz-1;
mic_key_indicator_block=mic_key_bufsiz/INDICATOR_SIZE;
txout_fftsize=2*mic_key_block*tx_resample_ratio*tx_pre_resample;
make_power_of_two(&txout_fftsize);
txda.frame=ui.tx_da_bytes*ui.tx_da_channels;
txda.block_bytes=txout_fftsize*txda.frame/2;
open_tx_output();
if(kill_all_flag)return;
tx_ssb_resamp_block_factor=1;
tx_output_upsample=1;
txout_sizhalf=txda.block_bytes/txda.frame;
txout_fftsize=2*txout_sizhalf;
tx_ideal_delay=1.2/txda.interrupt_rate+1.2/txad.interrupt_rate+0.002;
//fprintf( dmp,"\nideal  del %f\n",tx_ideal_delay);

max_tx_cw_waveform_points=ui.tx_da_speed*(0.0022*CW_MAX_TIME_CONSTANT);
init_memalloc(txmem, MAX_TXMEM_ARRAYS);
if(ui.tx_ad_bytes == 2)
  {
  mem(51,&mictimf_shi,mictimf_bufsiz*sizeof(short int),0);
  }
else
  {
  mem(51,&mictimf_int,mictimf_bufsiz*sizeof(int),0);
  }
mem(52,&tx_mic_key,mic_key_bufsiz*sizeof(float),0);
mem(53,&txout,2*txout_fftsize*sizeof(float),0);
mem(54,&txout_waveform,max_tx_cw_waveform_points*sizeof(float),0);
if(ui.tx_da_bytes==4)
  {
  mem(55,&tx_out_int,ui.tx_da_channels*txout_fftsize*sizeof(int)/2,0);
  }
else
  {  
  mem(56,&tx_out_shi,ui.tx_da_channels*txout_fftsize*sizeof(int)/2,0);
  }
txmem_size=memalloc((int*)(&txmem_handle),"txmem");
if(txmem_size == 0)
  {
  lirerr(1261);
  return;
  }
tx_wrblock_no=0;
tx_wttim_ref=0;
tx_wttim_sum=0;
tx_wttim_diff=0;
tx_ssbproc_fqshift=0;
make_tx_phstep();
make_tx_modes();  
make_tx_cw_waveform();
mic_key_pa=0;
mic_key_px=0;
mictimf_pa=0;
mictimf_px=0;
txout_pa=0;
mic_key_fx=0;
tx_waveform_pointer=0;
}

void init_txmem_spproc(void)
{
unsigned int j;
int i, k;
float t1;
// Speech processing is done in both the time domain and in the
// frequency domain.
// The length of an FFT block determines the frequency
// resolution as well as the total time delay through
// the processes.
// Set micsize to span a suitable time and suggest the same
// time span for a dma buffer to the tx input soundcard.
init_txmem_common(MICBLOCK_SSBTIME);
open_tx_input();
if(kill_all_flag)return;
// The open function may have changed the block size.
// Set micsize to fit the dma buffer we actually have.
set_txmem_sizes();
txad_hz_per_bin=(0.5*ui.tx_ad_speed)/micsize;
tx_lowest_bin=txssb.minfreq/txad_hz_per_bin;
if(tx_lowest_bin < 1)tx_lowest_bin=1;
if(tx_lowest_bin > micsize/3)tx_lowest_bin=micsize/3;
tx_highest_bin=txssb.maxfreq/txad_hz_per_bin;
if(tx_highest_bin > micsize/2-2)tx_highest_bin=micsize/2-2;
if(tx_highest_bin < tx_lowest_bin+1)tx_highest_bin=tx_lowest_bin+1;
t1=(tx_highest_bin-tx_lowest_bin+1)*txad_hz_per_bin;
if(ui.tx_da_channels == 1)t1*=2;
if(t1 > 0.75*ui.tx_da_speed)
  {
  t1=ui.tx_da_speed;
  if(ui.tx_da_channels == 1)t1/=2;
  t1/=txad_hz_per_bin;
  tx_highest_bin=tx_lowest_bin+t1;
  }
mic_fftsize=micsize;
mic_fftn=micn;
k=1.2*(tx_highest_bin-tx_lowest_bin);
// The microphone sampling speed may be low since we have no interest
// in frequencies above 2.5 kHz or so.
// An amplitude clipper in Hilbert space must however be oversampled.
// Oversampling by 16 will place most of the overtones generated by 
// the clipping process outside the passband. Without oversampling 
// all the overtones will alias into the passband and destroy the
// clipping process.
k*=16;
while(k < mic_fftsize)
  {
  mic_fftsize/=2;
  mic_fftn--;
  }    
mic_sizhalf=mic_fftsize/2;
micfft_block=2*mic_fftsize;
micfft_bufsiz=32*mic_fftsize;
micfft_indicator_block=micfft_bufsiz/INDICATOR_SIZE;
micfft_mask=micfft_bufsiz-1;
tx_filter_ia1=mic_fftsize+(tx_lowest_bin/2-tx_highest_bin/2-1);
tx_filter_ib1=tx_filter_ia1+tx_highest_bin-tx_lowest_bin-mic_fftsize;
k=mic_fftsize/tx_filter_ib1;
alc_fftn=mic_fftn;
alc_fftsize=mic_fftsize;
tx_pre_resample=1;
k++;
while(k<8)
  {
  tx_pre_resample*=2;
  k*=2;
  alc_fftn++;
  alc_fftsize*=2;
  }
// We allow data to stack up in the alc buffer when the resampler
// runs too fast. Make a big buffer.
alc_bufsiz=64*alc_fftsize;
alc_mask=alc_bufsiz-1;
alc_sizhalf=alc_fftsize/2;      
alc_block=2*alc_fftsize;
tx_resample_ratio=(float)(2*ui.tx_da_speed)/ui.tx_ad_speed;
tx_output_upsample=0.5;
txout_fftsize=micsize;
txout_fftn=micn;
while(tx_resample_ratio > 1.5)
  {
  tx_resample_ratio/=2;
  tx_output_upsample*=2;
  txout_fftsize*=2;
  txout_fftn++;
  }
txda.frame=ui.tx_da_bytes*ui.tx_da_channels;
txda.block_bytes=txout_fftsize*txda.frame/2;
j=txda.block_bytes;
open_tx_output();
if(kill_all_flag)return;
tx_ssb_resamp_block_factor=1;
while(j < txda.block_bytes)
  {
  j*=2;
  txout_fftsize*=2;
  tx_ssb_resamp_block_factor*=2;
  txout_fftn++;
  }
while(j > txda.block_bytes)
  {
  j/=2;
  txout_fftsize/=2;
  tx_ssb_resamp_block_factor/=2;
  txout_fftn--;
  }
// Time for minimum delay data expressed as samples of txad.  
t1=1.5*mic_fftsize;  // Samples in mictimf and cliptimf.

t1+=0.5*(1+SSB_DELAY_EXTRA)*alc_fftsize/tx_pre_resample;
tx_ideal_delay=2/txda.interrupt_rate+2*t1/ui.tx_ad_speed;
//fprintf( dmp,"\nideal samps %f  del %f\n",t1, tx_ideal_delay);

new_tx_resample_ratio=tx_resample_ratio;
txout_sizhalf=txout_fftsize/2;
init_memalloc(txmem, MAX_TXMEM_ARRAYS);
if(ui.tx_ad_bytes == 2)
  {
  mem(1,&mictimf_shi,mictimf_bufsiz*sizeof(short int),0);
  }
else
  {
  mem(1,&mictimf_int,mictimf_bufsiz*sizeof(int),0);
  }
mem(2,&micfft,micfft_bufsiz*sizeof(float),0);
mem(3,&cliptimf,micfft_bufsiz*sizeof(float),0);
mem(4,&mic_tmp,2*micsize*sizeof(float),0);
mem(5,&mic_table,micsize*sizeof(COSIN_TABLE),0);
mem(6,&mic_permute,2*micsize*sizeof(short int),0);
mem(7,&mic_win,(1+micsize)*sizeof(float),0);
mem(8,&mic_filter,micsize*sizeof(float),0);
mem(9,&micfft_table,micsize*sizeof(COSIN_TABLE)/2,0);
mem(10,&micfft_permute,micsize*sizeof(short int),0);
mem(11,&micfft_win,(1+mic_sizhalf)*sizeof(float),0);
mem(12,&clipfft,alc_bufsiz*sizeof(float),0);
mem(13,&alctimf,alc_bufsiz*sizeof(float),0);
mem(14,&alctimf_pwrf,alc_bufsiz*sizeof(float)/2,0);
mem(15,&alctimf_pwrd,alc_bufsiz*sizeof(float)/2,0);
mem(16,&tx_resamp,2*txout_fftsize*sizeof(float),0);
mem(17,&resamp_tmp,alc_fftsize*sizeof(float),0);
mem(18,&alc_permute,alc_fftsize*sizeof(short int),0);
mem(19,&alc_table,alc_sizhalf*sizeof(COSIN_TABLE),0);
mem(20,&alc_win,(1+alc_sizhalf)*sizeof(float),0);
mem(21,&txout,2*txout_fftsize*sizeof(float),0);
mem(22,&txout_permute,txout_fftsize*sizeof(short int),0);
mem(23,&txout_table,txout_sizhalf*sizeof(COSIN_TABLE),0);
mem(24,&txout_tmp,txout_fftsize*sizeof(float),0);
if(ui.tx_da_bytes==4)
  {
  mem(25,&tx_out_int,ui.tx_da_channels*txout_fftsize*sizeof(int)/2,0);
  }
else
  {  
  mem(25,&tx_out_shi,ui.tx_da_channels*txout_fftsize*sizeof(int)/2,0);
  }
mem(26,&cliptimf_mute,micfft_bufsiz*sizeof(char)/mic_fftsize,0);  
mem(27,&clipfft_mute,alc_bufsiz*sizeof(char)/alc_block,0);  
mem(28,&alctimf_mute,alc_bufsiz*sizeof(char)/alc_fftsize,0);  
txmem_size=memalloc((int*)(&txmem_handle),"txmem");
if(txmem_size == 0)
  {
  lirerr(1261);
  return;
  }
make_permute(2, micn, micsize, mic_permute);
make_window(2,micsize, 2, mic_win);
if(ui.tx_ad_bytes != 2)
  {
  for(i=0; i<micsize; i++)mic_win[i]/=0x10000;
  }
t1=1/(1.62*0x7fff*mic_sizhalf);  
for(i=0; i<micsize; i++)mic_win[i]*=t1;
make_sincos(2, micsize, mic_table); 
init_fft(0,mic_fftn, mic_fftsize, micfft_table, micfft_permute);
make_window(2,mic_sizhalf, 2, micfft_win);
micfft_winfac=1.62*mic_fftsize;
t1=1/micfft_winfac;
for(i=0; i<mic_sizhalf; i++)
  {
  micfft_win[i]*=t1;
  }
for(i=0; i<micfft_bufsiz; i++)
  {
  micfft[i]=0;
  cliptimf[i]=0;
  }
init_fft(0,alc_fftn, alc_fftsize, alc_table, alc_permute);
make_window(2,alc_sizhalf, 2, alc_win);
t1=1/(1.62*alc_fftsize);
for(i=0; i<alc_sizhalf; i++)
  {
  alc_win[i]*=t1;
  }
for(i=0; i<alc_bufsiz; i++)
  {
  alctimf[i]=0;
  clipfft[i]=0;
  }
for(i=0; i<alc_bufsiz/alc_block; i++)clipfft_mute[i]=0;
for(i=0; i<alc_bufsiz/2; i++)alctimf_pwrf[i]=0;
init_fft(0,txout_fftn, txout_fftsize, txout_table, txout_permute);
for(i=0; i<2*txout_fftsize; i++)tx_resamp[i]=0;
tx_filter_ia1=mic_fftsize+(tx_lowest_bin/2-tx_highest_bin/2-1);
tx_filter_ia2=alc_fftsize+(tx_lowest_bin/2-tx_highest_bin/2-1);
tx_filter_ia3=txout_fftsize+(tx_lowest_bin/2-tx_highest_bin/2-1);
tx_filter_ib1=tx_filter_ia1+tx_highest_bin-tx_lowest_bin-mic_fftsize;
tx_filter_ib2=tx_filter_ia2+tx_highest_bin-tx_lowest_bin-alc_fftsize;
tx_filter_ib3=tx_filter_ia3+tx_highest_bin-tx_lowest_bin-txout_fftsize;
tx_ssbproc_fqshift=(float)(tx_highest_bin-tx_filter_ib1)*txad_hz_per_bin;
// Construct the microphone input filter from the
// parameters supplied by the user.
for(i=0; i<mic_fftsize; i++)mic_filter[i]=0;
// Store the desired frequency response in dB.
t1=0;
for(i=tx_lowest_bin; i<=tx_highest_bin; i++)
  {
  mic_filter[i]=t1;
  t1+=0.001*txssb.slope*txad_hz_per_bin;
  }
k=(tx_highest_bin-tx_lowest_bin)/4;
t1=0;
for(i=tx_lowest_bin+k; i>=tx_lowest_bin; i--)
  {
  t1+=0.001*txssb.bass*txad_hz_per_bin;
  mic_filter[i]+=t1;
  }
t1=0;
for(i=tx_highest_bin-k; i<=tx_highest_bin; i++)
  {
  t1+=0.001*txssb.treble*txad_hz_per_bin;
  mic_filter[i]+=t1;
  }
// Convert from dB to linear scale (amplitude) and normalize
// the filter curve.
t1=0;
for(i=tx_lowest_bin; i<=tx_highest_bin; i++)
  {
  mic_filter[i]=pow(10.,0.05*mic_filter[i]);
  t1+=mic_filter[i]*mic_filter[i];
  }
t1=1/sqrt(t1/(tx_highest_bin-tx_lowest_bin+1));
for(i=tx_lowest_bin; i<=tx_highest_bin; i++)
  {
  mic_filter[i]*=t1;
  }
mic_filter[tx_lowest_bin]*=0.5;
mic_filter[tx_highest_bin]*=0.5;
micfft_bin_minpower_ref=pow(10.,0.1*(txssb.mic_f_threshold-100));
micfft_minpower_ref=pow(10.,0.1*(txssb.mic_t_threshold+txssb.mic_f_threshold-90));
tx_agc_factor=1;
tx_agc_decay=pow(2.718,-100.0/(txssb.mic_agc_time*txad_hz_per_bin));
txpwr_decay=pow(2.718,-1/(ui.tx_ad_speed*0.025));
tx_forwardpwr=0;
tx_backpwr=0;
tx_ph1=1;
tx_wrblock_no=0;
tx_wttim_ref=0;
tx_wttim_sum=0;
tx_wttim_diff=0;
memcheck(1,txmem,(int*)&txmem_handle);
make_tx_modes();
make_tx_phstep();
tx_resamp_ampfac1=1;
tx_resamp_ampfac2=1;
micfft_pa=0;
micfft_px=0;
mictimf_pa=0;
mictimf_px=0;
cliptimf_pa=0;
cliptimf_px=0;
clipfft_pa=0;
clipfft_px=0;
alctimf_pa=SSB_DELAY_EXTRA*alc_fftsize;
alctimf_pa=0;//öö
alctimf_pb=0;
alctimf_fx=0;
tx_output_flag=0;
}

void tx_input(void)
{
int i, ia, ib, pa, pb, pc;
int k;
int twosiz, ssb_bufmin;
float *z;
float t1;
twosiz=2*micsize;
ssb_bufmin=twosiz*ui.tx_ad_channels;
pb=mictimf_px;
thread_status_flag[THREAD_TX_INPUT]=THRFLAG_ACTIVE;
while(!kill_all_flag && 
                    thread_command_flag[THREAD_TX_INPUT] == THRFLAG_ACTIVE)
  {
  if(ui.tx_ad_bytes == 2)
    {
    lir_tx_adread((char*)&mictimf_shi[mictimf_pa]);
    }
  else
    {
    lir_tx_adread((char*)&mictimf_int[mictimf_pa]); 
    }
  mictimf_pa=(mictimf_pa+mictimf_block)&mictimf_mask;
  switch (rx_mode)
    {
    case MODE_SSB:
    if( ((mictimf_pa-mictimf_px+mictimf_bufsiz)&mictimf_mask) >= ssb_bufmin)
      {
// Copy the input to micfft while multiplying with the window function.
      ib=twosiz-1;
      pa=mictimf_px;
      switch(tx_input_mode)
        {
        case 0:
// One channel, two bytes.        
        pb=(pa+ib)&mictimf_mask;
        for( ia=0; ia<micsize; ia++)
          {
          mic_tmp[mic_permute[ia]]=(float)(mictimf_shi[pa])*mic_win[ia];
          if(abs(mictimf_shi[pa]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_shi[pa]);
            }
          mic_tmp[mic_permute[ib]]=(float)(mictimf_shi[pb])*mic_win[ia];
          if(abs(mictimf_shi[pb]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_shi[pb]);
            }
          ib--; 
          pa=(pa+1)&mictimf_mask;
          pb=(pb+mictimf_mask)&mictimf_mask;
          }
        break;
        
        case 1:
// In case there are two channels for the microphone, just use one of them!!
        pb=(pa+2*ib)&mictimf_mask;
        for( ia=0; ia<micsize; ia++)
          {
          mic_tmp[mic_permute[ia]]=mictimf_shi[pa]*mic_win[ia];
          if(abs(mictimf_shi[pa]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_shi[pa]);
            }
          mic_tmp[mic_permute[ib]]=mictimf_shi[pb]*mic_win[ia];
          if(abs(mictimf_shi[pb]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_shi[pb]);
            }
          ib--; 
          pa=(pa+2)&mictimf_mask;
          pb=(pb+mictimf_mask-1)&mictimf_mask;
          }
        break;
        
        case 2:
        pb=(pa+ib)&mictimf_mask;
        for( ia=0; ia<micsize; ia++)
          {
          mic_tmp[mic_permute[ia]]=mictimf_int[pa]*mic_win[ia];
          if(abs(mictimf_int[pa]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_int[pa]);
            }
          mic_tmp[mic_permute[ib]]=mictimf_int[pb]*mic_win[ia];
          if(abs(mictimf_int[pb]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_int[pb]);
            }
          ib--; 
          pa=(pa+1)&mictimf_mask;
          pb=(pb+mictimf_mask)&mictimf_mask;
          }
        break;
        
        case 3:
// In case there are two channels for the microphone, just use one of them!!
        pb=(pa+2*ib)&mictimf_mask;
        for( ia=0; ia<micsize; ia++)
          {
          mic_tmp[mic_permute[ia]]=mictimf_int[pa]*mic_win[ia];
          if(abs(mictimf_int[pa]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_int[pa]);
            }
          mic_tmp[mic_permute[ib]]=mictimf_int[pb]*mic_win[ia];
          if(abs(mictimf_int[pb]) > tx_ad_maxamp)
            {
            tx_ad_maxamp=abs(mictimf_int[pb]);
            }
          ib--; 
          pa=(pa+2)&mictimf_mask;
          pb=(pb+mictimf_mask-1)&mictimf_mask;
          }
        }
// The microphone signal should be maximum 0x7fff or 0x7fffffffffff
// depending on whether 16 or 32 bit format is selected. 
      mictimf_px=(mictimf_px+mictimf_block)&mictimf_mask;
      fft_real_to_hermitian( mic_tmp, twosiz, micn, mic_table);
// Output is {Re(z^[0]),...,Re(z^[n/2),Im(z^[n/2-1]),...,Im(z^[1]).
// Clear those parts of the spectrum that we do not want and
// transfer the rest of the points to micfft while multiplying
// with our filter function. 
// Place the spectrum center at frequency zero so we can use
// the lowest possible sampling rate in further processing steps.
      z=&micfft[micfft_pa];
      pc=2*(tx_highest_bin/2-tx_lowest_bin/2);
      k=micfft_block-pc-2;
      while(pc < k)
        {
        z[pc  ]=0;
        z[pc+1]=0;
        pc+=2;
        }
      k=tx_lowest_bin;
      pc=2*tx_filter_ia1;
      while(pc < 2*mic_fftsize)
        {
        z[pc+1]=mic_tmp[k  ]*mic_filter[k];
        z[pc  ]=mic_tmp[twosiz-k]*mic_filter[k];
        pc+=2;
        k++;
        }
      pc=0;    
      while(k <= tx_highest_bin)
        {
        z[pc+1]=mic_tmp[k  ]*mic_filter[k];
        z[pc  ]=mic_tmp[twosiz-k]*mic_filter[k];
        pc+=2;
        k++;
        }
      micfft_pa=(micfft_pa+micfft_block)&micfft_mask;
      }
    break;

    case MODE_WCW:
    case MODE_NCW:
    case MODE_HSMS:
    case MODE_QRSS:
    switch(tx_input_mode)
      {
      case 0:
      while(mictimf_px != mictimf_pa)
        {
        t1=0;
        for(i=0; i<tx_pre_resample; i++)
          {
          t1+=abs(mictimf_shi[mictimf_px]-mictimf_shi[pb]);
          pb=mictimf_px;
          mictimf_px++;
          }
        mictimf_px=mictimf_px&mictimf_mask;
        tx_mic_key[mic_key_pa]=t1/(tx_pre_resample*1000);
        mic_key_pa=(mic_key_pa+1)&mic_key_mask;
        }
      break;

      case 1:
// In case there are two channels for the microphone, just use one of them!!
      while(mictimf_pa != mictimf_px)
        {
        t1=0;
        for(i=0; i<tx_pre_resample; i++)
          {
          t1+=abs(mictimf_shi[mictimf_px]-mictimf_shi[pb]);
          pb=mictimf_px;
          mictimf_px+=2;
          }
        mictimf_px=mictimf_px&mictimf_mask;
        tx_mic_key[mic_key_pa]=t1/(tx_pre_resample*1000);
        mic_key_pa=(mic_key_pa+1)&mic_key_mask;
        }
      break;
        
      case 2:
      while(mictimf_pa != mictimf_px)
        {
        t1=0;
        for(i=0; i<tx_pre_resample; i++)
          {
          t1+=abs(mictimf_int[mictimf_px]-mictimf_int[pb]);
          pb=mictimf_px;
          mictimf_px++;
          }
        mictimf_px=mictimf_px&mictimf_mask;
        tx_mic_key[mic_key_pa]=(t1/tx_pre_resample)/(0x7fff*1000);
        mic_key_pa=(mic_key_pa+1)&mic_key_mask;
        }
      break;
        
      case 3:
      while(mictimf_pa != mictimf_px)
        {
        t1=0;
        for(i=0; i<tx_pre_resample; i++)
          {
          t1+=abs(mictimf_int[mictimf_px]-mictimf_int[pb]);
          pb=mictimf_px;
          mictimf_px+=2;
          }
        mictimf_px=mictimf_px&mictimf_mask;
        tx_mic_key[mic_key_pa]=t1/(tx_pre_resample*1000);
        mic_key_pa=(mic_key_pa+1)&mic_key_mask;
        }
      break;
      }
    break;

    case MODE_RADAR:
    while(mictimf_px != mictimf_pa)
      {
      mictimf_px=(mictimf_px+tx_pre_resample)&mictimf_mask;
      if(radar_cnt < radar_pulse_cnt)
        {
        i=10;
        }
      else
        {
        i=0;
        }
      radar_cnt++;
      if(radar_cnt >= radar_cnt_max)radar_cnt=0;
      tx_mic_key[mic_key_pa]=i;
      mic_key_pa=(mic_key_pa+1)&mic_key_mask;
      }
    break;  
    }
  lir_sem_post(SEM_TXINPUT);
  }
thread_status_flag[THREAD_TX_INPUT]=THRFLAG_RETURNED;
lir_sem_post(SEM_TXINPUT);
while(thread_command_flag[THREAD_TX_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void make_tx_modes(void)
{
if(ui.tx_ad_bytes == 2)
  {
  if(ui.tx_ad_channels==1)
    {
    tx_input_mode=0;
    }
  else
    {
    tx_input_mode=1;
    }  
  }
else
  {
  if(ui.tx_ad_channels==1)
    {
    tx_input_mode=2;
    }
  else
    {
    tx_input_mode=3;
    }  
  }
if(ui.tx_da_bytes == 2)
  {
  if(ui.tx_da_channels==1)
    {
    tx_output_mode=0;
    }
  else
    {
    tx_output_mode=1;
    }  
  }
else
  {
  if(ui.tx_da_channels==1)
    {
    tx_output_mode=2;
    }
  else
    {
    tx_output_mode=3;
    }  
  }
}

void make_tx_cw_waveform(void)
{
int i, n1, n2;
double dt1, dt2;
// Use the (complementary) error function erfc(x) to get the optimum 
// shape for the rise time.
// Start at erfc(-3.3) and go to erfc(3.3). In this interval the function
// goes from  1.9999969423 to 0.0000030577 and jumping abruptly to 0 or
// 2.0 respectively corresponds to an abrupt keying of another signal
// that is 117 dB below the main signal.
n1=ui.tx_da_speed*(0.000001*ui.tx_pilot_tone_prestart);
n2=ui.tx_da_speed*(0.000001*txcw.rise_time);
n2&=-2;
tx_cw_waveform_points=n1+n2-1;
if(tx_cw_waveform_points >= max_tx_cw_waveform_points)
  {
  lirerr(1076);
  return;
  }
// Set a very small (non-zero) amplitude for use by the pilot tone
// so it will start the desired time before the actual output
// signal will start to follow the erfc function.
for(i=1; i<=n1; i++) txout_waveform[i]=0.5*PILOT_TONE_CONTROL;
dt1=3.3;
dt2=6.6/(n2-3);
for(i=n1+1; i<=tx_cw_waveform_points; i++)
  {
  txout_waveform[i]=erfc(dt1)/2;
  dt1-=dt2;
  }
txout_waveform[0]=0;
old_cw_rise_time=txcw.rise_time;
radar_cnt_max=0.001*txcw.radar_interval*ui.tx_ad_speed/tx_pre_resample;
radar_pulse_cnt=0.001*txcw.radar_pulse*ui.tx_ad_speed/tx_pre_resample;
radar_cnt=radar_cnt_max-radar_pulse_cnt;
}

int tx_setup(void)
{
int return_flag;
return_flag=FALSE;
if(read_txpar_file() == FALSE)set_default_spproc_parms();
rxad.frame=2*ui.rx_ad_channels;
if( (ui.rx_input_mode&DWORD_INPUT) != 0) rxad.frame*=2;
rxad.block_frames=ui.rx_ad_speed/1000;
if(rxad.block_frames < 4)rxad.block_frames=4;
make_power_of_two((int*)&rxad.block_frames);
rxad.block_bytes=2*ui.rx_ad_channels*rxad.block_frames;
if( (ui.rx_input_mode&DWORD_INPUT) != 0)rxad.block_bytes*=2;
genparm[DA_OUTPUT_SPEED]=ui.rx_min_da_speed;
if(genparm[DA_OUTPUT_SPEED] < 8000)genparm[DA_OUTPUT_SPEED]=8000;
rx_daout_bytes=ui.rx_max_da_bytes;
rx_daout_channels=ui.rx_max_da_channels;
rxda.frame=rx_daout_bytes*rx_daout_channels;
txda.frame=ui.tx_da_bytes*ui.tx_da_channels;
txda.block_bytes=(txda.frame*ui.tx_da_speed)/1000;
if(check_tx_devices() == FALSE)goto do_tx_setup;
// *******************************************************
// ***   The A/D and A/D setup is already done.
// ***   Ask the user what he wants.
// ***   The transmit functions of Linrad are in a very early stage
// ***   and there is not much to choose from yet............
// ******************************************************
repeat_menu:;
if(kill_all_flag) goto txend;
clear_screen();
if(check_tx_devices() == FALSE)goto do_tx_setup;
lir_text(5,6,"B=Soundcard setup for Tx.");
lir_text(5,7,"C=Speech processor setup.");
lir_text(5,8,"D=CW setup.");
lir_text(5,9,"X=Return to main menu");
await_processed_keyboard();
if(kill_all_flag) goto txend;
switch (lir_inkey)
  {
  case 'B':
do_tx_setup:;
  set_tx_io();
  return_flag=check_tx_devices();
  goto txend;
  
  case 'C':
  open_rx_sndin();
  open_rx_sndout();
  spproc_setup();
  close_rx_sndout();
  close_rx_sndin();
  break;

  case 'D':
  open_rx_sndin();
  open_rx_sndout();
  cwproc_setup();
  close_rx_sndout();
  close_rx_sndin();
  break;

  case 'X':
txend:;
  lir_inkey=0;
  return return_flag;
  }
goto repeat_menu;
}

