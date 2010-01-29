
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "globdef.h"
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "screendef.h"
#include "seldef.h"
#include "sigdef.h"
#include "hwaredef.h"
#include "thrdef.h"
#include "conf.h"
#include "rusage.h"
#include "options.h"
#include "blnkdef.h"
#include "txdef.h"

#define MOUSE_MIN_TIME 0.05

#if BUFBARS == TRUE
#define RX_INDICATOR_MAXNUM 8
int rx_indicator_bufpos[RX_INDICATOR_MAXNUM];
#endif

#define NBPROC_FFT1 1

void clear_baseb(void)
{
int i, n;
float raw_wttim,t1;
// We have to recalculate timf3 from old transforms.
// Find out how much time the data waiting before fft1 sums up to.
make_ad_wttim();
raw_wttim=ad_wttim;
// We need a total delay from input sample to output sample
// that can accomodate the most unfavourable situation while collecting
// enough data to make a new transform.
// Make da_wait_time the desired number of seconds from input sample
// to output sample.
// First a safety margin for processing delay and adjusting speed
// errors in case different soundboards are used for input and output.
// All delays are not maximum simultaneously. Subtract 10 ms
// just in case...
da_wait_time=0.001*genparm[OUTPUT_DELAY_MARGIN];
if(diskread_flag == 0)
  {
// Maximum delay inside device driver when buffer seems empty
  da_wait_time+=daout_samps/genparm[DA_OUTPUT_SPEED];
  }
// Maximum data waiting in timf3 that is not enough for a new fft3 transform
// plus the number of 50% interleaved transforms we want to wait.
da_wait_time+=fft3_size/timf3_sampling_speed;
t1=da_wait_time-raw_wttim;
// If we run in RDWR mode, devices can not be stopped immediately.
// Therefore we do not try to flush the output buffer.
// There is no reason to calculate fresh data for the time it takes
// to empty the output buffer.
if(rx_audio_out == rx_audio_in &&  rx_audio_out != -1)
  {
  da_wts=make_da_wts();
  if(kill_all_flag) return;
  da_wttim=(float)(da_wts)/genparm[DA_OUTPUT_SPEED];
  t1-=da_wttim;
  if(t1<0)t1=0;
  }
// We use sin squared windows for fft3 so we need to recalculate 
// one extra half transform to get the first transform output right.
t1+=0.5*fft3_size/timf3_sampling_speed;
// make data for 0.1 seconds extra
t1+=.1;
if(genparm[SECOND_FFT_ENABLE]!=0)
  {
// Add time for data waiting in timf2.
  make_fft2_wttim();
  raw_wttim+=timf2_wttim;
// Maximum data waiting in timf2 that is not enough for a new fft2 transform
  da_wait_time+=fft2_size/timf1_sampling_speed;
  sc[SC_HG_FQ_SCALE]++;
  n=t1/fft2_blocktime; 
  if(fft2_blocktime > 0.25)n++;
  if(genparm[AFC_ENABLE] != 0)
    {
    n+=afct_delay_points;
    da_wait_time+=afct_delay_points*fft2_blocktime;
    }
  if(n > fft2_nm)n=fft2_nm;
  if(n > max_fft2n-3)n=max_fft2n-3;
  fft2_nx=(fft2_na-n+max_fft2n)&fft2n_mask;
  fft2_nc=fft2_nx;
  new_fft2_averages();
  }
else
  {
// Maximum raw data waiting that is not enough for a new fft1 transform
  da_wait_time+=fft1_size/timf1_sampling_speed;
  n=t1/fft1_blocktime; 
  if(fft1_blocktime > 0.25)n++;
  if(genparm[AFC_ENABLE] != 0)
    {
    n+=afct_delay_points;
    da_wait_time+=afct_delay_points*fft1_blocktime;
    }
  i=fft1_nx;
  if(n > max_fft1n-3)n=max_fft1n-3;
  if(n>fft1_nm)n=fft1_nm;
  fft1_nx=(fft1_na-n+max_fft1n)&fft1n_mask;
  fft1_nc=fft1_nx;
  fft1_px=fft1_nx*fft1_block;
  }
timf3_pa=timf3_block;
timf3_px=0;
timf3_py=0;
timf3_ps=0;
for(i=0;i<timf3_block;i++)timf3_float[i]=0;
daout_pa=0;
daout_pb=0;
basebnet_pa=0;
basebnet_px=0;
da_resample_ratio=genparm[DA_OUTPUT_SPEED]/baseband_sampling_speed;
new_da_resample_ratio=da_resample_ratio;
for(i=0; i<genparm[MIX1_NO_OF_CHANNELS]; i++)
  {
  mix1_status[i]=0;
  }  
old_afct_delay=afct_delay_points;
poleval_pointer=0;
}

void rx_output(void)
{
#if RUSAGE_OLD == TRUE
int local_workload_counter;
#endif
int dasync_time_interval;
int dasync_errors;
char s[80];
double dt1;
int local_channels, local_bytes;
int i, local_workload_reset;
int speed_cnt;
float t1, t2;
double total_time2;
#if OSNUM == OS_FLAG_LINUX
clear_thread_times(THREAD_RX_OUTPUT);
#endif
#if RUSAGE_OLD == TRUE
local_workload_counter=workload_counter;
#endif
thread_command_flag[THREAD_RX_OUTPUT]=THRFLAG_IDLE;
local_channels=rx_daout_channels;
local_bytes=rx_daout_bytes;
open_rx_sndout();
if(ui.rx_dadev_no==-2)
  {
  rx_da_wrbuf=malloc(rx_daout_block);
  if(rx_da_wrbuf == NULL)
    {
    lirerr(102500);
    return;
    }
  }
if(kill_all_flag)goto da_output_error;
if(audio_dump_flag == 0 && rx_audio_out != -1)
  {
  for(i=0; i<rx_daout_block; i++)
    {
    rx_da_wrbuf[i]=0; 
    }
  lir_rx_dawrite();
  lir_rx_dawrite();
  }
da_start_samps=-1;  
dasync_errors=0;
goto idle;
resume:; 
if(da_start_samps == -1)
  {
  dasync_counter=0;
  dasync_sum=0;
  dasync_avg1=-1.5;
  dasync_avg2=-1.5;
  dasync_avg3=-1.5;
  dasync_time=current_time();
  dasync_avgtime=dasync_time;
  daspeed_time=dasync_time+1;
  }  
dasync_time_interval=7*fft3_size/timf3_sampling_speed;
if(dasync_time_interval < 7*fft1_size/timf1_sampling_speed)
                       dasync_time_interval = 7*fft1_size/timf1_sampling_speed;
if(genparm[SECOND_FFT_ENABLE]!=0)
  {
  if(dasync_time_interval < 7*fft2_size/timf1_sampling_speed)
                      dasync_time_interval = 7*fft2_size/timf1_sampling_speed;
  }
if(dasync_time_interval < 0.5)dasync_time_interval=0.5;
local_workload_reset=workload_reset_flag;
rx_da_maxbytes=0;
thread_status_flag[THREAD_RX_OUTPUT]=THRFLAG_ACTIVE;
speed_cnt=rx_output_blockrate;
while(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_ACTIVE)
  {
#if RUSAGE_OLD == TRUE
  if(local_workload_counter != workload_counter)
    {
    local_workload_counter=workload_counter;
    make_thread_times(THREAD_RX_OUTPUT);
    }
#endif
  speed_cnt--;
  if(speed_cnt==0)
    {
    if(local_workload_reset!=workload_reset_flag)
      {
      rx_da_maxbytes=0;
      local_workload_reset=workload_reset_flag;
      }
    speed_cnt=rx_output_blockrate;
    total_time2=recent_time;
    if(total_time2-daspeed_time > 0)
      {
// Compute the D/A speed once every second.
      daspeed_time+=1;
      i=lir_rx_output_bytes();
      if(kill_all_flag) goto da_output_error;
      if(i == -1)
        {
        da_start_samps=-1;  
        goto resume;
        }
      if(da_start_samps == -1)
        {
        da_start_time=total_time2; 
        da_block_counter=0;
        da_start_samps=i;
        rx_da_maxbytes=0;
        }
      else
        {
        measured_da_speed=(da_block_counter*rx_daout_block+i-da_start_samps)/
                    (rxda.frame*(total_time2-da_start_time));
        }
      }  
// Keep track of the error in the frequency ratio between input and output.
// Average the total processing delay and adjust resampling rate to 
// keep it constant. 
#define DASYNC_MAXCOUNT 15 //Number used to form average
    dt1=current_time();
    if(  diskread_flag == 0 && 
         new_baseb_flag == 0 &&
         dt1-dasync_avgtime > dasync_time_interval )
      {
      dasync_avgtime=dt1;
      lir_sched_yield();
      make_timing_info();
      if(kill_all_flag) goto da_output_error;
      if(timinfo_flag != 0)
        {
        sprintf(s,"sync:%f ",total_wttim-dasync_avg3);
        lir_text(26,screen_last_line-7,s);
        }
      if(dasync_avg3 >= 0 && fabs(total_wttim-dasync_avg3)>0.5)
        {
        dasync_errors++;
        sprintf(s,"DA SYNC ERRORS %d",dasync_errors);
        wg_error(s,WGERR_DASYNC);
        baseb_reset_counter++;
        da_start_samps=-1;  
        goto resume;
        }
      dasync_counter++;
      dasync_sum+=total_wttim;
      if(dasync_counter >= DASYNC_MAXCOUNT)
        {
        dasync_avg1=dasync_sum/dasync_counter;
        dasync_counter=0;
        dasync_sum=0;
        t2=dt1-dasync_time;
        dasync_time=dt1;
        if(dasync_avg2 > 0)
          {
// The drift between input and output after t2 seconds is
// the difference between the two averages.
// Convert to frequency ratio, but use half the error only 
// to avoid oscillations.
// Correct for the deviation from the first average, but only 20%
// so we drift slowy towards it if we are off.
          t1= (t2+0.5*(dasync_avg2-dasync_avg1)+
                                   0.2*(dasync_avg3-dasync_avg1))/t2;
          new_da_resample_ratio=da_resample_ratio*t1;
          dasync_avg2=dasync_avg1;
          }
        else
          {
          if(dasync_avg3 <0) 
            {
            dasync_avg3=dasync_avg1;
            }
          else
            {
            dasync_avg2=dasync_avg1;
            }
          }
        }
      }
    }
  thread_status_flag[THREAD_RX_OUTPUT]=THRFLAG_SEM_WAIT;
  lir_sem_wait(SEM_RX_DASIG);

  if(thread_command_flag[THREAD_RX_OUTPUT]!=THRFLAG_ACTIVE)goto daend;
  thread_status_flag[THREAD_RX_OUTPUT]=THRFLAG_ACTIVE;
// Write data to the output device.
  for(i=0; i<rx_daout_block; i++)
    {
    rx_da_wrbuf[i]=daout[daout_px]; 
    daout_px=(daout_px+1)&daout_bufmask;
    }
  if(audio_dump_flag == 0 && rx_audio_out != -1)
    {
    lir_rx_dawrite();
    }
  if(wav_write_flag != 0)
    {
    if(fwrite(rx_da_wrbuf,rx_daout_block,1,wav_file)!=1)wavsave_start_stop(0);
    if(kill_all_flag) goto da_output_error;
    wavfile_bytes+=rx_daout_block;
    }
  da_block_counter+=1;
daend:;
  }
if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_IDLE)
  {
  if(rx_audio_out != -1)lir_empty_da_device_buffer();
  thread_status_flag[THREAD_RX_OUTPUT] = THRFLAG_IDLE;
  lir_sleep(10000);
  while(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_IDLE)
    {
    lir_sleep(5000);
    }
  while(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_SEMCLEAR)
    {
    thread_status_flag[THREAD_RX_OUTPUT] = THRFLAG_SEMCLEAR;
    lir_sem_wait(SEM_RX_DASIG);
    }
  while(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_IDLE)
    {
idle:;  
    daout_px=0;
    thread_status_flag[THREAD_RX_OUTPUT] = THRFLAG_IDLE;
    lir_sleep(5000);
    }
  if(thread_command_flag[THREAD_RX_OUTPUT] == THRFLAG_ACTIVE)
    {
    if( local_channels != rx_daout_channels ||
        local_bytes != rx_daout_bytes)
      {
      if(rx_audio_out != -1 && 
          (ui.rx_damode != O_RDWR || ui.rx_dadev_no != ui.tx_addev_no))
        {
        close_rx_sndout();
        lir_sched_yield();
        open_rx_sndout();
        if(kill_all_flag) goto da_output_error;
        lir_empty_da_device_buffer();
        da_start_samps=-1;
        }
      local_channels=rx_daout_channels;
      local_bytes=rx_daout_bytes;
      }
    goto resume;
    }
  }
da_output_error:; 
if(rx_audio_out >= 0)
  {
  while( rx_audio_out == tx_audio_in)lir_sleep(10000);
  close_rx_sndout();
  }
if(ui.rx_dadev_no==-2)
  {
  free(rx_da_wrbuf);
  }
thread_status_flag[THREAD_RX_OUTPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_RX_OUTPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

int local_spurcancel_flag;
int local_spurinit_flag;

void spur_removal(void)
{
int i, k, m, mm;
double dt1;
float t2, t3;
k=0;
if(genparm[AFC_ENABLE] == 2 && wg.spur_inhibit == 0)
  {
  if(autospur_point < spur_search_last_point)
    {
    t2=1/interrupt_rate;
    t3=0.1*fftx_size/ui.rx_ad_speed;
    if(t2<t3)t2=t3;
    dt1=current_time();
    m=512;
    mm=16;
    if(m < fftx_size/1024)m=fftx_size/1024;
    if(mm < genparm[MAX_NO_OF_SPURS]/100)mm=genparm[MAX_NO_OF_SPURS]/100;
more_spurs:;
    i=no_of_spurs;
    k=0;
    lir_sched_yield();
    while( k<m && autospur_point < spur_search_last_point && 
                                                         no_of_spurs-i < mm)
      {
      init_spur_elimination();
      k++;
      }
    if(autospur_point < spur_search_last_point)
      {      
      if(current_time()-dt1 < t2)goto more_spurs;
      }                                
    }
  }  
if(local_spurinit_flag != spurinit_flag)
  {
  local_spurinit_flag=spurinit_flag;
  init_spur_elimination();
  }
if(local_spurcancel_flag != spurcancel_flag)
  {
  local_spurcancel_flag=spurcancel_flag;
  no_of_spurs=0;
  }
}

void second_fft(void)
{
#if RUSAGE_OLD == TRUE
int local_workload_counter;
#endif
#if BUFBARS == TRUE
int k;
#endif
#if OSNUM == OS_FLAG_LINUX
clear_thread_times(THREAD_SECOND_FFT);
#endif
#if RUSAGE_OLD == TRUE
local_workload_counter=workload_counter;
#endif
thread_status_flag[THREAD_SECOND_FFT]=THRFLAG_ACTIVE;
while(!kill_all_flag && 
       thread_command_flag[THREAD_SECOND_FFT] == THRFLAG_ACTIVE)
  {
#if RUSAGE_OLD == TRUE
  if(local_workload_counter != workload_counter)
    {
    local_workload_counter=workload_counter;
    make_thread_times(THREAD_SECOND_FFT);
    }
#endif
restart:;
  if( ((timf2_pn2-timf2_px+timf2_totbytes)&timf2_mask) <
                                              4*ui.rx_rf_channels*fft2_size )
    {
    if(genparm[AFC_ENABLE] >= 1 && genparm[SECOND_FFT_ENABLE] != 0)
      {
      fftx_na=fft2_na;
      fftx_nc=fft2_nc;
      fftx_nm=fft2_nm;
      fftx_nx=fft2_nx;
      spur_removal();
      }
    while( ((timf2_pn2-timf2_px+timf2_totbytes)&timf2_mask) < 
                                              4*ui.rx_rf_channels*fft2_size )
      {
      thread_status_flag[THREAD_SECOND_FFT]=THRFLAG_SEM_WAIT;
      lir_sem_wait(SEM_FFT2);
      thread_status_flag[THREAD_SECOND_FFT]=THRFLAG_ACTIVE;
      if(kill_all_flag || thread_command_flag[THREAD_SECOND_FFT] != 
                                                 THRFLAG_ACTIVE)goto fft2_x;
      }
    }
  else
    {
    lir_sched_yield();
    }  
// Add the two parts of timf2 so we get a single time function
// and produce the second fft.
// Note that the fft2 routine still is split into a number of small
// chunks so we have to call until the flag is set.
  make_fft2_status=FFT2_NOT_ACTIVE;
  while(make_fft2_status != FFT2_COMPLETE)
    {
    make_fft2();
    if(yieldflag_fft2_fft2)lir_sched_yield();
    fft1_nx=fft1_px/fft1_block;
    }
  lir_sem_post(SEM_FFT1);
#if BUFBARS == TRUE
  if(timinfo_flag!=0)
    {
    k=(fft2_na-fft2_nx+max_fft2n)&fft2n_mask;
    k*=fft2_size;
    k/=fft2n_indicator_block;
    if(k != rx_indicator_bufpos[3])
      {
      rx_indicator_bufpos[3]=k;
      lir_hline(indicator_first_pixel,indicator_ypix-9,
                             indicator_first_pixel+k-1,15);
      lir_hline(indicator_first_pixel+k,indicator_ypix-9,
                             indicator_first_pixel+INDICATOR_SIZE-1,1);
      }
    }
#endif    
  }
if(thread_command_flag[THREAD_SECOND_FFT]==THRFLAG_IDLE)
  {
  thread_status_flag[THREAD_SECOND_FFT]=THRFLAG_IDLE;
  while(thread_command_flag[THREAD_SECOND_FFT] == THRFLAG_IDLE)
    {
    lir_sleep(3000);
    }
  if(kill_all_flag) goto fft2_x;
  if(thread_command_flag[THREAD_SECOND_FFT] == THRFLAG_ACTIVE)goto restart;
  }  
fft2_x:;  
thread_status_flag[THREAD_SECOND_FFT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_SECOND_FFT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void timf2_routine(void)
{ 
#if RUSAGE_OLD == TRUE
int local_workload_counter;
#endif
#if BUFBARS == TRUE
int k;
#endif
int local_timf2_px;
#if OSNUM == OS_FLAG_LINUX
clear_thread_times(THREAD_TIMF2);
#endif
#if RUSAGE_OLD == TRUE
local_workload_counter=workload_counter;
#endif
local_timf2_px=timf2_px;
thread_status_flag[THREAD_TIMF2]=THRFLAG_ACTIVE;
while(!kill_all_flag && 
       thread_command_flag[THREAD_TIMF2] == THRFLAG_ACTIVE)
  {
#if RUSAGE_OLD == TRUE
  if(local_workload_counter != workload_counter)
    {
    local_workload_counter=workload_counter;
    make_thread_times(THREAD_TIMF2);
    }
#endif
restart:;
  thread_status_flag[THREAD_TIMF2]=THRFLAG_SEM_WAIT;
  lir_sem_wait(SEM_TIMF2);
  thread_status_flag[THREAD_TIMF2]=THRFLAG_ACTIVE;
  if(kill_all_flag || thread_command_flag[THREAD_TIMF2] != 
                                                 THRFLAG_ACTIVE)goto timf2_x;
  make_timf2();
#if BUFBARS == TRUE
  if(timinfo_flag!=0)
    {
    k=(timf2_pa+fft1_block-timf2_px+timf2_mask+1)&timf2_mask;
    k/=timf2_indicator_block;
    if(k != rx_indicator_bufpos[2])
      {
      rx_indicator_bufpos[2]=k;
      lir_hline(indicator_first_pixel,indicator_ypix-6,
                           indicator_first_pixel+k-1,15);
      lir_hline(indicator_first_pixel+k,indicator_ypix-6,
                           indicator_first_pixel+INDICATOR_SIZE-k,1);
      }
    }
#endif  
  first_noise_blanker();
  if( ((timf2_pn2-local_timf2_px+timf2_totbytes)&timf2_mask) >=
                                              4*ui.rx_rf_channels*fft2_size )
    {
    lir_sem_post(SEM_FFT2);
    local_timf2_px=(local_timf2_px+timf2_output_block)&timf2_mask;
    }
  }
if(thread_command_flag[THREAD_TIMF2]==THRFLAG_IDLE)
  {
  thread_status_flag[THREAD_TIMF2]=THRFLAG_IDLE;
  while(thread_command_flag[THREAD_TIMF2] == THRFLAG_IDLE)
    {
    lir_sleep(3000);
    }
  if(kill_all_flag) goto timf2_x;
  if(thread_command_flag[THREAD_TIMF2] == THRFLAG_ACTIVE)goto restart;
  }  
timf2_x:;  
thread_status_flag[THREAD_TIMF2]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_TIMF2] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void wideband_dsp(void)
{
#if RUSAGE_OLD == TRUE
int local_workload_counter;
#endif
int local_fft1_sumsq_pa;
int k;
int local_ampinfo_reset;
int local_timf2_px;
#if OSNUM == OS_FLAG_LINUX
clear_thread_times(THREAD_WIDEBAND_DSP);
#endif
#if RUSAGE_OLD == TRUE
local_workload_counter=workload_counter;
#endif
// Allow this thread to use 100% of one CPU if the computer has
// more than one CPU and FFT2 is not selected.
// If FFT2 is selected and we have more than 3 CPUs we can
// also run this thread without yeilds.
local_fft1_sumsq_pa=fft1_sumsq_pa;
local_ampinfo_reset=workload_reset_flag;
local_spurcancel_flag=spurcancel_flag;
local_spurinit_flag=spurinit_flag;
if(fft1_n > 12)
  {
  yieldflag_timf2_fft1=TRUE;
  }
else
  {
  yieldflag_timf2_fft1=TRUE;
  }
if( genparm[SECOND_FFT_ENABLE] != 0 ) 
  {
  linrad_thread_create(THREAD_SECOND_FFT);
  if(ui.max_blocked_cpus > 4)yieldflag_timf2_fft1=FALSE;
  if(no_of_processors > 1)
    {
    linrad_thread_create(THREAD_TIMF2);
    }
  }
else
  {
  if(ui.max_blocked_cpus > 2)yieldflag_timf2_fft1=FALSE;
  }  
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_WIDEBAND_DSP] == 
                                           THRFLAG_KILL)goto wide_error_exit;
    lir_sleep(10000);
    }
  }
restart:;
timf1p_px=timf1p_pb;
if( (ui.network_flag & NET_RXIN_FFT1) == 0)
  {
  while(!kill_all_flag && timf1p_px==timf1p_pb)
    {
    lir_sem_wait(SEM_TIMF1);
    }
  }  
timf1p_px=timf1p_pb;
local_timf2_px=timf2_px;
#if BUFBARS == TRUE
//for(i=0; i<RX_INDICATOR_MAXNUM; i++)rx_indicator_bufpos[i]=-1;
#endif
mouse_time_wide=current_time();
thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_ACTIVE;
while(!kill_all_flag &&
        thread_command_flag[THREAD_WIDEBAND_DSP] == THRFLAG_ACTIVE)
  {
  if(ampinfo_flag != 0)
    {
    if(local_ampinfo_reset!=workload_reset_flag)
      {
      local_ampinfo_reset=workload_reset_flag;
      clear_wide_maxamps();
      }
    }
#if RUSAGE_OLD == TRUE
  if(local_workload_counter != workload_counter)
    {
    local_workload_counter=workload_counter;
    make_thread_times(THREAD_WIDEBAND_DSP);
    }
#endif
  if( (ui.network_flag & NET_RXIN_FFT1) == 0)
    {
    if( ((timf1p_pb-timf1p_px+timf1_bytes)&timf1_bytemask) < timf1_blockbytes)
      {
      if(genparm[AFC_ENABLE] >= 1 && genparm[SECOND_FFT_ENABLE] == 0)
        {
        fftx_na=fft1_nb;
        fftx_nc=fft1_nc;
        fftx_nm=fft1_nm;
        fftx_nx=fft1_nx;
        spur_removal();
        }
      while( ((timf1p_pb-timf1p_px+timf1_bytes)&timf1_bytemask) < timf1_blockbytes)
        {
        thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_SEM_WAIT;
        lir_sem_wait(SEM_TIMF1);
        thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_ACTIVE;
        if(kill_all_flag || thread_command_flag[THREAD_WIDEBAND_DSP] != 
                                                   THRFLAG_ACTIVE)goto wideend;
        }
      }
    else
      {
      lir_sched_yield();
      }
    }
  else
    {
    if(genparm[AFC_ENABLE] >= 1 && genparm[SECOND_FFT_ENABLE] == 0)
      {
      fftx_na=fft1_nb;
      fftx_nc=fft1_nc;
      fftx_nm=fft1_nm;
      fftx_nx=fft1_nx;
      spur_removal();
      }
    thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_SEM_WAIT;
    lir_sem_wait(SEM_TIMF1);
    thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_ACTIVE;
    }
  if(kill_all_flag || thread_command_flag[THREAD_WIDEBAND_DSP] != 
                                                   THRFLAG_ACTIVE)goto wideend;
// Here we do the mouse actions that affect the wide graph.
// Currently only the waterfall memory area may become re-allocated
  if(recent_time-mouse_time_wide > MOUSE_MIN_TIME)
    {
    mouse_time_wide=current_time();
    if(mouse_task!=-1)
      {
      k=mouse_task&GRAPH_MASK;
      if( k < MAX_WIDEBAND_GRAPHS)
        {
        set_button_states();
        if(mouse_active_flag == 0)
          {
          switch (k)
            {
            case WIDE_GRAPH:
            if( (mouse_task&GRAPH_RIGHTPRESSED) != 0)
              {
              wide_graph_add_signal();
              if(rightpressed==BUTTON_IDLE)mouse_task=-1;
              goto mouse_x;
              }
            else
              {
              mouse_on_wide_graph();
              }
            break;

            case HIRES_GRAPH:
            mouse_on_hires_graph();
            break;

            case TRANSMIT_GRAPH:
            mouse_on_tx_graph();
            break;

            case FREQ_GRAPH:
            mouse_on_freq_graph();
            break;

            case RADAR_GRAPH:
            mouse_on_radar_graph();
            break;
            }  
          if(mouse_active_flag == 0)
            {
            lirerr(28877);
            goto wide_error_exit;
            }
          }
        if(numinput_flag==0)
          {
          current_mouse_activity();
          if(kill_all_flag)goto wide_error_exit;
          }  
        if( (numinput_flag&DATA_READY_PARM) != 0)
          {
          par_from_keyboard_routine();
          if(kill_all_flag)goto wide_error_exit;
          par_from_keyboard_routine=NULL;
          mouse_active_flag=0;
          numinput_flag=0;
          leftpressed=BUTTON_IDLE;  
          }
        if(mouse_active_flag == 0)
          {
          mouse_task=-1;
          }
mouse_x:;
        }
      }
    }
  if(kill_all_flag) goto wide_error_exit;
  if( (ui.network_flag & NET_RXIN_FFT1) == 0)
    {
    while( fft1_px == ((fft1_pa+fft1_block)&fft1_mask) )
      {
      lir_sleep(5000); 
      if(kill_all_flag || thread_command_flag[THREAD_WIDEBAND_DSP] != 
                                                   THRFLAG_ACTIVE)goto wideend;
      }
#if BUFBARS == TRUE
    if(timinfo_flag!=0)
      {
      k=rxad.block_bytes;
      if(ui.rx_addev_no >= 256)k<<=1;
      k=(timf1p_pa+k-timf1p_px+timf1_bytes)&timf1_bytemask;
      k/=timf1_indicator_block;
      if(k != rx_indicator_bufpos[0])
        {
        rx_indicator_bufpos[0]=k;
        lir_hline(indicator_first_pixel,indicator_ypix,
                                indicator_first_pixel+k-1,15);
        lir_hline(indicator_first_pixel+k,indicator_ypix,
                                   indicator_first_pixel+INDICATOR_SIZE-1,1);
        }
      }
#endif    
    fft1_b();
    }
  fft1_c();
#if BUFBARS == TRUE
  if(timinfo_flag!=0)
    {
    k=(fft1_pa+fft1_block-fft1_px+fft1_mask+1)&fft1_mask;
    k/=fft1_indicator_block;
    if(k != rx_indicator_bufpos[1])
      {
      rx_indicator_bufpos[1]=k;
      lir_hline(indicator_first_pixel,indicator_ypix-3,
                           indicator_first_pixel+k-1,15);
      lir_hline(indicator_first_pixel+k,indicator_ypix-3,
                           indicator_first_pixel+INDICATOR_SIZE-1,1);
      }
    }
#endif    
  if(fft1_waterfall_flag)
    {  
    if(rx_mode != MODE_TXTEST)
      {
      if( ((fft1_sumsq_pa-local_fft1_sumsq_pa+fft1_sumsq_bufsize)&
                   fft1_sumsq_mask) >= (1+((4+wg_fft_avg2num)>>3))*fft1_size)
        {
        sc[SC_SHOW_FFT1]++;
        lir_sem_post(SEM_SCREEN);
        local_fft1_sumsq_pa = fft1_sumsq_pa;
        }
      }  
    else
      {
      if(local_fft1_sumsq_pa != fft1_sumsq_pa)
        {
        sc[SC_SHOW_FFT1]++;     
        lir_sem_post(SEM_SCREEN);
        local_fft1_sumsq_pa = fft1_sumsq_pa;
        }
      }
    }
  if( genparm[SECOND_FFT_ENABLE] == 0 ) 
    {
    if(fft1_waterfall_flag)fft1_waterfall();
    lir_sem_post(SEM_FFT1);
    }
  else
    {
    if(no_of_processors > 1)
      {
      lir_sem_post(SEM_TIMF2);
      }
    else
      {  
      if(yieldflag_timf2_fft1)lir_sched_yield();
      make_timf2();
#if BUFBARS == TRUE
      if(timinfo_flag!=0)
        {
        k=(timf2_pa+fft1_block-timf2_px+timf2_mask+1)&timf2_mask;
        k/=timf2_indicator_block;
        if(k != rx_indicator_bufpos[2])
          {
          rx_indicator_bufpos[2]=k;
          lir_hline(indicator_first_pixel,indicator_ypix-6,
                               indicator_first_pixel+k-1,15);
          lir_hline(indicator_first_pixel+k,indicator_ypix-6,
                               indicator_first_pixel+INDICATOR_SIZE-k,1);
          }
        }
#endif  
      first_noise_blanker();
      if( ((timf2_pn2-local_timf2_px+timf2_totbytes)&timf2_mask) >=
                                              4*ui.rx_rf_channels*fft2_size )
        {
        lir_sem_post(SEM_FFT2);
        local_timf2_px=(local_timf2_px+timf2_output_block)&timf2_mask;
        }
      }
    }
wideend:;
  }
if(thread_command_flag[THREAD_WIDEBAND_DSP]==THRFLAG_IDLE)
  {
// The wideband dsp thread is running and it puts out
// the waterfall graph. 
// We stop it now on order from the tx_test thread.
  thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_IDLE;
  while(thread_command_flag[THREAD_WIDEBAND_DSP]==THRFLAG_IDLE)
    {
    lir_sem_wait(SEM_TIMF1);
    timf1p_px=(timf1p_px+timf1_blockbytes)&timf1_bytemask;
    }
  if(thread_command_flag[THREAD_WIDEBAND_DSP] == THRFLAG_ACTIVE)goto restart;
  }
wide_error_exit:;  
if( genparm[SECOND_FFT_ENABLE] != 0 ) 
  {
  linrad_thread_stop_and_join(THREAD_SECOND_FFT);
  if(no_of_processors > 1)
    {
    linrad_thread_stop_and_join(THREAD_TIMF2);
    }
  }
thread_status_flag[THREAD_WIDEBAND_DSP]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_WIDEBAND_DSP] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void narrowband_dsp(void)
{
#if RUSAGE_OLD == TRUE
int local_workload_counter;
#endif
int fftx_flag;
int local_baseb_reset_counter;
int resblk;
int i, k, baseb_output_block;
int syncflag;
int idle_flag;
double dt1, disksync_time;
double total_time1, baseb_clear_time;
float t1;
#if OSNUM == OS_FLAG_LINUX
clear_thread_times(THREAD_NARROWBAND_DSP);
#endif
#if RUSAGE_OLD == TRUE
local_workload_counter=workload_counter;
#endif
#if BUFBARS == TRUE
for(i=0; i<RX_INDICATOR_MAXNUM; i++)rx_indicator_bufpos[i]=-1;
#endif
new_baseb_flag=-1;
total_time1=current_time();
dt1=total_time1;  
disksync_time=total_time1;
mouse_time_narrow=total_time1;
idle_flag=0;
syncflag=0;
baseb_output_block=128;
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// *******************************************************
//                   MAIN RECEIVE LOOP
// *******************************************************
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
local_baseb_reset_counter=0;
baseb_reset_counter=0;
init_baseband_sizes();
baseb_clear_time=0;
fftx_flag=0;
while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
      thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
      thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
 {
 if(thread_command_flag[THREAD_NARROWBAND_DSP] == 
                                           THRFLAG_KILL)goto wcw_error_exit;
 lir_sleep(10000);
 }
thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_NARROWBAND_DSP] == THRFLAG_ACTIVE)
  {
#if RUSAGE_OLD == TRUE
  if(local_workload_counter != workload_counter)
    {
    local_workload_counter=workload_counter;
    make_thread_times(THREAD_NARROWBAND_DSP);
    }
#endif
  dt1=current_time();
  if(diskwrite_flag)
    {
    if(fabs(disksync_time-dt1) > 0.2)
      {
      lir_sync();
      dt1=current_time();
      disksync_time = dt1;
      }
    }
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  idle_flag++;
  switch (idle_flag)
    {
    case 1:
    if(eme_active_flag != 0)
      {
      if(dt1 - eme_time > 3)
        {
        sc[SC_COMPUTE_EME_DATA]++;
        eme_time=dt1;
        }
      }
    break;

    case 2:
    if(fft1_handle != NULL)
      {
      memcheck(198,fft1mem,(int*)&fft1_handle);
      if(kill_all_flag) goto wcw_error_exit;
      }
    if(baseband_handle != NULL)
      {
      memcheck(298,basebmem,(int*)&baseband_handle);
      if(kill_all_flag) goto wcw_error_exit;
      }
    break;

    case 3:
    if(afc_handle != NULL)
      {
      memcheck(398,afcmem,(int*)&afc_handle);
      if(kill_all_flag) goto wcw_error_exit;
      }
    if(fft3_handle != NULL)
      {
      memcheck(498,fft3mem,(int*)&fft3_handle);
      if(kill_all_flag) goto wcw_error_exit;
      }
    if(blanker_handle != NULL)
      {
      memcheck(598,blankermem,(int*)&blanker_handle);
      if(kill_all_flag) goto wcw_error_exit;
      }
    break;

    case 4:
    if(hires_handle != NULL)
      {
      memcheck(598,hiresmem,(int*)&hires_handle);
      if(kill_all_flag) goto wcw_error_exit;
      }
    if( allow_parport != 0)
      {
// Read now and then to make apm aware Linrad is running.
// See z_APM.txt
      lir_inb(ui.parport);
      }
    break;

    default:
    idle_flag=0;
    break;
    }
  thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_SEM_WAIT;
  lir_sem_wait(SEM_FFT1);
  if(kill_all_flag) goto wcw_error_exit;
  if(thread_command_flag[THREAD_NARROWBAND_DSP]
                                             !=THRFLAG_ACTIVE)goto narend;
// Here we do the mouse actions that affect the narrowband graphs.
// Note that memory areas may become re-allocated and that no other
// threads may try to acces the narrowband arrays in the meantime.
// The screen thread is stopped by the memory allocation routines
  if(recent_time-mouse_time_narrow > MOUSE_MIN_TIME)
    {
    mouse_time_narrow=current_time();
    k=mouse_task&GRAPH_MASK;
    if( k > MAX_WIDEBAND_GRAPHS)
      {
      set_button_states();
      if( (mouse_task&GRAPH_RIGHTPRESSED) != 0)
        {
        rightpressed=BUTTON_IDLE;
        mouse_task=-1;
        goto mouse_x;
        }
      if(mouse_active_flag == 0)
        {
        switch (k)
          {
          case AFC_GRAPH:
          mouse_on_afc_graph();
          break;

          case BASEBAND_GRAPH:
          mouse_on_baseband_graph();
          break;

          case POL_GRAPH:
          mouse_on_pol_graph();
          break;

          case COH_GRAPH:
          mouse_on_coh_graph();
          break;

          case EME_GRAPH:
          mouse_on_eme_graph();
          break;

          case METER_GRAPH:
          mouse_on_meter_graph();
          break;

          default:
          mouse_on_users_graph();
          break;
          } 
        if(mouse_active_flag == 0)lirerr(8877);
        }
      if(numinput_flag==0)
        {
        current_mouse_activity();
        if(kill_all_flag) goto wcw_error_exit;
        }
      if( (numinput_flag&DATA_READY_PARM) != 0)
        {
        par_from_keyboard_routine();
        if(kill_all_flag) goto wcw_error_exit;
        par_from_keyboard_routine=NULL;
        mouse_active_flag=0;
        numinput_flag=0;
        leftpressed=BUTTON_IDLE;  
        }
      if(mouse_active_flag == 0)
        {
        mouse_task=-1;
        }
mouse_x:;        
      }
    }
  if(local_baseb_reset_counter!=baseb_reset_counter)
    {
do_baseb_reset:;    
    local_baseb_reset_counter=baseb_reset_counter;
    count_rx_underrun_flag=FALSE;
    if( new_baseb_flag == 0 && 
          thread_status_flag[THREAD_RX_OUTPUT] != THRFLAG_NOT_ACTIVE &&
            ((ui.rx_input_mode&NO_DUPLEX) == 0 || rx_audio_in == -1) )
      {
      lir_sched_yield();
      if(thread_status_flag[THREAD_RX_OUTPUT] != THRFLAG_ACTIVE &&
           thread_status_flag[THREAD_RX_OUTPUT] != THRFLAG_SEM_WAIT)
        {
        lirerr(563291);
        goto wcw_error_exit;
        } 
      pause_thread(THREAD_RX_OUTPUT);
      thread_command_flag[THREAD_RX_OUTPUT]=THRFLAG_SEMCLEAR;
      while(thread_status_flag[THREAD_RX_OUTPUT] != THRFLAG_SEMCLEAR)
        {
        lir_sleep(1000);
        }            
      lir_sleep(1000);
      thread_command_flag[THREAD_RX_OUTPUT]=THRFLAG_IDLE;
      lir_sleep(1000);
      lir_sem_post(SEM_RX_DASIG);        
      while(thread_status_flag[THREAD_RX_OUTPUT] != THRFLAG_IDLE)
        {
        lir_sleep(1000);
        }
      }  
    if(mix1_selfreq[0] >= 0)
      {
      new_baseb_flag=3;
      init_baseband_sizes();
      make_baseband_graph(TRUE);
      init_basebmem();
      clear_baseb();
      baseb_clear_time=current_time();
      local_baseb_reset_counter=baseb_reset_counter;
      thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_ACTIVE;
      goto narend_chk;
      }
    else
      {
      if( (ui.network_flag & NET_RX_INPUT) != 0)
        {
        net_send_slaves_freq();
        }
      new_baseb_flag=-1;
      }  
    }
  thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_ACTIVE;
more_fftx:;
  if(local_baseb_reset_counter!=baseb_reset_counter)goto do_baseb_reset;
  fftx_flag=0;
  if( genparm[SECOND_FFT_ENABLE] == 0 )
    {
    if(new_baseb_flag >= 0)
      {
      if(genparm[AFC_ENABLE] != 0 && 
         (rx_mode < MODE_SSB || rx_mode == MODE_AM || rx_mode == MODE_FM))
        {
        if( fft1_nb != fft1_nc) 
          {
          make_afc();
          if( ag.mode_control != 0 && afc_graph_filled != 0)sc[SC_SHOW_AFC]++;
          }
        if( fft1_nm >= afc_tpts)
          {
          if( ((fft1_nc-fft1_nx+max_fft1n)&fft1n_mask) > afct_delay_points 
               &&((timf3_px-timf3_pa+timf3_mask)&timf3_mask) >= timf3_block) 
             {
             fft1_mix1_afc();
             }
          }
        if( ((fft1_nb-fft1_nx+max_fft1n)&fft1n_mask) > 
                                              afct_delay_points )fftx_flag|=1;
        if( ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) < 
                                                     timf3_block)fftx_flag|=2;
        }
      else
        {
        if( fft1_nb != fft1_nx &&
                ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) >= timf3_block)
          {
          fft1_mix1_fixed();
          if(fft1_nb != fft1_nx)fftx_flag|=1;
          if( ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) <
                                                     timf3_block)fftx_flag|=2;
          }
        }
      }
    else
      {
      fft1_nx=fft1_nb;
      fft1_px=fft1_pb;
      fft1_nc=fft1_nb;
      goto clear_select;
      }
    }
  else
    {
    if(new_baseb_flag >= 0)
      {
      if(genparm[AFC_ENABLE] != 0 && 
         (rx_mode < MODE_SSB || rx_mode == MODE_AM) )
        {
        if(fft2_na != fft2_nc)
          {
          make_afc();
          if( ag.mode_control != 0 && afc_graph_filled != 0)sc[SC_SHOW_AFC]++;
          }
        if( ((fft2_nc-fft2_nx+max_fft2n)&fft2n_mask) > afct_delay_points &&
                 ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) >= timf3_block)                                         
          {
          fft2_mix1_afc();
          }
        if(((fft2_na-fft2_nx+max_fft2n)&fft2n_mask) > 
                                               afct_delay_points)fftx_flag|=1;
        if( ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) <
                                                     timf3_block)fftx_flag|=2;
        }
      else
        {
        if(fft2_na != fft2_nx &&
                ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) >= timf3_block)
          {        
          fft2_mix1_fixed();
          }
        if(fft2_na != fft2_nx)fftx_flag|=1;
        if( ((timf3_px-timf3_pa+timf3_mask)&timf3_mask) <
                                                     timf3_block)fftx_flag|=2;
        }
      }  
    else
      {
clear_select:;
      timf3_px=timf3_pa;
      timf3_py=timf3_pa;
      fft3_px=fft3_pa;
      baseb_wts=2;
      baseb_fx=(baseb_pa-2+baseband_size)&baseband_mask;
      baseb_py=baseb_pa;
      goto narend_chk;
      }
    }
#if BUFBARS == TRUE
  if(timinfo_flag!=0)
    {
    i=(timf3_pa+timf3_block-timf3_px+timf3_size)&timf3_mask;
    i/=timf3_indicator_block;
    if(i != rx_indicator_bufpos[4])
      {
      rx_indicator_bufpos[4]=i;
      lir_hline(indicator_first_pixel,indicator_ypix-12,
                           indicator_first_pixel+i-1,15);
      lir_hline(indicator_first_pixel+i,indicator_ypix-12,
                           indicator_first_pixel+INDICATOR_SIZE-1,1);
      }
    }
#endif  
more_fft3:;
  if( ((timf3_pa-timf3_px+timf3_size)&timf3_mask) >= 
                                             twice_rxchan*fft3_size &&
                    ((fft3_px-fft3_pa+fft3_mask)&fft3_mask) > fft3_block)
    {
    make_fft3_all();
    }
#if BUFBARS == TRUE
  if(fft3_handle != NULL && timinfo_flag!=0)
    {
    i=(fft3_pa+fft3_block-fft3_px+fft3_totsiz)&fft3_mask;
    i/=fft3_indicator_block;
    if(i != rx_indicator_bufpos[5])
      {
      rx_indicator_bufpos[5]=i;
      lir_hline(indicator_first_pixel,indicator_ypix-15,
                           indicator_first_pixel+i-1,15);
      lir_hline(indicator_first_pixel+i,indicator_ypix-15,
                           indicator_first_pixel+INDICATOR_SIZE-1,1);
      }
    }
#endif    
more_mix2:;    
  if( ((fft3_pa-fft3_px+fft3_totsiz)&fft3_mask) >= fft3_block &&
      (((int)(baseb_fx)-baseb_pa+baseband_mask)&baseband_mask) > mix2.size+16) 
    {
    fft3_mix2();
    if(kill_all_flag) goto wcw_error_exit;
    if(new_baseb_flag ==0)
      {
      if(genparm[CW_DECODE_ENABLE] != 0)
        {
        coherent_cw_detect();
        if(kill_all_flag) goto wcw_error_exit;
        }
      if(cg.oscill_on != 0)sc[SC_CG_REDRAW]++;
      }
    }    
#if BUFBARS == TRUE
  if(baseband_handle != NULL && timinfo_flag!=0)
    {
    i=(baseb_pa+fft3_block+2-(int)(baseb_fx)+baseband_mask+1)&baseband_mask;
    i/=baseb_indicator_block;
    if(i != rx_indicator_bufpos[6])
      {
      rx_indicator_bufpos[6]=i;
      lir_hline(indicator_first_pixel,indicator_ypix-18,
                           indicator_first_pixel+i-1,15);
      lir_hline(indicator_first_pixel+i,indicator_ypix-18,
                           indicator_first_pixel+INDICATOR_SIZE-1,1);
      }
    }
#endif  
more_audio:;
  resblk=rxda.frame*da_resample_ratio;
  if(resblk <1)resblk=1;
  if( ((baseb_pa-baseb_py+baseband_size)&baseband_mask) > 
                                        baseb_output_block || new_baseb_flag > 0)
    {
    if(new_baseb_flag == 0)
      {
      if( ((daout_px-daout_pa+daout_bufmask)&daout_bufmask) > rx_daout_block)
        {
        make_audio_signal();
        if(kill_all_flag) goto wcw_error_exit;
        }
      while( ((daout_pa-daout_pb+daout_bufmask+1)&daout_bufmask) >=
                                                    rx_daout_block+resblk)
        {
        if(thread_command_flag[THREAD_NARROWBAND_DSP] != 
                                              THRFLAG_ACTIVE)goto narend_chk;
        lir_sem_post(SEM_RX_DASIG);
        daout_pb=(daout_pb+rx_daout_block)&daout_bufmask;
        }
#if BUFBARS == TRUE
      if(baseband_handle != NULL && timinfo_flag!=0)
        {
        i=(daout_pa+2-daout_px+daout_bufmask+1)&daout_bufmask;
        i/=daout_indicator_block;
        if(i != rx_indicator_bufpos[7])
          {
          rx_indicator_bufpos[7]=i;
          lir_hline(indicator_first_pixel,indicator_ypix-21,
                            indicator_first_pixel+i-1,15);
          lir_hline(indicator_first_pixel+i,indicator_ypix-21,
                            indicator_first_pixel+INDICATOR_SIZE-1,1);
          }
        }    
#endif    
      goto narend_chk; 
      }
    t1=1.5+daout_pa/resblk;
    i=baseb_fx+t1;
    baseb_wts=((baseb_pa-i+baseband_size)&baseband_mask);
    lir_sched_yield();
    make_timing_info();
    if(kill_all_flag) goto wcw_error_exit;
    t1=total_wttim-da_wait_time-da_wttim;
    if(!audio_dump_flag && new_baseb_flag == 3)
      {
      if(t1 > 0.01 )
        {
        new_baseb_flag=2;
        }
      goto narend_chk;
      }
    if(baseb_clear_time+2+2*da_wait_time < current_time())lirerr(1058);
// new_baseb_flag == 1 or 2
// We want data in buffers corresponding to da_wait_time.
// We arrive here when there is more than that, move pointers
// to discard data corresponding to total_wttim-da_wait_time
// If there is too much data in memory, move the baseb pointer
// to discard more data.
    if(new_baseb_flag == 2)
      {
      if(t1*genparm[DA_OUTPUT_SPEED] > 4)
        {
        i=t1*baseband_sampling_speed;
        if(i > baseb_wts-1)
          {
          baseb_fx+=baseb_wts-1;
          baseb_wts=1;
          goto narend_chk;
          }
        baseb_fx+=i-1;
        if(baseb_fx>baseband_size)baseb_fx-=baseband_size;
        baseb_wts-=i-1;
        }
      new_baseb_flag=1;
      goto narend_chk;
      }
    if( (ui.rx_input_mode&NO_DUPLEX) == 0 || rx_audio_in == -1)
      {
      resume_thread(THREAD_RX_OUTPUT);
      lir_sched_yield();
      while(thread_status_flag[THREAD_RX_OUTPUT]!=THRFLAG_SEM_WAIT)
        {
        if(thread_command_flag[THREAD_NARROWBAND_DSP] != 
                                             THRFLAG_ACTIVE)goto narend_chk;
        lir_sleep(3000);
        if(kill_all_flag) goto wcw_error_exit;
        }
      }  
    new_baseb_flag=0;
    count_rx_underrun_flag=TRUE;
    t1=0.3/rx_output_blockrate;
    if(t1<0.002)
      {
      t1=0.3/rx_output_blockrate;
      if(t1>0.002)t1=0.002;
      }
    baseb_output_block=t1*baseband_sampling_speed*baseb_channels;
    if(baseb_output_block < 2)baseb_output_block=2;
    sc[SC_BG_WATERF_REDRAW]+=3;
    lir_sched_yield();
    if( (ui.network_flag & NET_RXIN_FFT1) == 0)
      {
      i=timf1p_pb;
      thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_INPUT_WAIT;
      while(i == timf1p_pb)
        {
        if(thread_command_flag[THREAD_NARROWBAND_DSP] != THRFLAG_ACTIVE)
          {
          thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_ACTIVE;
          goto narend_chk;
          }
        lir_sleep(1000);
        if(kill_all_flag) goto wcw_error_exit;
        }
      thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_ACTIVE;
      }
    make_timing_info();
    if(kill_all_flag) goto wcw_error_exit;
    t1=total_wttim-da_wait_time-da_wttim;
    if(cg.meter_graph_on != 0)sc[SC_MG_REDRAW]++;
    workload_reset_flag++;
    if( (ui.network_flag & NET_RX_INPUT) != 0)
      {
      net_send_slaves_freq();
      }
    if(t1*genparm[DA_OUTPUT_SPEED] > 4)
      {
      i=t1*baseband_sampling_speed;
      if(i > baseb_wts)i=baseb_wts;
      baseb_fx+=i-1;
      if(baseb_fx>baseband_size)baseb_fx-=baseband_size;
      baseb_wts-=i-1;
      }
    if(kill_all_flag)goto wcw_error_exit;  
    goto more_fftx;
    }
narend_chk:;
  if(kill_all_flag)goto wcw_error_exit;  
  if(new_baseb_flag == 0)
    {
    if( ((baseb_pa-baseb_py+baseband_size)&baseband_mask) > 
                                                      baseb_output_block &&
        ((daout_px-daout_pa+daout_bufmask)&daout_bufmask) > rx_daout_block)
      {
      lir_sched_yield();
      goto more_audio;
      }
    }
  if( ((fft3_pa-fft3_px+fft3_totsiz)&fft3_mask) >= fft3_block &&
      (((int)(baseb_fx)-baseb_pa+baseband_mask)&baseband_mask) > mix2.size+16) 
    {
    lir_sched_yield();
    goto more_mix2;
    }
  if( ((timf3_pa-timf3_px+timf3_size)&timf3_mask) >= 
                                             twice_rxchan*fft3_size &&
                    ((fft3_px-fft3_pa+fft3_mask)&fft3_mask) > fft3_block)
    {    
    lir_sched_yield();
    goto more_fft3;
    }
  if(fftx_flag != 0) 
    {
    if( (fftx_flag&2) == 0)
      {
      lir_sched_yield();
      }
    else
      {
      lir_sleep(5000);
      }  
    if(!kill_all_flag && thread_command_flag[THREAD_NARROWBAND_DSP] == 
                                                THRFLAG_ACTIVE)goto more_fftx;
    }
narend:;
  }    
wcw_error_exit:;
thread_status_flag[THREAD_NARROWBAND_DSP]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_NARROWBAND_DSP] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

