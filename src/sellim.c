    

#include "globdef.h"
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "screendef.h"
#include "seldef.h"


#define YU 0.0000001

#define RELEASE_FACTOR 1.15
#define MAX_LIMINFO_REG 64
#define FFT1_STEEPNESS_FACTOR 2.

void selfreq_liminfo(void)
{
int i, k, ia, ib;
float t1, t2;
float *old_liminfo;
old_liminfo=&liminfo[fft1_size];
ia=mix1_selfreq[0]*fftx_points_per_hz;
if(ia < 0)goto selfrx;
k=baseband_bw_fftxpts*.7;
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
  ia/=fft2_to_fft1_ratio;
  k/=fft2_to_fft1_ratio;
  if(k<3)k=3;
  }
ib=ia+k;
ia-=k;
if(ia<0)ia=0;
if(ib>=fft1_size)ib=fft1_size-1;
t1=0;
t2=2;
for(i=ia;i<=ib;i++)
  {
  if(liminfo[i]<0)t1=1;
  if(liminfo[i]>0)
    {
    if(t2>liminfo[i])t2=liminfo[i];
    }
  }
// liminfo[i]  < 0  => bin to timf2_strong with amplitude = 1;
// liminfo[i] == 0  => bin to timf2_weak with amplitude = 1;
// liminfo[i]  > 0  => bin to timf2_strong with amplitude = liminfo[i];
if(t2 > 1)
  {
// do nothing if liminfo is zero over the entire frequency range.
  if(t1 == 0)goto selfrx;
  t2=1;
  }
t1=1/t2;
t1*=sqrt((float)(fft2_size/fft1_size));
if(t1 < 0x7fff/genparm[SELLIM_MAXLEVEL])t2=0;
// If the signal is really strong, allow the limiter to work as usual
// with the same gain over the whole baseband.
// Otherwise clear liminfo for the selected passband.
for(i=ia;i<=ib;i++)
  {
  liminfo[i]=t2;
  }
selfrx:;
k=0;
for(i=fft1_first_inband; i<=fft1_last_inband; i++)
  {
  if(liminfo[i]!=0)k++;
  }
// The blanker evaluates pulse amplitudes based on a reduced bandwidth
// because some frequency points are among strong signals while
// the reference pulse that we subtract is stored assuming the
// full bandwidth is present.
// Make liminfo_amplitude_factor suitable to compensate for the
// loss of amplitude due to the missing parts of the spectrum.
// At half the bandwidth the pulse power is reduced by a factor
// of two. The pulse is also lengthened by a factor of two so
// the amplitude is reduced by a factor of two, not 1.414 as one
// would think from the power ratio.
i=fft1_last_inband-fft1_first_inband+1;
if(i==0)i=1;
liminfo_amplitude_factor=(float)(i)/(i-k);  
// If 50% of the frequencies are strong signals, skip the smart blanker.
if(liminfo_amplitude_factor > 2)liminfo_amplitude_factor=0;
for(i=0; i<fft1_size; i++) old_liminfo[i]=liminfo[i];    
}


void fft2_update_liminfo(void)
{
unsigned wait_n;
int i, j, k, ia, ib, ja, jb;
int nn,tflag;
float t1,t2;
float reg_noise[MAX_LIMINFO_REG];
int reg_first_point[MAX_LIMINFO_REG];
int reg_length[MAX_LIMINFO_REG];
int reg_no;
nn=fft2_size/fft1_size;
wait_n=1+(1+(fft2_blocktime*wg.waterfall_avgnum))/
                                             (wg.fft_avg1num*fft1_blocktime);
if(wait_n > 255)wait_n=255;
// Locate a weak signal region.
ia=fft1_first_point;
reg_no=0;
lc2:;
while(liminfo[ia] > 0 && ia < fft1_last_point)ia++;
if(ia != fft1_last_point)
  {
  ib=ia;
  while(liminfo[ib] <= 0 && ib < fft1_last_point)ib++;
// We have a weak signal region from ia to ib-1. 
// If the weak signal region is very short, do nothing.
  if(ib-ia < 6)
    {
    ib--;
    }
  else     
    {
// Points that are already classed as strong signals will produce
// interference in neighbouring points because of the dumb blanker.
// Do not include nearest neighbours so this artificial noise
// will not widen the strong signal region. 
    ia++;
    ib--;
// Put the noise level from fft2 in fftf_tmp  
// while getting the lowest value in t2;
    ja=nn*ia;
    t2=BIG;
    if(sw_onechan)
      {
      for(i=ia; i<ib; i++) 
        {
        jb=ja+nn;
        t1=0;
        for(j=ja; j<jb; j++)
          {
          t1+=fft2_powersum_float[j];
          }
        t1*=wg_waterf_yfac[i];  
        fftf_tmp[i]=t1; 
        if(t1 < t2)
          {
          if(i>=fft1_first_inband && i <=fft1_last_inband)
            {
            t2=t1;
            }
          }  
        ja+=nn;
        }
      }
    else
      {
      for(i=ia; i<ib; i++) 
        {
        jb=ja+nn;
        t1=0;
        for(j=ja; j<jb; j++)
          {
          t1+=fft2_xysum[j].x2+fft2_xysum[j].y2;
          }
        t1*=wg_waterf_yfac[i];  
        fftf_tmp[i]=t1; 
        if(t1 < t2)
          {
          if(i>=fft1_first_inband && i <=fft1_last_inband)
            {
            t2=t1;
            }
          }  
        ja+=nn;
        }
      }
    fftf_tmp[ia-1]=fftf_tmp[ia];  
    fftf_tmp[ib]=fftf_tmp[ib-1];
// Get the noise floor as the average of those points that are
// below a threshold, 2*(1+2./wg.waterfall_avgnum) times larger than the 
// minimum value.      
    t2*=2*(1+2./wg.waterfall_avgnum);
    t1=0;
    j=0;
    ja=ia;
    if(ja < fft1_first_inband)ja=fft1_first_inband;
    jb=ib;
    if(jb > fft1_last_inband)jb=fft1_last_inband+1;
get_avgn:;
    for(i=ja; i<jb; i++) 
      {
      if(fftf_tmp[i] < t2)
        {
        j++;
        t1+=fftf_tmp[i];
        }
      } 
    if(j == 0)goto skipit;      
    if(j<(jb-ja)/4)
      {
      t2*=3;
      goto get_avgn;
      }
    t1/=j;
    reg_noise[reg_no]=t1;
    reg_first_point[reg_no]=ia-1;
    ja=-1;
    t2=t1*hg.blanker_ston_fft2;
    for(i=ia; i<ib; i++) 
      {
      if(fftf_tmp[i] >t2)
        {
        if(ja<0)ja=i;
        jb=i;
        liminfo[i]=-1;
        liminfo_wait[i]=wait_n;
        }
      }  
    if(ja < 0)
      {
      reg_length[reg_no]=ib-ia+1;
      reg_no++;
      }
    else
      {
      reg_length[reg_no]=ja-ia+1;
      reg_no++;
      if(ib-jb > 4)
        {
        reg_noise[reg_no]=t1;
        reg_first_point[reg_no]=jb+1;
        reg_length[reg_no]=ib-jb;
        reg_no++;
        }
      }  
// If we have too many regions, eliminate some of them.
    if(reg_no >= MAX_LIMINFO_REG-2)
      {
// Get the average noise floor.
      k=0;
      t1=0;
      for(i=0; i<reg_no; i++)
        {
        k+=reg_length[i];      
        t1+=reg_noise[i]*reg_length[i];
        }
      t1/=k;
// Transfer regions for which the noise floor is above threshold
// to strong signals.        
      t1*=hg.blanker_ston_fft2;
      for(k=0; k<reg_no; k++)
        {
        if(reg_noise[k]>t1)
          {
          for(i=0; i<reg_length[i]; i++)
            {
            liminfo[i+reg_first_point[k]]=-1;
            liminfo_wait[i+reg_first_point[k]]=wait_n;
            }   
          for(j=k+1; j<reg_no; j++)
            {
            reg_noise[j-1]=reg_noise[j];
            reg_first_point[j-1]=reg_first_point[j];
            reg_length[j-1]=reg_length[j];
            }
          reg_no--;
          }
        }
// If we still have many regions, just forget those which are
// below the average noise floor (not very smart, but easy)        
      if(reg_no >= 3*MAX_LIMINFO_REG/4)
        {
        t1/=hg.blanker_ston_fft2;
        for(k=0; k<reg_no; k++)
          {
          if(reg_noise[k]<t1)
            {
            for(j=k+1; j<reg_no; j++)
              {
              reg_noise[j-1]=reg_noise[j];
              reg_first_point[j-1]=reg_first_point[j];
              reg_length[j-1]=reg_length[j];
              }
            }  
          reg_no--;
          k--;
          }
// If we still have to many regions, just skip 
// some of them (not clever at all)          
        if(reg_no >= 3*MAX_LIMINFO_REG/4)reg_no = 3*MAX_LIMINFO_REG/4;       
        }
      }
    }
skipit:;    
  ia=ib+1;
  goto lc2;
  }
// Get the average noise floor.
k=0;
t1=0;
if(reg_no == 0)goto fft2updx;
for(i=0; i<reg_no; i++)
  {
  k+=reg_length[i];      
  t1+=reg_noise[i]*reg_length[i];
  }
t1/=k;
// Transfer regions for which the noise floor is above threshold
// to strong signals.        
t1*=hg.blanker_ston_fft2;
tflag=0;
for(k=0; k<reg_no; k++)
  {
  if(reg_noise[k]>t1)
    {
    tflag=1;
    for(i=0; i<reg_length[k]; i++)
      {
      liminfo[i+reg_first_point[k]]=-1; 
      liminfo_wait[i+reg_first_point[k]]=wait_n;
      }   
    reg_noise[k]=-1;
    }
  }

if(tflag==1)
  {
// Get the average noise floor again.
  k=0;
  t1=0;
  for(i=0; i<reg_no; i++)
    {
    if(reg_noise[i]<0)
      {
      for(j=i+1; j<reg_no;j++)
        {
        reg_noise[j-1]=reg_noise[j];
        reg_first_point[j-1]=reg_first_point[j];
        reg_length[j-1]=reg_length[j];
        }
      i--;
      reg_no--;
      }
    else
      {
      k+=reg_length[i];      
      t1+=reg_noise[i]*reg_length[i];
      }
    }
  t1/=k;
  t1*=hg.blanker_ston_fft2;
  }
// Remove points that are above the threshold 
for(k=0; k<reg_no; k++)
  {
  for(i=0; i<reg_length[k]; i++)
    {
    if(fftf_tmp[i+reg_first_point[k]] >t1)
      {    
      liminfo[i+reg_first_point[k]]=-1;
      liminfo_wait[i+reg_first_point[k]]=wait_n;
      }
    }   
  }
fft2updx:;
selfreq_liminfo();
}

void fft1_update_liminfo(void)
{
unsigned int wait_n;
int i,j,k,ia,ib,ja,jb,ix,iy;
float limit,maxval;
float t1,t2,t3,noise_floor;
float *old_liminfo;
old_liminfo=&liminfo[fft1_size];
fft1_sumsq_tot+=wg.fft_avg1num;
if(fft1_sumsq_tot > wg.spek_avgnum)fft1_sumsq_tot = wg.spek_avgnum;
// This routine is called if second fft is enabled.
// liminfo is an array used to split the fft1 signal into two
// signals: timf2_weak and timf2_strong.
// timf2_strong contains all strong narrow band signals while
// timf2_weak contains weak narrow band signals, noise, pulses and
// all other wide band interference.
// The timf2 signals are calculated by a back transformation from
// fft1.
// Each frequency is processed as follows: 
// liminfo[i]  < 0  => bin to timf2_strong with amplitude = 1;
// liminfo[i] == 0  => bin to timf2_weak with amplitude = 1;
// liminfo[i]  > 0  => bin to timf2_strong with amplitude = liminfo[i];
// We try to set liminfo equal over the full bandwidth of each strong
// signal.
// The gain reduction caused by positive liminfo values acts as an AGC
// that sets gain independently for each signal.
// We want the strong signal to fit in 16 bit, so it should be limited
// to below +/- 32656.
// To have some margin, set the limit at limit.
// In the worst case all energy is in one channel out of four (two arrays
// with complex data). 
// Then fft1_sumsq becomes limit*limit* wg.fft_avg1num
// We get the attenuation required as the square root 
// of  limit*limit*wg.fft_avg1num/fft1_sumsq[i] 
//
// Wait 1 second before making weak once a bin is classified as strong.
k=1+1/(wg.fft_avg1num*fft1_blocktime);
if(k < 255)wait_n=k; else wait_n=255;
//
//
// fft1_sumsq holds wg.fft_avg1num summed power spectra at the
// current location when we arrive here called by get fft1.
// If fft2_size is bigger, narrowband signals grow more!!
t1=genparm[SELLIM_MAXLEVEL];
limit=t1*t1*wg.fft_avg1num*ui.rx_rf_channels;
limit*=fft1_size;
limit/=fft2_size;
ia=fft1_sumsq_pa+fft1_first_point;
ix=ia;
iy=fft1_sumsq_pa+fft1_last_point-1;
limloop:;
if(fft1_sumsq[ia] > limit)
  {
  maxval=fft1_sumsq[ia];
  ib=ia+1;
  while(fft1_sumsq[ib] > limit && ib<=iy)
    {
    if(fft1_sumsq[ib] > maxval) maxval=fft1_sumsq[ib];
    ib++;
    }
  while(ia>ix && fft1_sumsq[ia-1]/fft1_sumsq[ia] < 0.3)ia--;
  while(ib<iy && fft1_sumsq[ib+1]/fft1_sumsq[ib] < 0.3)ib++;
  ja=ia-fft1_sumsq_pa;
  jb=ib-fft1_sumsq_pa;
  for(j=ja; j<=jb; j++)liminfo[j]=sqrt(limit/maxval);
  ia=ib;
  }
else
  {
  liminfo[ia-fft1_sumsq_pa]=0;
  }
ia++;
if( ia <iy )goto limloop;
if(fft1_sumsq_tot < wg.spek_avgnum)goto fft1updx;
// In order to get a good noise blanker function, it is nessecary
// to remove all sinewave like components all the way down to near the
// noise floor.
// This is trivial on VHF bands where there is a well defined noise
// floor of essentially white noise.
// Use the full dynamic range spectrum, the wide graph data stored
// in fft1_slowsum to find a noise floor for the current situation.
// Do that by dividing the spectrum into liminfo_groups segments and 
// find the minimum within each of them.
// Store these minimum values in liminfo_group_min.
ja=fft1_first_inband/liminfo_group_points;
jb=ja+1;
ib=jb*liminfo_group_points;
t1=BIG;
t2=BIG;
t3=BIG;
for(i=0;i<fft1_first_inband;i++)fftw_tmp[i]=wg_waterf_yfac[i]*fft1_slowsum[i];
for(i=fft1_first_inband; i<ib; i++)
  {
  fftw_tmp[i]=wg_waterf_yfac[i]*fft1_slowsum[i];
  if(fftw_tmp[i]<=t1)
    {
    t3=t2;
    t2=t1;
    t1=fftw_tmp[i];
    }
  }  
liminfo_group_min[ja]=0.3333333*(t1+t2+t3);
get_grmin:;
ia=ib;
ib+=liminfo_group_points;
if(ib > fft1_last_inband)ib=fft1_last_inband+1;
t1=BIG;
t2=BIG;
t3=BIG;
for(i=ia; i<ib; i++)
  {
  fftw_tmp[i]=wg_waterf_yfac[i]*fft1_slowsum[i];
  if(fftw_tmp[i]<=t3)
    {
    if(fftw_tmp[i]<=t1)
      {
      t3=t2;
      t2=t1;
      t1=fftw_tmp[i];
      }
    else
      {
      if(fftw_tmp[i]<=t2)
        {
        t3=t2;
        t2=fftw_tmp[i];
        }
      else
        {
        t3=fftw_tmp[i];
        }
      }
    }      
  }
liminfo_group_min[jb]=0.3333333*(t1+t2+t3);
jb++;
if(ib < fft1_last_inband)goto get_grmin;
for(i=ib; i<fft1_last_point; i++)fftw_tmp[i]=
                                         wg_waterf_yfac[i]*fft1_slowsum[i];
// Get the average of the min values.
t1=0;
for(j=ja; j<jb; j++)t1+=liminfo_group_min[j];
t1/=jb-ja;
// Get the average of the points below twice the average of min values
// if avgnum is large.  
// If avgnum is small include larger values in the average.
k=0;
noise_floor=0;
t1*=2*(1+2./wg.spek_avgnum);
for(j=ja; j<jb; j++)
  {
  if(liminfo_group_min[j]<t1)
    {
    noise_floor+=liminfo_group_min[j];
    k++;
    }
  }
if(noise_floor < 0.0001)noise_floor=0.0001;
if(k!=0)
  {
// noise_floor is the noise floor if wg_spek_avgnum is very large.
// Picking the minimum as the average of the 3 smallest points
// underestimates the minimum noise floor by about 3 times (in power)
// if wg_spek_avgnum=1
  noise_floor*=(1+2./wg.spek_avgnum)/k;  
// We may want to know where the fft1 noise floor is at some later time.
// Calculate it as a RMS voltage expressed in bits.
// The power at each frequency comes from the sum of 2*ui.rx_rf_channels
// amplitudes. Get the amplitude for each one of them.
  fft1_noise_floor=noise_floor/(wg_waterf_yfac[0]*wg.spek_avgnum*
                                                       2*ui.rx_rf_channels);
  noise_floor*=hg.blanker_ston_fft1;
// Anything that is more than genparm[SELLIM_STON] times above 
// the noise floor is a signal.
// Set liminfo negative for the corresponding data points so they will
// be processed together with the strong signals.
  ia=0;
  while(ia < fft1_first_point || ia < 2)
    {
    if(liminfo[ia]==0)liminfo[ia]=-1;
    ia++;
    }
  while(fftw_tmp[ia]>noise_floor && ia<fft1_size)
    {
    if(liminfo[ia]==0)liminfo[ia]=-1;
    ia++;
    }
  while(3*fftw_tmp[ia+1]<fftw_tmp[ia] && ia<fft1_size)
    {
    ia++;
    if(liminfo[ia]==0)liminfo[ia]=-1;
    }
get_above:;
  while(fftw_tmp[ia]<=noise_floor && ia<fft1_last_point)ia++;
  if(ia<fft1_last_point)
    {
    ib=ia;
    if(liminfo[ia]==0)liminfo[ia]=-1;
    while( (FFT1_STEEPNESS_FACTOR*fftw_tmp[ib-1] < fftw_tmp[ib]|| 
               FFT1_STEEPNESS_FACTOR*FFT1_STEEPNESS_FACTOR*fftw_tmp[ib-2]
                                       <fftw_tmp[ib]) && ib>fft1_first_point)
      {
      ib--;
      if(liminfo[ib]==0)liminfo[ib]=-1;
      }
    while(fftw_tmp[ia+1]>noise_floor && ia<fft1_last_point)
      {
      ia++;
      if(liminfo[ia]==0)liminfo[ia]=-1;
      }
    if(ia!=fft1_last_point)
      {
      while( (FFT1_STEEPNESS_FACTOR*fftw_tmp[ia+1] < fftw_tmp[ia] || 
               FFT1_STEEPNESS_FACTOR*FFT1_STEEPNESS_FACTOR*fftw_tmp[ia+2] 
                                       < fftw_tmp[ia]) && ia<fft1_last_point)
        {
        ia++;
        if(liminfo[ia]==0)liminfo[ia]=-1;
        }
      ia++; 
      }
    if(ia<fft1_last_point)goto get_above;
    }
  }
while(ia < fft1_size)
  {
  if(liminfo[ia]==0)liminfo[ia]=-1;
  ia++;
  }
// In case we recently attenuated a frequency, do not allow
// the gain to grow too quickly. When two back transforms are
// joined there will be a transcient if the gain was different
// for a strong signal.
// We could solve this by making a gradual transfer between back
// transforms as is done with sin squared windows but it is a good
// idea anyway to keep the gain down for a little while at the
// frequency of a very strong signal. It may have been just a key up.
for(i=0; i<fft1_size; i++)
  {
  if(liminfo[i] != 0)
    {
    liminfo_wait[i]=wait_n;
    }
  else
    {  
    if(liminfo_wait[i] > 0)
      {
      liminfo_wait[i]--;
      }
    if(liminfo_wait[i] >0)liminfo[i]=-1;
    }
  if(old_liminfo[i]>0)
    {
    t1=old_liminfo[i]*RELEASE_FACTOR;
    if(t1 < 1)
      {
      if(liminfo[i]>0 && liminfo[i] > t1)liminfo[i]=t1;
      }
    }
  }          
//for(i=fft1_size/4;i<3*fft1_size/4;i++)liminfo[i]=-1;
fft1updx:;
selfreq_liminfo();
//for(i=0;i<fft1_size;i++)liminfo[i]=0;
//for(i=0;i<8192;i++)liminfo[i]=-1;
//for(i=4900;i<4950;i++)liminfo[i]=0;
}
