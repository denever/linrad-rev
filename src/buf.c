#include <unistd.h>
#include <string.h>
#include <globdef.h>
#include <uidef.h>
#include <fft1def.h>
#include <fft2def.h>
#include <fft3def.h>
#include <screendef.h>
#include <seldef.h>
#include <blnkdef.h>
#include <sigdef.h>
#include <hwaredef.h>

#define TIMF3_OSCILLOSCOPE_RATE 2

void fft1_block_timing(void)
{
fft1_interleave_ratio=(float)(fft1_interleave_points)/fft1_size;
fft1_new_points=fft1_size-fft1_interleave_points;
timf1_sampling_speed=ui.rx_ad_speed;
if( (ui.rx_input_mode&IQ_DATA) == 0)
  {
  timf1_sampling_speed*=0.5;
  }
fft1_blocktime=(float)(fft1_new_points)/timf1_sampling_speed;
fft1_rate=1/fft1_blocktime;
if(fft1_rate==0)fft1_rate=1;
}

void prepare_mixer(MIXER_VARIABLES *m, int nn)
{
int i, j, k;
float t1;
if( genparm[nn]!= 0 && (genparm[nn]!= 2 || rx_mode == MODE_TXTEST))
  {
  make_window(3,m[0].size, genparm[nn], m[0].window);
  }
init_fft(0,m[0].n, m[0].size, m[0].table, m[0].permute);
// If we use a window for mix1 we need crossover functions, sin and cos
// squared. Will not be used in case the window itself is sin squared.
// Find out how long to make the crossover region.
// Stop it when the window has fallen by 20dB compared to the
// start of the crossover region.
m[0].crossover_points=0;
if( genparm[nn] != 0 && genparm[nn] != 2 )
  {
  if(genparm[nn] == 9)
    {
    m[0].crossover_points=m[0].size/8;
    }
  else
    {
    if(genparm[nn] == 8)
      {
      m[0].crossover_points=m[0].size/16;
      }
    else
      {  
      i=m[0].interleave_points/2;
      t1=m[0].window[i];
      while(m[0].window[i]>0.1*t1)
        {
        i--;
        m[0].crossover_points++;
        }
      if(m[0].crossover_points > m[0].new_points/2)
                                 m[0].crossover_points=m[0].new_points/2;
      if(m[0].crossover_points > m[0].interleave_points/2)
                                 m[0].crossover_points=m[0].interleave_points/2;
      }
    }  
  t1=0.25*PI_L/m[0].crossover_points;
  j=(m[0].size-m[0].new_points)/2;
  k=j;
  k+=m[0].crossover_points/2;
  j-=m[0].crossover_points/2;
  for(i=0; i<m[0].crossover_points; i++)
    {
    m[0].cos2win[i]=m[0].window[k]*pow(cos(t1),2.0);
    m[0].sin2win[i]=m[0].window[j]*pow(sin(t1),2.0);
    k--;
    j++;
    t1+=0.5*PI_L/m[0].crossover_points;
    }
  }
}

float make_interleave_ratio(int nn)
{ 
// Make interleave_ratio the distance between the points where the
// window function is 0.5.
if(genparm[nn] != 0)
  {
  if(genparm[nn] == 9)
    {
    return 0.625;
    }
  else
    {
    if(genparm[nn] == 8)
      {
      return 0.8;
      }
    else
      {  
      return 2*asin(pow(0.5,1.0/genparm[nn]))/PI_L;
      }
    }
  }  
return 0;
}


void get_wideband_sizes(void)
{
float t1;
int i, j, m, k, bwfac;
if(fft1_handle != NULL)
  {
  lirerr(1002);
  return;
  }
fft1mode=(ui.rx_input_mode&(TWO_CHANNELS+IQ_DATA))/2;
// Before doing anything else, set output format information
rx_daout_bytes=1+(genparm[OUTPUT_MODE]&1);               //bit 0
rx_daout_channels=1+((genparm[OUTPUT_MODE]>>1)&1);       //bit 1
if(rx_daout_channels < ui.rx_min_da_channels)
  {
  rx_daout_channels = ui.rx_min_da_channels;
  }
// Tell assembly routines how many channels we have (so we make no
// mistake when changing struct ui.
rx_channels=ui.rx_rf_channels;
twice_rxchan=2*ui.rx_rf_channels;
// *********************************************************************
// First find size of first fft.
// In case a window was selected we need more points for the 
// same bandwidth. Get the interleave ratio.
fft1_interleave_ratio=make_interleave_ratio(FIRST_FFT_SINPOW); 
if( (ui.network_flag & NET_RXIN_FFT1) == 0)
  {
  if(genparm[FIRST_FFT_BANDWIDTH]==0)
    {
    bwfac=65536;
    }
  else
    {  
    bwfac=(0.3536*ui.rx_ad_speed)/
                  ((1-fft1_interleave_ratio)*genparm[FIRST_FFT_BANDWIDTH]);
    }
  j=bwfac;  
  if( (ui.rx_input_mode&IQ_DATA) != 0)j*=2;
// j is the number of points we need for the whole transform to get the
// desired bandwidth multiplied by 1.414.
// Make j a power of two so the bandwidth will be in the range
// 0.707*desired < bw < 1.414*desired. 
  fft1_n=1;
  i=j;
  while(j != 0)
   {
   j/=2;
   fft1_n++;
   }
// Never make the size below n=6 (=64)
// Note that i becomes small so we arrive at fft1_n=6
  if(fft1_n < 7)fft1_n=7; 
  fft1_size=1<<fft1_n;
  if( (float)(fft1_size)/i > 1.5)
    {
reduce_fft1_size:;
    fft1_size/=2;
    fft1_n--;
    }
  }
if(fft1_n > 12)
  {
  yieldflag_wdsp_fft1=TRUE;
  }
else
  {
  yieldflag_wdsp_fft1=FALSE;
  }  
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
  if(ui.max_blocked_cpus > 0)yieldflag_wdsp_fft1=FALSE;
  }
else  
  {
  if(ui.max_blocked_cpus > 2)yieldflag_wdsp_fft1=FALSE;
  }
    
// Each channel output is 2*float (re, im)
fft1_block=twice_rxchan*fft1_size;
if(rx_mode == MODE_TUNE)
  {  
  fft1_costab_size=fft1_size;
  fft1_permute_size=fft1_size;
  fft1_window_size=fft1_size;
  }
else  
  {  
  fft1_costab_size=fft1_size/2;
  fft1_permute_size=fft1_size;
  fft1_window_size=fft1_size;
  if(fft_cntrl[FFT1_CURMODE].permute == 2)
    {
    fft1_costab_size*=2;
    fft1_permute_size*=2;
    fft1_window_size+=16;
    }
// To save cash we use short int for the unscramble table.
// Make sure the unscramble array is not too big.  
  if(fft1_permute_size > 0x10000)goto reduce_fft1_size;  
  if(genparm[FIRST_FFT_SINPOW] == 0)fft1_window_size=0;
  }
fft1_blockbytes=fft1_block*sizeof(float);
// Each channel input is 16 bit or 32 bit.
rxad.frame=2*ui.rx_ad_channels;
if( (ui.rx_input_mode&DWORD_INPUT) != 0) rxad.frame*=2;
// Get number of points to overlap transforms so window function does
// not drop below 0.5.
// In this way all points contribute by approximately the same amount
// to the averaged power spectrum.
// We also need this to be able to use MMX for the back transform.
fft1_interleave_points=1+fft1_interleave_ratio*fft1_size;
fft1_interleave_points&=0xfffe;
fft1_bandwidth=0.5*ui.rx_ad_speed/((1-fft1_interleave_ratio)*fft1_size);
if( (ui.rx_input_mode&IQ_DATA) != 0)fft1_bandwidth*=2;
// *******************************************************
// Get the sizes for the second fft and the first mixer
// The first frequency mixer shifts the signal in timf1 or timf2 to
// the baseband.
// Rather tham multiplying with a sin/cos table of the selected frequency
// we use the corresponding fourier transforms from which a group of lines
// are selected, frequency shifted and back transformed.
new_mix1_n:;
mix1.n=fft1_n-genparm[MIX1_BANDWIDTH_REDUCTION_N];
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
  fft2_size=0;
  timf2_size=0;
// If we get signals from back transformation of fft1
// we use an interleave ratio that makes the interleave points
// go even up in mix1.size.     
  if(mix1.n < 3)mix1.n=3;
  mix1.size=1<<mix1.n;
  mix1.interleave_points=fft1_interleave_ratio*mix1.size;
  mix1.interleave_points&=0xfffffffe;  
  fft1_interleave_points=mix1.interleave_points*(fft1_size/mix1.size);
  fft1_block_timing();
  fftx_blocktime=fft1_blocktime;
  spur_freq_factor=(float)(fft1_new_points)/fft1_size;
  timf3_sampling_speed=timf1_sampling_speed/fft1_size;
  } 
else
  {
  if(calibrate_flag == 0 && fft1_n > 15)goto reduce_fft1_size;
// Make the time constant for the blanker noise floor about 1 second.
// The first fft outputs data in blocks of fft1_new_points
  fft1_block_timing();
  j=(float)(ui.rx_ad_speed+fft1_new_points/2)/fft1_new_points;
  if(j<1)j=1;
  timf2_noise_floor_avgnum=j;
  blanker_info_update_counter=0;
  fft1_lowlevel_fraction=.75;
  j/=8;
  if(j<1)j=1;
  blanker_info_update_interval=j;
  timf2_display_interval=timf2_noise_floor_avgnum/10;
  if(timf2_display_interval < 2)timf2_display_interval=2;
  timf2_ovfl=0;
  timf2_display_counter=0;
  timf2_display_maxpoint=0;
  timf2_display_maxval_uint=0;
  timf2_display_maxval_float=0;
  fft2_interleave_ratio=make_interleave_ratio(SECOND_FFT_SINPOW); 
  j=1<<(genparm[SECOND_FFT_NINC]);
  fft2_n=fft1_n;
fft2_sizeset:;  
  fft2_size=1<<fft2_n;
  if(fft2_n > 12 && ui.max_blocked_cpus == 0)
    {
    yieldflag_fft2_fft2=TRUE;
    }
  else  
    {
    yieldflag_fft2_fft2=FALSE;
    }
  fft2_bandwidth=0.5*ui.rx_ad_speed/((1-fft2_interleave_ratio)*fft2_size);
  if( (ui.rx_input_mode&IQ_DATA) != 0)fft2_bandwidth*=2;
  if(fft2_bandwidth*j < 1.5*fft1_bandwidth)goto fft2_size_x;
  fft2_n++;
  goto fft2_sizeset;
fft2_size_x:;
  fft2_interleave_points=1+fft2_interleave_ratio*fft2_size;
  fft2_interleave_points&=0xfffe;
// Make timf2 hold 2 seconds of data
  timf2_size=2*timf1_sampling_speed;
// In case we are in the powtim function, fft2 is not
// in use although we use its arrays. Change accordingly:
  if(lir_status==LIR_POWTIM)
    {
    timf2_size=timf1_sampling_speed*genparm[FFT1_STORAGE_TIME];
    }
  if(timf2_size > 300000000)timf2_size=300000000;
  make_power_of_two(&timf2_size);
// or four fft2 transforms if that is more
  if(timf2_size < 4*fft2_size) timf2_size = 4*fft2_size;
  while(timf2_size < 2*screen_width*ui.rx_rf_channels)timf2_size*=2;
// There are 2 transforms (strong, weak) for each rx channel
  timf2pow_mask=timf2_size-1;
  timf2_totbytes=4*ui.rx_rf_channels*timf2_size;
  timf2_mask=timf2_totbytes-1;
  timf2_input_block=(fft1_size-fft1_interleave_points)*4*ui.rx_rf_channels;
  timf2_pa=0;
  timf2_pn1=0;
  timf2_pn2=0;
  timf2_pt=0;
  timf2p_fit=0;
  timf2_px=0;
// Set start noise floor 23dB above one bit amplitude
// and set channel noise = 1 so sum never becomes zero to avoid
// problems with log function.
  timf2_noise_floor=200; 
  timf2_despiked_pwr[0]=timf2_noise_floor;
  timf2_despiked_pwrinc[0]=1;
  if(ui.rx_rf_channels == 2)
    {
    timf2_despiked_pwr[1]=timf2_noise_floor;
    timf2_despiked_pwrinc[1]=1;
    timf2_noise_floor*=2;
    }
  timf2_fitted_pulses=0;
  timf2_cleared_points=0;
  timf2_blanker_points=0;
  clever_blanker_rate=0;
  stupid_blanker_rate=0;
  mix1.n+=fft2_n-fft1_n;
  if(mix1.n < 3)mix1.n=3;
  mix1.size=1<<mix1.n;
  if(mix1.n > 12)
    {
    yieldflag_ndsp_mix1=TRUE;
    }
  else
    {
    yieldflag_ndsp_mix1=FALSE;
    }
  if(genparm[SECOND_FFT_ENABLE] == 0)
    {
    if(ui.max_blocked_cpus > 1)yieldflag_ndsp_mix1=FALSE;
    }
  else  
    {
    if(ui.max_blocked_cpus > 3)yieldflag_ndsp_mix1=FALSE;
    }
  mix1.interleave_points=fft2_interleave_ratio*mix1.size;
  mix1.interleave_points&=0xfffffffe;  
  fft2_interleave_points=mix1.interleave_points*(fft2_size/mix1.size);
  fft2_interleave_ratio=(float)(fft2_interleave_points)/fft2_size;
  fft2_new_points=fft2_size-fft2_interleave_points;
  fft2_blocktime=(float)(fft2_new_points)/timf1_sampling_speed;
  fftx_blocktime=fft2_blocktime;
// Keep fft2 transforms for genparm[FFT2_STORAGE_TIME] seconds.
  max_fft2n=genparm[FFT2_STORAGE_TIME]/fft2_blocktime;
  make_power_of_two(&max_fft2n);
  if(max_fft2n<4)max_fft2n=4;
  t1=(float)(max_fft2n)*fft2_blockbytes;
  if(t1 > 0x20000000)
    {
    max_fft2n=0x20000000/fft2_blockbytes;
    }
  fft2n_mask=max_fft2n-1;
  fft2_blockbytes=twice_rxchan*fft2_size;
  fft2_totbytes=fft2_blockbytes*max_fft2n;
  fft2_mask=fft2_totbytes-1;
  fft2_pa=0;
  fft2_pt=0;
  fft2_na=0;
  fft2_nc=0;
  fft2_nb=fft2n_mask;
  fft2_nx=0;
  fft2_nm=0;
  timf3_sampling_speed=timf1_sampling_speed/fft2_size;
  timf2_output_block=fft2_new_points*4*ui.rx_rf_channels;
  spur_freq_factor=(float)(fft2_new_points)/fft2_size;
  }
timf3_sampling_speed*=mix1.size;
// ***********************************************************
// Transfer from the hardware device driver to our part of memory is
// done by read statements.
// If too few bytes are read each time we loose too much time because
// of the overhead of each read statement.
// If too many bytes are read each time we get an extra delay that
// is not acceptable in some receive modes.
// One output transform is fft1_blockbytes or fft1_size*twice_rxchan data words
// In I/Q mode (complex input format) the input contains the same
// number of data words as the output and the same number of fragments.
// In real mode (real input format) the fft routine contains a reduction
// of sampling speed (real to complex conversion). 
// The number of input words is still the same as the number of output words
// but the fragment size is 2 times smaller at twice the sampling frequency.
// Make the maximum delay time 25% of the time for a full transform.
// This allows the user to force low delay times by selecting large
// fft1 bandwidths. (At questionable noise blanker performance)
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
  min_delay_time=fft1_size;
  }
else  
  {
  min_delay_time=fft2_size;
  }
min_delay_time/=4*timf1_sampling_speed;
if(min_delay_time>0.1)min_delay_time=0.1;
rxad.block_frames=min_delay_time*ui.rx_ad_speed;
make_power_of_two((int*)&rxad.block_frames);
interrupt_rate=(float)(ui.rx_ad_speed)/rxad.block_frames;
while(interrupt_rate > ui.max_dma_rate)
  {
  interrupt_rate/=2;
  rxad.block_frames*=2;
  }
i=0;
switch (os_flag)
  {
  case OS_FLAG_LINUX:
  i=131072;
  break;

  case OS_FLAG_WINDOWS:
  if(diskread_flag != 2 && ui.rx_addev_no == PERSEUS_DEVICE_CODE)
    {
    i=131072;
    }
  else
    {  
    i=4096;
    }
  if(ui.rx_addev_no > 255)i<<=1;
  break;
  
  case OS_FLAG_X:
  i=131072;
  break;

  default:
  lirerr(89231);
  return;
  }
if( (ui.network_flag&NET_RX_INPUT)!=0)
  {
  rxad.block_frames=rxad.block_bytes/rxad.frame;
  interrupt_rate=(float)(ui.rx_ad_speed)/rxad.block_frames;
  }
else
  {    
  if(diskread_flag != 2 && (ui.rx_addev_no == SDR14_DEVICE_CODE ||
                            ui.rx_addev_no == SDRIQ_DEVICE_CODE ))
    {
    rxad.block_bytes=8192;
    rxad.frame=4;
    rxad.block_frames=2048;
    interrupt_rate=(float)(ui.rx_ad_speed)/rxad.block_frames;
    }
  else
    {  
    rxad.block_bytes=rxad.block_frames*rxad.frame;
// Set the interrupt rate high in case we write to the network.
// This will spread out the packages in time and make them easier
// to pick up on a slow computer.
    k=10;
    if( (ui.network_flag & (NET_RXOUT_FFT1+NET_RXOUT_FFT2+NET_RXOUT_TIMF2))!=0)
      {
      k=400;
      }
    if( (ui.network_flag&(NET_RXOUT_RAW16+NET_RXOUT_RAW18+NET_RXOUT_RAW24))!=0)
      {
      k=200;
      }
// Set interrupt rate high if the transmit side is enabled.
// This will reduce time jitter on the input from the morse hand key.
    if(ui.tx_enable != 0)k=400;
    if(k>ui.max_dma_rate)k=ui.max_dma_rate;
    if(k<10)k=10;
// If we read from disk, allow big buffers.
// But not if we send to the network.
    if( diskread_flag >=2 && (ui.network_flag & NET_RX_OUTPUT)==0)
      {
      while(interrupt_rate > 200)
        {
        rxad.block_frames*=2;
        rxad.block_bytes*=2;
        interrupt_rate/=2;
        }
      }
    else
      { 
      while( 2*interrupt_rate < k || (int)rxad.block_bytes > i)
        {
        rxad.block_frames/=2;
        rxad.block_bytes/=2;
        interrupt_rate*=2;
        }
      while(rxad.block_bytes < 32)
        {
        rxad.block_frames*=2;
        rxad.block_bytes*=2;
        interrupt_rate/=2;
        }
      }  
    }
  }
// With the Delta44 we save 18 bit out of the 32 bits we read.
// Out of the 24 bits of data the last 6 contain only noise.
// Allocate rxad.block_bytes even if we use 16 bit format because
// the network write may use this scratch area.
save_rw_bytes=rxad.block_bytes;
rx_read_bytes=rxad.block_bytes;
if((ui.rx_input_mode&DWORD_INPUT) != 0)save_rw_bytes=18*rxad.block_bytes/32;
ad_bufmargin=rxad.block_bytes+fft1_size*rxad.frame;
timf1_blockbytes=fft1_new_points*rxad.frame;
timf1_usebytes=fft1_size*rxad.frame;
// In case fft1_size is small, the input thread might wake up the 
// wideband routine very often causing needless overhead.
if(timf1_usebytes < (int)rxad.block_bytes)timf1_usebytes=rxad.block_bytes;
fft1_hz_per_point=(float)ui.rx_ad_speed/fft1_size;
if( (ui.rx_input_mode&IQ_DATA) == 0)
  {
  fft1_hz_per_point/=2;
  timf1_blockbytes*=2;
  timf1_usebytes*=2;
  }
if( (ui.rx_input_mode&IQ_DATA) != 0)
// In case the oscilloscope for timf3 is enabled we want to
// update the screen at about TIMF3_OSCILLOSCOPE_RATE Hz.
timf3_osc_interval=twice_rxchan*(int)(timf3_sampling_speed/
                                                      TIMF3_OSCILLOSCOPE_RATE);
m=1+twice_rxchan;
t1=screen_height/(2*m);
for(i=0; i<8; i++)timf3_y0[i]=screen_height-(i+1)*t1;

// Allow for genparm[MIX1_NO_OF_CHANNELS] signals 
// during the time for which we have transforms plus
// the duration of a transform (with a 5 s margin)
if( genparm[SECOND_FFT_ENABLE] == 0)
  {
  t1=genparm[FFT1_STORAGE_TIME]+fft1_size/timf1_sampling_speed;
  }
else
  {
  t1=genparm[FFT2_STORAGE_TIME]+fft2_size/timf1_sampling_speed;
  }
t1+=5;
timf3inc:;  
timf3_size=t1*timf3_sampling_speed;
make_power_of_two(&timf3_size);
while(timf3_size < 2*screen_width)timf3_size *= 2;
timf3_size*=twice_rxchan;
timf3_mask=timf3_size-1;
timf3_totsiz=genparm[MIX1_NO_OF_CHANNELS]*timf3_size;
if(timf3_totsiz < 32000)
  {
  t1*=2;
  goto timf3inc;
  }
mix1.new_points=mix1.size-mix1.interleave_points;
timf3_block=twice_rxchan*mix1.new_points;
timf3_pa=0;
timf3_px=0;
timf3_py=0;
timf3_oscilloscope_limit=timf3_osc_interval+
                                    twice_rxchan*(mix1.new_points+screen_width/2);
// ***********************************************************
// In case second fft is not enabled we need a buffer that can hold 
// transforms during the time we average over in the AFC process
// in case it is enabled.
max_fft1n=8;
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
// Keep fft1 transforms for genparm[FFT1_STORAGE_TIME] seconds.
  max_fft1n=genparm[FFT1_STORAGE_TIME]/fft1_blocktime;
  make_power_of_two(&max_fft1n);
  }  
// Always allocate at least 8 buffers.
// We may average over 5 buffers and use 2 for raw data processing.
if(max_fft1n < 8)max_fft1n=8;
t1=(float)(max_fft1n)*fft1_size;
if(t1 > 0x8000000)
  {
  max_fft1n=0x8000000/fft1_size;
  }
fft1_bytes=max_fft1n*fft1_size;
if(fft1_bytes < 8*(int)rxad.block_bytes)fft1_bytes=rxad.block_bytes*8;
max_fft1n=fft1_bytes/fft1_size;
fft1n_mask=max_fft1n-1;
fft1_bytes*=twice_rxchan;
fft1_mask=fft1_bytes-1;
fft1_bytes*=sizeof(float);
// ******************************************************
// We store fft1 power spectra in memory for fast calculation of averages.
// Check wide_graph.c for details.
max_fft1_sumsq=(1.+genparm[FFT1_STORAGE_TIME])/fft1_blocktime;
// We may group averages up to 5 to save time and space if avg time is long
if(rx_mode == MODE_RADAR)
  {
  max_fft1_sumsq*=10;
  if((float)(max_fft1_sumsq)*fft1_size > 100000000.)
    {
    max_fft1_sumsq=100000000./fft1_size;
    }
  make_power_of_two(&max_fft1_sumsq);
  
  }
else
  {  
  max_fft1_sumsq/=5;
  k=0;
  while(max_fft1_sumsq != 0)
    {
    max_fft1_sumsq/=2;
    k++;
    }
  max_fft1_sumsq=1<<k;
  if(max_fft1_sumsq<64)max_fft1_sumsq=max_fft1n;
  }
if(genparm[SECOND_FFT_ENABLE] != 0 ||
     (genparm[SECOND_FFT_ENABLE] == 0 && genparm[AFC_ENABLE] == 0))
  {
  fft1afc_flag=0;
  }
else
  {
  fft1afc_flag=1;
  }  
fft1_sumsq_bufsize=max_fft1_sumsq*fft1_size;
max_fft1_sumsq--;
fft1_sumsq_mask=fft1_sumsq_bufsize-1;
// The raw data from the ad converter is read into timf1.
// This buffer is used as input to fft1. For this purpose only a 
// small buffer is needed.
timf1_bytes=4*fft1_blockbytes;  
// We read data into timf1 in blocks of rxad.block_bytes.
if(timf1_bytes < 2*(int)rxad.block_bytes+ad_bufmargin)
                        timf1_bytes=2*rxad.block_bytes+ad_bufmargin;
timf1_bytes+=rxad.frame;
// When data is saved on the hard disk we send the contents of 
// the buffer to the hard disk through a fwrite statement that will
// require the data to stay unchanged until the write is completed.
// Allow the data to reside in the buffer for 0.5 seconds because
// disk writes become pretty slow occasionally.
t1=0.5*ui.rx_ad_speed*rxad.frame;
// In case we use Windows, allow 1 second.
// This is necessary for writing files at 2MHz sampling rate
// with the Perseus HF receiver on a modern computer.
// It is not quite enough for a Compaq 6510b
if(os_flag == OS_FLAG_WINDOWS )t1*=2;
if(timf1_bytes < t1)timf1_bytes=t1;
// Make sure timf1 is also a power of two
make_power_of_two(&timf1_bytes);
if(rx_mode== MODE_TUNE)timf1_bytes=2*fft1_bytes;
timf1_bytemask=timf1_bytes-1; 
timf1_neg=timf1_bytes/2;
timf1p_pa=0;
timf1p_pb=0;
timf1p_px=0;
timf1p_pnx=0;
fft1_pa=0;
fft1_na=0;
fft1_pb=0;
fft1net_pa=0;
fft1net_px=0;
fft1_px=0;
fft1_nm=0;
fft1_nb=0;
fft1_nc=0;
fft1_nx=0;
// In case we use approximate conversion from real to complex data
// we have to allocate memory and prepare for the assembly routines
// in getiq.s
// Set a value in case we run setup first
fft1_sumsq_recalc=fft1_size/2;

// To save time i.e. when oversampling we only compute spectrum between
// the points actually used.
// Set up values (full spectrum) for use in test modes. 
wg.first_xpoint=0; 
wg_last_point=fft1_size-1;
fft1_tmp_bytes=fft1_blockbytes;
if(genparm[SECOND_FFT_ENABLE] != 0 && fft_cntrl[FFT1_BCKCURMODE].mmx == 0)
  {
  fft1_tmp_bytes*=2;
  }
if(fft1_tmp_bytes < (int)rxad.block_bytes)fft1_tmp_bytes=rxad.block_bytes;
// If second fft is enabled, we will calculate the noise floor
// in liminfo_groups segments of the entire spectrum.
// Start by assuming 500Hz is a reasonable bandwidth to get the
// noise floor in.
fftx_points_per_hz=1.0/fft1_hz_per_point;
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
  fftx_size=fft2_size;
  max_fftxn=max_fft2n;
  liminfo_groups=500./fft1_hz_per_point;
  make_power_of_two(&liminfo_groups);
  if(liminfo_groups<16)liminfo_groups=16;
  liminfo_group_points=fft1_size/liminfo_groups;
  fft2_att_limit=fft2_n-1-genparm[SECOND_FFT_ATT_N];
  fft2_to_fft1_ratio=fft2_size/fft1_size;
  fftx_points_per_hz*=fft2_to_fft1_ratio;
  }
else
  {
  fftx_size=fft1_size;
  max_fftxn=max_fft1n;
  liminfo_groups=0;
  }
if(genparm[AFC_ENABLE]==0) genparm[MAX_NO_OF_SPURS] = 0;
fftn_tmp_bytes=fft1_tmp_bytes;
if(2*ui.rx_rf_channels*mix1.size*sizeof(float) > (unsigned)(fftn_tmp_bytes))
  {
  fftn_tmp_bytes=2*ui.rx_rf_channels*mix1.size*sizeof(float);
  }
if(fft1_bytes < fft1_tmp_bytes)fft1_bytes=fft1_tmp_bytes;
if(rx_mode == MODE_TXTEST)
  {
  if(mix1.size*fft1_hz_per_point > 5300)
    {
    genparm[MIX1_BANDWIDTH_REDUCTION_N]++;
    goto new_mix1_n;
    }
  if(mix1.size*fft1_hz_per_point < 2550)
    {
    genparm[MIX1_BANDWIDTH_REDUCTION_N]--;
    goto new_mix1_n;
    }
  }  
if(mix1.size > 32768)
  {
  genparm[MIX1_BANDWIDTH_REDUCTION_N]++;
  goto new_mix1_n;
  }
for(i=0; i<MAX_SC; i++)
  {
  sd[i]=0;
  sc[i]=0;
  }
}

void get_buffers(int filldat)
{
int i,j,k;
int split_size, afcbuf_size;
float t1;
int *inttab;
short int ishort;
afcbuf_size=0;
sw_onechan=(ui.rx_rf_channels==1);
// ***************************************************************
// The memory allocation is essential to program speed.
// Some time critical loops use several arrays. Place them next
// to each other in memory space so cash works well (?).
// (Was important for Pentium MMX, maybe not with modern computers)
init_memalloc(fft1mem, MAX_FFT1_ARRAYS);
mem( 2,&timf1_char,timf1_bytes,0);
mem( 3,&fft1_window,fft1_window_size*sizeof(float),0);
mem( 4,&fft1tab,fft1_costab_size*sizeof(COSIN_TABLE),16*sizeof(float)); 
// ***************************************************************
// fftw_tmp is used in THREAD_WIDEBAND_DSP in: 
// fftw_tmp is also used in THREAD_POWTIM in:
mem( 5,&fftw_tmp,2*fft1_tmp_bytes,8*sizeof(float)); 
mem( 551,&timf2_tmp,2*fft1_tmp_bytes,8*sizeof(float)); 
// ***************************************************************
mem( 552,&fftn_tmp,fftn_tmp_bytes,8*sizeof(float)); 
mem( 553,&rawsave_tmp,rxad.block_bytes,0);
mem( 6,&fft1_permute,fft1_permute_size*sizeof(short int),8*sizeof(int));
mem( 7,&fft1_filtercorr,fft1_blockbytes,8*sizeof(float));
mem( 8,&fft1_char, fft1_bytes,0);
mem( 9,&wg_waterf_sum,fft1_size*sizeof(float),0);
mem(10,&fft1_sumsq,fft1_sumsq_bufsize*sizeof(float),0);
if(fft1afc_flag != 0)
  {
// AFC based on fft1 !!  
  if(sw_onechan)
    {
    mem(1011,&fft1_power,fft1_size*max_fft1n*sizeof(float),0);
    mem(1012,&fft1_powersum,fft1_size*sizeof(float),0);
    }
  else  
    { 
    mem(2011,&fft1_xypower,fft1_size*max_fft1n*sizeof(TWOCHAN_POWER),0);
    mem(2012,&fft1_xysum,fft1_size*sizeof(TWOCHAN_POWER),0);
    }
  }
mem(13,&fft1_slowsum,fft1_size*sizeof(float),0);
if( (ui.rx_input_mode&IQ_DATA) != 0)
  {
  mem(14,&fft1_foldcorr,twice_rxchan*fft1_size*sizeof(float),0);
  }
mem(15,&fft1_spectrum,screen_width*sizeof(short int),0);
mem(16,&fft1_desired,fft1_size*sizeof(float),0);
mem(17,&wg_waterf_yfac,fft1_size*sizeof(float),0);
mem(18,&mix1.permute,mix1.size*sizeof(short int),0);
mem(19,&mix1.table,mix1.size*sizeof(COSIN_TABLE)/2,0);
mem(20,&timf3_float,2*timf3_totsiz*sizeof(float),0);
mem(21,&timf3_graph,ui.rx_rf_channels*screen_width*sizeof(short int),0);
mem(22,&liminfo_wait,fft1_size*sizeof(char),0);
mem(23,&liminfo,2*fft1_size*sizeof(float),0);
mem(24,&liminfo_group_min,liminfo_groups*sizeof(float),0);
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
  split_size=4*ui.rx_rf_channels*fft1_size;
  swfloat=fft_cntrl[FFT1_BCKCURMODE].mmx == 0 && 
                   fft_cntrl[FFT2_CURMODE].mmx == 0;
  if(swfloat)                   
    {
    mem(25,&fft1_split_float, split_size*sizeof(float),0);
    }
  else
    {    
    mem(25,&fft1_split_shi, split_size*sizeof(short int),4*sizeof(int));
    }
  mem( 554,&fftf_tmp,fft2_size*sizeof(float),0); 
  }
mem(26,&mix1_fqwin,(mix1.size/2+16)*sizeof(float),0);
mem(27,&mix1.cos2win,mix1.new_points*sizeof(float),0);
mem(28,&mix1.sin2win,mix1.new_points*sizeof(float),0);
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
  if( genparm[FIRST_FFT_SINPOW]!=0 && 
      (genparm[FIRST_FFT_SINPOW]!= 2 || rx_mode == MODE_TXTEST))
    {
    mem(1029,&mix1.window,(mix1.size/2+1)*sizeof(float),0);
    }
  }
else
  {  
  if( genparm[FIRST_FFT_SINPOW]!= 2)
    {
    if( genparm[FIRST_FFT_SINPOW]!=0 && genparm[FIRST_FFT_SINPOW]!= 2)
      {
      if( fft_cntrl[FFT1_BCKCURMODE].mmx != 0)
        {
        mem(2029,&fft1_inverted_mmxwin,2*(16+fft1_size/2)*4*sizeof(short int),8);
        }
      else
        { 
        mem(3029,&fft1_inverted_window,(16+fft1_size/2)*sizeof(float),0);
        }
      }
    }    
  mem(30,&fft1_backbuffer,4*ui.rx_rf_channels*fft1_size*sizeof(short int),0);
  mem(31,&fft1_back_scramble,fft1_size*sizeof(short int),0);
  mem(3925,&hg_fft2_pwrsum,2*screen_width*sizeof(float),0);
  if(!sw_onechan)
    {
    mem(3926,&hg_fft2_pwr,max_fft2n*2*screen_width*sizeof(float),0);
    }
  if(swfloat)                   
    {
    mem(32,&timf2_float,4*ui.rx_rf_channels*timf2_size*sizeof(float),0);
    if(lir_status != LIR_POWTIM)
      {
      mem(33,&timf2_pwr_float,timf2_size*sizeof(float),0);
      }
    else
      {
      mem(33,&timf2_pwr_int,2*screen_width*sizeof(float),0);
      }  
    }
  else
    {
    mem(32,&timf2_shi,4*ui.rx_rf_channels*timf2_size*sizeof(short int),0);
    mem(33,&timf2_pwr_int,(timf2_size+8)*sizeof(int),4);
    }
  if( fft_cntrl[FFT1_CURMODE].permute == 2)
    {
    mem(34,&fft1_backtab,fft1_size*sizeof(COSIN_TABLE)/2,0); 
    }
  if( fft_cntrl[FFT1_BCKCURMODE].mmx != 0)
    {
    mem(35,&fft1_mmxcosin,
                       fft1_size*sizeof(MMX_COSIN_TABLE)/2, 8*sizeof(float));
    }
  if(sw_onechan)
    {
    mem(1039,&fft2_power_float,fft2_size*max_fft2n*sizeof(float),0);
    mem(1040,&fft2_powersum_float,fft2_size*sizeof(float),0);
    }
  else  
    { 
    mem(2039,&fft2_xypower,fft2_size*max_fft2n*sizeof(TWOCHAN_POWER),0);
    mem(2040,&fft2_xysum,fft2_size*sizeof(TWOCHAN_POWER),0);
    }
  mem(3039,&fft2_bigpermute,fft2_size*sizeof(int),16*sizeof(int));
  if(fft_cntrl[FFT2_CURMODE].mmx == 0)
    { 
    mem(1036,&fft2_float,max_fft2n*2*fft2_size*
                                             ui.rx_rf_channels*sizeof(float),0);
    mem(1037,&fft2_permute,fft2_size*sizeof(short int),0);
    mem(1038,&fft2_tab,fft2_size*sizeof(COSIN_TABLE)/2,0);
    if(genparm[SECOND_FFT_SINPOW] != 0)
      {
      mem(1041,&fft2_window,fft2_size*sizeof(float),0);
      }
    }
  else
    {
    mem(3038,&fft2_short_int,max_fft2n*2*fft2_size*
                               ui.rx_rf_channels*sizeof(short int),0);
    mem(3040,&fft2_mmxcosin,
                      fft2_size*sizeof(MMX_COSIN_TABLE)/2, 16*sizeof(float));
    mem(3041,&fft2_mmxwin,fft2_size*sizeof(short int),0);
    }
  if( lir_status==LIR_POWTIM || 
      (genparm[SECOND_FFT_SINPOW]!= 0 && genparm[SECOND_FFT_SINPOW]!= 2))
    {
    mem(3042,&mix1.window,(mix1.size/2+1)*sizeof(float),0);
    }
  }
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
  afcbuf_size=max_fft1n*MAX_MIX1;
  }
else
  {
  afcbuf_size=max_fft2n*MAX_MIX1;
  }
if(genparm[AFC_ENABLE] != 0)
  {
  mem(43,&mix1_fq_mid,afcbuf_size*sizeof(float),0);
  mem(44,&mix1_fq_start,afcbuf_size*sizeof(float),0);
  mem(45,&mix1_fq_curv,afcbuf_size*sizeof(float),0);
  mem(46,&mix1_fq_slope,afcbuf_size*sizeof(float),0);
  mem(47,&mix1_eval_avgn,afcbuf_size*sizeof(short int),0);
  mem(48,&mix1_eval_fq,afcbuf_size*sizeof(float),0);
  mem(49,&mix1_eval_sigpwr,afcbuf_size*sizeof(float),0);
  mem(50,&mix1_eval_noise,afcbuf_size*sizeof(float),0);
  mem(51,&mix1_fitted_fq,afcbuf_size*sizeof(float),0);
  }
// Allocate memory for automatic spur cancellation 
if(genparm[MAX_NO_OF_SPURS] != 0)
  {
  if(genparm[MAX_NO_OF_SPURS] > fftx_size/SPUR_WIDTH)
    {
    genparm[MAX_NO_OF_SPURS] = fftx_size/SPUR_WIDTH;
    }
  spur_block=SPUR_WIDTH*max_fftxn*twice_rxchan;
  if(genparm[SECOND_FFT_ENABLE] !=0 && fft_cntrl[FFT2_CURMODE].mmx != 0)
    {
    mem(1052,&spur_table_mmx, 
               genparm[MAX_NO_OF_SPURS]*spur_block*sizeof(short int),0);
    }
  else
    {  
    mem(2052,&spur_table, 
                   genparm[MAX_NO_OF_SPURS]*spur_block*sizeof(float),0);
    }
  mem(53,&spur_location, genparm[MAX_NO_OF_SPURS]*sizeof(int),0);
  mem(54,&spur_flag, genparm[MAX_NO_OF_SPURS]*sizeof(int),0);
  mem(55,&spur_power, SPUR_WIDTH*sizeof(float),0);
  mem(56,&spur_d0pha, genparm[MAX_NO_OF_SPURS]*sizeof(float),0);
  mem(57,&spur_d1pha, genparm[MAX_NO_OF_SPURS]*sizeof(float),0);
  mem(58,&spur_d2pha, genparm[MAX_NO_OF_SPURS]*sizeof(float),0);
  mem(59,&spur_ampl, genparm[MAX_NO_OF_SPURS]*sizeof(float),0);
  mem(60,&spur_noise, genparm[MAX_NO_OF_SPURS]*sizeof(float),0);
  mem(61,&spur_avgd2, genparm[MAX_NO_OF_SPURS]*sizeof(float),0);
  mem(62,&spur_pol,genparm[MAX_NO_OF_SPURS]*3*sizeof(float),0);
  mem(63,&spur_spectra,NO_OF_SPUR_SPECTRA*SPUR_SIZE*sizeof(float),0);
  mem(64,&spur_freq, genparm[MAX_NO_OF_SPURS]*sizeof(int),0);
  mem(65,&spur_ind, genparm[MAX_NO_OF_SPURS]*max_fftxn*sizeof(int),0);
  mem(66,&spur_signal, max_fftxn*twice_rxchan*
                              genparm[MAX_NO_OF_SPURS]*sizeof(float),0);
  if(sw_onechan)
    {
    mem(67,&spursearch_powersum,fftx_size*sizeof(float),0);
    }
  else
    {
    mem(68,&spursearch_xysum,fftx_size*sizeof(TWOCHAN_POWER),0);
    }
  mem(67,&spursearch_spectrum,fftx_size*sizeof(float),0);
  spur_speknum=0.1*genparm[SPUR_TIMECONSTANT]/fftx_blocktime;
  if(spur_speknum < 4)spur_speknum=4;
  if(4*spur_speknum > max_fftxn && genparm[MAX_NO_OF_SPURS] != 0)
    {
    lir_status=LIR_SPURERR;
    return;
    }
  spursearch_sum_counter=0;
  sp_numsub=spur_speknum-1;
  sp_avgnum=spur_speknum/3;
  if(sp_avgnum > 10)sp_avgnum=10;
  spur_max_d2=PI_L*spur_freq_factor/spur_speknum;
// The spur signal is coherently averaged over spur_speknum transforms
// while noise is non-coherent rms value.  
// Set the minimum S/N for at least 3dB in the narrower bandwidth for
// the noise we would get if the noise vas evaluated at the 
// same bandwidth as the signal.
  spur_minston=1/sqrt(0.5*(float)(spur_speknum));
// Make spur_weiold/new These parameters prevent the second
// derivative of the phase (frequency drift) to change rapidly. 
  t1=0.5*spur_speknum;
  spur_weiold=t1/(1+t1);
  spur_weinew=1/(1+t1);
// To fit a straight line to spur phase we need sum of squares.
  t1=-0.5*sp_numsub;
  spur_linefit=0;
  for(i=0; i<spur_speknum; i++)
    {
    spur_linefit+=t1*t1;
    t1+=1;
    }
  }
no_of_spurs=0;
baseband_totmem=0;
afc_totmem=0;
hires_totmem=0;
fft3_totmem=0;
// *********************************************************
// Memory allocations for tx test mode.
if(rx_mode == MODE_TXTEST)
  {
  mem(7201,&txtest_ypeak, screen_width*sizeof(short int),0);
  mem(7211,&txtest_ypeak_decay, screen_width*sizeof(short int),0);
  mem(7202,&txtest_yavg, screen_width*sizeof(short int),0);
  i=(1+5*max_fft1_sumsq)*screen_width;
  mem(7203,&txtest_power, i*sizeof(float),0);
  mem(7204,&txtest_powersum, screen_width*sizeof(float),0);
  mem(7205,&txtest_peak_power, screen_width*sizeof(float),0);
  mem(7215,&txtest_peak_power_decay, screen_width*sizeof(float),0);
  mem(7206,&txtest_old_yavg, screen_width*sizeof(short int),0);
  mem(7207,&txtest_old_ypeak, screen_width*sizeof(short int),0);
  mem(7217,&txtest_old_ypeak_decay, screen_width*sizeof(short int),0);
  mem(7208,&fft1_old_spectrum, screen_width*sizeof(short int),0);
  }
i=screen_width;
make_power_of_two(&i);
mg_size=i;
mg_mask=mg_size-1;  
mem(7246,&mg_barbuf, 3*text_width*screen_height*sizeof(char),0);
mem(7247,&mg_rms_meter, mg_size*ui.rx_rf_channels*sizeof(float),0);
mem(7248,&mg_peak_ypix, 2*mg_size*ui.rx_rf_channels*sizeof(short int),0);
mem(7249,&mg_rms_ypix, 2*mg_size*ui.rx_rf_channels*sizeof(short int),0);
mem(7250,&mg_behind_meter, screen_height*sizeof(char),0);
mem(7254,&mg_peak_meter, mg_size*ui.rx_rf_channels*sizeof(float),0);
if( (ui.network_flag&NET_RXOUT_FFT1) != 0)
  {
  fft1net_size=fft1_blockbytes*FFT1_NETSEND_MAXBUF;
  fft1net_mask=fft1net_size-1;
  mem(7467,&fft1_netsend_buffer,fft1net_size,0);
  }
if( (ui.network_flag&NET_RXOUT_BASEB) != 0)
  {
// Allocate space for 0.3 seconds of data.  
  basebnet_size=0.3*genparm[DA_OUTPUT_SPEED]*2*
                                      ui.rx_rf_channels*sizeof(short int);
  make_power_of_two(&basebnet_size);
  basebnet_mask=basebnet_size-1;
  mem(7577,&baseb_netsend_buffer,basebnet_size,0);
  basebnet_block_bytes=(2*ui.rx_rf_channels*genparm[DA_OUTPUT_SPEED])
                                                       /ui.max_dma_rate;
  make_power_of_two(&basebnet_block_bytes);
  }
fftx_totmem=memalloc((int*)(&fft1_handle),"fft1,fft2");
if(fftx_totmem==0)
  {
  lir_status=LIR_FFT1ERR;
  return;
  }
if(filldat==0)return;
fft1_calibrate_flag=0;
for(i=0; i<genparm[MAX_NO_OF_SPURS]*max_fftxn; i++)spur_ind[i]=-1;
if(genparm[AFC_ENABLE] != 0)
  {
  for(i=0; i<afcbuf_size; i++)mix1_fq_mid[i]=-1;
  for(i=0; i<afcbuf_size; i++)mix1_fq_start[i]=-1;
  for(i=0; i<afcbuf_size; i++)mix1_fq_slope[i]=0;
  for(i=0; i<afcbuf_size; i++)mix1_fq_curv[i]=0;
  for(i=0; i<MAX_MIX1; i++)mix1_phase_step[i]=0;
  for(i=0; i<MAX_MIX1; i++)mix1_phase[i]=0;
  for(i=0; i<MAX_MIX1; i++)mix1_phase_rot[i]=0;
  for(i=0; i<MAX_MIX1; i++)mix1_old_phase[i]=0;
  for(i=0; i<MAX_MIX1; i++)mix1_old_point[i]=0;
  }
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
  for(i=0; i<2*screen_width; i++)hg_fft2_pwrsum[i]=1;
  }
timf1_float=(void*)(timf1_char);
timf1_int=(void*)(timf1_char);
timf1_short_int=(void*)(timf1_char);
fft1_short_int=(void*)(fft1_char);
fft1_int=(void*)(fft1_char);
fft1_float=(void*)(fft1_char);
timf3_int=(void*)(timf3_float);
if(genparm[MAX_NO_OF_SPURS] != 0)
  {
// Spur removal runs with fft1 or fft2. Place its scratch areas
// on the correct thread.  
  if(genparm[SECOND_FFT_ENABLE] == 0)
    {
    sp_sig=(void*)(fftw_tmp);
    }
  else
    {
    sp_sig=(void*)(fftf_tmp);
    }
  sp_der=(void*)(sp_sig)+fftx_size*sizeof(float)/4;
  sp_pha=(void*)(sp_der)+fftx_size*sizeof(float)/4;
  sp_tmp=(void*)(sp_pha)+fftx_size*sizeof(float)/4;
  for(i=0; i<fftx_size; i++)spursearch_spectrum[i]=1;
  }
// *********************************************************************
make_window(5,mix1.size, 4, mix1_fqwin);
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
  if(lir_status != LIR_POWTIM) prepare_mixer(&mix1, FIRST_FFT_SINPOW);
  }
else
  {
  prepare_mixer(&mix1, SECOND_FFT_SINPOW);
  if( genparm[FIRST_FFT_SINPOW]!= 2)
    {
    if( fft_cntrl[FFT1_BCKCURMODE].mmx != 0)
      {
      make_mmxwindow(3,fft1_size, genparm[FIRST_FFT_SINPOW], fft1_inverted_mmxwin);
      }
    else
      {
      make_window(3,fft1_size, genparm[FIRST_FFT_SINPOW], fft1_inverted_window);
      }
    }
  make_permute(fft_cntrl[FFT1_BCKCURMODE].permute, fft1_n, 
                                                fft1_size, fft1_back_scramble);
  if(fft_cntrl[FFT1_CURMODE].permute == 2)
    {
    make_sincos(0, fft1_size, fft1_backtab); 
    }  
  else
    {
    fft1_backtab=fft1tab; 
    }
  if( fft_cntrl[FFT1_BCKCURMODE].mmx != 0)
    {
    init_mmxfft(fft1_size,fft1_mmxcosin);
    }
  make_bigpermute(fft_cntrl[FFT2_CURMODE].permute, 
                                        fft2_n, fft2_size, fft2_bigpermute);
  if(fft_cntrl[FFT2_CURMODE].mmx == 0)
    { 
    make_sincos(1, fft2_size, fft2_tab); 
/*
// ******************  öööööööööö
// use fft2_permute. May be needed for calibrate or powtim
// but probably not. Just some wasted memory?????
    if(fft2_size>65536)
      {
      i=65536;
      j=16;
      }
    else
      {  
      i=fft2_size;
      j=fft2_n;
      }
    make_permute(fft_cntrl[FFT2_CURMODE].permute, j, i, fft2_permute);
*/
// ****************************************************
    if(genparm[SECOND_FFT_SINPOW] != 0)
      {
      make_window(fft_cntrl[FFT2_CURMODE].window,fft2_size,
                                    genparm[SECOND_FFT_SINPOW], fft2_window);
      }
    }
  else
    {
    if(genparm[SECOND_FFT_SINPOW] != 0)
      {
      make_mmxwindow(0,fft2_size, genparm[SECOND_FFT_SINPOW], fft2_mmxwin);
// Scramble the window so we get it in the order it will be used.
      for(i=0; i<fft2_size; i++)
        {
        j=fft2_bigpermute[i];
        if(j<i)
          {
          ishort=fft2_mmxwin[i];
          fft2_mmxwin[i]=fft2_mmxwin[j];
          fft2_mmxwin[j]=ishort;
          }
        }  
      }
// Scale fft2_bigpermute so mmx routines do not have to multiply addresses.
    if(sw_onechan)
      {
      k=8;
      }
    else
      {
      k=16;
      }
    for(i=0; i<fft2_size; i++)fft2_bigpermute[i]*=k;    
// The mmx routine wants to negate the imaginary parts in the first
// loop. Put suitable pattern for before bigpermute.
    fft2_bigpermute[-1]=0xffff0000;
    fft2_bigpermute[-2]=0xffff0000;
    fft2_bigpermute[-3]=0x10000;
    fft2_bigpermute[-4]=0x10000;
    fft2_bigpermute[-5]=0xffff;
    fft2_bigpermute[-6]=0xffff;
    fft2_bigpermute[-7]=1;
    fft2_bigpermute[-8]=1;
    fft2_bigpermute[-9]=0xffff;
    fft2_bigpermute[-10]=0xffff0000;
    fft2_bigpermute[-11]=1;
    fft2_bigpermute[-12]=0x10000;
    init_mmxfft(fft2_size,fft2_mmxcosin);
    }
  }
i=1;
if(fft_cntrl[FFT1_CURMODE].permute == 2)i=2;
make_sincos(i, fft1_size, fft1tab); 
// Store bitmask for use by SIMD routine in scratch area before fft1tab
inttab=(void*)(fft1tab);
inttab[-1]=0x80000000;
inttab[-2]=0;
inttab[-3]=0x80000000;
inttab[-4]=0;
// Bitmask for use by simd routine in scratch area before fft1_filtercorr
inttab=(void*)(fft1_filtercorr);
inttab[-1]=0x80000000;
inttab[-2]=0;
inttab[-3]=0x80000000;
inttab[-4]=0;
if(kill_all_flag)return;
make_permute(fft_cntrl[FFT1_CURMODE].permute, fft1_n, fft1_size, fft1_permute);
make_window(fft_cntrl[FFT1_CURMODE].window,fft1_size,
                                    genparm[FIRST_FFT_SINPOW], fft1_window);
if( (ui.network_flag & NET_RXIN_FFT1) == 0)
  {
  init_fft1_filtercorr();
  if(kill_all_flag)return;
  if( (ui.rx_input_mode&IQ_DATA) != 0)
    {
    init_foldcorr();
    }
  if(kill_all_flag)return;
  }
memset(fft1_char,0,fft1_bytes);
memset(liminfo,0,2*fft1_size*sizeof(float));
memset(liminfo_wait,0,fft1_size*sizeof(unsigned char));
memset(fft1_slowsum,0,fft1_size*sizeof(float));
memset(wg_waterf_sum,0,fft1_size*sizeof(float));
memset(timf1_char,0,timf1_bytes);
memset(fft1_sumsq,0,fft1_sumsq_bufsize*sizeof(float));
memset(timf3_float,0,2*timf3_totsiz*sizeof(float));
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
  if(swfloat)                   
    {
    if(lir_status != LIR_POWTIM)
      {
      for(i=0; i<timf2_size; i++)
        {
        timf2_pwr_float[i]=0.5;
        }
      }  
    memset(timf2_float,0,4*ui.rx_rf_channels*timf2_size*sizeof(float));
    }
  else  
    {
    for(i=0;i<timf2_size+8; i++)timf2_pwr_int[i]=4;
    memset(timf2_shi,0,4*ui.rx_rf_channels*timf2_size*sizeof(short int));
    }
  }
memset(mg_rms_meter,0,mg_size*ui.rx_rf_channels*sizeof(float));
memset(mg_peak_meter,0,mg_size*ui.rx_rf_channels*sizeof(float));
memset(mg_rms_ypix,0,2*mg_size*ui.rx_rf_channels*sizeof(short int));
memset(mg_peak_ypix,0,2*mg_size*ui.rx_rf_channels*sizeof(short int));
measured_ad_speed=-1;
measured_da_speed=-1;
fft1_sumsq_counter=0;
fft1_sumsq_tot=0;
wg_flag=0;
hg_flag=0;
ag_flag=0;
bg_flag=0;
pg_flag=0;
cg_flag=0;
eg_flag=0;
fg_flag=0;
mg_flag=0;
tg_flag=0;
rg_flag=0;
eme_active_flag=0;
timinfo_flag=0;
ampinfo_flag=0;
numinput_flag=0;
no_of_scro=0;
fft1_sumsq_pa=0;
fft1_sumsq_pwg=0;
fft1_sumsq_pws=0;
wg_waterf_sum_counter=0;
wg_waterf=NULL;
wg_waterf_ptr=0;
bg_waterf_sum_counter=0;
bg_waterf=NULL;
bg_waterf_ptr=0;
local_bg_waterf_ptr=0;
wg_background=NULL;
fft1_noise_floor=0;
overrun_count=0;
new_baseb_flag=-1;
diskread_pause_flag=0;
fft1back_lowgain_n=genparm[FIRST_BCKFFT_ATT_N];
// In case fft2 is much bigger than fft1 it will be a good idea to
// split fft2 processing into smaller chunks so overrun problems
// will not occur.
t1=(float)(fft2_new_points)/fft1_new_points; 
fft2_chunk_n=(int)(1+fft2_n/t1)&0xfffffffe;
if(fft2_chunk_n<2)fft2_chunk_n=2;
for(i=0; i<genparm[MIX1_NO_OF_CHANNELS]; i++)
  {
  mix1_selfreq[i]=-1;
  mix1_curx[i]=-1;
  new_mix1_curx[i]=-1;
  }
// Clear variables that might become uninitialised
total_wttim=0;
ad_wttim=0;
fft1_wttim=0;
timf2_wttim=0;
fft2_wttim=0;
timf3_wttim=0;
fft3_wttim=0;
baseb_wttim=0;
db_wttim=0;
da_wttim=0;
afc_tpts=0;
afc_curx=-1;
hg_curx=-1;
// Initialise fftx variables (used by AFC)
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
  fftxn_mask=fft2n_mask;
  fftx_na=fft2_na;
  fftx_pwr=fft2_power_float;
  fftx_nm=fft2_nm;
  fftx_nx=fft2_nx;
  fftx_xypower=fft2_xypower;
  fftx=fft2_float;
  }
else
  {
  fftxn_mask=fft1n_mask;
  fftx_na=fft1_nb;
  fftx_pwr=fft1_power;
  fftx_nm=fft1_nm;
  fftx_nx=fft1_nx;
  fftx=fft1_float;
  fftx_xypower=fft1_xypower;
  }
if(fft1afc_flag != 0)
  {
// AFC based on fft1 !!  
  if(sw_onechan)
    {
    memset(fft1_power,0,max_fft1n*fft1_size*sizeof(float));
    }
  else  
    { 
    memset(fft1_xypower,0,fft1_size*max_fft1n*sizeof(TWOCHAN_POWER));
    }
  }  
if(genparm[AFC_ENABLE] != 0)
  {
  for(i=0; i<afcbuf_size; i++)
    {
    mix1_eval_noise[i]=1;
    mix1_eval_fq[i]=1;
    }
  }  
if(genparm[SECOND_FFT_ENABLE] != 0)
  { 
  if(fft_cntrl[FFT2_CURMODE].mmx == 0)
    { 
    memset(fft2_float,0,max_fft2n*2*fft2_size*ui.rx_rf_channels*sizeof(float));
    }
  else
    {
    memset(fft2_short_int,0,max_fft2n*2*fft2_size*ui.rx_rf_channels*sizeof(short int));
    }
  if(sw_onechan)
    {
    memset(fft2_power_float,0,max_fft2n*fft2_size*sizeof(float));
    }
  else  
    { 
    memset(fft2_xypower,0,fft2_size*max_fft2n*sizeof(TWOCHAN_POWER));
    }  
  }
if(genparm[MAX_NO_OF_SPURS] != 0)
  {
  init_spur_spectra();
  }
if(rx_mode == MODE_TXTEST)
  {
  for(i=0; i<screen_width; i++)txtest_peak_power_decay[i]=0.1;
  for(i=0; i<screen_width; i++)txtest_powersum[i]=0.1;
  k=(1+5*max_fft1_sumsq)*screen_width;
  for(i=0; i<k; i++)txtest_power[i]=0.1;

/*
Arrays that seem to not need initialization.
  mem(7201,&txtest_ypeak, screen_width*sizeof(short int),0);
  mem(7211,&txtest_ypeak_decay, screen_width*sizeof(short int),0);
  mem(7202,&txtest_yavg, screen_width*sizeof(short int),0);
  mem(7205,&txtest_peak_power, screen_width*sizeof(float),0);
  mem(7206,&txtest_old_yavg, screen_width*sizeof(short int),0);
  mem(7207,&txtest_old_ypeak, screen_width*sizeof(short int),0);
  mem(7217,&txtest_old_ypeak_decay, screen_width*sizeof(short int),0);
  mem(7208,&fft1_old_spectrum, screen_width*sizeof(short int),0);
*/
  }
  
  
wav_write_flag=0;
audio_dump_flag=0;
swmmx_fft2=(genparm[SECOND_FFT_ENABLE] != 0 
                                     && fft_cntrl[FFT2_CURMODE].mmx != 0);
swmmx_fft1=(genparm[SECOND_FFT_ENABLE] != 0 
                                     && fft_cntrl[FFT1_BCKCURMODE].mmx != 0);
old_passband_center=-1;
indicator_first_pixel=29*text_width;
indicator_ypix=screen_height-4.5*text_height;
timf1_indicator_block=timf1_bytes/INDICATOR_SIZE;
fft1_indicator_block=(fft1_mask+1)/INDICATOR_SIZE;
timf2_indicator_block=(timf2_mask+1)/INDICATOR_SIZE;
fft2n_indicator_block=max_fft2n*fft2_size/INDICATOR_SIZE;
timf3_indicator_block=timf3_size/INDICATOR_SIZE;
clear_wide_maxamps();
for(i=0; i<MAX_SC; i++)sc[i]=0;
// Disable the S-meter averaging. (F2 while running to enable)
s_meter_avgnum=-1;
// Make the output buffer big enough to hold data for at least 6
// times longer than the rate at which the wideband thread posts
// to the narrowband thread.
t1=(3*fftx_size)/ui.rx_ad_speed;
// Make sure minimum is 0.25 seconds
if(t1<0.25)t1=0.25;
// Set the output block size for a time similar to min_delay_time.
// Note that the open_rx_sndout routine may change this block size!!!
rx_daout_block=0.75*rxda.frame*min_delay_time*genparm[DA_OUTPUT_SPEED];
make_power_of_two(&rx_daout_block);
// make the minimum block 32 bytes in accordance with the minimum
// fragment size of OSS (frag=4)
if(rx_daout_block < 32)rx_daout_block=32;
if(os_flag == OS_FLAG_WINDOWS && min_delay_time < 0.02)rx_daout_block*=4;
// Print some info in case we have a dump file
if(dmp!=NULL)
  {
  PERMDEB"ui.rx_rf_channels=%d, ",ui.rx_rf_channels);
  PERMDEB"ui.rx_ad_channels=%d\n",ui.rx_ad_channels);
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    PERMDEB"NOT ");
    }
  PERMDEB"DWORD INPUT, "); 
  if( (ui.rx_input_mode&IQ_DATA) == 0)
    {
    PERMDEB"NOT ");
    }
  PERMDEB"IQ DATA, rxad.block_bytes %d, ",rxad.block_bytes);
  PERMDEB"RXin interrupt_rate %.1f\n",interrupt_rate);
//
  PERMDEB"fft1_n %d, ",fft1_n);
  PERMDEB"fft1_size %d, ",fft1_size);
  PERMDEB"interleave (_ratio %f  _points %d)\n",
                              fft1_interleave_ratio,fft1_interleave_points);
//
  PERMDEB"fft1mode %d, ",fft1mode);
  PERMDEB"rxad.frame %d, ",rxad.frame);
  PERMDEB"fft1 vers [%d] %s\n",genparm[FIRST_FFT_VERNR],
                                        fft_cntrl[FFT1_CURMODE].text);
//
  PERMDEB"timf1_blockbytes %d, ",timf1_blockbytes);
  PERMDEB"fft1_blockbytes %d, ",fft1_blockbytes);
  PERMDEB"timf1_bytes %d\n",timf1_bytes);
//
  PERMDEB"fft1_bytes %d, ",fft1_bytes);
  PERMDEB"max_fft1_sumsq %d, ",max_fft1_sumsq);
  PERMDEB"fft1win (N) %d, ",genparm[FIRST_FFT_SINPOW]);
  PERMDEB"mix1.size %d\n",mix1.size);
//
  PERMDEB"backwards fft1: ");
  if(swfloat)
    {
    PERMDEB"32 bit float");
    }
  else
    {
    PERMDEB"16 bit int");
    }
  PERMDEB", fft2_size %d, ",fft2_size);
  if(fft_cntrl[FFT2_CURMODE].mmx == 0)
    {
    PERMDEB"32 bit float");
    }
  else
    {
    PERMDEB"16 bit int");
    }
  PERMDEB"\ntimf3_size %d, ",timf3_size);
  PERMDEB"timf3_sampling_speed %f\n",timf3_sampling_speed);
  fflush(dmp);
  }
memcheck(1,fft1mem,(int*)&fft1_handle);
no_of_rx_overrun_errors=0;
no_of_rx_underrun_errors=0;
no_of_tx_overrun_errors=0;
no_of_tx_underrun_errors=0;
count_rx_underrun_flag=FALSE;
internal_generator_flag=0;
internal_generator_phase1=0;
internal_generator_phase2=0;
internal_generator_shift=0;
internal_generator_key=0;
truncate_flag=0;
ag.mode_control=0;
accumulated_netwait_time=0;
}

void init_blanker(void)
{
int i,j,k,m;
unsigned short int *tmp_permute;
COSIN_TABLE *tmp_tab;
float t1,t2,t3,t4,t5,t6,r1,r2,phrot;
float *buf2, *buf3;
int me01, me04;
buf2=(void*)(fft1_float)+2*fft1_blockbytes;
buf3=(void*)(fft1_float)+3*fft1_blockbytes;
me04=((twice_rxchan+1)*(screen_width+1)*sizeof(short int)+15)&0xfffffff0;
// In case the system is properly calibrated we can use
// fft1_desired to get the pulse response of our system.
init_memalloc(blankermem,MAX_BLANKER_ARRAYS);
mem(5,&timf2pix,me04,0);
if( (fft1_calibrate_flag&CALAMP)==CALAMP)
  {
// Place the pulse at 8 different positions from -0.5 to +0.5
// in order to even out oscillatory behaviors.
// Store the accumulated pulse power in buf2
  tmp_permute=(void*)(fft1_float);
  tmp_tab=(void*)(fft1_float)+fft1_blockbytes;
  init_fft(0,fft1_n, fft1_size, tmp_tab, tmp_permute);
  for(i=0; i<fft1_size/2; i++)buf2[i]=0;  
  t3=0.25*PI_L/fft1_size;
  for(k=0; k<8; k++)
    {
    t1=0;
    t2=(k-3.75)*t3;
    for(i=0; i<fft1_size; i++)
      {
      fftw_tmp[2*i  ]=cos(t1)*fft1_desired[i];
      fftw_tmp[2*i+1]=sin(t1)*fft1_desired[i];
      t1+=t2;
      }
    fftback(fft1_size, fft1_n, fftw_tmp, tmp_tab, tmp_permute,FALSE);
    buf2[0]+=pow(fftw_tmp[1],2.)+pow(fftw_tmp[0],2.);
    for(i=1; i<fft1_size/2; i++)
      {
      j=fft1_size-i;
      buf2[i]+=pow(fftw_tmp[2*i+1],2.)+pow(fftw_tmp[2*i  ],2.)
              +pow(fftw_tmp[2*j+1],2.)+pow(fftw_tmp[2*j  ],2.);
      }
    } 
// Store the relative pulse level=bln[i].rest that remains outside 
// a region of a width=bln[i].size corresponding to a power of n=bln[i].n
  for(i=0; i<BLN_INFO_SIZE; i++)
    {
    bln[i].n=i+2;
    bln[i].size=1<<bln[i].n;
    bln[i].rest=0.5*(buf2[bln[i].size/2]+buf2[bln[i].size/2-1])/buf2[0];
    bln[i].avgmax=(i+1)*(i+1)*bln[i].size;
    }
// skip if rest is too small (-90 dB).
  i=BLN_INFO_SIZE-1;
  while(bln[i].rest < 0.000000001) 
    {
    bln[i].rest=0;
    i--;
    }
  largest_blnfit=i;
  refpul_n=bln[i].n;
  refpul_size=bln[i].size;
  me01=2*MAX_REFPULSES*refpul_size;
// Calculate the summed power of the outer size/2 points 
// for each size. 
// Store relative to buf2[0] in bln[i].avgpwr
  j=1;
  i=0;
  k=2;
check_avgpwr:;
  t1=0;  
  while(k < bln[i].size)
    {
    t1+=buf2[j]; 
    j++;
    k+=2;
    }
  bln[i].avgpwr=t1/buf2[0];
  i++;
  if(i <=largest_blnfit)goto check_avgpwr; 
// Find the -15dB width of the pulse response but do not make 
// blanker_pulsewidth smaller than 2.
  blanker_pulsewidth=2;
  while(buf2[blanker_pulsewidth]>0.033*buf2[0])blanker_pulsewidth++;  
// ********************************************************
// We now know refpul_size, the total length of the pulse response
// for the current hardware and calibration.
// Allocate memory for the clever (linear) noise blanker.
// ********************************************************
// ********************************************************
// ********************************************************
// ********************************************************
  mem(1,&blanker_refpulse,me01*sizeof(float),0);
  mem(2,&blanker_phasefunc,2*refpul_size*sizeof(float),0);
  mem(3,&blanker_pulindex,MAX_REFPULSES*sizeof(int),0);
  mem(4,&blanker_input,twice_rxchan*refpul_size*sizeof(float),0);
  mem(6,&blanker_flag,((ui.rx_rf_channels*timf2_mask/4+16)*sizeof(char)),0);
  i=memalloc((int*)(&blanker_handle),"blanker");
  if(i == 0 || lir_status==LIR_MEMERR)
    {
    lirerr(1061);
    return;
    }
// **************************
// Use fft1_desired to get time domain in fft1_size points.
// Reduce to the number of points we actually want.
// Produce MAX_REFPULSES standard pulses, all shifted differently in time.
// The time shifted pulses are obtained by change of the phase slope.
// To shift by one sample point in time, the phase slope has to change
// by 2*pi/refpul_size.
// Normalise the reference pulses so the amplitude = 1 and the phase = 0
// for the max point
// **************************
  for(i=0; i<fft1_size; i++)
    {
    fftw_tmp[2*i  ]=fft1_desired[i];
    fftw_tmp[2*i+1]=0;
    t1+=t2;
    }
  fftback(fft1_size, fft1_n, fftw_tmp, tmp_tab, tmp_permute, FALSE);
  init_fft(0,refpul_n, refpul_size, tmp_tab, tmp_permute);
  for(i=0; i<refpul_size; i++)fftw_tmp[2*refpul_size-1-i]=fftw_tmp[2*fft1_size-1-i];
  t1=8*PI_L/refpul_size;
  t2=t1;
  i=0;
  while(t2 < PI_L/2)
    {
    t3=sin(t2);
    fftw_tmp[refpul_size-2*i  ]*=t3;
    fftw_tmp[refpul_size-2*i+1]*=t3;
    fftw_tmp[refpul_size+2*i  ]*=t3;
    fftw_tmp[refpul_size+2*i+1]*=t3;
    i++;
    t2+=t1;
    } 
  fftforward(refpul_size, refpul_n, fftw_tmp, tmp_tab, tmp_permute,FALSE);
  for(i=0; i<me01; i++)blanker_refpulse[i]=0;
  k=ui.rx_rf_channels*me01;
  liminfo_amplitude_factor=1;
// Now we have the desired spectrum in refpul_size points.
// Since fft1_desired is a real function the imaginary part
// is rounding errors only so it is not used.
  phrot = PI_L/2;
  t6=PI_L/MAX_REFPULSES;
  for(k=0; k<MAX_REFPULSES; k++)
    {
    t5=phrot;
    for(i=0; i<refpul_size; i++)
      {
      buf3[2*i  ]=fftw_tmp[2*i]*cos(t5);
      buf3[2*i+1]=fftw_tmp[2*i]*sin(t5);
      t5-=phrot/(refpul_size/2);
      }
    fftback(refpul_size, refpul_n, buf3, tmp_tab, tmp_permute, FALSE);
// move the pulse from location 0 to the midpoint refpul_size/2
    r1=buf3[0];
    r2=buf3[1];
    for(i=0; i<refpul_size; i++)
      {
      t1=buf3[i];
      buf3[i]=buf3[i+refpul_size];
      buf3[i+refpul_size]=t1;
      }  
    t1=r1*r1+r2*r2;
    r1/=t1;
    r2/=t1;
    m=2*k*refpul_size;
    for(i=0; i<refpul_size; i++)
      {
      t1=buf3[2*i  ];
      t2=buf3[2*i+1];
      blanker_refpulse[m+2*i  ]=t1*r1+t2*r2;
      blanker_refpulse[m+2*i+1]=t2*r1-t1*r2;
      }
    phrot-=t6;
    }
// ********************************
// In case the passband is not symmetric the phase of a pulse
// will vary linearly with time.
// Find out by how much.
// Produce a phase function which we can use to transform
// pulses for the phase to become constant. 
// ********************************
  blanker_phaserot=0;
  t2=0;
  for(i=0; i<fft1_size; i++)
    {
    blanker_phaserot+=i*pow(fft1_desired[i],2.);
    t2+=pow(fft1_desired[i],2.);
    }
  blanker_phaserot*=-2*PI_L/(t2*fft1_size);
  for(i=0; i<refpul_size/2; i++)
    {
    t1=i*blanker_phaserot;
    blanker_phasefunc[refpul_size+2*i]=cos(t1);  
    blanker_phasefunc[refpul_size+2*i+1]=sin(t1);  
    blanker_phasefunc[refpul_size-2*i]=cos(t1);  
    blanker_phasefunc[refpul_size-2*i+1]=-sin(t1);  
    }
  t1=0.5*i*refpul_size*blanker_phaserot;
  blanker_phasefunc[0]=cos(t1);  
  blanker_phasefunc[1]=sin(t1);  
// ********************************
// The blanker routine will use the known impulse response to
// subtract a standard pulse that is taken from the
// selection of standard pulses in blanker_refpulse.
// This way oscillations on both sides of a pulse will be removed
// without much loss of data.
// To decide which standard pulse to use the blanker
// routine first transforms the phase in the region of
// the peak by use of blanker_phasefunc
// It then uses the 3 strongest points to evaluate the peak
// position with decimals.
// Do the same here using the standard pulses and make a table
// that can be used by the blanker routine to find the optimum
// standard pulse.
// ********************************
  for(i=0; i<MAX_REFPULSES; i++)blanker_pulindex[i]=-1;
  for(k=0; k<MAX_REFPULSES; k++)
    {
    m=2*k*refpul_size;
    j=0;
    for(i=refpul_size/2-1; i<=refpul_size/2+1; i++)
      {
      t1=blanker_refpulse[m+2*i  ];
      t2=blanker_refpulse[m+2*i+1];
      t3=blanker_phasefunc[2*i  ];
      t4=blanker_phasefunc[2*i+1];
      buf3[2*j  ]=t1*t3+t2*t4;
      j++;
      }
// Fit a parabola to the max point and it's nearest neighbours.
// As input we have 3 data points y(x)
// Find parameters a,b and c to place these points on the curve:
//  y(x)=a+b*(x-c)**2=a+b*x**2-2*x*b*c+b*c**2
// The equations to solve are:
// y(-1)=a + b + 2*b*c + b*c**2
// y( 0)=a     +         b*c**2
// y( 1)=a + b - 2*b*c + b*c**2
//  b has to be negative (for a positive peak)
//  abs(c) has to be below 0.5 since y(0)is the largest value.
//  t4=y(-1)-y(1)=4*b*c
//  t3=2*(y(-1)+y(1)-2*y(0))=4*b
//  t4/t3=c=peak offset
    t4=buf3[0]-buf3[4];
    t3=2*(buf3[0]+buf3[4]-2*buf3[2]);
    t4/=t3;
    if(t4<0)
      {
      t4=-sqrt(0.5)*sqrt(-t4);    
      }
    else
      {
      t4=sqrt(0.5)*sqrt(t4);    
      }
    j=MAX_REFPULSES*(t4+0.5)+0.5;
    if(j<0)j=0;
    if(j>=MAX_REFPULSES)j=MAX_REFPULSES-1;
    blanker_pulindex[j]=k;
    }
  i=0;
mkindex:;  
  while(blanker_pulindex[i] != -1 && i<MAX_REFPULSES)i++;
  if(i==0)
    {
    while(blanker_pulindex[i] == -1)
      {
      blanker_pulindex[i]=0;
      i++;
      }
    goto mkindex;      
    }
  if(i<MAX_REFPULSES)
    {
    j=i;
    while(blanker_pulindex[j] == -1 && j<MAX_REFPULSES)j++;
    if(j>=MAX_REFPULSES)
      {
      while(i<MAX_REFPULSES)
        {
        blanker_pulindex[i] = MAX_REFPULSES-1;
        i++;
        }
      }
    else
      {
      j--;
      while(i-j<2)
        {
        blanker_pulindex[i] = blanker_pulindex[i-1];  
        blanker_pulindex[j] = blanker_pulindex[j+1];  
        i++;
        j--;
        }
      i--;  
      goto mkindex;      
      }
    }      
  blnfit_range=bln[largest_blnfit].size/2+blanker_pulsewidth;
  blnclear_range=blnfit_range/4;
  if(blnclear_range < 3*blanker_pulsewidth)blnclear_range=3*blanker_pulsewidth;
  DEB"blanker_pulsewidth %d range %d\n", blanker_pulsewidth, blnclear_range);
  }
else
  {
  i=memalloc((int*)(&blanker_handle),"blanker");
  if(i == 0 || lir_status==LIR_MEMERR)
    {
    lirerr(1061);
    return;
    }
  blnclear_range=32;
  blnfit_range=48;
  largest_blnfit=0;
  blanker_pulsewidth=0;
  refpul_n=0;
  refpul_size=0;
  }
t1=screen_height/(2*(1+4*ui.rx_rf_channels));
for(i=0; i<16; i++)
  {
  timf2_y0[i]=screen_height-(i+1)*t1;
  timf2_ymax[i]=timf2_y0[i]+t1;
  timf2_ymin[i]=timf2_y0[i]-t1;
  if(timf2_ymax[i]>=screen_height)timf2_ymax[i]=screen_height-1;
  }
k=me04/sizeof(short int);
for(i=0; i<k; i++)timf2pix[i]=timf2_y0[0];
timf2_show_pointer=-1;
timf2_display_counter=0;
memcheck(22,blankermem,(int*)&blanker_handle);
}


void free_buffers(void)
{
if(!kill_all_flag)
  {
  if(fft1_handle != NULL)memcheck(99,fft1mem,(int*)&fft1_handle);
  if(baseband_handle != NULL)memcheck(99,basebmem,(int*)&baseband_handle);
  if(afc_handle != NULL)memcheck(99,afcmem,(int*)&afc_handle);
  if(fft3_handle != NULL)memcheck(99,fft3mem,(int*)&fft3_handle);
  if(hires_handle != NULL)memcheck(99,hiresmem,(int*)&hires_handle);
  }
if(dmp != NULL)fflush(dmp);
if(diskwrite_flag == 1)
  {
  fclose(save_wr_file);
  save_wr_file=NULL;
  lir_sync();
  diskwrite_flag = 0;
  }
fft1_handle=chk_free(fft1_handle);
wg_waterf=chk_free(wg_waterf);
fft3_handle=chk_free(fft3_handle);
hires_handle=chk_free(hires_handle);
blanker_handle=chk_free(blanker_handle);
afc_handle=chk_free(afc_handle);
baseband_handle=chk_free(baseband_handle);
dx=chk_free(dx);
}

