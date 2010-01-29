
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
#include "keyboard_def.h"

float tx_resamp_maxamp;
float tx_resamp_old_maxamp;
void tx_ssb_buftim(char cc);  

int tx_ssb_step2(float *totpwr)
{
int i, k;
float t1, t2;
// ************* Step 2. *************
// Compute the power for each bin within the passband.
// Apply muting in the frequency domain by setting bins with too small
// signal power to zero.
// Collect the sum of the powers from surviving bins and mute the entire
// block in case the total power is below threshold.
// Here we use the agc factor as determined from the the
// previous block and attenuated with the decay factor.
// Note that the agc factor is limited to 20 dB at this point.
// A big pulse will not kill the signal for a long time!!
t1=0;
k=0;
for(i=tx_filter_ia1; i<mic_fftsize; i++)
  {
  t2=micfft[micfft_px+2*i  ]*micfft[micfft_px+2*i  ]+
     micfft[micfft_px+2*i+1]*micfft[micfft_px+2*i+1];
  if(t2 > micfft_bin_minpower)
    {
    k++;
    t1+=t2;
    }
  else
    {
    micfft[micfft_px+2*i  ]=0;
    micfft[micfft_px+2*i+1]=0;
    }
  }
for(i=0; i<=tx_filter_ib1; i++)
  {
  t2=micfft[micfft_px+2*i  ]*micfft[micfft_px+2*i  ]+
     micfft[micfft_px+2*i+1]*micfft[micfft_px+2*i+1];
  if(t2 > micfft_bin_minpower)
    {
    k++;
    t1+=t2;
    }
  else
    {
    micfft[micfft_px+2*i  ]=0;
    micfft[micfft_px+2*i+1]=0;
    }
  }
if(k>0 && t1 < micfft_minpower)
  {
  for(i=tx_filter_ia1; i<mic_fftsize; i++)
    {
    micfft[micfft_px+2*i  ]=0;
    micfft[micfft_px+2*i+1]=0;
    }
  for(i=0; i<=tx_filter_ib1; i++)
    {
    micfft[micfft_px+2*i  ]=0;
    micfft[micfft_px+2*i+1]=0;
    }
  k=0;
  }
totpwr[0]=t1;    
return k;
}

void tx_ssb_step3(float *totamp)
{
int i;
float t1, t2, r1;
// ************* Step 3. *************
// Find out what the average amplitude would become if the old AGC
// factor would be applied. Change the agc factor in case the
// average amplitude is above the limit.
// Note that this step is only executed if k!=0.
// When k==0, the entire block is muted. The operation
// would be meaningless on a zero transform.
t1=sqrt(totamp[0]);
r1=pow(10.0,0.05*txssb.mic_gain);
t2=r1*t1*tx_agc_factor*(tx_highest_bin-tx_lowest_bin+1);
if(t2 > TX_AGC_THRESHOLD*(tx_highest_bin-tx_lowest_bin+1))
  {
  tx_agc_factor=TX_AGC_THRESHOLD/(r1*t1);
  }
t2=tx_agc_factor*(tx_highest_bin-tx_lowest_bin+1)*r1;
t2*=pow(10.0,0.05*(txssb.mic_out_gain-60));
// Apply the AGC within the current fft block.
for(i=tx_filter_ia1; i<mic_fftsize; i++)
  {
  micfft[micfft_px+2*i  ]*=t2;
  micfft[micfft_px+2*i+1]*=t2;
  }
for(i=0; i<=tx_filter_ib1; i++)
  {
  micfft[micfft_px+2*i  ]*=t2;
  micfft[micfft_px+2*i+1]*=t2;
  }
totamp[0]=t1;
}

void tx_ssb_step4(float ampinv,float *t1, float *prat)
{
int i, k, panew;
char mute_current, mute_previous;
float r1, r2, t2;
prat[0]=0;
t1[0]=0;
if( (tx_filter_ia1&1) != 0)tx_ph1*=-1;
panew=(cliptimf_pa+mic_fftsize)&micfft_mask;
if(ampinv == 0)
  {
  mute_current=TRUE;
  }
else
  {  
  mute_current=FALSE;
  }
cliptimf_mute[cliptimf_pa/mic_fftsize]=mute_current;
mute_previous=cliptimf_mute[((cliptimf_pa-mic_fftsize+micfft_bufsiz)&
                           micfft_mask)/mic_fftsize];
// We do nothing in case both the current transform and the
// previous one are muted.
if( (mute_current&mute_previous) == FALSE)
  {  
  if( (mute_current|mute_previous) == FALSE)
    {
// None of the transforms is muted. 
// Do normal (continous) back transformation.
    r1=pow(10.0,0.05*txssb.rf1_gain)/TX_AGC_THRESHOLD;
    t2=0;
    r1*=ampinv;
    for(i=0; i<mic_fftsize; i+=2)
      {
      cliptimf[cliptimf_pa+i  ]=tx_ph1*
                      (cliptimf[cliptimf_pa+i  ]+r1*micfft[micfft_px+i  ]);
      cliptimf[cliptimf_pa+i+1]=tx_ph1*
                      (cliptimf[cliptimf_pa+i+1]+r1*micfft[micfft_px+i+1]);
      t2=cliptimf[cliptimf_pa+i  ]*cliptimf[cliptimf_pa+i  ]+
         cliptimf[cliptimf_pa+i+1]*cliptimf[cliptimf_pa+i+1];
      prat[0]+=t2;
      if(t2>t1[0])t1[0]=t2;
      if(t2 > 1)
        {
        t2=1/sqrt(t2);
        cliptimf[cliptimf_pa+i  ]*=t2;
        cliptimf[cliptimf_pa+i+1]*=t2;
        }
      }
    if( (tx_filter_ia1&1) != 0)r1*=-1;
    for(i=0; i<mic_fftsize; i++)
      {
      cliptimf[panew+i]=r1*micfft[micfft_px+mic_fftsize+i];
      }
    }
  else
    {
// This is the first or the last transform.
// Multiply the beginning and the end with a sine squared window
// to make the start and end filtered with a sine to power 4
// window in total.    
// The first and last transforms are likely to be incorrect
// because muting in the frequency domain will distort the
// signal if only the very strongest bins have survived
// the muting process. This is particularly obvious if the
// microphone signal is a keyed tone which is present only
// a short time in the last or first transform. In such a
// situation the signal would be extended over the entire
// transform in case only a single bin would survive.
// The added sine squared window will remove the abrupt
// start that the signal otherwise would have.
    if(mute_current)
      {
      k=0;
      for(i=mic_fftsize-2; i>=0; i-=2)
        {
        r1=micfft_winfac*micfft_win[k];
        cliptimf[cliptimf_pa+i  ]=tx_ph1*cliptimf[cliptimf_pa+i  ]*r1;
        cliptimf[cliptimf_pa+i+1]=tx_ph1*cliptimf[cliptimf_pa+i+1]*r1;
        k++;
        t2=cliptimf[cliptimf_pa+i  ]*cliptimf[cliptimf_pa+i  ]+
           cliptimf[cliptimf_pa+i+1]*cliptimf[cliptimf_pa+i+1];
        if(t1[0] < MAX_DYNRANGE && t2 < MAX_DYNRANGE)
          {
          cliptimf[cliptimf_pa+i  ]=0;
          cliptimf[cliptimf_pa+i+1]=0;
          }
        else
          {
          prat[0]+=t2;
          if(t2>t1[0])t1[0]=t2;
          if(t2 > 1)
            {
            t2=1/sqrt(t2);
            cliptimf[cliptimf_pa+i  ]*=t2;
            cliptimf[cliptimf_pa+i+1]*=t2;
        t2=cliptimf[cliptimf_pa+i  ]*cliptimf[cliptimf_pa+i  ]+
           cliptimf[cliptimf_pa+i+1]*cliptimf[cliptimf_pa+i+1];
            }
          }
        }
      for(i=0; i<mic_fftsize; i++)
        {
        cliptimf[panew+i]=0;
        }
      }
    else // if(mute_previous)
      {
      r1=pow(10.0,0.05*txssb.rf1_gain)/TX_AGC_THRESHOLD;
      t2=0;
      r1*=ampinv;
      for(i=0; i<mic_fftsize; i+=2)
        {
        r2=r1*micfft_winfac*micfft_win[i>>1];
        cliptimf[cliptimf_pa+i  ]=tx_ph1*r2*micfft[micfft_px+i  ];
        cliptimf[cliptimf_pa+i+1]=tx_ph1*r2*micfft[micfft_px+i+1];
        t2=cliptimf[cliptimf_pa+i  ]*cliptimf[cliptimf_pa+i  ]+
           cliptimf[cliptimf_pa+i+1]*cliptimf[cliptimf_pa+i+1];
        if(t1[0] < MAX_DYNRANGE && t2 < MAX_DYNRANGE)
          {
          cliptimf[cliptimf_pa+i  ]=0;
          cliptimf[cliptimf_pa+i+1]=0;
          }
        else
          {
          prat[0]+=t2;
          if(t2>t1[0])t1[0]=t2;
          if(t2 > 1)
            {
            t2=1/sqrt(t2);
            cliptimf[cliptimf_pa+i  ]*=t2;
            cliptimf[cliptimf_pa+i+1]*=t2;
            }
          }
        }
      if( (tx_filter_ia1&1) != 0)r1*=-1;
      for(i=0; i<mic_fftsize; i++)
        {
        cliptimf[panew+i]=r1*micfft[micfft_px+mic_fftsize+i];
        }
      }
    }
  }
cliptimf_pa=panew;
micfft_px=(micfft_px+micfft_block)&micfft_mask;
}

void tx_ssb_step5(void)
{
int i, j, pb, nx ,nb;
while( ((cliptimf_pa-cliptimf_px+micfft_bufsiz)&micfft_mask) >= 
                                                           2*mic_fftsize)
  {                                                  
  pb=(cliptimf_px+2*(mic_fftsize-1))&micfft_mask;
  nx=cliptimf_px/mic_fftsize;
  nb=((cliptimf_px+mic_fftsize)&micfft_mask)/mic_fftsize;
  if(cliptimf_mute[nx]==FALSE/* && cliptimf_mute[nb]==FALSE*/)
    {
    j=mic_fftsize-1;
    for(i=0; i<mic_sizhalf; i++)
      {
      clipfft[clipfft_pa+2*i  ]=cliptimf[cliptimf_px+2*i  ]*micfft_win[i]; 
      clipfft[clipfft_pa+2*i+1]=cliptimf[cliptimf_px+2*i+1]*micfft_win[i]; 
      clipfft[clipfft_pa+2*j  ]=cliptimf[pb  ]*micfft_win[i]; 
      clipfft[clipfft_pa+2*j+1]=cliptimf[pb+1]*micfft_win[i]; 
      j--;
      pb-=2;
      }      
    fftforward(mic_fftsize, mic_fftn, &clipfft[clipfft_pa], 
                                             micfft_table, micfft_permute,FALSE);
    if(tx_setup_flag == TRUE)
      {
      lir_sched_yield();
      if(screen_count>=MAX_SCREENCOUNT)
        {
        show_txfft(&clipfft[clipfft_pa],0,1,mic_fftsize);
        }
      }
    for(i=tx_filter_ib1+1; i<tx_filter_ia1; i++)
      {
      clipfft[clipfft_pa+2*i  ]=0;
      clipfft[clipfft_pa+2*i+1]=0;
      }
    clipfft_mute[clipfft_pa/alc_block]=FALSE;
    }
  else
    {
    clipfft_mute[clipfft_pa/alc_block]=TRUE;
    }      
  cliptimf_px=(cliptimf_px+mic_fftsize)&micfft_mask;
  clipfft_pa=(clipfft_pa+alc_block)&alc_mask;
  }
}

float tx_ssb_step6(float *prat)
{
int i, k, pb, mute_previous;
float t1, t2, pmax;
mute_previous=clipfft_mute[((clipfft_px-alc_block+alc_bufsiz)
                                                     &alc_mask)/alc_block];
pmax=0;
prat[0]=0;
if(clipfft_mute[clipfft_px/alc_block] == FALSE)
  {
  alctimf_mute[alctimf_pa/alc_fftsize]=FALSE;
  if(mic_fftsize < alc_fftsize)
    {
    i=tx_filter_ia1;
    k=tx_filter_ia2;
    while(i<mic_fftsize)
      {
      clipfft[clipfft_px+2*k  ]=clipfft[clipfft_px+2*i  ];
      clipfft[clipfft_px+2*k+1]=clipfft[clipfft_px+2*i+1];
      i++;
      k++;
      }      
    }  
  for(i=tx_filter_ib1+1; i<tx_filter_ia2; i++)
    {
    clipfft[clipfft_px+2*i]=0;
    clipfft[clipfft_px+2*i+1]=0;
    }      
  fftback(alc_fftsize, alc_fftn, &clipfft[clipfft_px], 
                                         alc_table, alc_permute, FALSE);
  if(mute_previous) 
    {
    for(i=0; i<alc_sizhalf; i++)
      {
      alctimf[alctimf_pa+2*i  ]=clipfft[clipfft_px+2*i  ];

      alctimf[alctimf_pa+2*i+1]=clipfft[clipfft_px+2*i+1];
      t2=alctimf[alctimf_pa+2*i  ]*alctimf[alctimf_pa+2*i  ]+
         alctimf[alctimf_pa+2*i+1]*alctimf[alctimf_pa+2*i+1];
      prat[0]+=t2;
      if(t2>pmax)pmax=t2;
      tx_forwardpwr*=txpwr_decay;
      if(tx_forwardpwr<t2)tx_forwardpwr=t2;
      alctimf_pwrf[(alctimf_pa>>1)+i]=tx_forwardpwr;
      }
    }
  else
    {
    for(i=0; i<alc_sizhalf; i++)
      {
      alctimf[alctimf_pa+2*i  ]+=clipfft[clipfft_px+2*i  ];
      alctimf[alctimf_pa+2*i+1]+=clipfft[clipfft_px+2*i+1];
      t2=alctimf[alctimf_pa+2*i  ]*alctimf[alctimf_pa+2*i  ]+
         alctimf[alctimf_pa+2*i+1]*alctimf[alctimf_pa+2*i+1];
      prat[0]+=t2;
      if(t2>pmax)pmax=t2;
      tx_forwardpwr*=txpwr_decay;
      if(tx_forwardpwr<t2)tx_forwardpwr=t2;
      alctimf_pwrf[(alctimf_pa>>1)+i]=tx_forwardpwr;
      }
    }
  }
else
  {
  if(mute_previous)
    {
    tx_forwardpwr=0;
    alctimf_mute[alctimf_pa/alc_fftsize]=TRUE;
    }
  else
    {
    alctimf_mute[alctimf_pa/alc_fftsize]=FALSE;
    for(i=0; i<alc_sizhalf; i++)
      {
      t2=alctimf[alctimf_pa+2*i  ]*alctimf[alctimf_pa+2*i  ]+
         alctimf[alctimf_pa+2*i+1]*alctimf[alctimf_pa+2*i+1];
      prat[0]+=t2;
      if(t2>pmax)pmax=t2;
      tx_forwardpwr*=txpwr_decay;
      if(tx_forwardpwr<t2)tx_forwardpwr=t2;
      alctimf_pwrf[(alctimf_pa>>1)+i]=tx_forwardpwr;
      }
    }
  }  
if( alctimf_mute[alctimf_pa/alc_fftsize] == FALSE)
  {
// Store an exponential power fall-off with the same time constant, 50 ms,
// in the reverse direction.
  i=alc_sizhalf-1;
  t1=0;
  while(i >= 0)
    {
    t1*=txpwr_decay;
    if(t1 < alctimf_pwrf[(alctimf_pa>>1)+i])t1=alctimf_pwrf[(alctimf_pa>>1)+i];
    alctimf_pwrd[(alctimf_pa>>1)+i]=t1;
    i--;
    }
  pb=alctimf_pa;
  t1=alctimf_pwrd[(pb>>1)];
  i=0;
  while( pb != ((int)(alctimf_fx+4+alc_bufsiz)&alc_mask) && i == 0)
    {
    t1*=txpwr_decay;
    pb=(pb+alc_mask)&alc_mask;
    if(t1 > alctimf_pwrf[(pb>>1)])
      {
      alctimf_pwrd[(pb>>1)]=t1;
      }
    else
      {
      i=1;
      }  
    }
  }  
alctimf_pa=(alctimf_pa+alc_fftsize)&alc_mask;
if(clipfft_mute[clipfft_px/alc_block] == FALSE)
  {
  for(i=0; i<alc_fftsize; i++)
    {
    alctimf[alctimf_pa+i]=clipfft[clipfft_px+alc_fftsize+i];
    }
  }
else
  {
  for(i=0; i<alc_fftsize; i++)
    {
    alctimf[alctimf_pa+i]=0;
    }
  }
clipfft_px=(clipfft_px+alc_block)&alc_mask;
return pmax;
}

float tx_ssb_step7(float *prat)
{
int i, p0;
float r1, t1, t2, t3, pmax;
if(alctimf_mute[alctimf_pb/alc_fftsize]==FALSE)
  {
  p0=(alctimf_pb-2+alc_bufsiz)&alc_mask;
  t1=alctimf[p0-2]*alctimf[p0-2]+
     alctimf[p0-1]*alctimf[p0-1];
  t2=alctimf[p0  ]*alctimf[p0  ]+
     alctimf[p0+1]*alctimf[p0+1];
  pmax=0;
  prat[0]=0;
  for(i=0; i<alc_sizhalf; i++)
    {
    r1=alctimf_pwrd[(alctimf_pb>>1)+i];    
    if(r1 > 1)
      {
      r1=1/sqrt(r1);
      alctimf[alctimf_pb+2*i  ]*=r1;
      alctimf[alctimf_pb+2*i+1]*=r1;
      }
    t3=alctimf[alctimf_pb+2*i  ]*alctimf[alctimf_pb+2*i  ]+
       alctimf[alctimf_pb+2*i+1]*alctimf[alctimf_pb+2*i+1];
// We do not want more than MAX_DYNRANGE dynamic range.
// any signal below is just rounding errors in muted periods.
    if(t1 < MAX_DYNRANGE && t2 < MAX_DYNRANGE && t3 < MAX_DYNRANGE)
      {  
      alctimf[p0  ]=0;
      alctimf[p0+1]=0;
      }
    p0=(p0+2)&alc_mask;
    if(t3 > pmax)pmax=t3;
    prat[0]+=t3;
    }
  }
else
  {
  pmax=0;
  prat[0]=0;
  for(i=0; i<alc_fftsize; i++)
    {
    alctimf[alctimf_pb+i]=0;
    }
  }
alctimf_pb=(alctimf_pb+alc_fftsize)&alc_mask;
return pmax;
}

void tx_ssb_step8(void)
{
if(tx_output_flag == 0)
  {
  if( ((alctimf_pb-(int)(alctimf_fx)+alc_bufsiz)&alc_mask) > 
                    tx_ssb_resamp_block_factor*SSB_DELAY_EXTRA*alc_fftsize)
    {
    tx_output_flag=1;
    tx_resamp_pa=0;
    tx_resamp_px=0;
    }
  }  
if(tx_output_flag == 1)
  {
  resample_tx_output();
  }
}

void use_tx_resamp(float ampfac)
{
int i, j, k, m;
float t1, t2, t3, r1, r2, pilot_ampl;
double dt1;
// The latest half block of data resides in tx_resamp[alc_fftsize]
// to tx_resamp[2*alc_fftsize-1]
// Copy the previous block from resamp_tmp and multiply with
// the window function.
dt1=1/sqrt(tx_daout_sin*tx_daout_sin+tx_daout_cos*tx_daout_cos);
tx_daout_sin*=dt1;
tx_daout_cos*=dt1;
k=alc_sizhalf;
m=k-1;
t1=0;
for(i=0; i<alc_sizhalf; i++)
  {
  tx_resamp[2*i  ]=resamp_tmp[2*i  ]*alc_win[i]; 
  tx_resamp[2*i+1]=resamp_tmp[2*i+1]*alc_win[i]; 
  resamp_tmp[2*i  ]=ampfac*tx_resamp[2*k  ];
  resamp_tmp[2*i+1]=ampfac*tx_resamp[2*k+1];
  tx_resamp[2*k  ]*=ampfac*alc_win[m];
  tx_resamp[2*k+1]*=ampfac*alc_win[m];
  m--;
  k++;
  } 
fftforward(alc_fftsize, alc_fftn, tx_resamp, alc_table, alc_permute, FALSE);
// Clear the spectrum outside the desired passband.
// These data points should be very small.
k=txout_fftsize-alc_fftsize;
if(k > 0)
  {
  for(i=alc_sizhalf; i<alc_fftsize; i++)
    {
    tx_resamp[2*(k+i)  ]=tx_resamp[2*i  ];
    tx_resamp[2*(k+i)+1]=tx_resamp[2*i+1];
    }
  i=tx_filter_ib3+1;
  k=tx_filter_ia3-1;
  t1=1;
  t3=1./tx_filter_ib3;
  t2=0;
  while(k>=i && t1 >0)
    {
    tx_resamp[2*k]*=t1;
    tx_resamp[2*k+1]*=t1;
    tx_resamp[2*i]*=t1;
    tx_resamp[2*i+1]*=t1;
    t2+=t3;
    t1=1-t2*t2;
    i++;
    k--;
    }
  while(k>=i)
    {
    tx_resamp[2*k]=0;
    tx_resamp[2*k+1]=0;
    tx_resamp[2*i]=0;
    tx_resamp[2*i+1]=0;
    i++;
    k--;
    }
  }
if(k < 0)
  {
  lirerr(77676);
  }
//
//if(screen_count>=MAX_SCREENCOUNT)
//  {
//  if(tx_setup_flag == TRUE)show_txfft(tx_resamp,0,2,txout_fftsize);
//  lir_refresh_screen();
//  }
// Now we have the output in the frequency domain at the size txout_fftsize
// with a sampling rate that fits our D/A converter. Transform it back
// to the time domain into txout.
if(tx_onoff_flag !=0)
  {
  fftback(txout_fftsize, txout_fftn, tx_resamp, txout_table, 
                                                  txout_permute, FALSE);
  for(i=0; i<txout_fftsize; i+=2)
    {
    tx_pilot_tone*=-1;
    if(tx_resamp_maxamp < 0.0001 && tx_resamp_old_maxamp < 0.0001)
      {
      pilot_ampl=0;
      }
    else
      {
      pilot_ampl=tx_pilot_tone;
      }
    dt1=tx_daout_cos;
    tx_daout_cos=dt1*tx_daout_phstep_cos+tx_daout_sin*tx_daout_phstep_sin;
    tx_daout_sin=tx_daout_sin*tx_daout_phstep_cos-dt1*tx_daout_phstep_sin;
    r1=tx_resamp[i  ]+txout_tmp[i  ];
    r2=tx_resamp[i+1]+txout_tmp[i+1];
    t1=tx_daout_cos*tx_output_amplitude;
    t2=-tx_daout_sin*tx_output_amplitude;
    txout[i  ]=r1*t1+r2*t2+pilot_ampl;
    txout[i+1]=-r1*t2+r2*t1-pilot_ampl;
    }
  for(i=0; i<txout_fftsize; i++)
    {
    txout_tmp[i]=tx_resamp[txout_fftsize+i];
    }
  }
else
  {
  for(i=0; i<txout_fftsize; i++)
    {
    txout_tmp[i]=0;
    txout[i]=0;
    }   
  }  
// Compute the total time from tx input to tx output and
// express it as samples of the tx input clock and store in tx_wsamps. 
tx_wsamps=lir_tx_input_samples();
tx_wsamps+=(mictimf_pa-mictimf_px+mictimf_bufsiz)&mictimf_mask;
// The microphone input is real data at the soundcard speed.
// The complex output of micfft is at half the speed
// and transforms overlap by 50%.
tx_wsamps*=2;
tx_wsamps+=((micfft_pa-micfft_px+micfft_bufsiz)&micfft_mask)/2;
tx_wsamps+=(cliptimf_pa-cliptimf_px+micfft_bufsiz)&micfft_mask; 
j=((clipfft_pa-clipfft_px+alc_bufsiz)&alc_mask)/2;
j+=(alctimf_pa-((int)(alctimf_fx))+4+alc_bufsiz)&alc_mask;
tx_wsamps+=j/tx_pre_resample;
// The maximum amplitude should ideally be 1.0 but since
// a litte power outside the passband was removed in the last 
// backwards transform, a small amount of re-peaking could occur
// Set the maximum level to TX_DA_MARGIN of full scale to make sure
// D/A converters will not overflow.
tx_send_to_da();
}

void resample_tx_output(void)
{
int i1, i2, i3, i4, mask2;
float t1, t2, t3, t4, t5, t6, t7, r1, r2;
float a1, a2, rdiff;
// The microphone is best sampled at a low sampling speed since
// we do not want much bandwidth nor dynamic range.
// For the output the requirements depend on the circumstances.
// In case the transmitter is used in a wideband up-conversion
// system, the output sampling rate may be very high and the
// requirements on the signal purity might be very high.
// We do the resampling in two steps. First do a fractional
// resampling to a speed near the speed of the microphone
// signal (but perhaps a power of two below the desired output
// speed as indicated by tx_output_upsample.)
// Subsequently, use an FFT to sample up the signal to the
// desired frequency while filtering out the false signals
// that non-linearities in the fractional resampling have
// introduced.
mask2=alc_mask-1;
begin:;
r1=tx_resamp_pa/tx_resample_ratio;
resamp:;
r2=(tx_resamp_pa+2)/tx_resample_ratio;
i2=alctimf_fx+r1+1;
i3=alctimf_fx+r2+1;
i2&=mask2;
i3&=mask2;
if(abs(i3-i2) > 2)
  {
  i2=alctimf_fx+(r1+r2)/2;
  i3=i2+2;
  i2&=mask2;
  i3&=mask2;
  }
else
  {
  if(i3==i2)
    {
    i2=alctimf_fx+r1;
    i2&=mask2;
    if(i3==i2)
      {
      i3=i2+2;
      i3&=mask2;
      }
    }
  }        
i4=(i3+2)&(mask2);
i1=(i2+mask2)&(mask2);
if( ((alctimf_pb-i4+alc_bufsiz)&alc_mask) > 4)
  {
  if(alctimf_mute[i2/alc_fftsize]==FALSE ||
     alctimf_mute[i3/alc_fftsize]==FALSE)
    {
    rdiff=r1+alctimf_fx-i2;
    if(rdiff > alc_bufsiz/2)
      {
      rdiff-=alc_bufsiz;
      }
    rdiff/=2; 
// Use Lagrange's interpolation formula to fit a third degree
// polynomial to 4 points:
//  a1=-rdiff *   (rdiff-1)*(rdiff-2)*alctimf[i1]/6
//     +(rdiff+1)*(rdiff-1)*(rdiff-2)*alctimf[i2]/2
//     -(rdiff+1)* rdiff   *(rdiff-2)*alctimf[i3]/2
//     +(rdiff+1)* rdiff   *(rdiff-1)*alctimf[i4]/6; 
// Rewrite slightly to save a few multiplications - do not
// think the compiler is smart enough to do it for us.
    t1=rdiff-1;
    t2=rdiff-2;
    t3=rdiff+1;
    t4=t1*t2;
    t5=t3*rdiff;
    t6=rdiff*t4;
    t4=t3*t4;
    t7=t5*t2;
    t5=t5*t1;
    a1=((t5*alctimf[i4  ]-t6*alctimf[i1  ])/3+t4*alctimf[i2  ]-t7*alctimf[i3  ])/2;
    a2=((t5*alctimf[i4+1]-t6*alctimf[i1+1])/3+t4*alctimf[i2+1]-t7*alctimf[i3+1])/2;
// The curve fitting is (of course) just an approximation.
// Make sure we do not go outside our range!
    t2=a1*a1+a2*a2;
    if(t2 > tx_resamp_maxamp)tx_resamp_maxamp=t2;

    if(t2 > tx_resamp_ampfac1)tx_resamp_ampfac1=t2;
    tx_resamp[tx_resamp_pa+alc_fftsize  ]=a1;
    tx_resamp[tx_resamp_pa+alc_fftsize+1]=a2;
    }
  else
    {
    tx_resamp[tx_resamp_pa+alc_fftsize  ]=0;
    tx_resamp[tx_resamp_pa+alc_fftsize+1]=0;
    }
  r1=r2;
  tx_resamp_pa+=2;
  if(tx_resamp_pa >= tx_ssb_resamp_block_factor*alc_fftsize)
    {
    alctimf_fx+=r2;
    if(alctimf_fx>alc_bufsiz)alctimf_fx-=alc_bufsiz;
    t1=tx_resamp_ampfac1;
    if(t1 < tx_resamp_ampfac2)t1=tx_resamp_ampfac2;
    use_tx_resamp(sqrt(1/t1));
    tx_resamp_old_maxamp=tx_resamp_maxamp;
    tx_resamp_maxamp=0;
    tx_resamp_ampfac2=tx_resamp_ampfac1;
    tx_resamp_ampfac1=1;
    tx_resamp_pa=0;
    goto begin;
    }
  goto resamp;  
  }
}



void tx_bar(int xt,int yt,int val1, int val2)
{
int x, y;
x=xt*text_width;
y=yt*text_height+1;
if(val1 == val2)
  {
  if(val1 >= 0)return;
  lir_fillbox(x,y,TX_BAR_SIZE,text_height-2,7);
  return;
  }
if(val2 > val1)
  { 
  lir_fillbox(x+val1,y,val2-val1,text_height-2,12);
  }
else
  {  
  lir_fillbox(x+val2,y,val1-val2,text_height-2,7);
  } 
}


void show_txfft(float *z, float lim, int type, int siz)
{
int i, ia, ib, k, m;
int pixels_per_bin, bins_per_pixel;
float t1,t2;
char color;
short int *trc;
if(type >= MAX_DISPLAY_TYPES)
  {
  lirerr(762319); 
  return;
  }
trc=&txtrace[type*tx_show_siz];  
ia=0;
t2=0;
pixels_per_bin=(screen_width-1)/siz;
bins_per_pixel=1;
while(pixels_per_bin == 0)
  {
  siz/=2;
  pixels_per_bin=(screen_width-1)/siz;
  bins_per_pixel*=2;
  }
ib=pixels_per_bin;
lir_sched_yield();
for(i=1; i<siz; i++)
  {
  lir_line(ia,trc[i-1],ib,trc[i],0);
  ia=ib;
  ib+=pixels_per_bin;
  }
lir_sched_yield();
ia=0;
ib=pixels_per_bin;
m=0;
mailbox[0]=0;
for(i=0; i<siz; i++)
  {
  t1=0;
  for(k=0; k<bins_per_pixel; k++)
    {
    t1=z[2*m]*z[2*m]+z[2*m+1]*z[2*m+1];
    m++;
    }
  t1/=bins_per_pixel;  
  k=-0.07*log10(txtrace_gain*(t1+0.0000000000001))*screen_height;
  if(k>txtrace_height)k=txtrace_height;
  if(k<0)k=0;
  trc[i]=k+screen_height-txtrace_height-1;
  if(i>0)
    {
    if(type==0)
      {
      if(t2 > lim)
        {
        color=15;
        }
      else
        {
        color=12;
        }
      }
    else
      {
      color=display_color[type];
      }      
    lir_line(ia,trc[i-1],ib,trc[i],color);
    ia=ib;
    ib+=pixels_per_bin;
    }
  t2=t1;
  }
lir_sched_yield();
}

void spproc_setup(void)
{
char s[80];
int i, k;
int setup_mode;
int ad_pix1;
int ad_pix2;
int mute_pix1;
int mute_pix2;
int micagc_pix1;
int micagc_pix2;
int rf1agc_pix1;
int rf1agc_pix2;
int rf1clip_pix1;
int rf1clip_pix2;
int alc_pix1;
int alc_pix2;
float t1, t2, r2;
float prat, pmax;
int old_display_ptr;
int old_admax[DISPLAY_HOLD_SIZE];
float old_rf1agc[DISPLAY_HOLD_SIZE];
float old_rf1clip[DISPLAY_HOLD_SIZE];
float old_prat1[DISPLAY_HOLD_SIZE];
float old_pmax1[DISPLAY_HOLD_SIZE];
float old_prat2[DISPLAY_HOLD_SIZE];
float old_pmax2[DISPLAY_HOLD_SIZE];
float old_prat3[DISPLAY_HOLD_SIZE];
float old_pmax3[DISPLAY_HOLD_SIZE];
int default_spproc_no;
rx_mode=MODE_SSB;
set_hardware_tx_frequency();
default_spproc_no=tg.spproc_no;
tx_setup_flag=TRUE;
setup_mode=TX_SETUP_AD;
if(read_txpar_file()==FALSE)
  {
  set_default_spproc_parms();
  }
restart:;
screen_count=0;
if(txssb.minfreq < SSB_MINFQ_LOW)txssb.minfreq=SSB_MINFQ_LOW;
if(txssb.minfreq > SSB_MINFQ_HIGH)txssb.minfreq=SSB_MINFQ_HIGH;
if(txssb.maxfreq < SSB_MAXFQ_LOW)txssb.maxfreq=SSB_MAXFQ_LOW;
if(txssb.maxfreq > SSB_MAXFQ_HIGH)txssb.maxfreq=SSB_MAXFQ_HIGH;
if(txssb.slope > SSB_MAXSLOPE)txssb.slope=SSB_MAXSLOPE;
if(txssb.slope < SSB_MINSLOPE)txssb.slope=SSB_MINSLOPE;
if(txssb.bass > SSB_MAXBASS)txssb.bass=SSB_MAXBASS;
if(txssb.bass < SSB_MINBASS)txssb.bass=SSB_MINBASS;
if(txssb.treble > SSB_MAXTREBLE)txssb.treble=SSB_MAXTREBLE;
if(txssb.treble < SSB_MINTREBLE)txssb.treble=SSB_MINTREBLE;
if(txssb.mic_f_threshold < 0)txssb.mic_f_threshold=0;
if(txssb.mic_f_threshold > SSB_MAX_MICF)txssb.mic_f_threshold=SSB_MAX_MICF;
if(txssb.mic_t_threshold < 0)txssb.mic_t_threshold=0;
if(txssb.mic_t_threshold > SSB_MAX_MICT)txssb.mic_t_threshold=SSB_MAX_MICT;
if(txssb.mic_gain < 0)txssb.mic_gain=0;
if(txssb.mic_gain > SSB_MAX_MICGAIN)txssb.mic_gain=SSB_MAX_MICGAIN;
if(txssb.mic_agc_time < 0)txssb.mic_agc_time=0;
if(txssb.mic_agc_time > SSB_MAX_MICAGC_TIME)
                                   txssb.mic_agc_time=SSB_MAX_MICAGC_TIME;
if(txssb.mic_out_gain < 0)txssb.mic_out_gain=0;
if(txssb.mic_out_gain > SSB_MAX_MICOUT_GAIN)
                                    txssb.mic_out_gain=SSB_MAX_MICOUT_GAIN;
if(txssb.rf1_gain < SSB_MIN_RF1_GAIN)txssb.rf1_gain=SSB_MIN_RF1_GAIN;
if(txssb.rf1_gain > SSB_MAX_RF1_GAIN)txssb.rf1_gain=SSB_MAX_RF1_GAIN;
init_txmem_spproc();
tx_show_siz=micsize;
if(tx_show_siz < alc_fftsize)tx_show_siz=alc_fftsize;
if(tx_show_siz < txout_fftsize)tx_show_siz=txout_fftsize;
txtrace=malloc(MAX_DISPLAY_TYPES*tx_show_siz*sizeof(short int));
if(txtrace == NULL)
  {
  lirerr(778543);
  return;
  }
lir_mutex_init();
for(i=0; i<MAX_DISPLAY_TYPES*tx_show_siz; i++)txtrace[i]=screen_height-1;
txtrace_gain=0.1;
txtrace_height=screen_height-text_height*(TXPAR_INPUT_LINE+1);
lir_sleep(50000);
linrad_thread_create(THREAD_TX_INPUT);
old_display_ptr=0;
for(i=0; i<DISPLAY_HOLD_SIZE; i++)
  {
  old_admax[i]=0;
  old_rf1agc[i]=0;
  old_rf1clip[i]=0;
  old_prat1[i]=90;
  old_pmax1[i]=0;
  old_prat2[i]=90;
  old_pmax2[i]=0;
  old_prat3[i]=90;
  old_pmax3[i]=0;
  }
clear_screen();
tx_ad_maxamp=0;
ad_pix1=0;
ad_pix2=0;
micagc_pix1=0;
micagc_pix2=0;
rf1agc_pix1=0;
rf1agc_pix2=0;
mute_pix1=0;
mute_pix2=0;
rf1clip_pix1=0;
rf1clip_pix2=0;
alc_pix1=0;
alc_pix2=0;
settextcolor(14);
make_txproc_filename();
sprintf(s,"Press 'S' to save %s, 'R' to read.  '+' or '-' to change file no.",
                                                 txproc_filename);
lir_text(0,TX_BAR_AD_LINE-2,s);
lir_text(0,TX_BAR_AD_LINE-1,"Arrow up/down to change spectrum scale.");
lir_text(0,TX_BAR_AD_LINE,"'M' =");
lir_text(0,TX_BAR_MUTE_LINE,"'Q' =");
lir_text(0,TX_BAR_MICAGC_LINE,"'A' =");
lir_text(0,TX_BAR_RF1AGC_LINE,"'D' =");
lir_text(0,TX_BAR_RF1CLIP_LINE,"'C' =");
settextcolor(7);
lir_text(6,TX_BAR_AD_LINE,"Soundcard");
lir_text(6,TX_BAR_MUTE_LINE,"Mute");
tx_bar(16,TX_BAR_AD_LINE,-1,-1);
tx_bar(16,TX_BAR_MUTE_LINE,-1,-1);
lir_text(6,TX_BAR_MICAGC_LINE,"MIC AGC");
tx_bar(16,TX_BAR_MICAGC_LINE,-1,-1);
lir_text(6,TX_BAR_RF1AGC_LINE,"RF1 AGC");
tx_bar(16,TX_BAR_RF1AGC_LINE,-1,-1);
lir_text(6,TX_BAR_RF1CLIP_LINE,"RF clip");
tx_bar(16,TX_BAR_RF1CLIP_LINE,-1,-1);
lir_text(6,TX_BAR_ALC_LINE,"ALC");
tx_bar(16,TX_BAR_ALC_LINE,-1,-1);
switch (setup_mode)
  {
  case TX_SETUP_AD:
  lir_text(0,0,
            "Verify AF clip level, (some device drivers give too few bits)");
  lir_text(0,1,"Use mixer program to set volume for the AF to never reach");
  lir_text(0,2,"its maximum during normal operation.");
  lir_text(0,3,
              "Press 'L' or 'H' to change cut-off frequencies,'F','B' or 'T'");
  lir_text(0,4,"for slope, bass or treble to change frequency response.");
  sprintf(s,
    "Low %.0f Hz  High %.0f Hz   Slope %d dB/kHz   Bass %d dB  Treble %d dB",
       tx_lowest_bin*txad_hz_per_bin, tx_highest_bin*txad_hz_per_bin,
                                            txssb.slope, txssb.bass, txssb.treble);
  lir_text(0,5,s);
  break;

  case TX_SETUP_MUTE:
  lir_text(0,0,"Press 'F' to set frequency domain threshold or");
  lir_text(0,1,"Press 'T' to set time domain threshold for signal");
  
  lir_text(0,2,"to become muted when only background noise is present.");
  sprintf(s,"Thresholds:  F=%d dB   T=%d dB",
                                 txssb.mic_f_threshold, txssb.mic_t_threshold);
  lir_text(0,3,s);
  break;
  
  case TX_SETUP_MICAGC:
  lir_text(0,0,"Press 'V' to set mic volume for suitable AGC action");
  lir_text(0,1,"or 'T' to set decay time constant");
  sprintf(s,"Vol=%d dB   T=%.2f sek",txssb.mic_gain, 0.01*txssb.mic_agc_time);
  lir_text(0,2,s);
  break;
  
  case TX_SETUP_RF1AGC:
  lir_text(0,0,"Press 'B' to set gain for some AGC action");
  sprintf(s,"Gain=%d dB",txssb.mic_out_gain);
  lir_text(0,2,s);
  break;

  case TX_SETUP_RF1CLIP:
  lir_text(0,0,"Press 'B' to set RF1 gain for desired clip level.");
  lir_text(0,1,"Bar range is 0 to 30 dB");
  sprintf(s,"RF1 gain=%d dB",txssb.rf1_gain);
  lir_text(0,2,s);
  break;
  }
if(!kill_all_flag)lir_sem_wait(SEM_TXINPUT);
for(i=0; i<10; i++)
  {
  micfft_px=micfft_pa;
  lir_sched_yield();
  while(micfft_px == micfft_pa && !kill_all_flag)
    {
    lir_sem_wait(SEM_TXINPUT);
    t1=0;
    for(k=0;k<100;k++)t1+=sin(0.0001*k);
    lir_sched_yield();
    }
  }
micfft_px=micfft_pa;
while(!kill_all_flag)
  {
  if(screen_count>=MAX_SCREENCOUNT)screen_count=0;
  screen_count++;
  old_display_ptr++;
  if(old_display_ptr>=DISPLAY_HOLD_SIZE)old_display_ptr=0;
// Show the microphone input level. 
  ad_pix2=tx_ad_maxamp;
  if(ui.tx_ad_bytes != 2)
    {
    ad_pix2/=0x10000;
    }  
  old_admax[old_display_ptr]=ad_pix2;
  t1=0;
  for(i=0; i<DISPLAY_HOLD_SIZE; i++)
    {
    if(t1<old_admax[i])t1=old_admax[i];
    }
  t1=20*log10(t1/0x8000);  
  sprintf(s,"%.1fdB  ",t1);
  lir_text(18+TX_BAR_SIZE/text_width, TX_BAR_AD_LINE, s);
  ad_pix2=(ad_pix2*TX_BAR_SIZE)/0x8000;    
  tx_bar(16,TX_BAR_AD_LINE,ad_pix1,ad_pix2);
  ad_pix1=ad_pix2;
  tx_ad_maxamp=0.95*tx_ad_maxamp;
// **************************************************************
// The setup routine contains the same processing steps as the
// transmit routine, but it has varoius display functions added
// into it. The processing steps are numbered and described below.
// **************************************************************
//
// ************* Step 1. *************
// Wait for a new block of data from the mic input routine.
// Note that the data arrives as fourier transforms (micfft) and
// that the mic input routine tx_input() already has applied
// the filtering of the microphone signal.
  lir_refresh_screen();
//tx_ssb_buftim('a');  
  lir_sem_wait(SEM_TXINPUT);
  tx_agc_factor=tx_agc_decay*tx_agc_factor+(1-tx_agc_decay);
  micfft_bin_minpower=micfft_bin_minpower_ref*tx_agc_factor*tx_agc_factor;
  micfft_minpower=micfft_minpower_ref;//*tx_agc_factor*tx_agc_factor;
  if(screen_count>=MAX_SCREENCOUNT)
    {
    show_txfft(&micfft[micfft_px],micfft_bin_minpower,0,mic_fftsize);
    }
  k=tx_ssb_step2(&t1);
  if(k==0)
    {
    sprintf(s,"MUTED");
    }
  else
    {
    sprintf(s,"     ");
    tx_ssb_step3(&t1);
    }
  mute_pix2=(k*TX_BAR_SIZE)/(tx_highest_bin-tx_lowest_bin+1);    
  tx_bar(16,TX_BAR_MUTE_LINE,mute_pix1,mute_pix2);
  mute_pix1=mute_pix2;
  settextcolor(15);  
  lir_text(18+TX_BAR_SIZE/text_width,TX_BAR_MUTE_LINE,s);
  settextcolor(7);      
// In case AGC has reduced the gain by more than 20 dB, set the
// AGC factor to -20 dB immediately because voice should never
// be as loud. This is to avoid the agc release time constant
// for impulse sounds caused by hitting the microphone etc.       
  t1=20*log10(tx_agc_factor);  
  sprintf(s,"%.1fdB  ",t1);
  if(tx_agc_factor < 0.1)tx_agc_factor=0.1;
  lir_text(18+TX_BAR_SIZE/text_width, TX_BAR_MICAGC_LINE, s);
  micagc_pix2=-log10(tx_agc_factor)*TX_BAR_SIZE;
  tx_bar(16,TX_BAR_MICAGC_LINE,micagc_pix1,micagc_pix2);
  micagc_pix1=micagc_pix2;
// ************* Step 4. *************
// Go back to the time domain and store the signal in cliptimf
// Remember that we use sin squared windows and that
// the transforms overlap with 50%.
//
// Ideally, the peak amplitude should be 1.0, the audio AGC
// should be active and keep the power level nearly constant.
// For a voice signal the waveform will differ from time to time
// and therefore the peak to average power ratio will vary.
// Compute the peak amplitude for the current transform
// and save it for the next time we arrive here.
// Use the peak amplitude for an AGC in the time domain (Hilbert space).
// The Hilbert space AGC is a constant that may vary from one
// FFT block to the next one. It is equivalent with an AM modulator
// with a bandwidth corresponding to a single bin in the FFT so this
// AM modulation will not increase the bandwidth notably, but it
// will bring the RF amplitude near unity always.
//
// Finally, apply an amplitude limiter to the complex time domain signal.
// It will work as an RF limiter on the SSB signal and cause a lot of
// signal outside the passband.
  if(k!=0)
    {
    fftback(mic_fftsize, mic_fftn, &micfft[micfft_px], 
                                   micfft_table, micfft_permute,FALSE);
    lir_sched_yield();
    r2=0;
    for(i=0; i<mic_fftsize; i++)
      {
      t2=micfft[micfft_px+2*i  ]*micfft[micfft_px+2*i  ]+
         micfft[+2*i+1]*micfft[micfft_px+2*i+1];
      if(t2>r2)r2=t2;
      }
    if(r2 > 0.01*MAX_DYNRANGE)
      {
      t1=sqrt(r2);
      r2=1/t1;
      old_rf1agc[old_display_ptr]=t1;
      if(t1>10)t1=10;
      rf1agc_pix2=log10(t1)*TX_BAR_SIZE;
      if(r2>10/tx_agc_factor)r2=10/tx_agc_factor;
      if(rf1agc_pix2 < 0)rf1agc_pix2=0;
      }
    else
      {
      r2=0;
      rf1agc_pix2=0;
      old_rf1agc[old_display_ptr]=0;
      }
    }          
  else
    {
    r2=0;
    rf1agc_pix2=0;
    old_rf1agc[old_display_ptr]=0;
    }
  t1=0;
  for(i=0; i<DISPLAY_HOLD_SIZE; i++)
    {
    if(t1<old_rf1agc[i])t1=old_rf1agc[i];
    }
  if(t1 > 0)t1=20*log10(t1);  
  sprintf(s,"%.1fdB  ",t1);
  lir_text(18+TX_BAR_SIZE/text_width, TX_BAR_RF1AGC_LINE, s);
  tx_bar(16,TX_BAR_RF1AGC_LINE,rf1agc_pix1,rf1agc_pix2);
  rf1agc_pix1=rf1agc_pix2;
//tx_ssb_buftim('b');  
  tx_ssb_step4(r2,&pmax,&prat);
//tx_ssb_buftim('c');  
  prat/=mic_sizhalf;
  old_prat1[old_display_ptr]=prat;
  old_pmax1[old_display_ptr]=pmax;
  prat=0;
  for(i=0; i<DISPLAY_HOLD_SIZE; i++)
    {
    prat+=old_prat1[i];
    }
  prat/=DISPLAY_HOLD_SIZE;  
  for(i=0; i<DISPLAY_HOLD_SIZE; i++)
    {
    if(pmax<old_pmax1[i])pmax=old_pmax1[i];
    }
  if(prat>0.000000001 && pmax > prat)
    {
    prat=10*log10(pmax/prat);  
    }
  else
    {
    prat=99;
    } 
  sprintf(s,"[%.1f]  ",prat);
  lir_text(28+TX_BAR_SIZE/text_width, TX_BAR_MICAGC_LINE, s);
  old_rf1clip[old_display_ptr]=pmax;
  if(pmax > 1000)pmax=1000;  
  if(pmax > 1)
    {  
    rf1clip_pix2=log10(pmax)*TX_BAR_SIZE/3;
    }
  else
    {
    rf1clip_pix2=0;
    }
  t1=0;
  for(i=0; i<DISPLAY_HOLD_SIZE; i++)
    {
    if(t1<old_rf1clip[i])t1=old_rf1clip[i];
    }
  if(t1 > 1)
    {
    t1=10*log10(t1);  
    }
  else
    {
    t1=0;
    }  
  sprintf(s,"%.1fdB  ",t1);
  lir_text(18+TX_BAR_SIZE/text_width, TX_BAR_RF1CLIP_LINE, s);
  tx_bar(16,TX_BAR_RF1CLIP_LINE,rf1clip_pix1,rf1clip_pix2);
  rf1clip_pix1=rf1clip_pix2;
// ************* Step 5. *************
// At this point we have applied an RF clipper by limiting power in
// Hilbert space. As a consequence we have produced intermodulation
// products outside the desired passband.
// Go to the frequency domain and remove undesired frequencies.
  tx_ssb_step5();
// ************* Step 6. *************
// Go back to the time domain and store the signal in alctimf.
// Remember that we use sin squared windows and that
// the transforms overlap with 50%.
// Compute power and store the peak power with
// an exponential decay corresponding to a time constant
// of 50 milliseconds.
// We expand the fft size from mic_fftsize to tx_fftsiz2 because
// the fractional resampling that will follow this step needs
// the signal to be oversampled by a factor of four to avoid
// attenuation at high frequencies. The polynomial fitting
// works as a low pass filter and we do not want any attenuation
// at the high side of our passband.
//tx_ssb_buftim('d');  
  if(lir_tx_output_samples() < txda.buffer_frames-2*txda.block_frames)
    {    
    while(clipfft_px != clipfft_pa)
      {
      pmax=tx_ssb_step6(&prat);
      lir_sched_yield();
      prat/=alc_sizhalf;
      old_prat2[old_display_ptr]=prat;
      old_pmax2[old_display_ptr]=pmax;
      prat=0;
      for(i=0; i<DISPLAY_HOLD_SIZE; i++)
        {
        prat+=old_prat2[i];
        }
      prat/=DISPLAY_HOLD_SIZE;  
      for(i=0; i<DISPLAY_HOLD_SIZE; i++)
        {
        if(pmax<old_pmax2[i])pmax=old_pmax2[i];
        }
      if(prat>0.000000001 && pmax > prat)
        {
        prat=10*log10(pmax/prat);  
        }
      else
        {
        prat=99;
        } 
      sprintf(s,"[%.1f]  ",prat);
      lir_text(28+TX_BAR_SIZE/text_width, TX_BAR_RF1CLIP_LINE, s);
      if(pmax > 1)
        {  
        pmax=2*log10(pmax);
        if(pmax>1)pmax=1;
        alc_pix2=pmax*TX_BAR_SIZE;
        }
      else
        {
        alc_pix2=0;
        }
      tx_bar(16,TX_BAR_ALC_LINE,alc_pix1,alc_pix2);
      alc_pix1=alc_pix2;
      }
// ************* Step 7. *************
// Use the slowly decaying bi-directional peak power as an ALC
// but make sure we allow at least two data blocks un-processed
// to allow the backwards fall-off to become re-calculated.
// Using the slowly varying function for an ALC (= AM modulation)
// will not increase the bandwidth by more than the bandwidth
// of the modulation signal (50 ms or 20 Hz)
    while( ((alctimf_pa-alctimf_pb+alc_bufsiz)&alc_mask) >= 3*alc_fftsize)
      {
      pmax=tx_ssb_step7(&prat);
      prat/=alc_sizhalf;
      old_prat3[old_display_ptr]=prat;
      old_pmax3[old_display_ptr]=pmax;
      prat=0;
      for(i=0; i<DISPLAY_HOLD_SIZE; i++)
        {
        prat+=old_prat3[i];
        }
      prat/=DISPLAY_HOLD_SIZE;  
      for(i=0; i<DISPLAY_HOLD_SIZE; i++)
        {
        if(pmax<old_pmax3[i])pmax=old_pmax3[i];
        }
      if(prat>0.000000001 && pmax > prat)
        {
        prat=10*log10(pmax/prat);  
        }
      else
        {
        prat=99;
        } 
      sprintf(s,"[%.1f]  ",prat);
      lir_text(28+TX_BAR_SIZE/text_width, TX_BAR_ALC_LINE, s);
      }
// ************* Step 8. *************
// In case we have enough signal in the buffer, start the output.
    tx_ssb_step8();
    }
  test_keyboard();
  if(lir_inkey != 0)
    {
    process_current_lir_inkey();
    if(lir_inkey=='X')goto end_tx_setup;
    if(lir_inkey==ARROW_UP_KEY)
      {
      txtrace_gain*=1.33;
      goto continue_setup;
      }
    if(lir_inkey==ARROW_DOWN_KEY)
      {
      txtrace_gain/=1.25;
      goto continue_setup;
      }
    close_tx_output();
    linrad_thread_stop_and_join(THREAD_TX_INPUT);
    close_tx_input();
    if(txmem_handle != NULL)free(txmem_handle);
    txmem_handle=NULL;
    lir_sem_free(SEM_TXINPUT);
    lir_mutex_destroy();
    if(lir_inkey=='S')
      {
      clear_screen();
      save_tx_parms(TRUE);
      }
    if(lir_inkey=='R')
      {
      clear_screen();
      read_txpar_file();
      }
    switch (setup_mode)
      {
      case TX_SETUP_AD:
      if(lir_inkey=='L')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Low:");
        txssb.minfreq=lir_get_integer(5,TXPAR_INPUT_LINE,4,1,SSB_MINFQ_HIGH);
        }
      if(lir_inkey=='H')
        {
        lir_text(0,TXPAR_INPUT_LINE,"High:");
        txssb.maxfreq=lir_get_integer(6,TXPAR_INPUT_LINE,5,
                                            SSB_MAXFQ_LOW,ui.tx_ad_speed/2);
        }
      if(lir_inkey=='F')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Slope:");
        txssb.slope=lir_get_integer(7,TXPAR_INPUT_LINE,4,SSB_MINSLOPE,
                                                               SSB_MAXSLOPE);
        }
      if(lir_inkey=='B')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Bass:");
        txssb.bass=lir_get_integer(6,
                                 TXPAR_INPUT_LINE,4,SSB_MINBASS,SSB_MAXBASS);
        }
      if(lir_inkey=='T')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Treble:");
        txssb.treble=lir_get_integer(8,
                             TXPAR_INPUT_LINE,4,SSB_MINTREBLE,SSB_MAXTREBLE);
        }
      break;

      case TX_SETUP_MUTE:
      if(lir_inkey=='F')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Freq domain:");
        txssb.mic_f_threshold=lir_get_integer(13,
                                          TXPAR_INPUT_LINE,3,0,SSB_MAX_MICF);
        }
      if(lir_inkey=='T')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Time domain:");
        txssb.mic_t_threshold=lir_get_integer(13,
                                          TXPAR_INPUT_LINE,3,0,SSB_MAX_MICT);
        }
      break;

      case TX_SETUP_MICAGC:
      if(lir_inkey=='V')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Volume:");
        txssb.mic_gain=lir_get_integer(8,
                                      TXPAR_INPUT_LINE,3,0,SSB_MAX_MICGAIN);
        }
      if(lir_inkey=='T')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Time constant:");
        t1=lir_get_float(15,TXPAR_INPUT_LINE,6,0,SSB_MAX_MICAGC_TIME);
        txssb.mic_agc_time=100*t1;  
        }
      break;



      case TX_SETUP_RF1AGC:
      if(lir_inkey=='B')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Gain:");
        txssb.mic_out_gain=lir_get_integer(15,
                                   TXPAR_INPUT_LINE,2,0,SSB_MAX_MICOUT_GAIN);
        }
      break;

      case TX_SETUP_RF1CLIP:
      if(lir_inkey=='B')
        {
        lir_text(0,TXPAR_INPUT_LINE,"Clipper gain:");
        txssb.rf1_gain=lir_get_integer(15,
                       TXPAR_INPUT_LINE,3,SSB_MIN_RF1_GAIN,SSB_MAX_RF1_GAIN);
        }
      break;
      }
    if(lir_inkey=='A')setup_mode=TX_SETUP_MICAGC;
    if(lir_inkey=='M')setup_mode=TX_SETUP_AD;
    if(lir_inkey=='Q')setup_mode=TX_SETUP_MUTE;
    if(lir_inkey=='D')setup_mode=TX_SETUP_RF1AGC;
    if(lir_inkey=='C')setup_mode=TX_SETUP_RF1CLIP;
    if(lir_inkey=='+')
      {
      tg.spproc_no++;
      if(tg.spproc_no > MAX_SSBPROC_FILES)tg.spproc_no=MAX_SSBPROC_FILES;
      }
    if(lir_inkey=='-')
      {
      tg.spproc_no--;
      if(tg.spproc_no < 0)tg.spproc_no=0;
      }
    goto restart;
    }
continue_setup:;  
  }
end_tx_setup:;
close_tx_output();
linrad_thread_stop_and_join(THREAD_TX_INPUT);
close_tx_input();
if(txmem_handle != NULL)free(txmem_handle);
txmem_handle=NULL;
lir_sem_free(SEM_TXINPUT);
lir_mutex_destroy();
tg.spproc_no=default_spproc_no;
}

