
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
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
#include "rusage.h"
#include "options.h"

extern char netsend_rx_multi_group[];

void set_raw_userfreq(void)
{
net_rxdata_16.userx_freq=-1;
net_rxdata_16.userx_no=-1;
net_rxdata_18.userx_freq=-1;
net_rxdata_18.userx_no=-1;
net_rxdata_24.userx_freq=-1;
net_rxdata_24.userx_no=-1;
}

void finish_rx_read(short int *buf)
{
char *charbuf;
int comp_flag;
int read_time;
int i, j, k, m, ix, nn, mm;
float *za, *zb;
double dt1;
short int *ya, *yb;
// Here we post to the screen routine every 0.1 second.      
screen_loop_counter--;
if(screen_loop_counter == 0)
  {
  screen_loop_counter=screen_loop_counter_max;
  lir_sem_post(SEM_SCREEN);
  }
// ************************************************************* 
// Temporary code.
// Having a pointer transferred through short int *buf is confusing
// it might be easier to follow the code by explicitly using
// timf1 and associated pointers.
// In case net writes will be transferred to a separate thread
// it would be necessary.
// Check our pointer
i=(int)buf;
j=(int)timf1_char;
if(j+timf1p_pnb != i)lirerr(8888999);
timf1p_pna=timf1p_pnb+rxad.block_bytes;
if( (timf1p_pna&timf1_bytemask) != timf1p_pa)lirerr(777999);
// *********************************
if(hware_flag != 0)
  {
  dt1=current_time();
  if(dt1 - hware_time > 0.005)
    {
    control_hware();
    hware_time=dt1;
    }
  }
hware_hand_key();
if( (ui.network_flag&(NET_RX_OUTPUT)) != 0)
  {
  if(diskread_flag < 2)
    {
    read_time=ms_since_midnight(TRUE);
    accumulated_netwait_time*=0.75;
    }
  else
    {
    netstart_time=current_time();
    dt1=diskread_time+diskread_block_counter*rxad.block_frames/ui.rx_ad_speed;
    read_time=dt1/(24*3600);
    dt1-=24*3600*read_time;
    read_time=1000*dt1;
    read_time%=24*3600000;
    accumulated_netwait_time=0;
    }
  }
else
  {
  read_time=0;
  }    
if( (ui.network_flag & NET_RX_INPUT) != 0)
  { 
  if(abs(latest_listsend_time-read_time) > 1500)
    {
    latest_listsend_time=read_time;
    net_send_slaves_freq();
    }
  }  
comp_flag=FALSE;
if(diskwrite_flag == 1)
  {
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    i=fwrite(buf,1,rxad.block_bytes,save_wr_file);
    if(i != (int)rxad.block_bytes)disksave_start_stop();
    }
  else
    {  
    comp_flag=TRUE;
    compress_rawdat();
    i=fwrite(rawsave_tmp,1,save_rw_bytes,save_wr_file);
    if(i != (int)save_rw_bytes)disksave_start_stop();
    }
  }
set_raw_userfreq();
charbuf=(void*)buf;
// öööööööööööööö
//    ******************* SEND RAW16 *****************************
if( (ui.network_flag & NET_RXOUT_RAW16) != 0)
  {
  net_rxdata_16.time=read_time;
  net_rxdata_16.passband_center=fg.passband_center;
  net_rxdata_16.passband_direction=fg.passband_direction;
  j=0;
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    while(j < (int)rxad.block_bytes)
      {
      net_rxdata_16.buf[netsend_ptr_16  ]=charbuf[j  ];
      net_rxdata_16.buf[netsend_ptr_16+1]=charbuf[j+1];
      netsend_ptr_16+=2;
      j+=2;
      if(netsend_ptr_16 >= NET_MULTICAST_PAYLOAD)
        {
        netraw16_blknum++;
        net_rxdata_16.block_no=netraw16_blknum;
        net_rxdata_16.ptr=next_blkptr_16;
        lir_send_raw16();
        next_blkptr_16=j;
        if(next_blkptr_16 >= rxad.block_bytes)next_blkptr_16=0;
        netsend_ptr_16=0;
        }
      }  
    }
  else
    {
    while(j < (int)rxad.block_bytes)
      {
      net_rxdata_16.buf[netsend_ptr_16  ]=charbuf[j+2];
      net_rxdata_16.buf[netsend_ptr_16+1]=charbuf[j+3];
      netsend_ptr_16+=2;
      j+=4;
      if(netsend_ptr_16 >= NET_MULTICAST_PAYLOAD)
        {
        netraw16_blknum++;
        net_rxdata_16.block_no=netraw16_blknum;
        net_rxdata_16.ptr=next_blkptr_16;
        lir_send_raw16();
        next_blkptr_16=j/2;
        if(next_blkptr_16 >= rxad.block_bytes/2)next_blkptr_16=0;
        netsend_ptr_16=0;
        }
      }  
    }  
  }
//    ******************* SEND RAW18 *****************************
if( (ui.network_flag & NET_RXOUT_RAW18) != 0)
  {
  if(comp_flag == FALSE)
    {
    compress_rawdat();
    }
  net_rxdata_18.time=read_time;
  net_rxdata_18.passband_center=fg.passband_center;
  net_rxdata_18.passband_direction=fg.passband_direction;
  j=0;
  while(j < (int)save_rw_bytes)
    {
    net_rxdata_18.buf[netsend_ptr_18]=rawsave_tmp[j];
    netsend_ptr_18++;
    j++;
    if(netsend_ptr_18 >= NET_MULTICAST_PAYLOAD)
      {
      netraw18_blknum++;
      net_rxdata_18.block_no=netraw18_blknum;
      net_rxdata_18.ptr=next_blkptr_18;
      lir_send_raw18();
      next_blkptr_18=j;
      if( next_blkptr_18 >= save_rw_bytes)next_blkptr_18=0;
      netsend_ptr_18=0;
      }
    }
  }
//    ******************* SEND RAW24 *****************************
if( (ui.network_flag & NET_RXOUT_RAW24) != 0)
  {
  net_rxdata_24.time=read_time;
  net_rxdata_24.passband_center=fg.passband_center;
  net_rxdata_24.passband_direction=fg.passband_direction;
  j=0;
  while(j < (int)rxad.block_bytes)
    {
    net_rxdata_24.buf[netsend_ptr_24  ]=charbuf[j+1];
    net_rxdata_24.buf[netsend_ptr_24+1]=charbuf[j+2];
    net_rxdata_24.buf[netsend_ptr_24+2]=charbuf[j+3];
    netsend_ptr_24+=3;
    j+=4;
    if(netsend_ptr_24 >= NET_MULTICAST_PAYLOAD)
      {
      netraw24_blknum++;
      net_rxdata_24.block_no=netraw24_blknum;      
      net_rxdata_24.ptr=next_blkptr_24;
      lir_send_raw24();
      next_blkptr_24=j;
      if( next_blkptr_24 >= rxad.block_bytes)next_blkptr_24=0;
      netsend_ptr_24=0;
      }
    }
  }
if( (ui.network_flag&NET_RXOUT_FFT1) != 0)
  {
// It is important for slow computers that may be connected via the
// network that we distribute packets evenly in time.  
// The fft1 transform may be very large and it could arrive at a rate 
// of 25 Hz or even lower.
// The A/D interrupt rate is high when we send fft1 transforms.
//  (see buf.c) 
// It may be different if input is from the network or from the
// hard disk.
  mm=rxad.block_bytes/(1-fft1_interleave_ratio);
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)mm*=2;

  k=(fft1net_pa-fft1net_px+fft1net_size)&fft1net_mask;
  if( k > mm+fft1_blockbytes)
    {
    mm+=rxad.block_bytes/2;
    }
  if( k < 1.5*mm)
    {
    mm-=rxad.block_bytes/2;
    }
  mm&=-4;
  if( k > mm)
    {
    net_rxdata_fft1.time=read_time;
    net_rxdata_fft1.passband_center=fg.passband_center;
    net_rxdata_fft1.passband_direction=fg.passband_direction;
    j=0;
    while(j < mm)
      {
      net_rxdata_fft1.buf[netsend_ptr_fft1  ]=fft1_netsend_buffer[fft1net_px  ];
      net_rxdata_fft1.buf[netsend_ptr_fft1+1]=fft1_netsend_buffer[fft1net_px+1];
      net_rxdata_fft1.buf[netsend_ptr_fft1+2]=fft1_netsend_buffer[fft1net_px+2];
      net_rxdata_fft1.buf[netsend_ptr_fft1+3]=fft1_netsend_buffer[fft1net_px+3];
      netsend_ptr_fft1+=4;
      j+=4;
      fft1net_px=(fft1net_px+4)&fft1net_mask;
      if(netsend_ptr_fft1 >= NET_MULTICAST_PAYLOAD)
        {
        netfft1_blknum++;
        net_rxdata_fft1.block_no=netfft1_blknum;      
        net_rxdata_fft1.ptr=next_blkptr_fft1;
        lir_send_fft1();
        next_blkptr_fft1=fft1net_px&(fft1_blockbytes-1);
        netsend_ptr_fft1=0;
        }
      }
    }  
  }
if( (ui.network_flag&NET_RXOUT_BASEB) != 0)
  {
  while( ((basebnet_pa-basebnet_px+basebnet_size)&basebnet_mask) > 
                                                    basebnet_block_bytes+32)
// The baseband signal is timed to fit a loudspeaker output so
// it should already be evenly distributed in time. 
    {
    j=0;
    while(j < (int)basebnet_block_bytes)
      {
      net_rxdata_baseb.buf[netsend_ptr_baseb  ]=baseb_netsend_buffer[basebnet_px  ];
      net_rxdata_baseb.buf[netsend_ptr_baseb+1]=baseb_netsend_buffer[basebnet_px+1];
      net_rxdata_baseb.buf[netsend_ptr_baseb+2]=baseb_netsend_buffer[basebnet_px+2];
      net_rxdata_baseb.buf[netsend_ptr_baseb+3]=baseb_netsend_buffer[basebnet_px+3];
      netsend_ptr_baseb+=4;
      basebnet_px=(basebnet_px+4)&basebnet_mask;
      j+=4;
      if(netsend_ptr_baseb >= NET_MULTICAST_PAYLOAD)
        {
        netbaseb_blknum++;
        net_rxdata_baseb.block_no=netbaseb_blknum;
        net_rxdata_baseb.ptr=next_blkptr_baseb;
        lir_send_baseb();
        next_blkptr_baseb=j;
        if((int)next_blkptr_baseb >= basebnet_block_bytes)next_blkptr_baseb=0;
        netsend_ptr_baseb=0;
        }
      }  

    }
  }
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
//    ******************* SEND TIMF2 *****************************
  if( (ui.network_flag & NET_RXOUT_TIMF2) )
    {
    net_rxdata_timf2.time=read_time;
    net_rxdata_timf2.passband_center=fg.passband_center;
    net_rxdata_timf2.passband_direction=fg.passband_direction;
// Set userx_freq to sampling speed.
    net_rxdata_timf2.userx_freq=timf1_sampling_speed;
    j=0;
    mm=((timf2_pn2-timf2_pt+timf2_totbytes)&timf2_mask);
    if(mm>(timf2_mask>>1))mm=0;
    mm*=2;
    if( mm <= (int)rxad.block_bytes)
      {
      mm=0;
      }
    else
      {
      if( mm > 4*(int)rxad.block_bytes)
        {
        mm=1.25*rxad.block_bytes;
        }
      else
        {
        mm=rxad.block_bytes;
        }
      }    
    if( (ui.rx_input_mode&DWORD_INPUT) == 0)mm*=2;
    if(swfloat)
      {
// Set userx_no to the number of receiver RF channels with
// the sign negative to indicate float format.
      net_rxdata_timf2.userx_no=-ui.rx_rf_channels;
      mm*=2;
      while(j < mm)
        {
        za=(void*)&net_rxdata_timf2.buf[netsend_ptr_timf2];
        zb=&timf2_float[timf2_pt];
        for(nn=0; nn<twice_rxchan; nn++)
          {
          za[nn]=zb[nn]+zb[twice_rxchan+nn];
          }
        netsend_ptr_timf2+=twice_rxchan*sizeof(float);
        j+=twice_rxchan*sizeof(float);
        timf2_pt=(timf2_pt+2*twice_rxchan)&timf2_mask;
        if(netsend_ptr_timf2 >= NET_MULTICAST_PAYLOAD)
          {
          nettimf2_blknum++;
          net_rxdata_timf2.block_no=nettimf2_blknum;
          net_rxdata_timf2.ptr=next_blkptr_timf2;
          lir_send_timf2();
          next_blkptr_timf2=j/2;
          if(next_blkptr_timf2 >= rxad.block_bytes/2)next_blkptr_timf2=0;
          netsend_ptr_timf2=0;
          }
        }  
      }  
    else
      {
// Set userx_no to the number of receiver RF channels.
      net_rxdata_timf2.userx_no=ui.rx_rf_channels;
      while(j < mm)
        {
        ya=(void*)(&net_rxdata_timf2.buf[netsend_ptr_timf2]);
        yb=(void*)(&timf2_shi[timf2_pt]);
        for(nn=0; nn<twice_rxchan; nn++)
          {
          ya[nn]=yb[nn]+yb[twice_rxchan+nn];
          }
        netsend_ptr_timf2+=twice_rxchan*sizeof(short int);
        j+=twice_rxchan*sizeof(short int);
        timf2_pt=(timf2_pt+2*twice_rxchan)&timf2_mask;
        if(netsend_ptr_timf2 >= NET_MULTICAST_PAYLOAD)
          {
          nettimf2_blknum++;
          net_rxdata_timf2.block_no=nettimf2_blknum;
          net_rxdata_timf2.ptr=next_blkptr_timf2;
          lir_send_timf2();
          next_blkptr_timf2=j;
          if(next_blkptr_timf2 >= rxad.block_bytes)next_blkptr_timf2=0;
          netsend_ptr_timf2=0;
          }
        }  
      }
    }
//    ******************* SEND FFT2 *****************************
  if( (ui.network_flag & NET_RXOUT_FFT2) )
    {
// It is important for slow computers that may be connected via the
// network that we distribute packets evenly in time.  
// The fft2 transform may be very large and it could arrive at a rate 
// of 1 Hz or even lower.
// It may be different if input is from the network or from the
// hard disk.
    mm=1.1*rxad.block_bytes/(1-fft2_interleave_ratio);
    m=mm;
    if( (ui.rx_input_mode&DWORD_INPUT) != 0)mm/=2;
    k=(fft2_pa-fft2_pt+fft2_totbytes)&fft2_mask;
    if( k > mm+fft2_blockbytes)
      {
      mm+=m/2;
      }
    if( k < 1.5*mm)
      {
      mm-=m/2;
      }
    mm&=-4;
    if( k > mm)
      {
      net_rxdata_fft2.time=read_time;
      net_rxdata_fft2.passband_center=fg.passband_center;
      net_rxdata_fft2.passband_direction=fg.passband_direction;
      j=0;
      if(fft_cntrl[FFT2_CURMODE].mmx == 0)
        { 
        mm*=2;
        charbuf=(void*)(fft2_float);
        while(j < mm)
          {
          net_rxdata_fft2.buf[netsend_ptr_fft2  ]=charbuf[4*fft2_pt  ];
          net_rxdata_fft2.buf[netsend_ptr_fft2+1]=charbuf[4*fft2_pt+1];
          net_rxdata_fft2.buf[netsend_ptr_fft2+2]=charbuf[4*fft2_pt+2];
          net_rxdata_fft2.buf[netsend_ptr_fft2+3]=charbuf[4*fft2_pt+3];
          netsend_ptr_fft2+=4;
          j+=4;
          fft2_pt=(fft2_pt+1)&fft2_mask;
          if(netsend_ptr_fft2 >= NET_MULTICAST_PAYLOAD)
            {
            netfft2_blknum++;
            if( (netfft2_blknum&1) == 0)
              {
// If netfft2_blknum is even, send these things:              
// Set userx_no to the number of receiver RF channels with
// the sign negative to indicate float format.
              net_rxdata_fft2.userx_no=-ui.rx_rf_channels;
// Set userx_freq to sampling speed.
              net_rxdata_fft2.userx_freq=timf1_sampling_speed;
              }
            else
              {
// If fft2_blknum is odd, send these things:              
              net_rxdata_fft2.userx_no=fft2_n;
              net_rxdata_fft2.userx_freq=genparm[SECOND_FFT_SINPOW];
              }
            net_rxdata_fft2.block_no=netfft2_blknum;      
            net_rxdata_fft2.ptr=next_blkptr_fft2;
            lir_send_fft2();
            next_blkptr_fft2=fft2_pt&(fft2_blockbytes-1);
            netsend_ptr_fft2=0;
            }
          }
        }
      else
        {
        charbuf=(void*)(fft2_short_int);
        while(j < mm)
          {
          net_rxdata_fft2.buf[netsend_ptr_fft2  ]=charbuf[2*fft2_pt  ];
          net_rxdata_fft2.buf[netsend_ptr_fft2+1]=charbuf[2*fft2_pt+1];
          netsend_ptr_fft2+=2;
          j+=2;
          fft2_pt=(fft2_pt+1)&fft2_mask;
          if(netsend_ptr_fft2 >= NET_MULTICAST_PAYLOAD)
            {
            netfft2_blknum++;
            if( (netfft2_blknum&1) == 0)
              {
// If fft2_blknum is even, send these things:              
// Set userx_no to the number of receiver RF channels.
              net_rxdata_fft2.userx_no=ui.rx_rf_channels;
// Set userx_freq to sampling speed.
              net_rxdata_fft2.userx_freq=timf1_sampling_speed;
              }
            else
              {
// If fft2_blknum is odd, send these things:              
              net_rxdata_fft2.userx_no=fft2_n;
              net_rxdata_fft2.userx_freq=genparm[SECOND_FFT_SINPOW];
              }
            net_rxdata_fft2.block_no=netfft2_blknum;      
            net_rxdata_fft2.ptr=next_blkptr_fft2;
            lir_send_fft2();
            next_blkptr_fft2=fft2_pt&(fft2_blockbytes-1);
            netsend_ptr_fft2=0;
            }
          }
        }
      }
    }
  }
if(truncate_flag != 0) 
  {
  int *iib;
  int nbits, mask2, shft;
  char s[80];
  k=ui.rx_ad_channels;
  m=rxad.block_frames*k;
  iib=(int*)buf;
  mask2=truncate_flag^0xffffffff;
  shft=(truncate_flag+1)/2;
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    nbits=16;
    for(i=0; i<m; i+=k)
      {
      for(j=0; j<k; j++)
        {
        ix=buf[i+j];
        ix &= mask2;
        ix+=shft;
        buf[i+j]=ix;
        }
      }  
    }
  else
    {
    nbits=32;
    for(i=0; i<m; i+=k)
      {
      for(j=0; j<k; j++)
        {
        ix=iib[i+j];
        ix &= mask2;
        ix+=shft;
        iib[i+j]=ix;
        }
      }
    }
  mask2=truncate_flag;
  while(mask2 != 0)
    {
    mask2>>=1;
    nbits--;
    }
  sprintf(s,"INPUT TRUNCATED TO %d bits",nbits);
  lir_text(0,0,s); 
  }
if(ampinfo_flag != 0)
  {
  if(rxin_local_workload_reset != workload_reset_flag)
    {
    rxin_local_workload_reset = workload_reset_flag;
    for(i=0; i<ui.rx_ad_channels; i++)
      {
      ad_maxamp[i]=1;
      }
    }
  k=ui.rx_ad_channels;
  m=rxad.block_frames*k;
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    for(i=0; i<m; i+=k)
      {
      for(j=0; j<k; j++)
        {
        ix=buf[i+j];
        ix=abs(ix);        
        if(ix > ad_maxamp[j])ad_maxamp[j]=ix;
        }
      }  
    }
  else
    {
    for(i=0; i<m; i+=k)
      {
      for(j=0; j<k; j++)
        {
        ix=buf[1+((k+j)<<1)];
        ix=abs(ix);        
        if(ix > ad_maxamp[j])ad_maxamp[j]=ix;
        }
      }
    }
  }
// Increment the SEM_TIMF1 semaphore in case there is enough
// data for fft1 to make at least one transform.
if( ((timf1p_pa-timf1p_pb+timf1_bytes)&timf1_bytemask) >= timf1_usebytes)
  {  
  timf1p_pb=timf1p_pa;
  lir_sem_post(SEM_TIMF1);
  }
rxin_block_counter++;
if(yieldflag_wdsp_fft1)lir_sched_yield();
}

void rx_file_input(void)
{
#if RUSAGE_OLD == TRUE
int local_workload_counter;
#endif
char s[40];
fpos_t pos;
int i, j, k;
double speedcalc_counter;
char *ch;
double dt1, dt2, read_start_time, ideal_block_count;
double total_time1, total_time2;
float t2;
#if OSNUM == OS_FLAG_LINUX
clear_thread_times(THREAD_RX_FILE_INPUT);
#endif
#if RUSAGE_OLD == TRUE
local_workload_counter=workload_counter;
#endif
screen_loop_counter_max=0.1*interrupt_rate;
if(screen_loop_counter_max==0)screen_loop_counter_max=1;
screen_loop_counter=screen_loop_counter_max;
i=0;
if(savefile_repeat_flag == 1)fgetpos(save_rd_file,&pos);
total_time1=current_time();
read_start_time=total_time1;
total_time2=current_time();
diskread_block_counter=0;
speedcalc_counter=0;
thread_status_flag[THREAD_RX_FILE_INPUT]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_RX_FILE_INPUT] == THRFLAG_ACTIVE)
  {
#if RUSAGE_OLD == TRUE
  if(local_workload_counter != workload_counter)
    {
    local_workload_counter=workload_counter;
    make_thread_times(THREAD_RX_FILE_INPUT);
    }
#endif
  while(audio_dump_flag != 0 && !kill_all_flag &&
             ((timf1p_px-(int)rxad.block_bytes-timf1p_pb+timf1_bytes)&
                      timf1_bytemask) <2*(int) rxad.block_bytes )
    {
    lir_sem_post(SEM_TIMF1);
    lir_sleep(5000);
    }
  if(audio_dump_flag != 0)
    {
    lir_sched_yield();
    dt2=current_time();
    i=0;
    if(dt2 > total_time2+0.25)
      {
// Make sure we do not use 100% of the available CPU time.
// Leave the CPU idle (from Linrad tasks) four times per second.
// First make sure that all important threads have completed,
// Then wait 12.5 milliseconds extra to give at least 5% of the
// total CPU power to other tasks. 
      while(!kill_all_flag &&
             (thread_status_flag[THREAD_WIDEBAND_DSP] != THRFLAG_SEM_WAIT ||
              thread_status_flag[THREAD_SCREEN] != THRFLAG_SEM_WAIT) )
        {
        i++; 
        lir_sleep(5000);
        if(i>400)lirerr(88777);
        }
      if(genparm[SECOND_FFT_ENABLE] != 0)
        {
        while(!kill_all_flag && 
                    thread_status_flag[THREAD_SECOND_FFT] != THRFLAG_SEM_WAIT)
          { 
          lir_sleep(5000);
          i++;
          if(i>400)lirerr(88778);
          }
        }
      while(!kill_all_flag && new_baseb_flag <= 0 &&
             (thread_status_flag[THREAD_NARROWBAND_DSP] != THRFLAG_SEM_WAIT &&
              thread_status_flag[THREAD_NARROWBAND_DSP] != THRFLAG_INPUT_WAIT))
        { 
        lir_sleep(5000);
        i++;
        if(i>400)lirerr(88779);
        }
      lir_sleep(12500);  
      dt2=current_time();
      total_time2=dt2;    
      }
    ideal_block_count=speedcalc_counter-0.4;
    dt1=ideal_block_count/interrupt_rate;
    read_start_time=dt2-dt1;
    }
  else
    {  
    if(speedcalc_counter > 5)
      {
      total_time2=current_time();
      dt1=total_time2-read_start_time;
      ideal_block_count=dt1*interrupt_rate+1;
      t2=speedcalc_counter-ideal_block_count-.5;
      if(t2 >0)
        {
        t2/=interrupt_rate;
        lir_sleep(1000000*t2);
        }
      total_time2=current_time();
      dt1=total_time2-read_start_time;
      measured_ad_speed=(speedcalc_counter-0.75)*(rxad.block_frames/dt1);
      }
    }
  diskread_block_counter++;
  speedcalc_counter++;
  if(diskread_pause_flag !=0 )
    {
    lir_sleep(100000);
    goto skip_read;
    }
  if(internal_generator_flag == 0)
    {
    if( diskread_flag == 4)goto end_savfile;
    if( (ui.rx_input_mode&DWORD_INPUT) == 0)
      {
      rxin_isho=(void*)(&timf1_char[timf1p_pa]);
      timf1p_pnb=timf1p_pa;
      timf1p_pa=(timf1p_pa+rxad.block_bytes)&timf1_bytemask;
      if( (ui.rx_input_mode&BYTE_INPUT) != 0)
        {
        ch=(void*)rxin_isho;
        j=rxad.block_bytes>>1;
        k=j;
        i=fread(rxin_isho,1,j,save_rd_file);
        while(j > 0)
          {
          j--;
          rxin_isho[j]=(ch[j]+128)<<8;
          } 
        if(i != k)
          {
          i*=2;
          goto end_savfile;
          }
        }
      else
        { 
        i=fread(rxin_isho,1,rxad.block_bytes,save_rd_file);
        if(i != (int)rxad.block_bytes)goto end_savfile;
        }
      }
    else
      {  
      if( (ui.rx_input_mode&BYTE_INPUT) != 0)
        {
// Read 24 bit wav files here.      
        rxin_isho=(void*)(&timf1_char[timf1p_pa]);
        timf1p_pnb=timf1p_pa;
        timf1p_pa=(timf1p_pa+rxad.block_bytes)&timf1_bytemask;        
        ch=(void*)rxin_isho;
        j=3*(rxad.block_bytes>>2);
        k=j;
        i=fread(rxin_isho,1,j,save_rd_file);
        j/=3;
        while(j > 0)
          {
          j--;
          ch[4*j+3]=ch[3*j+2];
          ch[4*j+2]=ch[3*j+1];
          ch[4*j+1]=ch[3*j  ];
          ch[4*j  ]=0;
          } 
        if(i != k)
          {
          i*=4;
          i/=3;
          goto end_savfile;
          }
        }
      else
        {
        i=fread(rawsave_tmp,1,save_rw_bytes,save_rd_file);
        expand_rawdat();
        rxin_isho=(void*)(&timf1_char[timf1p_pa]);
        timf1p_pnb=timf1p_pa;
        timf1p_pa=(timf1p_pa+rxad.block_bytes)&timf1_bytemask;
        if(i != (int)save_rw_bytes)
          {
end_savfile:;        
          if(savefile_repeat_flag == 1)
            {
            fsetpos(save_rd_file,&pos);
            diskread_block_counter=0;
            }
          else
            {
            if(diskread_flag == 2)
              {
              diskread_block_counter=2;
              diskread_flag=4;
              }
            if( (diskread_flag & 4) != 0)
              {
              if(diskread_block_counter/interrupt_rate >= total_wttim)
                {
                diskread_flag=8;
                goto end_file_rxin;
                }
              }                                                                
            }
// Clear the 200 last bytes. WAV files contain big numbers
// appended to the signal data in the file that create huge pulses           
          k=(timf1p_pa-rxad.block_bytes-200+i+timf1_bytes)&timf1_bytemask;
          while(k != timf1p_pa)
            {
            timf1_char[k]=0;
            k=(k+1)&timf1_bytemask;
            }
          }
        }
      }
    }
  else
    {
    int *iib;
    double dt3;
    rxin_isho=(void*)(&timf1_char[timf1p_pa]);
    timf1p_pnb=timf1p_pa;
    timf1p_pa=(timf1p_pa+rxad.block_bytes)&timf1_bytemask;
#define IG_WIDTH .2
#define IG_CF 1.2
#define KEY_COUNT 512
/*
#define IG_WIDTH .02
#define IG_CF 1.2
#define KEY_COUNT 512
*/

    if(internal_generator_key > KEY_COUNT)
      {
      internal_generator_key=0;
      }
    internal_generator_key++;
    if( (ui.rx_input_mode&DWORD_INPUT) == 0)
      {
      for(i=0; i<(int)rxad.block_bytes/2; i+=2*ui.rx_rf_channels)
        {
        dt3=0x7e00*sin(internal_generator_phase1);
        dt2=0x7e00*cos(internal_generator_phase1);
        internal_generator_phase1+=fft1_direction*(IG_CF-IG_WIDTH+
                                                     internal_generator_shift);
        internal_generator_shift+=0.2/ui.rx_ad_speed;
        if(internal_generator_shift > 2*IG_WIDTH)internal_generator_shift=0;
        if(internal_generator_phase1 > 2*PI_L)internal_generator_phase1-=2*PI_L;
        if(internal_generator_phase1 < -2*PI_L)internal_generator_phase1+=2*PI_L;
        if(internal_generator_key < KEY_COUNT/4)
          {
          dt3+=0x7e00*0.001*sin(internal_generator_phase2);
          dt2+=0x7e00*0.001*cos(internal_generator_phase2);
          internal_generator_phase2+=fft1_direction*IG_CF;
          if(internal_generator_phase2 > 2*PI_L)internal_generator_phase2-=2*PI_L;
          if(internal_generator_phase2 < -2*PI_L)internal_generator_phase2+=2*PI_L;
          }
        if(internal_generator_noise != 0)
          {
          dt3+=lir_noisegen(internal_generator_noise-1);
          dt2+=lir_noisegen(internal_generator_noise-1);
          }
        if(truncate_flag != 0) 
          {
          rxin_isho[i  ]=floor(dt3);
          rxin_isho[i+1]=floor(dt2);
          }
        else
          {
          rxin_isho[i  ]=rint(dt3);
          rxin_isho[i+1]=rint(dt2);
          }  
        if(ui.rx_rf_channels == 2)
          {
          rxin_isho[i+2]=rxin_isho[i  ];
          rxin_isho[i+3]=rxin_isho[i+1];
          }
        }
      }
    else
      {
      iib=(void*)(&timf1_char[timf1p_pa]);
      for(i=0; i<(int)rxad.block_bytes/4; i+=2*ui.rx_rf_channels)
        {
        dt3=0x7e000000*sin(internal_generator_phase1);
        dt2=0x7e000000*cos(internal_generator_phase1);
        internal_generator_phase1+=fft1_direction*(IG_CF-IG_WIDTH+
                                                     internal_generator_shift);
        internal_generator_shift+=0.00000002;
        if(internal_generator_shift > 2*IG_WIDTH)internal_generator_shift=0;
        if(internal_generator_phase1 > 2*PI_L)internal_generator_phase1-=2*PI_L;
        if(internal_generator_phase1 < -2*PI_L)internal_generator_phase1+=2*PI_L;
        if(internal_generator_key < KEY_COUNT/16)
          {
          dt3+=0x7e000000*0.00001*sin(internal_generator_phase2);
          dt2+=0x7e000000*0.00001*cos(internal_generator_phase2);
          internal_generator_phase2+=fft1_direction*IG_CF;
          if(internal_generator_phase2 > 2*PI_L)internal_generator_phase2-=2*PI_L;
          if(internal_generator_phase2 < -2*PI_L)internal_generator_phase2+=2*PI_L;
          }
        if(internal_generator_noise != 0)
          {
          dt3+=lir_noisegen(internal_generator_noise-1);
          dt2+=lir_noisegen(internal_generator_noise-1);
          }
        if(truncate_flag != 0) 
          {
          iib[i  ]=floor(dt3);
          iib[i+1]=floor(dt2);
          }
        else
          {
          iib[i  ]=rint(dt3);
          iib[i+1]=rint(dt2);
          }  
        if(ui.rx_rf_channels == 2)
          {
          iib[i+2]=iib[i  ];
          iib[i+3]=iib[i+1];
          }
        }
      }
    if(internal_generator_noise != 0)
      {
      sprintf(s,"NOISE LEVEL %d bits",internal_generator_noise);
      lir_text(30,0,s); 
      }
    }  
  finish_rx_read(rxin_isho);
  if(kill_all_flag) goto file_rxin_error_exit;
skip_read:;
  }
end_file_rxin:;
file_rxin_error_exit:;
thread_status_flag[THREAD_RX_FILE_INPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_RX_FILE_INPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(5000);
  lir_sem_post(SEM_SCREEN);
  }
}
