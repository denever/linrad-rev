
#include "globdef.h"
#include "uidef.h"
#include "seldef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "screendef.h"
#include "sigdef.h"

#define BWFAC 0.03  // 3% of the bandwidth for AFC 2nd/3rd order modulation.

// The mix1 routines shift selected frequency bands to the baseband.
// Since we have overlapping fourier transforms already there is no
// need to multiply with the cos/sin table - we just select some
// lines in the fft and make a back transformation.
// We do that with a reduced transform size and get the reduced
// sampling rate that we want at the reduced bandwidth automatically.

float phrot_step_save=0;

void do_mix1(int ss, float dfq)
{
int i, j, n, pa, k, mm, ia, ib, ic, id;
int p0, poffs;
float t1,t2,t3,t4,r1,r2,r3,r4,w1,w2,a1,a2;
float old_phrot_step,phrot_step;
phrot_step=dfq*fftx_points_per_hz*2*PI_L/mix1.size;
old_phrot_step=phrot_step_save;
phrot_step_save=phrot_step;
mm=twice_rxchan;
k=mm*mix1.interleave_points/2;
poffs=ss*timf3_size;        
pa=timf3_pa+poffs;        
// There is a lot of memory in our computers but speed may be a problem
// Process in different loops depending on no of rx channels.
i=1;
j=mix1.size-2;
n=mix1.size/2-2;
if(sw_onechan)
  {
  while(j>i)
    {
    t1=mix1_fqwin[n];
    fftn_tmp[2*i  ]*=t1;      
    fftn_tmp[2*i+1]*=t1;      
    fftn_tmp[2*j  ]*=t1;      
    fftn_tmp[2*j+1]*=t1;      
    j--;
    n--;
    i++;
    }
  fftback(mix1.size, mix1.n, fftn_tmp, 
                             mix1.table, mix1.permute, yieldflag_ndsp_mix1);
// In case there is no window the whole thing is trival.
// Just place the back transforms after each other.
  if(mix1.interleave_points == 0)
    {
    t2=mix1_phase_rot[ss];
    t1=mix1_phase[ss];
    j=2*mix1.size;
    for(i=0; i<j; i+=2)
      {
      t3=sin(t1);
      t4=cos(t1);
      timf3_float[pa+i  ]=t4*fftn_tmp[i+k  ]-t3*fftn_tmp[i+k+1];     
      timf3_float[pa+i+1]=t4*fftn_tmp[i+k+1]+t3*fftn_tmp[i+k  ];     
      t1+=t2;
      }
    mix1_phase[ss]=t1;
    }
  else  
    {
// If a sin squared window is used, place the transforms with 50%
// overlap and add them together. sin squared + cos squared ==1
    pa=timf3_pa+poffs;        
    if(mix1.interleave_points == mix1.new_points)
      {
      ia=mix1.size;
      t2=mix1_phase_rot[ss];
      t1=mix1_phase[ss];
      r1=mix1_old_phase[ss];
      r2=t2-2*(mix1_old_point[ss]-mix1_point[ss])*PI_L/mix1.size;
      r2-=old_phrot_step/4;
      t2-=phrot_step/4;
      old_phrot_step/=mix1.size;
      phrot_step/=mix1.size;
      for(i=0; i<ia; i+=2)
        {
        t3=sin(t1);
        t4=cos(t1);
        r3=sin(r1);
        r4=cos(r1);
        a1=timf3_float[pa+i  ];
        a2=timf3_float[pa+i+1];
        timf3_float[pa+i  ]=r4*a1-r3*a2+t4*fftn_tmp[i  ]-t3*fftn_tmp[i+1];
        timf3_float[pa+i+1]=r4*a2+r3*a1+t4*fftn_tmp[i+1]+t3*fftn_tmp[i  ];
        r1+=r2;
        t1+=t2;
        r2-=old_phrot_step;
        t2+=phrot_step;
        }
      mix1_phase[ss]=t1;
      pa=((timf3_pa+timf3_block)&timf3_mask)+2*ss*timf3_size-timf3_block;        
      ib=2*mix1.size;
      for(i=ia; i<ib; i+=2)
        {
        timf3_float[pa+i  ]=fftn_tmp[i  ];     
        timf3_float[pa+i+1]=fftn_tmp[i+1];     
        }
      }
    else
      {
      p0=timf3_pa;        
      mix1_phase[ss]-=(mix1_old_point[ss]-mix1_point[ss])*
                              ((mix1.new_points-mix1.size)*PI_L)/mix1.size;
      t2=mix1_phase_rot[ss];
      t1=mix1_phase[ss];
      r1=mix1_old_phase[ss];
      r2=t2-2*(mix1_old_point[ss]-mix1_point[ss])*PI_L/mix1.size;
      r2+=old_phrot_step/4;
      t2+=phrot_step/4;
      old_phrot_step/=mix1.size;
      phrot_step/=mix1.size;
      k-=2*(mix1.crossover_points/2);
      j=k/2+mix1.crossover_points;
      ia=2*mix1.crossover_points;
      for(i=0; i<ia; i+=2)
        {
        pa=p0+poffs;
        t3=sin(t1);
        t4=cos(t1);
        r3=sin(r1);
        r4=cos(r1);
        w1=mix1.sin2win[i>>1];
        w2=mix1.cos2win[i>>1];
        a1=w2*timf3_float[pa  ];
        a2=w2*timf3_float[pa+1];
        timf3_float[pa  ]=r4*a1-r3*a2+
                               (t4*fftn_tmp[i+k  ]-t3*fftn_tmp[i+k+1])*w1;     
        timf3_float[pa+1]=r4*a2+r3*a1+
                               (t4*fftn_tmp[i+k+1]+t3*fftn_tmp[i+k  ])*w1;     
        t1+=t2;
        r1+=r2;
        r2+=old_phrot_step;
        t2+=phrot_step;
        p0=(p0+2)&timf3_mask;
        } 
      ib=mix1.new_points+2+2*(mix1.crossover_points/2);
      for(i=ia; i<ib; i+=2)
        {
        t3=sin(t1);
        t4=cos(t1);
        pa=p0+poffs;
        r1=mix1.window[j];
        timf3_float[pa  ]=(t4*fftn_tmp[i+k  ]-t3*fftn_tmp[i+k+1])*r1;     
        timf3_float[pa+1]=(t4*fftn_tmp[i+k+1]+t3*fftn_tmp[i+k  ])*r1;     
        p0=(p0+2)&timf3_mask;
        t1+=t2;
        t2+=phrot_step;
        j++;
        } 
      j--;  
      ic=2*mix1.new_points;  
      for(i=ib; i<ic; i+=2)
        {
        j--;
        pa=p0+poffs;
        t3=sin(t1);
        t4=cos(t1);
        r1=mix1.window[j];
        timf3_float[pa  ]=(t4*fftn_tmp[i+k  ]-t3*fftn_tmp[i+k+1])*r1;     
        timf3_float[pa+1]=(t4*fftn_tmp[i+k+1]+t3*fftn_tmp[i+k  ])*r1;     
        t1+=t2;
        t2+=phrot_step;
        p0=(p0+2)&timf3_mask;
        } 
      mix1_phase[ss]=t1;
      id=2*(mix1.crossover_points+mix1.new_points);
      for(i=ic; i<id; i+=2)
        {
        j--;
        pa=p0+poffs;
        timf3_float[pa  ]=fftn_tmp[i+k  ];     
        timf3_float[pa+1]=fftn_tmp[i+k+1];     
        p0=(p0+2)&timf3_mask;
        } 
      }
    }
  }
else
  {  
  while(j>i)
    {
    t1=mix1_fqwin[n];
    fftn_tmp[4*i  ]*=t1;      
    fftn_tmp[4*i+1]*=t1;      
    fftn_tmp[4*i+2]*=t1;      
    fftn_tmp[4*i+3]*=t1;      
    fftn_tmp[4*j  ]*=t1;      
    fftn_tmp[4*j+1]*=t1;      
    fftn_tmp[4*j+2]*=t1;      
    fftn_tmp[4*j+3]*=t1;      
    j--;
    n--;
    i++;
    }
  dual_fftback(mix1.size, mix1.n, fftn_tmp, 
                               mix1.table, mix1.permute, yieldflag_ndsp_mix1);
  if(mix1.interleave_points == 0)
    {
    t2=mix1_phase_rot[ss];
    t1=mix1_phase[ss];
    j=4*mix1.size;
    for(i=0; i<j; i+=4)
      {
      t3=sin(t1);
      t4=cos(t1);
      timf3_float[pa+i  ]=t4*fftn_tmp[i+k  ]-t3*fftn_tmp[i+k+1];     
      timf3_float[pa+i+1]=t4*fftn_tmp[i+k+1]+t3*fftn_tmp[i+k  ];     
      timf3_float[pa+i+2]=t4*fftn_tmp[i+k+2]-t3*fftn_tmp[i+k+3];     
      timf3_float[pa+i+3]=t4*fftn_tmp[i+k+3]+t3*fftn_tmp[i+k+2];     
      t1+=t2;
      }
    mix1_phase[ss]=t1;
    }
  else  
    {
    pa=timf3_pa+poffs;        
    if(mix1.interleave_points == mix1.new_points)
      {
      ia=2*mix1.size;
      t2=mix1_phase_rot[ss];
      t1=mix1_phase[ss];
      r1=mix1_old_phase[ss];
      r2=t2-2*(mix1_old_point[ss]-mix1_point[ss])*PI_L/mix1.size;
      r2-=old_phrot_step/4;
      t2-=phrot_step/4;
      old_phrot_step/=mix1.size;
      phrot_step/=mix1.size;
      for(i=0; i<ia; i+=4)
        {
        t3=sin(t1);
        t4=cos(t1);
        r3=sin(r1);
        r4=cos(r1);
        a1=timf3_float[pa+i  ];
        a2=timf3_float[pa+i+1];
        timf3_float[pa+i  ]=r4*a1-r3*a2+t4*fftn_tmp[i  ]-t3*fftn_tmp[i+1];
        timf3_float[pa+i+1]=r4*a2+r3*a1+t4*fftn_tmp[i+1]+t3*fftn_tmp[i  ];
        a1=timf3_float[pa+i+2];
        a2=timf3_float[pa+i+3];
        timf3_float[pa+i+2]=r4*a1-r3*a2+t4*fftn_tmp[i+2]-t3*fftn_tmp[i+3];
        timf3_float[pa+i+3]=r4*a2+r3*a1+t4*fftn_tmp[i+3]+t3*fftn_tmp[i+2];
        r1+=r2;
        t1+=t2;
        r2-=old_phrot_step;
        t2+=phrot_step;
        }
      mix1_phase[ss]=t1;
      pa=((timf3_pa+timf3_block)&timf3_mask)+4*ss*timf3_size-timf3_block;        
      ib=4*mix1.size;
      for(i=ia; i<ib; i+=4)
        {
        timf3_float[pa+i  ]=fftn_tmp[i  ];     
        timf3_float[pa+i+1]=fftn_tmp[i+1];     
        timf3_float[pa+i+2]=fftn_tmp[i+2];     
        timf3_float[pa+i+3]=fftn_tmp[i+3];     
        }
      }
    else
      {
      p0=timf3_pa;        
      mix1_phase[ss]-=(mix1_old_point[ss]-mix1_point[ss])*
                             ((mix1.new_points-mix1.size)*PI_L)/mix1.size;
      t2=mix1_phase_rot[ss];
      t1=mix1_phase[ss];
      r1=mix1_old_phase[ss];
      r2=t2-2*(mix1_old_point[ss]-mix1_point[ss])*PI_L/mix1.size;
      r2+=old_phrot_step/4;
      t2+=phrot_step/4;
      old_phrot_step/=mix1.size;
      phrot_step/=mix1.size;
      k-=4*(mix1.crossover_points/2);
      j=k/4+mix1.crossover_points;
      ia=4*mix1.crossover_points;
      for(i=0; i<ia; i+=4)
        {
        pa=p0+poffs;
        t3=sin(t1);
        t4=cos(t1);
        r3=sin(r1);
        r4=cos(r1);
        w1=mix1.sin2win[i>>2];
        w2=mix1.cos2win[i>>2];
        a1=w2*timf3_float[pa  ];
        a2=w2*timf3_float[pa+1];
        timf3_float[pa  ]=r4*a1-r3*a2+
                               (t4*fftn_tmp[i+k  ]-t3*fftn_tmp[i+k+1])*w1;     
        timf3_float[pa+1]=r4*a2+r3*a1+
                               (t4*fftn_tmp[i+k+1]+t3*fftn_tmp[i+k  ])*w1;     
        a1=w2*timf3_float[pa+2];
        a2=w2*timf3_float[pa+3];
        timf3_float[pa+2]=r4*a1-r3*a2+
                               (t4*fftn_tmp[i+k+2]-t3*fftn_tmp[i+k+3])*w1;     
        timf3_float[pa+3]=r4*a2+r3*a1+
                               (t4*fftn_tmp[i+k+3]+t3*fftn_tmp[i+k+2])*w1;     
        t1+=t2;
        r1+=r2;
        r2+=old_phrot_step;
        t2+=phrot_step;

        p0=(p0+4)&timf3_mask;
        } 
      ib=2*mix1.new_points+4+4*(mix1.crossover_points/2);;
      for(i=ia; i<ib; i+=4)
        {
        t3=sin(t1);
        t4=cos(t1);
        pa=p0+poffs;
        r1=mix1.window[j];
        timf3_float[pa  ]=(t4*fftn_tmp[i+k  ]-t3*fftn_tmp[i+k+1])*r1;     
        timf3_float[pa+1]=(t4*fftn_tmp[i+k+1]+t3*fftn_tmp[i+k  ])*r1;     
        timf3_float[pa+2]=(t4*fftn_tmp[i+k+2]-t3*fftn_tmp[i+k+3])*r1;     
        timf3_float[pa+3]=(t4*fftn_tmp[i+k+3]+t3*fftn_tmp[i+k+2])*r1;
        p0=(p0+4)&timf3_mask;
        t1+=t2;
        t2+=phrot_step;
        j++;
        } 
      j--;  
      ic=4*mix1.new_points;  
      for(i=ib; i<ic; i+=4)
        {
        j--;
        pa=p0+poffs;
        t3=sin(t1);
        t4=cos(t1);
        r1=mix1.window[j];
        timf3_float[pa  ]=(t4*fftn_tmp[i+k  ]-t3*fftn_tmp[i+k+1])*r1;     
        timf3_float[pa+1]=(t4*fftn_tmp[i+k+1]+t3*fftn_tmp[i+k  ])*r1;     
        timf3_float[pa+2]=(t4*fftn_tmp[i+k+2]-t3*fftn_tmp[i+k+3])*r1;     
        timf3_float[pa+3]=(t4*fftn_tmp[i+k+3]+t3*fftn_tmp[i+k+2])*r1;
        t1+=t2;
        t2+=phrot_step;
        p0=(p0+4)&timf3_mask;
        } 
      mix1_phase[ss]=t1;
      id=4*(mix1.crossover_points+mix1.new_points);
      for(i=ic; i<id; i+=4)
        {
        pa=p0+poffs;
        timf3_float[pa  ]=fftn_tmp[i+k  ];     
        timf3_float[pa+1]=fftn_tmp[i+k+1];     
        timf3_float[pa+2]=fftn_tmp[i+k+2];     
        timf3_float[pa+3]=fftn_tmp[i+k+3];
        p0=(p0+4)&timf3_mask;
        } 
      }
    }
  }
}

void do_mix1_afc(int ss)
{
int k, ia, ib;
int na, nx, ka, kb;
float error;
float curv;
int nn,kk;
float t1,t2,t3;
float *fq, *dfq, *d2fq, *fqs;
// The back transforms form a time function that is mixed to
// zero frequency by an oscillator that is stepped in frequency
// with a frequency value mix1_fq_mid[i] for each transform.
// We may be mixing from a long transform so we do not want to
// wait for the next one in order to step the frequency.
// The routine that supplied mix1_fq_mid is responsible for
// supplying a value for the next transform too.
// Based on the old frequencies we already have used we want
// the center frequency at the center of the next transform
// to be be the current frequency plus the first and second derivatives.
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
  fq=&mix1_fq_mid[ss*max_fft1n];
  dfq=&mix1_fq_slope[ss*max_fft1n];
  d2fq=&mix1_fq_curv[ss*max_fft1n];
  fqs=&mix1_fq_start[ss*max_fft1n];
  ka=(fft1_nx+fft1n_mask)&fft1n_mask;  
  kb=(fft1_nx+1)&fft1n_mask;  
  nx=fft1_nx;
  na=fft1_nb;
  }
else
  {
  fq=&mix1_fq_mid[ss*max_fft2n];
  dfq=&mix1_fq_slope[ss*max_fft2n];
  d2fq=&mix1_fq_curv[ss*max_fft2n];
  fqs=&mix1_fq_start[ss*max_fft2n];
  ka=(fft2_nx+fft2n_mask)&fft2n_mask;  
  kb=(fft2_nx+1)&fft2n_mask;  
  nx=fft2_nx;
  na=fft2_na;
  }
t1=fq[nx]+dfq[ka];
t2=fq[kb];
// If t1 and t2 differ we may have to make a compromise.
// Assuming the user has selected a reasonable bandwidth for
// Morse code, we do not want to modulate with frequencies
// above BWFAC of the bandwidth in this first AFC.
// Later coherent processing will be at lower bandwidth and
// will not handle high frequency errors we introduce here.
// Here the signal is noisy because of the high bandwidth required
// to follow a drifting signal.
if(fabs(t2-t1) < BWFAC*baseband_bw_hz)
  {
// Extrapolation of old used data fits well to our new frequency.
// Store new first and second derivatives.
  dfq [nx]= fq[kb]- fq[nx];
  d2fq[nx]=dfq[nx]-dfq[ka];
  }
else
  {
// Use the maximum allowed curvature to produce a frequency that
// goes as quickly as possible towards the desired frequency.
  error=t2-t1;
  curv=BWFAC*baseband_bw_hz;
  if(error < 0)curv=-curv;
  t3=fabs(error)/2;
  nn=(nx+5*(fftxn_mask+1)/4)&fftxn_mask;
  kk=nx;
  k=0;
  ia=ka;
  ib=kb;
  while(fabs(error) > t3 && kk != na)
    {
    d2fq[kk]=curv;
    dfq[kk]=dfq[ia]+curv;
    t1=fq[kk] + dfq[kk];
    error=fq[ib]-t1;
    if(t1 < mix1_lowest_fq)t1=mix1_lowest_fq;
    if(t1 > mix1_highest_fq)t1=mix1_highest_fq;
    fq[ib]=t1;
    ia=(ia+1)&fftxn_mask;
    kk=(kk+1)&fftxn_mask;
    ib=(ib+1)&fftxn_mask;
    k++;
    }
  t3=error;  
  curv=-curv;
// The error is reduced by 50% so we reverse the sign of the curvature.
  while(k > 0 && kk != na && t3*error>0)
    {
    d2fq[kk]=curv;
    dfq[kk]=dfq[ia]+curv;
    t1=fq[kk] + dfq[kk];
    error=fq[ib]-t1;
    fq[ib]=t1;
    ia=(ia+1)&fftxn_mask;
    kk=(kk+1)&fftxn_mask;
    ib=(ib+1)&fftxn_mask;
    k--;
    }
  }
// Using the data for the transform midpoints, we get the
// frequency for the endpoint by adding slope/2+curv/4
fqs[kb]=fq[nx]+0.5*dfq[nx]+0.25*d2fq[nx];
t1=fqs[nx]-fq[nx];
t2=fqs[kb]-fq[nx];
// The current baseband signal produced by mix1 has an error of
// t1 Hz at the first point and t2 Hz at the last point.
// **********************************************************
// Actually the frequency vs time function is not quite aqurate.
// The way the errors t1 and t1 are used below do not agree
// with the explanation above.
// The timing seems to be slightly different depending
// on what window was selected.
// The corrections below do make the AFC work well which
// can be verified by feeding a frequency modulated carrier
// into Linrad. Typically the frequency swing would be 20 Hz
// and the modulation frequency 0.1 Hz for a fft1/fft2 
// bandwidth of 2 to 5 Hz.
// ******************************************************************
do_mix1(ss,t2-t1);
}

void mix1_clear(int ss)
{
int i, poffs, pa;
poffs=ss*timf3_size;        
pa=timf3_pa+poffs;        
for(i=0; i<timf3_block; i++)
  {
  timf3_float[pa+i]=0;     
  }
}

void set_mix1_phases(float fq, int ss)
{
float t1, t2;
int k;
int fftx_pnt;
if(fq<mix1_lowest_fq)
  {
  lirerr(55232);
  return;
  }
if(fq>mix1_highest_fq)
  {
  lirerr(55233);
  return;
  }
// Find out what point in fft1/fft2 to pick as the center
// frequency in the back transformation into timf3 by mix1.
t1=fq*fftx_points_per_hz;
fftx_pnt=t1+0.5;
// When we do the back transformation there will be a phase shift
// from sample to sample (frequency shift) that depends on
// whether the the selected point goes even in mix1_points.
// Our selected frequency is a fractional number, add
// whatever frequency shift that originates in the decimals
// of t1.
k=fftx_pnt%mix1.size;
t2=mix1.size*(fftx_pnt/mix1.size);
t2=t1-t2-k;    
t2=t2-(int)(t2);
mix1_phase_rot[ss]=t2*2*PI_L/mix1.size;
// when we go from one transform to the next one there is a phase
// jump that we store in mix1_phase_step.
k=(k*(mix1.new_points))%mix1.size;
mix1_old_phase[ss]=mix1_phase[ss];
mix1_phase[ss]+=mix1_phase_step[ss];
mix1_phase_step[ss]=k*2*PI_L/mix1.size;
if(mix1_point[ss] != -1)
  {
  mix1_old_point[ss]=mix1_point[ss];
  }
else
  {
  mix1_old_point[ss]=fftx_pnt;
  }    
mix1_point[ss]=fftx_pnt;   
if(mix1_phase[ss] > PI_L)mix1_phase[ss]-=2*PI_L;
if(mix1_phase[ss] < PI_L)mix1_phase[ss]+=2*PI_L;
}

void fft2_mix1_afc(void)
{
// Use fft2 with a frequency shift that is different for each transform.
// The frequency shift is calculated by fft2_afc and stored 
// in mix1_fq_mid
int i,n,n2,ss,mm;
float t1;
int nn,kk,ia,ib,ic;
float *z;
short int *zxy;
n=mix1.size*ui.rx_rf_channels;
n2=2*n;
mm=twice_rxchan;
nn=twice_rxchan*fft2_to_fft1_ratio;
for(ss=0; ss<genparm[MIX1_NO_OF_CHANNELS]; ss++)
  {
  if(mix1_selfreq[ss] >= 0)
    {
// Frequency no ss is selected.
    t1=mix1_fq_mid[ss*max_fft2n+fft2_nx];
    set_mix1_phases(t1,ss);
    kk=mix1_point[ss]*mm;   
// Copy mix1.size points to fftn_tmp and make the transform
// This way we select a limited frequency range and reduce
// the sampling rate by mix1.size/fft1_size
    ia=nn*fft1_first_point;
    ib=n;
    if(ib > nn*fft1_last_point-kk)ib=nn*fft1_last_point-kk; 
    if(ib < 0)ib=0;
    ia=0;
    if(ia < nn*fft1_first_point-kk)ia=nn*fft1_first_point-kk;
    for(i=0; i<ia; i++)fftn_tmp[i]=0;    
    if(fft_cntrl[FFT2_CURMODE].mmx == 0)
      { 
      z=&fft2_float[kk+mm*fft2_nx*fft2_size];
      for(i=ia; i<ib; i++)fftn_tmp[i]=z[i];    
      }
    else
      { 
      zxy=&fft2_short_int[kk+mm*fft2_nx*fft2_size];
      for(i=ia; i<ib; i++)fftn_tmp[i]=zxy[i];    
      }    
    for(i=ib; i<n+1; i++)fftn_tmp[i]=0;
    kk-=n2;
    ib=n;
    if(ib < nn*fft1_first_point-kk)ib=nn*fft1_first_point-kk;
    for(i=n+1; i<ib; i++)fftn_tmp[i]=0;
    ic=n2;
    if(ic > nn*fft1_last_point-kk)ic=nn*fft1_last_point-kk; 
    if(fft_cntrl[FFT2_CURMODE].mmx == 0)
      { 
      z=&fft2_float[kk+mm*fft2_nx*fft2_size];
      for(i=ib; i<ic; i++)fftn_tmp[i]=z[i];    
      }
    else
      {
      zxy=&fft2_short_int[kk+mm*fft2_nx*fft2_size];
      for(i=ib; i<ic; i++)fftn_tmp[i]=zxy[i];    
      }
    for(i=ic; i<n2; i++)fftn_tmp[i]=0;    
    do_mix1_afc(ss);
    }   
  else
    {
    mix1_clear(ss);
    }
  }
timf3_pa=(timf3_pa+timf3_block)&timf3_mask;
fft2_nx=(fft2_nx+1)&fft2n_mask;  
}

void fft2_mix1_fixed(void)
{
// Use fft2 with a constant frequency shift given by mix1_point[]
int ib,i,k,n,n2,ss,mm,nn,kk;
float *z;
float t1;
short int *zxy;
n=mix1.size*ui.rx_rf_channels;
n2=2*n;
mm=twice_rxchan;
nn=twice_rxchan*fft2_to_fft1_ratio;
for(ss=0; ss<genparm[MIX1_NO_OF_CHANNELS]; ss++)
  {
  t1=mix1_selfreq[ss];
  if(t1 >= 0)
    {
// Frequency no ss is selected.
    set_mix1_phases(t1,ss);
    kk=mix1_point[ss]*mm;   
// Copy mix1.size points to fftn_tmp and make the transform
// This way we select a limited frequency range and reduce
// the sampling rate by mix1.size/fft1_size
    k=mix1_point[ss]*mm;   
    ib=n;
    if(ib > nn*fft1_last_point-k)ib=nn*fft1_last_point-k; 
    if(ib < 0) ib=0;
    if(fft_cntrl[FFT2_CURMODE].mmx == 0)
      { 
      z=&fft2_float[k+mm*fft2_nx*fft2_size];
      for(i=0; i<ib; i++)fftn_tmp[i]=z[i];    
      }
    else
      { 
      zxy=&fft2_short_int[k+mm*fft2_nx*fft2_size];
      for(i=0; i<ib; i++)fftn_tmp[i]=zxy[i];
      }    
    for(i=ib; i<n; i++)fftn_tmp[i]=0;
    k-=n2;
    ib=n;
    if(ib < nn*fft1_first_point-k)ib=nn*fft1_first_point-k;
    for(i=n; i<ib; i++)fftn_tmp[i]=0;
    if(fft_cntrl[FFT2_CURMODE].mmx == 0)
      { 
      z=&fft2_float[k+mm*fft2_nx*fft2_size];
      for(i=ib; i<n2; i++)fftn_tmp[i]=z[i];    
      }
    else
      {
      zxy=&fft2_short_int[k+mm*fft2_nx*fft2_size];
      for(i=ib; i<n2; i++)fftn_tmp[i]=zxy[i];    
      }
    do_mix1(ss,0);
    }   
  else
    {
    mix1_clear(ss);
    }
  }
timf3_pa=(timf3_pa+timf3_block)&timf3_mask;
fft2_nx=(fft2_nx+1)&fft2n_mask;  
}

void fft1_mix1_fixed(void)
{
// Use fft1 with a constant frequency shift given by mix1_point[]
int i,n,n2,ss,mm;
float *x;
int kk,ib;
float t1;
n=mix1.size*ui.rx_rf_channels;
n2=2*n;
mm=twice_rxchan;
for(ss=0; ss<genparm[MIX1_NO_OF_CHANNELS]; ss++)
  {
  t1=mix1_selfreq[ss];
  if(t1 >= 0)
    {
// Frequency no ss is selected.
    if(rx_mode != MODE_TXTEST)
      {
      set_mix1_phases(t1,ss);
      }
    kk=mix1_point[ss]*mm;   
// Copy mix1.size points to fftn_tmp and make the transform
// This way we select a limited frequency range and reduce
// the sampling rate by mix1.size/fft1_size
    x=&fft1_float[fft1_px+kk];
    ib=n;
    if(ib > mm*fft1_last_point-kk)ib=mm*fft1_last_point-kk; 
    if(ib < 0) ib=0;
    for(i=0; i<ib; i++)fftn_tmp[i]=x[i];    
    for(i=ib; i<=n; i++)fftn_tmp[i]=0;
    kk-=n2;
    x=&fft1_float[fft1_px+kk];
    ib=n;
    if(ib < mm*fft1_first_point-kk)ib=mm*fft1_first_point-kk;
    for(i=n; i<ib; i++)fftn_tmp[i]=0;
    for(i=ib; i<n2; i++)fftn_tmp[i]=x[i];    
    do_mix1(ss,0);
    }   
  else
    {
    mix1_clear(ss);
    }
  }
timf3_pa=(timf3_pa+timf3_block)&timf3_mask;
fft1_nx=(fft1_nx+1)&fft1n_mask;  
fft1_px=(fft1_px+fft1_block)&fft1_mask;  
}



void fft1_mix1_afc(void)
{
// Use fft1 with a frequency shift that is different for each transform.
// The frequency shift is calculated by fft1_afc and stored 
// in mix1_fq_mid
int i,n,n2,ss,mm;
float *x;
float t1;
int kk,ia,ib,ic;
n=mix1.size*ui.rx_rf_channels;
n2=2*n;
mm=twice_rxchan;
for(ss=0; ss<genparm[MIX1_NO_OF_CHANNELS]; ss++)
  {
  if(mix1_selfreq[ss] >= 0)
    {
// Frequency no ss is selected.
    t1=mix1_fq_mid[ss*max_fft1n+fft1_nx];
    set_mix1_phases(t1,ss);
    kk=mix1_point[ss]*mm;   
// Copy mix1.size points to fftn_tmp and make the transform
// This way we select a limited frequency range and reduce
// the sampling rate by mix1.size/fft1_size
    x=&fft1_float[fft1_nx*fft1_block+kk];
    ia=mm*fft1_first_point;
    ib=n;
    if(ib > mm*fft1_last_point-kk)ib=mm*fft1_last_point-kk; 
    if(ib < 0)ib=0;
    ia=0;
    if(ia < mm*fft1_first_point-kk)ia=mm*fft1_first_point-kk;
    for(i=0; i<ia; i++)fftn_tmp[i]=0;    
    for(i=ia; i<ib; i++)fftn_tmp[i]=x[i];    
    for(i=ib; i<n+1; i++)fftn_tmp[i]=0;
    kk-=n2;
    x=&fft1_float[fft1_nx*fft1_block+kk];
    ib=n;
    if(ib < mm*fft1_first_point-kk)ib=mm*fft1_first_point-kk;
    for(i=n+1; i<ib; i++)fftn_tmp[i]=0;
    ic=n2;
    if(ic > mm*fft1_last_point-kk)ic=mm*fft1_last_point-kk; 
    for(i=ib; i<ic; i++)fftn_tmp[i]=x[i];    
    for(i=ic; i<n2; i++)fftn_tmp[i]=0;    
    do_mix1_afc(ss);
    }   
  else
    {
    mix1_clear(ss);
    }
  }
timf3_pa=(timf3_pa+timf3_block)&timf3_mask;
fft1_nx=(fft1_nx+1)&fft1n_mask;  
fft1_px=(fft1_px+fft1_block)&fft1_mask;  
}




 



