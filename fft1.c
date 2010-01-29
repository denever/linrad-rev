

#include <unistd.h>
#include <string.h>
#include "globdef.h"
#include "uidef.h"
#include "fft1def.h"
#include "screendef.h"
#include "caldef.h"
#include "seldef.h"
#include "thrdef.h"
#include "options.h"

#define FRESH_RECALC 8

void fix2_dspnames(char *s,int i)
{
s[i]='c';  
s[i+1]='o';
s[i+2]='r';
s[i+3]='r';
s[i+4]=0;
s[0]='d';
s[1]='s';
s[2]='p';
}


int fix1_dspnames(char *s)
{
int i;
i=0;
while(rxpar_filenames[rx_mode][i] != 0)
  {
  s[i]=rxpar_filenames[rx_mode][i];
  i++;
  }
s[i]='_';
i++;  
return i;
}



void make_iqcorr_filename(char *s)
{
int i;
i=fix1_dspnames(s);
s[i]='i';
s[i+1]='q';
i+=2;
fix2_dspnames(s,i);
}  


void make_filfunc_filename(char *s)
{
int i;
i=fix1_dspnames(s);
fix2_dspnames(s,i);
}  

void update_fft1_slowsum(void)
{
int i,ia,pa,pb;
// Now that we have a new power spectrum in fft1_sumsq, update the
// average of the spectra in fft1_slowsum by adding it's power and 
// subtracting the power of the old one that should no longer 
// contribute to the sum.  
// This way of calculating an average may cause errors after a long
// time since bits may be lost occasionally due to rounding errors.
// Calculate a few points from scratch each time to eliminate this
// problem. 
pa=fft1_sumsq_pws;
pb=(pa-wg_fft_avg2num*fft1_size+fft1_sumsq_bufsize)&fft1_sumsq_mask;
if(fft1_sumsq_recalc == fft1_last_point) fft1_sumsq_recalc=fft1_first_point;
ia=fft1_sumsq_recalc;
fft1_sumsq_recalc+=wg.xpoints/FRESH_RECALC;
if(fft1_sumsq_recalc>fft1_last_point)fft1_sumsq_recalc=fft1_last_point;
new_fft1_averages(ia,fft1_sumsq_recalc);
for(i=fft1_first_point; i < ia; i++)
  {
  fft1_slowsum[i]+=fft1_sumsq[pa+i]-fft1_sumsq[pb+i];
  if(fft1_slowsum[i]<0)fft1_slowsum[i]=0;
  }
for(i=fft1_sumsq_recalc+1; i <= fft1_last_point; i++)
  {
  fft1_slowsum[i]+=fft1_sumsq[pa+i]-fft1_sumsq[pb+i];
  if(fft1_slowsum[i]<0.0000001)fft1_slowsum[i]=0.0000001;
  }
fft1_sumsq_pws=(fft1_sumsq_pws+fft1_size)&fft1_sumsq_mask; 
}

void update_wg_waterf(void)
{      
wg_waterf_ptr-=wg_xpixels;
if(wg_waterf_ptr < 0)wg_waterf_ptr+=wg_waterf_size;
wg_waterf_sum_counter=0;
if( wg_waterf_lines != -1)lir_sem_post(SEM_SCREEN);
}

void fft1_waterfall(void)
{
int i,j,k,m,ix;
float der,t1,t2,y,yval;
while(fft1_sumsq_pwg != fft1_sumsq_pa )
  {
  j=fft1_sumsq_pwg+wg.first_xpoint; 
  for(i=wg_first_point; i <= wg_last_point; i++)
    {
    wg_waterf_sum[i]+=fft1_sumsq[j];
    j++;
    }
  fft1_sumsq_pwg=(fft1_sumsq_pwg+fft1_size)&fft1_sumsq_mask; 
  wg_waterf_sum_counter+=wg.fft_avg1num;
  if(wg_waterf_sum_counter >= wg.waterfall_avgnum)
    {
// A new summed power spectrum has arrived.
// Convert it from linear power scale to log in units of 0.1dB.
// Expand or contract so we get the number of pixels that
// will fit on the screen.
// Store at the pos of our current line in waterfall
    if(wg.xpoints_per_pixel == 1 || wg.pixels_per_xpoint == 1)
      {
      i=wg.first_xpoint;
      for(ix=0; ix < wg_xpixels; ix++)
        {
        y=1000*log10(wg_waterf_sum[i]*wg_waterf_yfac[i]);
        if(y < -32767)y=-32767;
        if(y>32767)y=32767;
        wg_waterf[wg_waterf_ptr+ix]=y;
        i++;
        }
      }    
    else
      {
      if(wg.xpoints_per_pixel == 0)
        {
// There are more pixels than data points so we must interpolate.
        y=1000*log10(wg_waterf_sum[wg.first_xpoint]*
                                           wg_waterf_yfac[wg.first_xpoint]);
        yval=y;
        if(y < -32767)y=-32767;
        if(y>32767)y=32767;
        wg_waterf[wg_waterf_ptr]=y;
        i=wg.first_xpoint+1;
        m=wg_xpixels-wg.pixels_per_xpoint;
        for(ix=0; ix<m; ix+=wg.pixels_per_xpoint)
          {
          t1=1000*log10(wg_waterf_sum[i]*wg_waterf_yfac[i]);
          der=(t1-yval)/wg.pixels_per_xpoint;
          for(k=ix+1; k<=ix+wg.pixels_per_xpoint; k++)
            {
            yval=yval+der;
            y=yval;
            if(y < -32767)y=32767;
            if(y>32767)y=32767;
            wg_waterf[wg_waterf_ptr+k]=y;
            }  
          yval=t1;
          i++;
          }      
        if(i < fft1_size)
          {          
          t1=1000*log10(wg_waterf_sum[i]*wg_waterf_yfac[i]);
          der=(t1-yval)/wg.pixels_per_xpoint;
          for(k=ix+1; k<=ix+wg.pixels_per_xpoint; k++)
            {
            yval=yval+der;
            y=yval;
            if(y < -32767)y=32767;
            if(y>32767)y=32767;
            wg_waterf[wg_waterf_ptr+k]=y;
            }
          }  
        }
      else
        {
// There is more than one data point for each pixel.
// Pick the strongest bin. We want to enhance the visibility of 
// frequency stable signal.
// Slide a triangular filter across the spectrum to make it
// smoother before resampling.
        i=wg.first_xpoint;
        for(ix=0; ix<wg_xpixels; ix++)
          {
          t1=0;
          for(k=0; k<wg.xpoints_per_pixel; k++)
            {
            t2=wg_waterf_sum[i+k]*wg_waterf_yfac[i+k];
            if(t2 >t1)t1=t2;
            }
          y=1000*log10(t1);
          if(y < -32767)y=-32767;
          if(y>32767)y=32767;
          wg_waterf[wg_waterf_ptr+ix]=y;
          i+=wg.xpoints_per_pixel;
          }
        }  
      }
    for(i=wg_first_point; i <= wg_last_point; i++)
      {
// Set a very small value rather than zero to avoid errors
// due to sending zero into the log function.      
      wg_waterf_sum[i]=0.00001;
      }
    update_wg_waterf();      
    }
  }
}

void fft1_b(void)
{
int k, m, ia,ib,ic,id;
float t1,t2,t3,t4;
int pc;
if(  (ui.rx_input_mode&IQ_DATA) == 0)
  {
  if(ui.rx_rf_channels==1)
    {
    fft1_reherm_dit_one();
    }
  else  
    {
    fft1_reherm_dit_two();
    }
  }
else 
  {
  if(ui.rx_rf_channels==1)
    {
    fft1_complex_one();
    if( (fft1_calibrate_flag&CALIQ)==CALIQ)
      {    
// In direct conversion mode it is not possible to get perfect amplitude
// and phase relations between the two channels in the analog circuitry.
// As a result the signal has a mirror image that can be expected in the
// -40dB region for 1% analog filter components.
// The image is removed by the below section that orthogonalises the
// signal and it's mirror using data obtained in a hardware calibration
// process.
      pc=fft1_pa/2;
      m=fft1_first_sym_point;
      if(m==0)m=1;
      ib=pc+m;
      ic=pc+fft1_size-m;
      id=fft1_size-m;
      k=fft1_size/2;
      if(fft1_direction > 0)
        {
        for(ia=m; ia < k; ia++)
          {
          t1=   fft1_float[2*ib  ] * fft1_foldcorr[2*ia  ] 
              - fft1_float[2*ib+1] * fft1_foldcorr[2*ia+1];
          t2=   fft1_float[2*ib  ] * fft1_foldcorr[2*ia+1]
              + fft1_float[2*ib+1] * fft1_foldcorr[2*ia  ];
          fft1_float[2*ib  ] -=   fft1_float[2*ic  ] * fft1_foldcorr[2*id  ] 
                                + fft1_float[2*ic+1] * fft1_foldcorr[2*id+1];
          fft1_float[2*ib+1] -=   fft1_float[2*ic  ] * fft1_foldcorr[2*id+1] 
                                - fft1_float[2*ic+1] * fft1_foldcorr[2*id  ];
          fft1_float[2*ic  ]-=t1;
          fft1_float[2*ic+1]+=t2;
          ib++;
          id--;
          ic--;
          }
        }
      else        
        {
        for(ia=m; ia < k; ia++)
          {
          t1= fft1_float[2*ic  ] -
              fft1_float[2*ib  ] * fft1_foldcorr[2*ia  ] +
              fft1_float[2*ib+1] * fft1_foldcorr[2*ia+1];
          t2= fft1_float[2*ic+1] +
              fft1_float[2*ib  ] * fft1_foldcorr[2*ia+1] +
              fft1_float[2*ib+1] * fft1_foldcorr[2*ia  ];
          t3= fft1_float[2*ib  ] -
              fft1_float[2*ic  ] * fft1_foldcorr[2*id  ] - 
              fft1_float[2*ic+1] * fft1_foldcorr[2*id+1];
          t4= fft1_float[2*ib+1] -
              fft1_float[2*ic  ] * fft1_foldcorr[2*id+1] +
              fft1_float[2*ic+1] * fft1_foldcorr[2*id  ];
          fft1_float[2*ib+1]=t1;
          fft1_float[2*ib  ]=t2;
          fft1_float[2*ic+1]=t3;
          fft1_float[2*ic  ]=t4;
          ib++;
          id--;
          ic--;
          }
        }
      }
    else
      {    
      pc=fft1_pa/2;
      m=fft1_first_sym_point;
      if(m==0)m=1;
      ib=pc+m;
      ic=pc+fft1_size-m;
      id=fft1_size-m;
      k=fft1_size/2;
      if(fft1_direction < 0)
        {
        for(ia=m; ia < k; ia++)
          {
          t1= fft1_float[2*ic  ];
          t2= fft1_float[2*ic+1];
          fft1_float[2*ic+1]=fft1_float[2*ib  ];
          fft1_float[2*ic  ]=fft1_float[2*ib+1];
          fft1_float[2*ib+1]=t1;
          fft1_float[2*ib  ]=t2;
          ib++;
          ic--;
          }
        }
      }
    }
  else
    {
    fft1_complex_two();
    if( (fft1_calibrate_flag&CALIQ)==CALIQ)
      {    
// In direct conversion mode it is not possible to get perfect amplitude
// and phase relations between the two channels in the analog circuitry.
// As a result the signal has a mirror image that can be expected in the
// -40dB region for 1% analog filter components.
// The image is removed by the below section that orthogonalises the
// signal and it's mirror using data obtained in a hardware calibration
// process.
      pc=fft1_pa/4;
      m=fft1_first_sym_point;
      if(m==0)m=1;
      ib=pc+m;
      ic=pc+fft1_size-m;
      id=fft1_size-m;
      k=fft1_size/2;
      if(fft1_direction > 0)
        {
        for(ia=m; ia < k; ia++)
          {
          t1=   fft1_float[4*ib  ] * fft1_foldcorr[4*ia  ] 
              - fft1_float[4*ib+1] * fft1_foldcorr[4*ia+1];
          t2=   fft1_float[4*ib  ] * fft1_foldcorr[4*ia+1]
              + fft1_float[4*ib+1] * fft1_foldcorr[4*ia  ];
          fft1_float[4*ib  ] -=   fft1_float[4*ic  ] * fft1_foldcorr[4*id  ] 
                                + fft1_float[4*ic+1] * fft1_foldcorr[4*id+1];
          fft1_float[4*ib+1] -=   fft1_float[4*ic  ] * fft1_foldcorr[4*id+1] 
                              - fft1_float[4*ic+1] * fft1_foldcorr[4*id  ];
          fft1_float[4*ic  ]-=t1;
          fft1_float[4*ic+1]+=t2;
          t1=   fft1_float[4*ib+2] * fft1_foldcorr[4*ia+2] 
              - fft1_float[4*ib+3] * fft1_foldcorr[4*ia+3];
          t2=   fft1_float[4*ib+2] * fft1_foldcorr[4*ia+3]
              + fft1_float[4*ib+3] * fft1_foldcorr[4*ia+2];
          fft1_float[4*ib+2] -=   fft1_float[4*ic+2] * fft1_foldcorr[4*id+2] 
                                + fft1_float[4*ic+3] * fft1_foldcorr[4*id+3];
          fft1_float[4*ib+3] -=   fft1_float[4*ic+2] * fft1_foldcorr[4*id+3] 
                                - fft1_float[4*ic+3] * fft1_foldcorr[4*id+2];
          fft1_float[4*ic+2]-=t1;
          fft1_float[4*ic+3]+=t2;
          ib++;
          id--;
          ic--;
          }
        }
      else
        {
        for(ia=m; ia < k; ia++)
          {
          t1= fft1_float[4*ic  ] -
              fft1_float[4*ib  ] * fft1_foldcorr[4*ia  ] +
              fft1_float[4*ib+1] * fft1_foldcorr[4*ia+1];
          t2= fft1_float[4*ic+1] +
              fft1_float[4*ib  ] * fft1_foldcorr[4*ia+1] +
              fft1_float[4*ib+1] * fft1_foldcorr[4*ia  ];
          t3= fft1_float[4*ib  ] -
              fft1_float[4*ic  ] * fft1_foldcorr[4*id  ] - 
              fft1_float[4*ic+1] * fft1_foldcorr[4*id+1];
          t4= fft1_float[4*ib+1] -
              fft1_float[4*ic  ] * fft1_foldcorr[4*id+1] +
              fft1_float[4*ic+1] * fft1_foldcorr[4*id  ];
          fft1_float[4*ib+1]=t1;
          fft1_float[4*ib  ]=t2;
          fft1_float[4*ic+1]=t3;
          fft1_float[4*ic  ]=t4;
          t1= fft1_float[4*ic+2] -
              fft1_float[4*ib+2] * fft1_foldcorr[4*ia+2] +
              fft1_float[4*ib+3] * fft1_foldcorr[4*ia+3];
          t2= fft1_float[4*ic+3] +
              fft1_float[4*ib+2] * fft1_foldcorr[4*ia+3] +
              fft1_float[4*ib+3] * fft1_foldcorr[4*ia+2];
          t3= fft1_float[4*ib+2] -
              fft1_float[4*ic+2] * fft1_foldcorr[4*id+2] - 
              fft1_float[4*ic+3] * fft1_foldcorr[4*id+3];
          t4= fft1_float[4*ib+3] -
              fft1_float[4*ic+2] * fft1_foldcorr[4*id+3] +
              fft1_float[4*ic+3] * fft1_foldcorr[4*id+2];
          fft1_float[4*ib+3]=t1;
          fft1_float[4*ib+2]=t2;
          fft1_float[4*ic+3]=t3;
          fft1_float[4*ic+2]=t4;
          ib++;
          id--;
          ic--;
          }
        }
      }
    else
      {    
      pc=fft1_pa/4;
      m=fft1_first_sym_point;
      if(m==0)m=1;
      ib=pc+m;
      ic=pc+fft1_size-m;
      k=fft1_size/2;
      if(fft1_direction < 0)
        {
        for(ia=m; ia < k; ia++)
          {
          t1= fft1_float[4*ic  ];
          t2= fft1_float[4*ic+1];
          t3= fft1_float[4*ic+2];
          t4= fft1_float[4*ic+3];
          fft1_float[4*ic+1]=fft1_float[4*ib  ];
          fft1_float[4*ic  ]=fft1_float[4*ib+1];
          fft1_float[4*ic+3]=fft1_float[4*ib+2];
          fft1_float[4*ic+2]=fft1_float[4*ib+3];
          fft1_float[4*ib+1]=t1;
          fft1_float[4*ib  ]=t2;
          fft1_float[4*ib+3]=t3;
          fft1_float[4*ib+2]=t4;
          ib++;
          ic--;
          }
        }
      }
    }
  }
timf1p_px=(timf1p_px+timf1_blockbytes)&timf1_bytemask;
if( (ui.network_flag&NET_RXOUT_FFT1) != 0)
  {
  memcpy(&fft1_netsend_buffer[fft1net_pa],
                               &fft1_float[fft1_pa],fft1_blockbytes);
  fft1net_pa=(fft1net_pa+fft1_blockbytes)&fft1net_mask;
  }
fft1_pa=(fft1_pa+fft1_block)&fft1_mask;  
fft1_na=fft1_pa/fft1_block;
if(fft1_nm != fft1n_mask)fft1_nm++;
}

void fft1_c(void)
{
int i, ia, ib, ic, pa, ix;
float *pwr;
TWOCHAN_POWER *pxy;
float t1,t2;
// The total filter chain of the hardware, is normally not
// very flat, nor has it a good pulse response.
// Add one extra filter here so the total filter response becomes what
// the user has asked for.
// At the same time the data becomes normalised to an amplitude that
// is independent of the processes up to this point.
// Normalisation is for the noise floor to be around 0dB in the
// full dynamic range graph. 
// To save memory accesses, calculate fft1_sumsq at the same time.
// In case AFC is run from fft1 we are probably using a slow 
// computer and large fft1 sizes. Then summed powers are
// calculated elsewhere.
pa=fft1_nb*fft1_block;
ix=fft1_last_point;
ic=fft1_sumsq_pa+fft1_first_point;
if(fft1afc_flag <= 0)
  {
// We want fft1_sumsq to contain summed power spectra calculated from fft1
// over wg.fft_avg1num individual spectra.
  if(fft1_sumsq_counter == 0)
    {
    if(sw_onechan)
      {
      ib=pa/2+fft1_first_point;
      for(ia=fft1_first_point; ia <= ix; ia++)
        {
        t1=fft1_float[2*ib  ]*fft1_filtercorr[2*ia  ]-
           fft1_float[2*ib+1]*fft1_filtercorr[2*ia+1];
        fft1_float[2*ib+1]=fft1_float[2*ib+1]*fft1_filtercorr[2*ia  ]+
                           fft1_float[2*ib  ]*fft1_filtercorr[2*ia+1];
        fft1_float[2*ib]=t1;
        fft1_sumsq[ic]=t1*t1+fft1_float[2*ib+1]*fft1_float[2*ib+1];
        ib++;
        ic++;
        }
      }  
    else
      {
      ib=pa/4+fft1_first_point;
      for(ia=fft1_first_point; ia <= ix; ia++)
        {
        t1=fft1_float[4*ib  ]*fft1_filtercorr[4*ia  ]-
           fft1_float[4*ib+1]*fft1_filtercorr[4*ia+1];
        fft1_float[4*ib+1]=fft1_float[4*ib+1]*fft1_filtercorr[4*ia  ]+
                           fft1_float[4*ib  ]*fft1_filtercorr[4*ia+1];
        fft1_float[4*ib  ]=t1;
        t1=t1*t1+fft1_float[4*ib+1]*fft1_float[4*ib+1];
        t2=fft1_float[4*ib+2]*fft1_filtercorr[4*ia+2]-
           fft1_float[4*ib+3]*fft1_filtercorr[4*ia+3];
        fft1_float[4*ib+3]=fft1_float[4*ib+3]*fft1_filtercorr[4*ia+2]+
                           fft1_float[4*ib+2]*fft1_filtercorr[4*ia+3];
        fft1_float[4*ib+2]=t2;
        fft1_sumsq[ic]=t1+t2*t2+fft1_float[4*ib+3]*fft1_float[4*ib+3];
        ib++;
        ic++;
        }
      }    
    }
  else
    {
    if(sw_onechan)
      {
      ib=pa/2+fft1_first_point;
      for(ia=fft1_first_point; ia <= ix; ia++)
        {
        t1=fft1_float[2*ib  ]*fft1_filtercorr[2*ia  ]-
           fft1_float[2*ib+1]*fft1_filtercorr[2*ia+1];
        fft1_float[2*ib+1]=fft1_float[2*ib+1]*fft1_filtercorr[2*ia  ]+
                           fft1_float[2*ib  ]*fft1_filtercorr[2*ia+1];
        fft1_float[2*ib]=t1;
        fft1_sumsq[ic]+=t1*t1+fft1_float[2*ib+1]*fft1_float[2*ib+1];
        ib++;
        ic++;
        }
      }  
    else
      {
      ib=pa/4+fft1_first_point;
      for(ia=fft1_first_point; ia <= ix; ia++)
        {
        t1=fft1_float[4*ib  ]*fft1_filtercorr[4*ia  ]-
           fft1_float[4*ib+1]*fft1_filtercorr[4*ia+1];
        fft1_float[4*ib+1]=fft1_float[4*ib+1]*fft1_filtercorr[4*ia  ]+
                           fft1_float[4*ib  ]*fft1_filtercorr[4*ia+1];
        fft1_float[4*ib  ]=t1;
        t1=t1*t1+fft1_float[4*ib+1]*fft1_float[4*ib+1];               
        t2=fft1_float[4*ib+2]*fft1_filtercorr[4*ia+2]-
           fft1_float[4*ib+3]*fft1_filtercorr[4*ia+3];
        fft1_float[4*ib+3]=fft1_float[4*ib+3]*fft1_filtercorr[4*ia+2]+
                           fft1_float[4*ib+2]*fft1_filtercorr[4*ia+3];
        fft1_float[4*ib+2]=t2;
        fft1_sumsq[ic]+=t1+t2*t2+fft1_float[4*ib+3]*fft1_float[4*ib+3];
        ib++;
        ic++;
        }
      }    
    }
  }    
else
  {
// This code is executed to make the last step of fft1 in case
// fft2 is not selected and AFC is in use.
  if(no_of_spurs > 0)  
    {
    if(sw_onechan)
      {
      ib=pa/2+fft1_first_point;
      for(ia=fft1_first_point; ia <= ix; ia++)
        {
        t1=fft1_float[2*ib  ]*fft1_filtercorr[2*ia  ]-
           fft1_float[2*ib+1]*fft1_filtercorr[2*ia+1];
        fft1_float[2*ib+1]=fft1_float[2*ib+1]*fft1_filtercorr[2*ia  ]+
                           fft1_float[2*ib  ]*fft1_filtercorr[2*ia+1];
        fft1_float[2*ib]=t1;
        ib++;
        }
      }  
    else
      {
      ib=pa/4+fft1_first_point;
      for(ia=fft1_first_point; ia <= ix; ia++)
        {
        t1=fft1_float[4*ib  ]*fft1_filtercorr[4*ia  ]-
           fft1_float[4*ib+1]*fft1_filtercorr[4*ia+1];
        fft1_float[4*ib+1]=fft1_float[4*ib+1]*fft1_filtercorr[4*ia  ]+
                           fft1_float[4*ib  ]*fft1_filtercorr[4*ia+1];
        fft1_float[4*ib  ]=t1;
        t2=fft1_float[4*ib+2]*fft1_filtercorr[4*ia+2]-
           fft1_float[4*ib+3]*fft1_filtercorr[4*ia+3];
        fft1_float[4*ib+3]=fft1_float[4*ib+3]*fft1_filtercorr[4*ia+2]+
                           fft1_float[4*ib+2]*fft1_filtercorr[4*ia+3];
        fft1_float[4*ib+2]=t2;
        ib++;
        }
      }    
    fftx_na=fft1_nb;
    eliminate_spurs();
    if(fft1_sumsq_counter == 0)
      {
      if(sw_onechan)
        {
        ib=pa/2+fft1_first_point;
        pwr=&fft1_power[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          pwr[ia]=fft1_float[2*ib]*fft1_float[2*ib]+
                  fft1_float[2*ib+1]*fft1_float[2*ib+1];
          fft1_sumsq[ic]=pwr[ia];
          ib++;
          ic++;
          }
        }  
      else
        {
        ib=pa/4+fft1_first_point;
        pxy=&fft1_xypower[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          pxy[ia].x2=fft1_float[4*ib  ]*fft1_float[4*ib  ]+
                     fft1_float[4*ib+1]*fft1_float[4*ib+1];
          pxy[ia].y2=fft1_float[4*ib+2]*fft1_float[4*ib+2]+
                     fft1_float[4*ib+3]*fft1_float[4*ib+3];
          pxy[ia].im_xy=-fft1_float[4*ib  ]*fft1_float[4*ib+3]+
                         fft1_float[4*ib+2]*fft1_float[4*ib+1];
          pxy[ia].re_xy= fft1_float[4*ib  ]*fft1_float[4*ib+2]+
                         fft1_float[4*ib+1]*fft1_float[4*ib+3];
          fft1_sumsq[ic]=pxy[ia].x2+pxy[ia].y2;
          ic++;
          ib++;
          }
        }    
      }
    else
      {
      if(sw_onechan)
        {
        ib=pa/2+fft1_first_point;
        pwr=&fft1_power[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          pwr[ia]=fft1_float[2*ib]*fft1_float[2*ib]+
                  fft1_float[2*ib+1]*fft1_float[2*ib+1];
          fft1_sumsq[ic]+=pwr[ia];
          ic++;
          ib++;
          }
        }  
      else
        {
        ib=pa/4+fft1_first_point;
        pxy=&fft1_xypower[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          pxy[ia].x2=fft1_float[4*ib  ]*fft1_float[4*ib  ]+
                     fft1_float[4*ib+1]*fft1_float[4*ib+1];
          pxy[ia].y2=fft1_float[4*ib+2]*fft1_float[4*ib+2]+
                     fft1_float[4*ib+3]*fft1_float[4*ib+3];
          pxy[ia].im_xy=-fft1_float[4*ib  ]*fft1_float[4*ib+3]+
                         fft1_float[4*ib+2]*fft1_float[4*ib+1];
          pxy[ia].re_xy= fft1_float[4*ib  ]*fft1_float[4*ib+2]+
                         fft1_float[4*ib+1]*fft1_float[4*ib+3];
          fft1_sumsq[ic]+=pxy[ia].x2+pxy[ia].y2;
          ic++;
          ib++;
          }
        }    
      }
    }  
  else
    {
    if(fft1_sumsq_counter == 0)
      {
      if(sw_onechan)
        {
        ib=pa/2+fft1_first_point;
        pwr=&fft1_power[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          t1=fft1_float[2*ib  ]*fft1_filtercorr[2*ia  ]-
             fft1_float[2*ib+1]*fft1_filtercorr[2*ia+1];
          fft1_float[2*ib+1]=fft1_float[2*ib+1]*fft1_filtercorr[2*ia  ]+
                             fft1_float[2*ib  ]*fft1_filtercorr[2*ia+1];
          fft1_float[2*ib]=t1;
          pwr[ia]=t1*t1+fft1_float[2*ib+1]*fft1_float[2*ib+1];
          fft1_sumsq[ic]=pwr[ia];
          ib++;
          ic++;
          }
        }  
      else
        {
        ib=pa/4+fft1_first_point;
        pxy=&fft1_xypower[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          t1=fft1_float[4*ib  ]*fft1_filtercorr[4*ia  ]-
             fft1_float[4*ib+1]*fft1_filtercorr[4*ia+1];
          fft1_float[4*ib+1]=fft1_float[4*ib+1]*fft1_filtercorr[4*ia  ]+
                             fft1_float[4*ib  ]*fft1_filtercorr[4*ia+1];
          fft1_float[4*ib  ]=t1;
          t2=fft1_float[4*ib+2]*fft1_filtercorr[4*ia+2]-
             fft1_float[4*ib+3]*fft1_filtercorr[4*ia+3];
          fft1_float[4*ib+3]=fft1_float[4*ib+3]*fft1_filtercorr[4*ia+2]+
                             fft1_float[4*ib+2]*fft1_filtercorr[4*ia+3];
          fft1_float[4*ib+2]=t2;
          pxy[ia].x2=t1*t1+fft1_float[4*ib+1]*fft1_float[4*ib+1];
          pxy[ia].y2=t2*t2+fft1_float[4*ib+3]*fft1_float[4*ib+3];
          pxy[ia].im_xy=-t1*fft1_float[4*ib+3]+t2*fft1_float[4*ib+1];
          pxy[ia].re_xy=t1*t2+fft1_float[4*ib+1]*fft1_float[4*ib+3];
          fft1_sumsq[ic]=pxy[ia].x2+pxy[ia].y2;
          ic++;
          ib++;
          }
        }    
      }
    else
      {
      if(sw_onechan)
        {
        ib=pa/2+fft1_first_point;
        pwr=&fft1_power[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          t1=fft1_float[2*ib  ]*fft1_filtercorr[2*ia  ]-
             fft1_float[2*ib+1]*fft1_filtercorr[2*ia+1];
          fft1_float[2*ib+1]=fft1_float[2*ib+1]*fft1_filtercorr[2*ia  ]+
                             fft1_float[2*ib  ]*fft1_filtercorr[2*ia+1];
          fft1_float[2*ib]=t1;
          pwr[ia]=t1*t1+fft1_float[2*ib+1]*fft1_float[2*ib+1];
          fft1_sumsq[ic]+=pwr[ia];
          ic++;
          ib++;
          }
        }  
      else
        {
        ib=pa/4+fft1_first_point;
        pxy=&fft1_xypower[fft1_nb*fft1_size];
        for(ia=fft1_first_point; ia <= ix; ia++)
          {
          t1=fft1_float[4*ib  ]*fft1_filtercorr[4*ia  ]-
             fft1_float[4*ib+1]*fft1_filtercorr[4*ia+1];
          fft1_float[4*ib+1]=fft1_float[4*ib+1]*fft1_filtercorr[4*ia  ]+
                             fft1_float[4*ib  ]*fft1_filtercorr[4*ia+1];
          fft1_float[4*ib  ]=t1;
          t2=fft1_float[4*ib+2]*fft1_filtercorr[4*ia+2]-
             fft1_float[4*ib+3]*fft1_filtercorr[4*ia+3];
          fft1_float[4*ib+3]=fft1_float[4*ib+3]*fft1_filtercorr[4*ia+2]+
                             fft1_float[4*ib+2]*fft1_filtercorr[4*ia+3];
          fft1_float[4*ib+2]=t2;
          pxy[ia].x2=t1*t1+fft1_float[4*ib+1]*fft1_float[4*ib+1];
          pxy[ia].y2=t2*t2+fft1_float[4*ib+3]*fft1_float[4*ib+3];
          pxy[ia].im_xy=-t1*fft1_float[4*ib+3]+t2*fft1_float[4*ib+1];
          pxy[ia].re_xy=t1*t2+fft1_float[4*ib+1]*fft1_float[4*ib+3];
          fft1_sumsq[ic]+=pxy[ia].x2+pxy[ia].y2;
          ic++;
          ib++;
          }
        }    
      }
    }
  if(genparm[MAX_NO_OF_SPURS] > 0)
    {  
    if(sw_onechan)
      {
      pwr=&fft1_power[fft1_nb*fft1_size];
      if(spursearch_sum_counter > 3*spur_speknum)
        {
        spursearch_sum_counter=0;
        for(i=spur_search_first_point; i <= spur_search_last_point; i++)
          {
          spursearch_spectrum[i]=spursearch_powersum[i]+pwr[i];
          }
        spursearch_spectrum_cleanup();
        }
      else
        {
        if(spursearch_sum_counter == 0)
          {
          for(i=spur_search_first_point; i <= spur_search_last_point; i++)
            {
            spursearch_powersum[i]=pwr[i];
            }
          }
        else
          {
          for(i=spur_search_first_point; i <= spur_search_last_point; i++)
            {
            spursearch_powersum[i]+=pwr[i];
            }
          }
        spursearch_sum_counter++;
        }
      }
    else
      {
      pxy=&fft1_xypower[fft1_nb*fft1_size];
      if(spursearch_sum_counter > 3*spur_speknum)
        {
        spursearch_sum_counter=0;
        for(i=wg_first_point; i <= wg_last_point; i++)
          {
          spursearch_xysum[i].x2+=pxy[i].x2;
          spursearch_xysum[i].y2+=pxy[i].y2;
          spursearch_xysum[i].re_xy+=pxy[i].re_xy;
          spursearch_xysum[i].im_xy+=pxy[i].im_xy;
          t1=spursearch_xysum[i].x2+spursearch_xysum[i].y2;
          spursearch_spectrum[i]=t1+
                      2*(spursearch_xysum[i].re_xy*spursearch_xysum[i].re_xy+
                         spursearch_xysum[i].im_xy*spursearch_xysum[i].im_xy-
                         spursearch_xysum[i].x2*spursearch_xysum[i].y2)/t1;
          }
        spursearch_spectrum_cleanup();
        }
      else
        {
        if(spursearch_sum_counter == 0)
          {
          for(i=wg_first_point; i <= wg_last_point; i++)
            {
            spursearch_xysum[i].x2=pxy[i].x2;
            spursearch_xysum[i].y2=pxy[i].y2;
            spursearch_xysum[i].re_xy=pxy[i].re_xy;
            spursearch_xysum[i].im_xy=pxy[i].im_xy;
            }
          }
        else
          {
          for(i=wg_first_point; i <= wg_last_point; i++)
            {
            spursearch_xysum[i].x2+=pxy[i].x2;
            spursearch_xysum[i].y2+=pxy[i].y2;
            spursearch_xysum[i].re_xy+=pxy[i].re_xy;
            spursearch_xysum[i].im_xy+=pxy[i].im_xy;
            }
          }
        spursearch_sum_counter++;
        }
      }
    }
  }
fft1_sumsq_counter++;
if(fft1_sumsq_counter >= wg.fft_avg1num)
  {
  fft1_sumsq_counter=0;
// We have a new average power spectrum.
// In case the second fft is enabled we have to set up information
// for the selective limiter.
  if(genparm[SECOND_FFT_ENABLE] != 0)
    {
    fft1_update_liminfo();
    }
  fft1_sumsq_pa=(fft1_sumsq_pa+fft1_size)&fft1_sumsq_mask;   
  }
fft1_nb=(fft1_nb+1)&fft1n_mask;
fft1_pb=fft1_nb*fft1_block;
if(genparm[SECOND_FFT_ENABLE] == 0)ag_pa=(ag_pa+1)&ag_mask;  
}

void set_fft1_endpoints(void)
{
// Set all pointers for a non-inverted spectrum.
wg_first_point=wg.first_xpoint;
wg_last_point=wg.first_xpoint+wg.xpoints;
if(wg_last_point >= fft1_size)wg_last_point=fft1_size-1;
fft1_first_point=0;
fft1_first_inband=0;
fft1_last_point=fft1_size-1;
fft1_last_inband=fft1_size-1;
if( (ui.network_flag&NET_RXOUT_FFT1) == 0 &&
     genparm[SECOND_FFT_ENABLE] == 0 )
  {
// In case second fft is not enabled, there is no reason to calculate
// the spectrum outside the range that we use on the display when
// we do not need fft1 transforms for the network.
  fft1_first_point=wg_first_point;
  fft1_last_point=wg_last_point;
  
  }
else
  {
  if( (fft1_calibrate_flag&CALAMP)==CALAMP)
    {
    while(fft1_desired[fft1_first_inband]<0.5 && 
                          fft1_first_inband<fft1_size-1) fft1_first_inband++;
    while(fft1_desired[fft1_last_inband]<0.5 && 
                           fft1_last_inband > 0) fft1_last_inband--;
    if(fft1_last_inband-fft1_first_inband<fft1_size/32)lirerr(1057);
    }
  }
fft1_sumsq_recalc=fft1_first_point;
fft1_first_sym_point=fft1_size-1-fft1_last_point;
if(fft1_first_sym_point > fft1_first_point) 
                                     fft1_first_sym_point=fft1_first_point;
fft1_last_sym_point=fft1_size-1-fft1_first_sym_point;
}

void make_filcorrstart(void)
{
fft1_filtercorr_start=150*fft1_size*pow((float)(fft1_size),-0.4);
if( (ui.rx_input_mode&DWORD_INPUT) != 0)
  {
  fft1_filtercorr_start*=4096;
  if( fft_cntrl[FFT1_CURMODE].permute == 2)fft1_filtercorr_start*=16;
  if( (ui.rx_input_mode&IQ_DATA) != 0)
    {
    fft1_filtercorr_start*=12;
    }
  }
fft1_filtercorr_start=genparm[FIRST_FFT_GAIN]/fft1_filtercorr_start;
}

void clear_fft1_filtercorr(void)
{
float t1, t2, t3;
int i, j, k, mm;
fft1_calibrate_flag&=CALIQ;
settextcolor(8);
if(ui.newcomer_mode == 0)
  {
  for(i=3; i<20; i++)lir_text(40,i,"FILTERS NOT CALIBRATED!!");
  }
make_filcorrstart();
settextcolor(7);
mm=twice_rxchan;
for(i=0; i<fft1_size; i++)
  {
  fft1_desired[i]=1;
  for(j=0; j<mm; j+=2)  
    {
    fft1_filtercorr[mm*i+j  ]=fft1_filtercorr_start;
    fft1_filtercorr[mm*i+j+1]=0;
    }
  }  
// A DC offset on the A/D converters cause signals
// at frequency=0 and at frequency=fft1_size/2. 
// These frequencies are not properly separated
// by the backwards fft in case a window N other
// than 0 or 2 is used.
// Signals very close to the Nyquist frequency
// are artifacts anyway so we set the desired response
// to filter them out.
  {     
  t1=0.125*PI_L;
  t2=0;
  i=0;
  k=fft1_size-1;
  while(t2 < 0.5*PI_L)
    {
    t3=pow(sin(t2),2);
    if(t2<0)t3=0;
    if( (ui.rx_input_mode&IQ_DATA) != 0)
      {
      fft1_desired[i]=t3;
      for(j=0; j<mm; j+=2)fft1_filtercorr[mm*i+j  ]=t3*fft1_filtercorr_start;
      }
    fft1_desired[k]=t3;
    for(j=0; j<mm; j+=2)fft1_filtercorr[mm*k+j  ]=t3*fft1_filtercorr_start;
    t2+=t1;
    i++;
    k--;
    }
  }  
}

void clear_iq_foldcorr(void)
{
int i, mm;
settextcolor(8);
if(ui.newcomer_mode == 0)
  {
  for(i=3; i<20; i++)lir_text(10,i,"I/Q PHASE NOT CALIBRATED!!");
  }
mm=fft1_size*twice_rxchan;
for(i=0; i<mm; i++)fft1_foldcorr[i]=0;
fft1_calibrate_flag&=CALAMP;
settextcolor(7);
bal_updflag=1;
}

void use_iqcorr(void)
{
cal_table=malloc(fft1_size*sizeof(COSIN_TABLE )/2);
cal_permute=malloc(fft1_size*sizeof(short int));
if( cal_table == NULL || cal_permute == NULL)
  {
  lirerr(1110);
  return;
  }
init_fft(0,fft1_n, fft1_size, cal_table, cal_permute);
expand_foldcorr(fft1_foldcorr,fftw_tmp);
free(cal_table);
free(cal_permute);
if(DISREGARD_RAWFILE_IQCORR)
  {
  fft1_calibrate_flag&=CALAMP;
  }
else
  {  
  fft1_calibrate_flag|=CALIQ;
  }
}

void init_foldcorr(void)
{
int rdbuf[10],chkbuf[10];
int i, kk;
FILE *file;
char s[80];
#if SAVE_RAWFILE_IQCORR == TRUE
FILE *new_corr_file;
new_corr_file=NULL;
#endif
if(  (ui.rx_input_mode&DIGITAL_IQ) != 0 || (ui.rx_input_mode&IQ_DATA) == 0)
  {
  return;
  }
make_iqcorr_filename(s);
if(diskread_flag == 2 && (save_init_flag&2) == 2)
  {
  file=save_rd_file;
#if SAVE_RAWFILE_IQCORR == TRUE
  new_corr_file=fopen(s, "wb");
  if(new_corr_file == NULL)
    {
    lirerr(594222);
    return;
    }
#endif
  }
else
  {
  file = fopen(s, "rb");
  }
if (file == NULL)
  {
  clear_iq_foldcorr();
  return;
  }
else
  {
  i=fread(rdbuf, sizeof(int),10,file);
  if(i != 10)goto corrupt;
#if SAVE_RAWFILE_IQCORR == TRUE
  if(new_corr_file != NULL)
    {
    i=fwrite(rdbuf, sizeof(int),10,new_corr_file);
    if(i != 10)
      {
new_corrfile_errx:;
      fclose(new_corr_file);
      lirerr(497326);
      return;
      }    
    }
#endif
  if( rdbuf[1] != (ui.rx_input_mode&IQ_DATA) ||
      rdbuf[2] != ui.rx_ad_speed ||
      rdbuf[3] != ui.rx_rf_channels ||
      rdbuf[4] != FOLDCORR_VERNR )
    {
    if(diskread_flag == 2 && (save_init_flag&2) == 2)
      {
corrupt:;
      lirerr(1060);
      return;
      }
    clear_iq_foldcorr();
    }
  else  
    {  
    bal_segments=rdbuf[0];
    kk=twice_rxchan*sizeof(float)*4*bal_segments;
    contracted_iq_foldcorr=malloc(kk);
    if(contracted_iq_foldcorr == NULL)
      {
      lirerr(1171);
      return;
      }
    if(kk != (int)fread(contracted_iq_foldcorr, 1, kk, file))goto err;
#if SAVE_RAWFILE_IQCORR == TRUE
    if(new_corr_file != NULL)
      {
      if(kk != (int)fwrite(contracted_iq_foldcorr, 1, kk,  new_corr_file))
                                                  goto new_corrfile_errx;
      }
#endif
    if(10 != fread(chkbuf, sizeof(int),10,file))goto err;
#if SAVE_RAWFILE_IQCORR == TRUE
    if(new_corr_file != NULL)
      {
      if(10 != fwrite(chkbuf, sizeof(int),10, new_corr_file))
                                                  goto new_corrfile_errx;
      }
#endif
    for(i=0; i<10; i++)
      {
      if(rdbuf[i]!=chkbuf[i])
        {
err:;      
        lirerr(1053);
        goto exx;
        }
      }
#if SAVE_RAWFILE_IQCORR == TRUE
    if(new_corr_file != NULL)fclose(new_corr_file);
#endif
    if(diskread_flag != 2 || (save_init_flag&2) == 0)fclose(file);
    use_iqcorr();
exx:;
    free(contracted_iq_foldcorr);
    }
  }
}


void resize_filtercorr_td_to_fd(int to_fd, int size_in, float *buf_in, 
                       int n_out, int size_out, float *buf_out)
{
int ka, kb;
int i, j, k, mm;
int mask, ia, ib;
double sum[4],isum[4];
D_COSIN_TABLE *out_ffttab; 
unsigned short int *out_permute;
double *ww;
float t1, t2, t3, t4, r1, r2;
if(size_in < 8)lirerr(1161);
mm=twice_rxchan;
out_ffttab=malloc(size_out*sizeof(D_COSIN_TABLE )/2);
if( out_ffttab == NULL)
  {
mem_err1:;
  lirerr(1141);
  return;
  }
out_permute=malloc(size_out*sizeof(short int));
if(out_permute == NULL)
  {
mem_err2:;  
  free(out_ffttab);
  goto mem_err1;
  }
ww=malloc(mm*size_out*sizeof(double));
if(ww == NULL)
  {
  free(out_permute);
  goto mem_err2;
  }
init_d_fft(0, n_out, size_out, out_ffttab, out_permute);
#define ZZ 1000000000.
for(j=0; j<mm; j+=2)
  {
  if(size_out > size_in)
    {
// Increase the number of points from size_in to size_out.  
// Store the impulse response at buf_out[0]
// Fill zeroes in the undefined points.
    for(i=0; i<size_in/2; i++)
      {
      ww[2*i  ]=buf_in[mm*i+j  ];
      ww[2*i+1]=buf_in[mm*i+j+1];
      }
    for(i=size_in/2; i<size_in; i++)
      {
      k=i+size_out-size_in;
      ww[2*k  ]=buf_in[mm*i+j  ];
      ww[2*k+1]=buf_in[mm*i+j+1];
      }
    for(i=size_in/2; i<size_out-size_in/2; i++)
      {
      ww[2*i  ]=0;
      ww[2*i+1]=0;
      }
// The fourier transform has to be periodic. We have now introduced
// two discontinuities in case the pulse response is long enough
// to not have fallen off completely at the midpoint of the input waveform.
// To remove the discontinuity, make a smooth transition to
// the zero region using the periodicity of the transform.
    ia=size_in/2-1;
    ib=ia+1;
    ka=size_out-size_in/2;
    kb=ka-1;
    t3=0.25*PI_L;
    t4=10*t3/size_in;
    while(t3 >0 && ia >= 0)
      {
      t1=ww[2*ia];
      t2=ww[2*ka];
      r1=pow(sin(t3),2.0);
      r2=pow(cos(t3),2.0);
      ww[2*ib]=r1*t2;
      ww[2*kb]=r1*t1;
      ww[2*ia]=r2*t1+r1*t2;
      ww[2*ka]=r1*t1+r2*t2;
      t1=ww[2*ia+1];
      t2=ww[2*ka+1];
      ww[2*ib+1]=r1*t2;
      ww[2*kb+1]=r1*t1;
      ww[2*ia+1]=r2*t1+r1*t2;
      ww[2*ka+1]=r1*t1+r2*t2;
      ia--;
      ka++;
      ib++;
      kb--;
      t3-=t4;
      }
    }
  else
    {
    for(i=0; i<size_out/2; i++)
      {    
      ww[2*i  ]=buf_in[mm*i+j  ];
      ww[2*i+1]=buf_in[mm*i+j+1];
      }
    for(i=size_out/2; i<size_out; i++)
      {    
      k=size_in-size_out+i;
      ww[2*i  ]=buf_in[mm*k+j  ];
      ww[2*i+1]=buf_in[mm*k+j+1];
      }     
// When limiting the pulse response in the time domain, we may
// introduce a discontinuity. It will occur between size_out/2 and
// fft1/size/2-1.
// Differentiate the pulse response starting at the discontinuity:
    mask=size_out-1;
    ia=size_out/2;
    ib=ia+1;
    for(i=1; i<size_out; i++)
      {
      ww[ia*2  ]=ww[2*ia  ]-ww[2*ib  ];
      ww[ia*2+1]=ww[2*ia+1]-ww[2*ib+1];
      ia=(ia+1)&mask;
      ib=(ib+1)&mask;
      }  
    ww[ia*2  ]=0;
    ww[ia*2+1]=0;
    sum[j  ]=0;
    sum[j+1]=0;
// Get the DC component so we can remove it from the derivative
    for(i=0; i<size_out; i++)
      {
      sum[j  ]+=ww[2*i  ];
      sum[j+1]+=ww[2*i+1];
      }
    sum[j]/=size_out;
    isum[j]=0;
    sum[j+1]/=size_out;
    isum[j+1]=0;
// Integrate the pulse response:
    for(i=0; i<size_out; i++)
      {
      isum[j]+=ww[2*ia  ]-sum[j];
      ww[2*ia  ]=isum[j];
      isum[j+1]+=ww[2*ia+1]-sum[j+1];
      ww[2*ia+1]=isum[j+1];
      ia=(ia+mask)&mask;
      }
    }
// Take the back transform to get the calibration function in 
// size_out points in the frequency domain.    
  if(to_fd)d_fftback(size_out, n_out, ww, out_ffttab, out_permute);
  for(i=0; i<size_out; i++)
    {
    buf_out[mm*i+j  ]=ww[2*i  ];
    buf_out[mm*i+j+1]=ww[2*i+1];
    }
  }
free(out_ffttab);
free(out_permute);
free(ww);
}

void resize_fft1_desired(int siz_in, float *buf_in, 
                         int siz_out, float *buf_out)
{
int i, j, k, m, mm;
float t1, t2;
mm=twice_rxchan;
if(siz_out > siz_in)
  {
  m=siz_out/siz_in;
  t2=0;
  for(j=siz_in; j>0; j--)
    {
    t1=t2;
    t2=buf_in[j-1];
    for(i=0; i<m; i++)
      {
      buf_out[m*j-i-1]=t1+i*(t2-t1)/m;
      }
    }
  buf_out[0]=0;
  }
else
  {
// Reduce buf_out.      
  m=siz_in/siz_out;
  i=m/2;
  buf_out[0]=0;
  for(j=1; j<siz_out; j++)
    {
    buf_out[j]=0;
    for(k=0; k<m; k++)
      {
      buf_out[j]+=buf_in[i];
      i++;
      }      
    buf_out[j]/=m;
    }
  }  
}


void use_filtercorr_td(int cal_size, float *corr, float *desired)
{
int i, j, k, mm;
mm=twice_rxchan;
resize_filtercorr_td_to_fd(TRUE, 
                      cal_size, corr, fft1_n, fft1_size, fft1_filtercorr); 
resize_fft1_desired(cal_size, desired, fft1_size, fft1_desired);
i=0;
while(fft1_desired[i] == 0 && i<fft1_size)
  {
  for(j=0; j<mm; j++) fft1_filtercorr[mm*i+j]=0;
  i++;
  }
k=fft1_size-1;
while(fft1_desired[k] == 0 && k>=0)
  {
  for(j=0; j<mm; j++) fft1_filtercorr[mm*k+j]=0;
  k--;
  }
if(k-i < fft1_size/16+16)
  {
  lirerr(1259);
  return;
  }   
normalise_fft1_filtercorr(fft1_filtercorr);
make_filcorrstart();    
for(i=0; i<mm*fft1_size; i++)
  {
  fft1_filtercorr[i]*=fft1_filtercorr_start;
  } 
for(i=0; i<fft1_size; i++)wg_waterf_sum[i]=0.00001;
fft1_calibrate_flag|=CALAMP;
}

void convert_filtercorr_fd_to_td(int n, int siz, float *buf)
{
int i, j, mm;
D_COSIN_TABLE *out_ffttab; 
unsigned short int *out_permute;
double *ww;
mm=twice_rxchan;
out_ffttab=malloc(siz*sizeof(D_COSIN_TABLE )/2);
if( out_ffttab == NULL)
  {
mem_err1:;
  lirerr(1142);
  return;
  }
out_permute=malloc(siz*sizeof(short int));
if(out_permute == NULL)
  {
mem_err2:;  
  free(out_ffttab);
  goto mem_err1;
  }
ww=malloc(mm*siz*sizeof(double));
if(ww == NULL)
  {
  free(out_permute);
  goto mem_err2;
  }
init_d_fft(0, n, siz, out_ffttab, out_permute);
for(j=0; j<mm; j+=2)
  {
  for(i=0; i<siz; i++)
    {
    ww[2*i  ]=buf[mm*i+j  ];
    ww[2*i+1]=buf[mm*i+j+1];
    }
  d_fftforward(siz, n, ww, out_ffttab, out_permute);
  for(i=0; i<siz; i++)
    {
    buf[mm*i+j  ]=ww[2*i  ];
    buf[mm*i+j+1]=ww[2*i+1];
    }
  }
}



void use_filtercorr_fd(int cal_n, int cal_size, 
                                          float *cal_corr, float *cal_desired)
{
convert_filtercorr_fd_to_td( cal_n, cal_size, cal_corr);
use_filtercorr_td(cal_size, cal_corr, cal_desired);
}

void init_fft1_filtercorr(void)
{
float *tmpbuf;
int rdbuf[10], chkbuf[10];
int i, mm;
FILE *file;
char s[80];
float t1;
#if DISREGARD_RAWFILE_CORR == TRUE
int disregard_flag;
#endif
#if SAVE_RAWFILE_CORR == TRUE
FILE *new_corr_file;
new_corr_file=NULL;
#endif
#if DISREGARD_RAWFILE_CORR == TRUE
#if SAVE_RAWFILE_CORR == TRUE
lirerr(713126);
#endif
disregard_flag=0;
#endif
file=NULL;
fft1_filtercorr_direction=1;
mm=twice_rxchan;
make_filfunc_filename(s);
if(diskread_flag == 2 && (save_init_flag&1) == 1)
  {
  file=save_rd_file;
#if SAVE_RAWFILE_CORR == TRUE
  new_corr_file=fopen(s, "wb");
  if(new_corr_file == NULL)
    {
    lirerr(594221);
    return;
    }
#endif
  }
else
  {
  file = fopen(s, "rb");
  }
if (file == NULL)
  {
#if DISREGARD_RAWFILE_CORR == TRUE
disregard:;
#endif
  clear_fft1_filtercorr();
  return;
  }
i=fread(rdbuf, sizeof(int),10,file);
if(i != 10)goto calerr;
#if SAVE_RAWFILE_CORR == TRUE
if(new_corr_file != NULL)
  {
  i=fwrite(rdbuf, sizeof(int),10,new_corr_file);
  if(i != 10)
    {
new_corrfile_errx:;
    fclose(new_corr_file);
    lirerr(497325);
    return;
    }    
  }
#endif
t1=rdbuf[5]-ui.rx_ad_speed;
t1=fabs(200*t1/(rdbuf[5]+ui.rx_ad_speed));
// t1 is now the speed error in %.
if( rdbuf[2] <0 ||
    rdbuf[2] >= MAX_RX_MODE ||
    (rdbuf[3]&IQ_DATA) != (ui.rx_input_mode&IQ_DATA) ||
    t1 > 0.05 ||
    rdbuf[6] != ui.rx_rf_channels ||
    rdbuf[8] != 0 ||
    rdbuf[9] != 0 ) 
  {
calerr:;
  if(diskread_flag == 2 && (save_init_flag&1) == 1)
    {
    lirerr(1059);
    return;
    }
  fclose(file);  
  clear_fft1_filtercorr();
#if SAVE_RAWFILE_CORR == TRUE
if(new_corr_file != NULL)fclose(new_corr_file);
#endif
  return;
  }
if(rdbuf[7] == 0)
  {
  cal_fft1_n=rdbuf[0];
  cal_fft1_size=rdbuf[1];
// The filtercorr function is saved in the frequency domain in 
// rdbuf[1] points.
// This format is used during the initialisation procedures and
// it may be kept for normal operation in case the number of points
// will not be reduced by saving in the time domain.  
  tmpbuf=malloc(rdbuf[1]*(mm+1)*sizeof(float));
  if( tmpbuf == NULL )
    {
    lirerr(1143);
    return;
    }
  i=fread(tmpbuf, mm*sizeof(float), rdbuf[1], file);
  if(i != rdbuf[1])goto calerr;
#if SAVE_RAWFILE_CORR == TRUE
  if(new_corr_file != NULL)
    {
    i=fwrite(tmpbuf, mm*sizeof(float), rdbuf[1], new_corr_file);
    if(i != rdbuf[1])goto new_corrfile_errx;
    }
#endif
  i=fread(&tmpbuf[mm*rdbuf[1]], sizeof(float), rdbuf[1], file);
  if(i != rdbuf[1])goto calerr;
#if SAVE_RAWFILE_CORR == TRUE
  if(new_corr_file != NULL)
    {
    i=fwrite(&tmpbuf[mm*rdbuf[1]], sizeof(float), rdbuf[1], new_corr_file);
    if(i != rdbuf[1])goto new_corrfile_errx;
    }
#endif
  use_filtercorr_fd(rdbuf[0], rdbuf[1], tmpbuf, &tmpbuf[mm*rdbuf[1]]);
  if(kill_all_flag)return;
  }
else
  {
// The correction function was stored in the time domain in rdbuf[7] points
  tmpbuf=malloc(rdbuf[7]*(mm+1)*sizeof(float));
  if( tmpbuf == NULL )
    {
    lirerr(1292);
    return;
    }
  cal_fft1_size=rdbuf[7];
  cal_fft1_n=make_power_of_two(&cal_fft1_size);
  i=fread(tmpbuf, mm*sizeof(float), rdbuf[7], file);
  if(i != rdbuf[7])goto calerr;
#if SAVE_RAWFILE_CORR == TRUE
  if(new_corr_file != NULL)
    {
    i=fwrite(tmpbuf, mm*sizeof(float), rdbuf[7], new_corr_file);
    if(i != rdbuf[7])goto new_corrfile_errx;
    }
#endif
  i=fread(&tmpbuf[mm*rdbuf[7]], sizeof(float), rdbuf[7], file);
  if(i != rdbuf[7])goto calerr;
#if SAVE_RAWFILE_CORR == TRUE
  if(new_corr_file != NULL)
    {
    i=fwrite(&tmpbuf[mm*rdbuf[7]], sizeof(float), rdbuf[7], new_corr_file);
    if(i != rdbuf[7])goto new_corrfile_errx;
    }
#endif
  use_filtercorr_td(rdbuf[7], tmpbuf, &tmpbuf[mm*rdbuf[7]]);
  if(kill_all_flag)return;
  }
if(10 != fread(chkbuf, sizeof(int),10,file))goto exx;
#if SAVE_RAWFILE_CORR == TRUE
  if(new_corr_file != NULL)
    {
    i=fwrite(chkbuf, sizeof(int),10, new_corr_file);
    if(i != 10)goto new_corrfile_errx;
    }
#endif
for(i=0; i<10; i++)
  {
  if(rdbuf[i]!=chkbuf[i])
    {
exx:;    
    if(diskread_flag==2)lirerr(1198); else lirerr(1052);
    return;
    }
  }
#if SAVE_RAWFILE_CORR == TRUE
if(new_corr_file != NULL)fclose(new_corr_file);
#endif
if(diskread_flag == 2 && (save_init_flag&1) == 1)
  {
#if DISREGARD_RAWFILE_CORR == TRUE
  disregard_flag++;
  if(disregard_flag < 2)
    {
    file = fopen(s, "rb");
    goto disregard;
    }
  fclose(file);
#endif
  return;
  }
fclose(file);
}

