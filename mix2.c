

#include "globdef.h"
#include "uidef.h"
#include "fft1def.h"
#include "fft3def.h"
#include "sigdef.h"
#include "seldef.h"
#include "screendef.h"

#define ZZ 0.000000001

float mix2_phase=0;      

// Power to the fourth may become a very large number.
// Define something small to multiply by to prevent overflows.
#define P4SCALE 0.000000000000000000001
#define P2SCALE 0.00000000001
void fft3_mix2(void)
{
int i, j, k, ia, ib, ic, id, ix, iy, pb, p0, sizhalf;
int last_point;
float ampfac,cg_mid;
float big;
int nn, mm;
int siz, pa, resamp;
#define NOISE_FILTERS 2
#define NOISE_POINTS 5
float noise[2*NOISE_FILTERS];
float *carr;
float t1,t2,t3,t4,t5;
float r1,r2,r3,r4,c1,c2,c3;
float noise_floor, a2, b2, re, im;
float noi2, sina, a2s, b2s;
float sellim_correction, fac;
carr=&fft3_tmp[twice_rxchan*mix2.size];
resamp=fft3_size/mix2.size;
noise_floor=0;
sizhalf=mix2.size/2;
siz=mix2.size;
if(sw_onechan)
  {
  nn=2*(mix2.size-1);
  if(genparm[CW_DECODE_ENABLE] != 0)
    {
// Compute the power within several narrow filters near the passband.
// Get the average while excluding large values.
// This way occasional signals outside our filter will not
// be included in the noise floor.
    mm=bg_flatpoints+2*bg_curvpoints+NOISE_SEP;
    t3=BIG;
    for(k=0; k<NOISE_FILTERS; k++)
      {
      t1=0;
      t2=0;
      for(i=0; i<NOISE_POINTS; i++)
        {
        j=mm+i+k*(NOISE_POINTS+NOISE_SEP);
        t1+=fft3_slowsum[fft3_size/2-bg_first_xpoint+j];
        t2+=fft3_slowsum[fft3_size/2-bg_first_xpoint-j];
        }
      noise[2*k]=t1;
      noise[2*k+1]=t2;
      if(t1<t3)t3=t1;
      if(t2<t3)t3=t2;
      }
    k=0;
    t1=0;
    t3*=1.5;
    for(i=0; i<2*NOISE_FILTERS; i++)
      {
      if(noise[i] < t3)
        {
        t1+=noise[i];
        k++;
        }
      }
    noise_floor=t1/(k*NOISE_POINTS);
    }
  if(bg.mixer_mode == 1)
    {
// Filter and decimate in the frequency domain.    
// Select mix2.size points centered at fft3_size/2, and move them 
// to fft3_tmp while multiplying with the baseband filter function.
    p0=fft3_px+fft3_size;
    k=fft3_size/2;
    for(i=0; i<sizhalf; i++)
      {
      fft3_tmp[2*i  ]=fft3[p0+2*i  ]*bg_filterfunc[k+i];    
      fft3_tmp[2*i+1]=fft3[p0+2*i+1]*bg_filterfunc[k+i];    
      }
    for(i=0; i<sizhalf; i++)
      {
      fft3_tmp[nn-2*i  ]=fft3[p0-2*i-2]*bg_filterfunc[k-i-1];    
      fft3_tmp[nn-2*i+1]=fft3[p0-2*i-1]*bg_filterfunc[k-i-1];    
      }
    fftback(mix2.size, mix2.n, fft3_tmp, mix2.table, mix2.permute, 
                                                       yieldflag_ndsp_mix2);
// Store baseb_raw as the time function we obtain from the back transformation.
    if(genparm[THIRD_FFT_SINPOW] == 2)
      {
      for(i=0; i<sizhalf; i++)
        {
        baseb_raw[2*baseb_pa+2*i  ]+=fft3_tmp[2*i];     
        baseb_raw[2*baseb_pa+2*i+1]+=fft3_tmp[2*i+1];     
        }
      p0=(baseb_pa+sizhalf)&baseband_mask;
      for(i=0; i<mix2.size; i++)
        {
        baseb_raw[2*p0+i]=fft3_tmp[mix2.size+i];     
        }
      }
    else
      {
      p0=baseb_pa;
      k=mix2.interleave_points-2*(mix2.crossover_points/2);
      j=k/2+mix2.crossover_points;
      ia=2*mix2.crossover_points;
      for(i=0; i<ia; i+=2)
        {
        baseb_raw[2*p0  ]=baseb_raw[2*p0  ]*mix2.cos2win[i>>1]+fft3_tmp[i+k  ]*mix2.sin2win[i>>1];
        baseb_raw[2*p0+1]=baseb_raw[2*p0+1]*mix2.cos2win[i>>1]+fft3_tmp[i+k+1]*mix2.sin2win[i>>1];
        p0=(p0+1)&baseband_mask;
        } 
      ib=mix2.new_points+2+2*(mix2.crossover_points/2);
      for(i=ia; i<ib; i+=2)
        {
        baseb_raw[2*p0  ]=fft3_tmp[i+k  ]*mix2.window[j];
        baseb_raw[2*p0+1]=fft3_tmp[i+k+1]*mix2.window[j];
        p0=(p0+1)&baseband_mask;
        j++;
        } 
      j--;  
      ic=2*mix2.new_points;  
      for(i=ib; i<ic; i+=2)
        {
        j--;
        baseb_raw[2*p0  ]=fft3_tmp[i+k  ]*mix2.window[j];
        baseb_raw[2*p0+1]=fft3_tmp[i+k+1]*mix2.window[j];
        p0=(p0+1)&baseband_mask;
        } 
      id=2*(mix2.crossover_points+mix2.new_points);
      for(i=ic; i<id; i+=2)
        {
        baseb_raw[2*p0  ]=fft3_tmp[i+k  ];
        baseb_raw[2*p0+1]=fft3_tmp[i+k+1];
        p0=(p0+1)&baseband_mask;
        } 
      }    
    }
  if(bg.mixer_mode == 2)
    {
// Use a FIR filter
// We have valid data in timf3 up to (timf3_py+2*fft3_size-1)
// The length of the FIR is basebraw_fir_pts
// The last FIR starts at timf3_py+2*chan*(fft3_size-basebraw_fir_pts)
// The first transform starts at timf3_py+2*chan*(fft3_size-
//                    basebraw_fir_pts-fft3_new_points)
// The resampling rate is fft3_size/mix2.size
    p0=baseb_pa;
    for(k=1; k<=mix2.new_points; k++)
      {
      pa=(timf3_py+2*(fft3_size-basebraw_fir_pts-fft3_new_points+k*resamp)+timf3_size)&timf3_mask;
      t1=0;
      t2=0;
      for(i=0; i<basebraw_fir_pts; i++)
        {
        t1+=timf3_float[pa  ]*basebraw_fir[i];
        t2+=timf3_float[pa+1]*basebraw_fir[i];
        pa=(pa+2)&timf3_mask;
        }
      baseb_raw[2*p0  ]=t1;
      baseb_raw[2*p0+1]=t2;
      p0=(p0+1)&baseband_mask;
      }
    }
// Select mix2.size points centered at fft3_size/2, and move them 
// to carr while multiplying with a filter function that is 
// bg.coh_factor times narrower than the baseband filter function
  k=fft3_size/2;
  for(i=0; i<sizhalf; i++)
    {
    carr[2*i  ]=fft3[fft3_px+fft3_size+2*i  ]*bg_carrfilter[k-i-1];    
    carr[2*i+1]=fft3[fft3_px+fft3_size+2*i+1]*bg_carrfilter[k-i-1];    
    }
  for(i=0; i<sizhalf; i++)
    {
    carr[nn-2*i  ]=fft3[fft3_px+fft3_size-2*i-2]*bg_carrfilter[k+i];    
    carr[nn-2*i+1]=fft3[fft3_px+fft3_size-2*i-1]*bg_carrfilter[k+i];    
    }
  }  
else
  {
  nn=4*(mix2.size-1);
// !!!!!!!!!!!!!! There are two rx channels. !!!!!!!!!!!!!!!!!!
  if( genparm[CW_DECODE_ENABLE] != 0)
    {
// Compute the power within several narrow filters near the passband.
// Get the average while excluding large values.
// This way occasional signals outside our filter will not
// be included in the noise floor.
    mm=bg_flatpoints+2*bg_curvpoints+NOISE_SEP;
    t3=BIG;
    for(k=0; k<NOISE_FILTERS; k++)
      {
      t1=0;
      t2=0;
      for(i=0; i<NOISE_POINTS; i++)
        {
        j=mm+i+k*(NOISE_POINTS+NOISE_SEP);
        t1+=fft3_slowsum[2*(fft3_size/2-bg_first_xpoint+j)];
        t2+=fft3_slowsum[2*(fft3_size/2-bg_first_xpoint-j)];
        }
      noise[2*k]=t1;
      noise[2*k+1]=t2;
      if(t1<t3)t3=t1;
      if(t2<t3)t3=t2;
      }
    k=0;
    t1=0;
    t3*=1.5;
    for(i=0; i<2*NOISE_FILTERS; i++)
      {
      if(noise[i] < t3)
        {
        t1+=noise[i];
        k++;
        }
      }
    noise_floor=t1/(k*NOISE_POINTS);
    }
// Select mix2.size points centered at fft3_size/2, and move them 
// to fft3_tmp while multiplying with the baseband filter function.
// Transform the polarization at the same time.
// We fill fft3_tmp even when the filter and decimation will be
// made in the time domain with a FIR filter in order to have
// the polarization computed here by use of the fft3 spectrum.  
  a2=0;
  b2=0;
  re=0;
  im=0;
  p0=fft3_px+2*fft3_size;
  k=fft3_size/2;
  for(i=0; i<sizhalf; i++)
    {
    if(bg_filterfunc[k+i] == 0)
      {
      fft3_tmp[4*i  ]=0;
      fft3_tmp[4*i+1]=0;
      fft3_tmp[4*i+2]=0;
      fft3_tmp[4*i+3]=0;
      }
    else
      {  
      t1=fft3[p0+4*i  ];
      t2=fft3[p0+4*i+1];
      t3=fft3[p0+4*i+2];
      t4=fft3[p0+4*i+3];
      fft3_tmp[4*i  ]=(pg.c1*t1+pg.c2*t3+pg.c3*t4)*bg_filterfunc[k+i];
      fft3_tmp[4*i+1]=(pg.c1*t2+pg.c2*t4-pg.c3*t3)*bg_filterfunc[k+i];
      fft3_tmp[4*i+2]=(pg.c1*t3-pg.c2*t1+pg.c3*t2)*bg_filterfunc[k+i];
      fft3_tmp[4*i+3]=(pg.c1*t4-pg.c2*t2-pg.c3*t1)*bg_filterfunc[k+i];
      r1=P4SCALE*(fft3_slowsum[fft3_size-2*bg_first_xpoint+2*i]-noise_floor);
      if(r1 > 0  && pg.adapt == 0)
        {
        a2+=r1*(fft3_tmp[4*i  ]*fft3_tmp[4*i  ]+
                fft3_tmp[4*i+1]*fft3_tmp[4*i+1]);
        b2+=r1*(fft3_tmp[4*i+2]*fft3_tmp[4*i+2]+
                fft3_tmp[4*i+3]*fft3_tmp[4*i+3]);
        re+=r1*(fft3_tmp[4*i  ]*fft3_tmp[4*i+2]+
                fft3_tmp[4*i+1]*fft3_tmp[4*i+3]);
        im+=r1*(-fft3_tmp[4*i  ]*fft3_tmp[4*i+3]+
                fft3_tmp[4*i+1]*fft3_tmp[4*i+2]);
        }
      }
    }
  for(i=0; i<sizhalf; i++)
    {
    if(bg_filterfunc[k-i-1] == 0)
      {
      fft3_tmp[nn-4*i  ]=0;
      fft3_tmp[nn-4*i+1]=0;
      fft3_tmp[nn-4*i+2]=0;
      fft3_tmp[nn-4*i+3]=0;
      }
    else
      {  
      t1=fft3[p0-4*i-4];
      t2=fft3[p0-4*i-3];
      t3=fft3[p0-4*i-2];
      t4=fft3[p0-4*i-1];
      fft3_tmp[nn-4*i  ]=(pg.c1*t1+pg.c2*t3+pg.c3*t4)*bg_filterfunc[k-i-1];
      fft3_tmp[nn-4*i+1]=(pg.c1*t2+pg.c2*t4-pg.c3*t3)*bg_filterfunc[k-i-1];
      fft3_tmp[nn-4*i+2]=(pg.c1*t3-pg.c2*t1+pg.c3*t2)*bg_filterfunc[k-i-1];
      fft3_tmp[nn-4*i+3]=(pg.c1*t4-pg.c2*t2-pg.c3*t1)*bg_filterfunc[k-i-1];
      r1=P4SCALE*(fft3_slowsum[fft3_size-2*bg_first_xpoint-2*i]-noise_floor);
      if(r1 > 0  && pg.adapt == 0)
        {
        a2+=r1*(fft3_tmp[nn-4*i  ]*fft3_tmp[nn-4*i  ]+
                fft3_tmp[nn-4*i+1]*fft3_tmp[nn-4*i+1]);
        b2+=r1*(fft3_tmp[nn-4*i+2]*fft3_tmp[nn-4*i+2]+
                fft3_tmp[nn-4*i+3]*fft3_tmp[nn-4*i+3]);
        re+=r1*(fft3_tmp[nn-4*i  ]*fft3_tmp[nn-4*i+2]+
                fft3_tmp[nn-4*i+1]*fft3_tmp[nn-4*i+3]);
        im+=r1*(-fft3_tmp[nn-4*i  ]*fft3_tmp[nn-4*i+3]+
                 fft3_tmp[nn-4*i+1]*fft3_tmp[nn-4*i+2]);
        }
      }
    }
// In case the polarization is already correct, a2 is large because
// it is S+N while the other numbers are small, containing noise only.
// Now, to follow the polarization correctly for really weak
// signals do not just use the data as they come here.
// Store the result in poleval_data and adjust pol first when it
// is quite certain that a new set of coefficients is better. 
  if(pg.adapt != 0)goto good_poldata;
  poleval_data[poleval_pointer].a2=a2;
  poleval_data[poleval_pointer].b2=b2;
  poleval_data[poleval_pointer].re=re;
  poleval_data[poleval_pointer].im=im;
  for(i=0; i<poleval_pointer; i++)
    {
    a2+=poleval_data[i].a2;
    b2+=poleval_data[i].b2;
    re+=poleval_data[i].re;
    im+=poleval_data[i].im;
    }
  poleval_pointer++;  
  if(poleval_pointer<POLEVAL_SIZE/5)goto good_poldata;
// Check afcsub.c or blank1 .c for an explanation how pol 
// coefficients are calculated.
// If we shall change pol coefficients now, we must be sure that
// the phase angle between a and b is accurate.
// Step through the data and sum the square of the phase deviation.
// Note that the phase is two dimensional. 
// The angle between polarization planes and the phase angle 
// between voltages. 
  t1=a2+b2;
  if(t1 == 0)goto bad_poldata;
  a2/=t1;
  b2/=t1;
  re/=t1;
  im/=t1;
  r1=0;
  r2=0;
  for(i=0; i<poleval_pointer; i++)
    {
    t1=poleval_data[i].a2+poleval_data[i].b2;
    t2=re*t1;
    t3=im*t1;
    r1+=poleval_data[i].re*poleval_data[i].re+
        poleval_data[i].im*poleval_data[i].im;
    r2+=-(t2-poleval_data[i].re)*(t2-poleval_data[i].re)+
        (t3-poleval_data[i].im)*(t3-poleval_data[i].im);
    }
  r2/=r1;
  if(r2 >0.2)goto bad_poldata;
  t2=re*re+im*im;
  noi2=a2*b2-t2;
  if(noi2 > 0.12)goto bad_poldata;
  a2s=a2-noi2;
  b2s=b2-noi2;
  if(b2s <=0.0001 || t2 == 0)
    {
    poleval_pointer=0;
    if(b2 > a2)
      {
      t1=atan2(pg.c2,pg.c3);
      t2=pg.c1;
      pg.c1=sqrt(pg.c2*pg.c2+pg.c3*pg.c3);
      pg.c2=t2*cos(t1);
      pg.c3=t2*sin(t1);
// The signs of c2 and c3 are most probably incorrect here!!! ööö
// but the situation is unusual and probably t2=0 always when 
// b2=1 and a2=0. Check and make sure the new pol parameters give
// the orthogonal polarization.....       
      }
    goto good_poldata; 
    }
  if(a2s > 0)
    {
    c1=sqrt(a2s);
    sina=sqrt(b2s);
    c2=sina*re/sqrt(t2);
    c3=-sina*im/sqrt(t2);
    t1=sqrt(c1*c1+c2*c2+c3*c3);
    if(c2 < 0)t1=-t1;
    c1/=t1;
    c2/=t1;
    c3/=t1;
    }
  else
    {
    c1=0;
    c2=1;
    c3=0;
    }    
// The current a and b signals were produced by:
// re_a=pg.c1*re_x+pg.c2*re_y+pg.c3*im_y (1)
// im_a=pg.c1*im_x+pg.c2*im_y-pg.c3*re_y (2)
// re_b=pg.c1*re_y-pg.c2*re_x+pg.c3*im_x (3)
// im_b=pg.c1*im_y-pg.c2*im_x-pg.c3*re_x (4)
// We have now found that improved a and b signals will be obtained from:
// new_re_a=c1*re_a+c2*re_b+c3*im_b (5)
// new_im_a=c1*im_a+c2*im_b-c3*re_b (6)
// new_re_b=c1*re_b-c2*re_a+c3*im_a (7)
// new_im_b=c1*im_b-c2*im_a-c3*re_a (8)
// Eliminate the old a and b signals
// new_re_a=c1*(pg.c1*re_x+pg.c2*re_y+pg.c3*im_y)
//         +c2*(pg.c1*re_y-pg.c2*re_x+pg.c3*im_x)
//         +c3*(pg.c1*im_y-pg.c2*im_x-pg.c3*re_x)
// new_re_a=c1*pg.c1*re_x+c1*pg.c2*re_y+c1*pg.c3*im_y
//         +c2*pg.c1*re_y-c2*pg.c2*re_x+c2*pg.c3*im_x
//         +c3*pg.c1*im_y-c3*pg.c2*im_x-c3*pg.c3*re_x
// new_re_a=(c1*pg.c1-c2*pg.c2-c3*pg.c3)*re_x
//         -(c3*pg.c2-c2*pg.c3)*im_x
//         +(c1*pg.c2+c2*pg.c1)*re_y
//         +(c1*pg.c3+c3*pg.c1)*im_y
  t1=c1*pg.c1-c2*pg.c2-c3*pg.c3;
  t2=c3*pg.c2-c2*pg.c3;
  t3=c1*pg.c2+c2*pg.c1;
  t4=c1*pg.c3+c3*pg.c1;
// We want t2 to be zero so we adjust the phase.
  c1=sqrt(t1*t1+t2*t2);
  r2=sqrt(t3*t3+t4*t4);
  r1=atan2(t3,t4)+atan2(t1,t2);
  c2=-r2*cos(r1);
  c3=r2*sin(r1);
  t1=sqrt(c1*c1+c2*c2+c3*c3);
  if(t1 > 0)
    {
    t2=c1*pg.c1+c2*pg.c2+c3*pg.c3;
    if(t2 < 0)t1=-t1;
    c1/=t1;
    c2/=t1;
    c3/=t1;
    t1=pg.avg-1;
    pg.c1=(t1*pg.c1+c1)/pg.avg;
    pg.c2=(t1*pg.c2+c2)/pg.avg;
    pg.c3=(t1*pg.c3+c3)/pg.avg;
    t1=sqrt(pg.c1*pg.c1+pg.c2*pg.c2+pg.c3*pg.c3);
    if(pg.c2 < 0)t1=-t1;
    pg.c1/=t1;
    pg.c2/=t1;
    pg.c3/=t1;
    sc[SC_SHOW_POL]++;
    }
  poleval_pointer=0;
  goto good_poldata;
bad_poldata:;
  if(poleval_pointer > 3*POLEVAL_SIZE/4)
    {
    j=poleval_pointer;
    k=POLEVAL_SIZE/4;
    poleval_pointer=0;
    for(i=k; i<j; i++)
      {
      poleval_data[poleval_pointer].a2=poleval_data[poleval_pointer+k].a2;
      poleval_data[poleval_pointer].b2=poleval_data[poleval_pointer+k].b2;
      poleval_data[poleval_pointer].re=poleval_data[poleval_pointer+k].re;
      poleval_data[poleval_pointer].im=poleval_data[poleval_pointer+k].im;
      poleval_pointer++;
      }
    }        
good_poldata:;
// Select mix2.size points centered at fft3_size/2, and move them 
// to fft3_tmp while multiplying with a filter function that is 
// bg.coh_factor times narrower than the baseband filter function
// Just process the first polarization. 
// The user is responsible for the polarization to be right!
  nn=2*(mix2.size-1);
  k=fft3_size/2;
  for(i=0; i<sizhalf; i++)
    {
    carr[2*i  ]=fft3_tmp[4*i  ]*bg_carrfilter[k-i-1];    
    carr[2*i+1]=fft3_tmp[4*i+1]*bg_carrfilter[k-i-1];    
    }
  for(i=0; i<sizhalf; i++)
    {
    carr[nn-2*i  ]=fft3_tmp[2*nn-4*i  ]*bg_carrfilter[k+i];    
    carr[nn-2*i+1]=fft3_tmp[2*nn-4*i+1]*bg_carrfilter[k+i];    
    }
  if(bg.mixer_mode == 1)
    {
    dual_fftback(mix2.size, mix2.n, fft3_tmp, 
                              mix2.table, mix2.permute, yieldflag_ndsp_mix2);
// Store baseb_raw and baseb_raw_orthog.
    if(genparm[THIRD_FFT_SINPOW] == 2)
      {
// Every second transform with reversed sign. 50% overlap because
// we are using sin squared windows.
      for(i=0; i<sizhalf; i++)
        {
        baseb_raw[2*baseb_pa+2*i  ]+=fft3_tmp[4*i  ];     
        baseb_raw[2*baseb_pa+2*i+1]+=fft3_tmp[4*i+1];     
        baseb_raw_orthog[2*baseb_pa+2*i  ]+=fft3_tmp[4*i+2];     
        baseb_raw_orthog[2*baseb_pa+2*i+1]+=fft3_tmp[4*i+3];     
        }
      k=sizhalf;
      p0=(baseb_pa+sizhalf)&baseband_mask;        
      for(i=0; i<sizhalf; i++)
        {
        baseb_raw[2*p0+2*i  ]=fft3_tmp[4*(i+k)  ];     
        baseb_raw[2*p0+2*i+1]=fft3_tmp[4*(i+k)+1];     
        baseb_raw_orthog[2*p0+2*i  ]=fft3_tmp[4*(i+k)+2];     
        baseb_raw_orthog[2*p0+2*i+1]=fft3_tmp[4*(i+k)+3];     
        }
      }
    else
      {
      p0=baseb_pa;
      k=mix2.interleave_points-2*(mix2.crossover_points/2);
      j=k/2+mix2.crossover_points;
      k*=2;
      ia=2*mix2.crossover_points;
      for(i=0; i<ia; i+=2)
        {
        baseb_raw[2*p0  ]=baseb_raw[2*p0  ]*mix2.cos2win[i>>1]+fft3_tmp[2*i+k  ]*mix2.sin2win[i>>1];
        baseb_raw[2*p0+1]=baseb_raw[2*p0+1]*mix2.cos2win[i>>1]+fft3_tmp[2*i+k+1]*mix2.sin2win[i>>1];
        baseb_raw_orthog[2*p0  ]=baseb_raw_orthog[2*p0  ]*mix2.cos2win[i>>1]+
                                                  fft3_tmp[2*i+k+2]*mix2.sin2win[i>>1];
        baseb_raw_orthog[2*p0+1]=baseb_raw_orthog[2*p0+1]*mix2.cos2win[i>>1]+
                                                   fft3_tmp[2*i+k+3]*mix2.sin2win[i>>1];
        p0=(p0+1)&baseband_mask;
        } 
      ib=mix2.new_points+2+2*(mix2.crossover_points/2);
      for(i=ia; i<ib; i+=2)
        {
        baseb_raw[2*p0  ]=fft3_tmp[2*i+k  ]*mix2.window[j];
        baseb_raw[2*p0+1]=fft3_tmp[2*i+k+1]*mix2.window[j];
        baseb_raw_orthog[2*p0  ]=fft3_tmp[2*i+k+2]*mix2.window[j];
        baseb_raw_orthog[2*p0+1]=fft3_tmp[2*i+k+3]*mix2.window[j];
        p0=(p0+1)&baseband_mask;
        j++;
        } 
      j--;  
      ic=2*mix2.new_points;  
      for(i=ib; i<ic; i+=2)
        {
        j--;
        baseb_raw[2*p0  ]=fft3_tmp[2*i+k  ]*mix2.window[j];
        baseb_raw[2*p0+1]=fft3_tmp[2*i+k+1]*mix2.window[j];
        baseb_raw_orthog[2*p0  ]=fft3_tmp[2*i+k+2]*mix2.window[j];
        baseb_raw_orthog[2*p0+1]=fft3_tmp[2*i+k+3]*mix2.window[j];
        p0=(p0+1)&baseband_mask;
        } 
      id=2*(mix2.crossover_points+mix2.new_points);
      for(i=ic; i<id; i+=2)
        {
        baseb_raw[2*p0  ]=fft3_tmp[2*i+k  ];
        baseb_raw[2*p0+1]=fft3_tmp[2*i+k+1];
        baseb_raw_orthog[2*p0  ]=fft3_tmp[2*i+k+2];
        baseb_raw_orthog[2*p0+1]=fft3_tmp[2*i+k+3];
        p0=(p0+1)&baseband_mask;
        } 
      }
    }
  if(bg.mixer_mode == 2)
    {
// Use a FIR filter
// We have valid data in timf3 up to (timf3_py+2*fft3_size-1)
// The length of the FIR is basebraw_fir_pts
// The last FIR starts at timf3_py+2*chan*(fft3_size-basebraw_fir_pts)
// The first transform starts at timf3_py+2*chan*(fft3_size-
//                    basebraw_fir_pts-fft3_new_points)
// The resampling rate is fft3_size/mix2.size
    p0=baseb_pa;
    for(k=1; k<=mix2.new_points; k++)
      {
      pa=(timf3_py+4*(fft3_size-basebraw_fir_pts-fft3_new_points+k*resamp)+timf3_size)&timf3_mask;
      r1=0;
      r2=0;
      r3=0;
      r4=0;
      for(i=0; i<basebraw_fir_pts; i++)
        {
        t1=timf3_float[pa  ]*basebraw_fir[i];
        t2=timf3_float[pa+1]*basebraw_fir[i];
        t3=timf3_float[pa+2]*basebraw_fir[i];
        t4=timf3_float[pa+3]*basebraw_fir[i];
        r1+=pg.c1*t1+pg.c2*t3+pg.c3*t4;
        r2+=pg.c1*t2+pg.c2*t4-pg.c3*t3;
        r3+=pg.c1*t3-pg.c2*t1+pg.c3*t2;
        r4+=pg.c1*t4-pg.c2*t2-pg.c3*t1;
        pa=(pa+4)&timf3_mask;
        }
      baseb_raw[2*p0  ]=r1;
      baseb_raw[2*p0+1]=r2;
      baseb_raw_orthog[2*p0  ]=r3;
      baseb_raw_orthog[2*p0+1]=r4;
      p0=(p0+1)&baseband_mask;
      }
    }
  }
if(bg.mixer_mode == 1)
  {
// Filter and decimate in the frequency domain.    
// We already stored the selected part of the spectrum in carr.
// take the back transform and construct the time function
// of baseb_carrier
  fftback(mix2.size, mix2.n, carr,mix2.table,mix2.permute,
                                                  yieldflag_ndsp_mix2);
// Store baseb_raw and baseb_carrier.
// Every second transform with reversed sign. 50% overlap because
// we are using sin squared windows.
// Compute baseb, the product of the carrier and the signal.
  if(genparm[THIRD_FFT_SINPOW] == 2)
    {
    for(i=0; i<sizhalf; i++)
      {
      baseb_carrier[2*baseb_pa+2*i  ]+=carr[2*i  ];
      baseb_carrier[2*baseb_pa+2*i+1]+=carr[2*i+1];
      }
    p0=(baseb_pa+sizhalf)&baseband_mask;        
    k=mix2.size;
    for(i=0; i<k; i++)
      {
      baseb_carrier[2*p0+i]=carr[i+k];
      }
    }
  else
    {
    p0=baseb_pa;
    k=mix2.interleave_points-2*(mix2.crossover_points/2);
    j=k/2+mix2.crossover_points;
    ia=2*mix2.crossover_points;
    for(i=0; i<ia; i+=2)
      {
      baseb_carrier[2*p0  ]=baseb_carrier[2*p0  ]*mix2.cos2win[i>>1]+carr[i+k  ]*mix2.sin2win[i>>1];
      baseb_carrier[2*p0+1]=baseb_carrier[2*p0+1]*mix2.cos2win[i>>1]+carr[i+k+1]*mix2.sin2win[i>>1];
      p0=(p0+1)&baseband_mask;
      } 
    ib=mix2.new_points+2+2*(mix2.crossover_points/2);
    for(i=ia; i<ib; i+=2)
      {
      baseb_carrier[2*p0  ]=carr[i+k  ]*mix2.window[j];
      baseb_carrier[2*p0+1]=carr[i+k+1]*mix2.window[j];
      p0=(p0+1)&baseband_mask;
      j++;
      } 
    j--;  
    ic=2*mix2.new_points;  
    for(i=ib; i<ic; i+=2)
      {
      j--;
      baseb_carrier[2*p0  ]=carr[i+k  ]*mix2.window[j];
      baseb_carrier[2*p0+1]=carr[i+k+1]*mix2.window[j];
      p0=(p0+1)&baseband_mask;
      } 
    id=2*(mix2.crossover_points+mix2.new_points);
    for(i=ic; i<id; i+=2)
      {
      baseb_carrier[2*p0  ]=carr[i+k  ];
      baseb_carrier[2*p0+1]=carr[i+k+1];
      p0=(p0+1)&baseband_mask;
      }
    }
  }
if(bg.mixer_mode == 2)
  {
// Filter and decimate in the frequency domain.    
// Use a FIR filter
// We have valid data in timf3 up to (timf3_py+2*fft3_size-1)
// The length of the FIR is basebcarr_fir_pts
// The last FIR starts at timf3_py+2*chan*(fft3_size-basebcarr_fir_pts)
// The first transform starts at timf3_py+2*chan*(fft3_size-
//                    basebcarr_fir_pts-fft3_new_points)
// The resampling rate is fft3_size/mix2.size
  p0=baseb_pa;
  if(sw_onechan)
    {
    for(k=1; k<=mix2.new_points; k++)
      {
      pa=(timf3_py+2*(fft3_size-basebcarr_fir_pts-fft3_new_points+k*resamp)+timf3_size)&timf3_mask;
      t1=0;
      t2=0;
      for(i=0; i<basebcarr_fir_pts; i++)
        {
        t1+=timf3_float[pa  ]*basebcarr_fir[i];
        t2+=timf3_float[pa+1]*basebcarr_fir[i];
        pa=(pa+2)&timf3_mask;
        }
      baseb_carrier[2*p0  ]=t1;
      baseb_carrier[2*p0+1]=t2;
      p0=(p0+1)&baseband_mask;
      }
    }
  else
    {  
    for(k=1; k<=mix2.new_points; k++)
      {
      pa=(timf3_py+4*(fft3_size-basebcarr_fir_pts-fft3_new_points+k*resamp)+timf3_size)&timf3_mask;
      r1=0;
      r2=0;
      r3=0;
      r4=0;
      for(i=0; i<basebcarr_fir_pts; i++)
        {
        t1=timf3_float[pa  ]*basebcarr_fir[i];
        t2=timf3_float[pa+1]*basebcarr_fir[i];
        t3=timf3_float[pa+2]*basebcarr_fir[i];
        t4=timf3_float[pa+3]*basebcarr_fir[i];
        r1+=pg.c1*t1+pg.c2*t3+pg.c3*t4;
        r2+=pg.c1*t2+pg.c2*t4-pg.c3*t3;
        pa=(pa+4)&timf3_mask;
        }
      baseb_carrier[2*p0  ]=r1;
      baseb_carrier[2*p0+1]=r2;
      p0=(p0+1)&baseband_mask;
      }
    }
  }        
if(genparm[CW_DECODE_ENABLE] != 0)
  {
// We want cw decoding. Compute baseb_wb_raw, a signal with a bandwidth
// that is at least two times bigger than the bandwidth of the
// filtered signal. (see computation of baseband_sampling_speed in
// baseb_graph.c)
  if(sw_onechan)
    {
    for(i=0; i<sizhalf; i++)
      {
      fft3_tmp[2*i  ]=fft3[fft3_px+fft3_size+2*i  ];    
      fft3_tmp[2*i+1]=fft3[fft3_px+fft3_size+2*i+1];    
      }
    nn=2*(mix2.size-1);
    for(i=0; i<sizhalf; i++)
      {
      fft3_tmp[nn-2*i  ]=fft3[fft3_px+fft3_size-2*i-2];    
      fft3_tmp[nn-2*i+1]=fft3[fft3_px+fft3_size-2*i-1];    
      }
    }
  else
    {
// !!!!!!!!!!!!!! There are two rx channels. !!!!!!!!!!!!!!!!!!
// Select mix2.size points centered at fft3_size/2, and move them 
// to fft3_tmp. Transform the polarization at the same time.
    for(i=0; i<sizhalf; i++)
      {
      fft3_tmp[2*i  ]=pg.c1*fft3[fft3_px+2*fft3_size+4*i  ]+
                      pg.c2*fft3[fft3_px+2*fft3_size+4*i+2]+
                      pg.c3*fft3[fft3_px+2*fft3_size+4*i+3];
      fft3_tmp[2*i+1]=pg.c1*fft3[fft3_px+2*fft3_size+4*i+1]+
                      pg.c2*fft3[fft3_px+2*fft3_size+4*i+3]-
                      pg.c3*fft3[fft3_px+2*fft3_size+4*i+2];
      }
    nn=2*(mix2.size-1);
    for(i=0; i<sizhalf; i++)
      {
      fft3_tmp[nn-2*i  ]=pg.c1*fft3[fft3_px+2*fft3_size-4*i-4]+
                         pg.c2*fft3[fft3_px+2*fft3_size-4*i-2]+
                         pg.c3*fft3[fft3_px+2*fft3_size-4*i-1];
      fft3_tmp[nn-2*i+1]=pg.c1*fft3[fft3_px+2*fft3_size-4*i-3]+
                         pg.c2*fft3[fft3_px+2*fft3_size-4*i-1]-
                         pg.c3*fft3[fft3_px+2*fft3_size-4*i-2];
      }
    }
  fftback(mix2.size, mix2.n, fft3_tmp, 
                               mix2.table, mix2.permute, yieldflag_ndsp_mix2);
// Store baseb_wb_raw
  if(genparm[THIRD_FFT_SINPOW] == 2)
    {
    for(i=0; i<sizhalf; i++)
      {
      baseb_wb_raw[2*baseb_pa+2*i  ]+=fft3_tmp[2*i  ];
      baseb_wb_raw[2*baseb_pa+2*i+1]+=fft3_tmp[2*i+1];
      }
    k=sizhalf;
    p0=(baseb_pa+sizhalf)&baseband_mask;
    for(i=0; i<sizhalf; i++)
      {
      baseb_wb_raw[2*p0+2*i  ]=fft3_tmp[2*(i+k)  ];
      baseb_wb_raw[2*p0+2*i+1]=fft3_tmp[2*(i+k)+1];
      }
    }
  else
    {
    p0=baseb_pa;
    k=mix2.interleave_points-2*(mix2.crossover_points/2);
    j=k/2+mix2.crossover_points;
    ia=2*mix2.crossover_points;
    for(i=0; i<ia; i+=2)
      {
      baseb_wb_raw[2*p0  ]=baseb_wb_raw[2*p0  ]*mix2.cos2win[i>>1]+fft3_tmp[i+k  ]*mix2.sin2win[i>>1];
      baseb_wb_raw[2*p0+1]=baseb_wb_raw[2*p0+1]*mix2.cos2win[i>>1]+fft3_tmp[i+k+1]*mix2.sin2win[i>>1];
      p0=(p0+1)&baseband_mask;
      } 
    ib=mix2.new_points+2+2*(mix2.crossover_points/2);
    for(i=ia; i<ib; i+=2)
      {
      baseb_wb_raw[2*p0  ]=fft3_tmp[i+k  ]*mix2.window[j];
      baseb_wb_raw[2*p0+1]=fft3_tmp[i+k+1]*mix2.window[j];
      p0=(p0+1)&baseband_mask;
      j++;
      } 
    j--;  
    ic=2*mix2.new_points;  
    for(i=ib; i<ic; i+=2)
      {
      j--;
      baseb_wb_raw[2*p0  ]=fft3_tmp[i+k  ]*mix2.window[j];
      baseb_wb_raw[2*p0+1]=fft3_tmp[i+k+1]*mix2.window[j];
      p0=(p0+1)&baseband_mask;
      } 
    id=2*(mix2.crossover_points+mix2.new_points);
    for(i=ic; i<id; i+=2)
      {
      baseb_wb_raw[2*p0  ]=fft3_tmp[i+k  ];
      baseb_wb_raw[2*p0+1]=fft3_tmp[i+k+1];
      p0=(p0+1)&baseband_mask;
      }
    }    
  }
last_point=(baseb_pa+mix2.new_points)&baseband_mask;
ampfac=cg_size*daout_gain/bg_amplimit;
// *******************************************************************
// Calculate the amplitude of the signal and the amplitude of the carrier.
// Store sines and cosines of the carrier phase.
// Make an I/Q demodulator using the carrier phase.
// Use this "zero level" coherent detect data to make the upper coherent
// graph that will help the user select coherence ratio for unstable
// signals.
t2=0;
t5=0;
cg_mid=cg_size/2;
big=0.499*cg_size;
ia=baseb_pa;
i=(baseb_pa+baseband_mask)&baseband_mask;
if(genparm[CW_DECODE_ENABLE] != 0 || bg.agc_flag == 1)
  {
  t2=baseb_upthreshold[i];
  }
if(bg.agc_flag == 2)
  {
  t2=baseb_upthreshold[2*i  ];
  t5=baseb_upthreshold[2*i+1];
  }
t3=0;
t4=0;
while(ia != last_point)
  {
  baseb_totpwr[ia]=baseb_raw[2*ia  ]*baseb_raw[2*ia  ]+
                   baseb_raw[2*ia+1]*baseb_raw[2*ia+1];
  t4+=baseb_totpwr[ia];
  if(t3 < baseb_totpwr[ia])t3=baseb_totpwr[ia];
  baseb_carrier_ampl[ia]=sqrt(baseb_carrier[2*ia  ]*baseb_carrier[2*ia  ]+
                              baseb_carrier[2*ia+1]*baseb_carrier[2*ia+1]);
  if(genparm[CW_DECODE_ENABLE] != 0 || bg.agc_flag == 1)
    {
    t2*=cg_decay_factor;
    if(t2 < baseb_totpwr[ia])t2=baseb_totpwr[ia];
    baseb_upthreshold[ia]=t2;
    }
  if(baseb_carrier_ampl[ia]==0)
    {
    baseb_carrier_ampl[ia]=0.0000000000001;
    }
  else
    {   
    baseb_carrier[2*ia  ]/=baseb_carrier_ampl[ia];
    baseb_carrier[2*ia+1]/=baseb_carrier_ampl[ia];
    }
  baseb[2*ia  ]=baseb_carrier[2*ia  ]*baseb_raw[2*ia  ]+
                baseb_carrier[2*ia+1]*baseb_raw[2*ia+1];
  baseb[2*ia+1]=baseb_carrier[2*ia  ]*baseb_raw[2*ia+1]-
                baseb_carrier[2*ia+1]*baseb_raw[2*ia  ];
  if(bg.agc_flag == 2)
    {
    baseb[2*ia  ]-=baseb_carrier_ampl[ia];
    t2*=cg_decay_factor;
    if(t2 < baseb[2*ia  ]*baseb[2*ia  ])t2=baseb[2*ia  ]*baseb[2*ia  ];
    baseb_upthreshold[2*ia  ]=t2;
    t5*=cg_decay_factor;
    if(t5 < baseb[2*ia+1]*baseb[2*ia+1])t5=baseb[2*ia+1]*baseb[2*ia+1];
    baseb_upthreshold[2*ia+1]=t5;
    }
  r1=ampfac*baseb[2*ia  ];
  r2=ampfac*baseb[2*ia+1];
  if(genparm[CW_DECODE_ENABLE] != 0)
    {
    baseb_fit[2*ia  ]=0;
    baseb_fit[2*ia+1]=0;
    baseb_envelope[2*ia  ]=0;
    baseb_envelope[2*ia+1]=0;
    }
  ia=(ia+1)&baseband_mask;
  if(fabs(r1)>big || fabs(r2)>big)
    {
    t1=big/sqrt(r1*r1+r2*r2);
    r1*=t1;
    r2*=t1;
    }
  ix=cg_mid+r1;
  iy=cg_mid+r2;
  cg_map[iy*cg_size+ix]+=1;
  }
basblock_maxpower[basblock_pa]=t3;
basblock_avgpower[basblock_pa]=t4/sizhalf;
basblock_pa=(basblock_pa+1)&basblock_mask;
// **********************************************************
// If morse decoding is enabled, compute the fourier transform
// of the real part of baseb_wb. Since this array is not needed
// for any other purpose, we do not store it so its real part
// is computed from baseb_carrier and baseb_wb_raw.
// The phase is zero in good
// regions where the carrier is correct. Use the level of the
// carrier as a weight factor to suppress regions where
// the signal is likely to be absent or incorrect.
if(genparm[CW_DECODE_ENABLE] != 0)
  {
  if(fft3_slowsum_cnt < 1)
    {
    lir_sched_yield();
    if(mix1_selfreq[0]>0)lirerr(341296);
    return;
    }
  noise_floor*=2.818/fft3_slowsum_cnt;
  baseband_noise_level=noise_floor*bgfil_weight;
  carrier_noise_level=noise_floor*carrfil_weight;
  ia=(last_point-mix2.size+baseband_mask)&baseband_mask;
  if( ((ia-baseb_px+baseband_size)&baseband_mask) < baseband_neg)
    {
    for(i=0; i<mix2.size; i++)
      {
      t1=P2SCALE*baseb_carrier_ampl[ia]*baseb_carrier_ampl[ia];
      if(t1 > 0)
        {
        mix2_tmp[2*i]=P4SCALE*(baseb_carrier[2*ia  ]*baseb_wb_raw[2*ia  ]+
                               baseb_carrier[2*ia+1]*baseb_wb_raw[2*ia+1])*
                               sqrt(t1)*cw_carrier_window[i];
        }
      else
        {
        mix2_tmp[2*i  ]=0;
        }
      mix2_tmp[2*i+1]=0;
      ia=(ia+1)&baseband_mask;
      }
    fftforward(mix2.size, mix2.n, mix2_tmp, 
                               mix2.table, mix2.permute, yieldflag_ndsp_mix2);
// The fourier transform here should have been computed
// with a real to hermitian fft implementation. Just filling
// the complex part with zeroes is inefficient.
//
// The keying spectra arrive at a rate of 2*baseband_sampling_speed/mix2.size.
// Accumulate power spectra in a leaky integrator that has a life time
// of 15 seconds.
    t2=1-keying_spectrum_f1;
    keying_spectrum_cnt++;
    for(i=0; i<keying_spectrum_size; i++)
      {
      keying_spectrum[i]=keying_spectrum_f1*keying_spectrum[i]+
                 t2*(pow(mix2_tmp[2*i  ],2.0)+pow(mix2_tmp[2*i+1],2.0));
      }
    sc[SC_SHOW_KEYING_SPECTRUM]++;
    }
  }
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
cg_update_count++;
if(cg_update_count > cg_update_interval)
  {
  sc[SC_SHOW_COHERENT]++;
  cg_update_count=0;
  }
// Make data for the S-meter graph.
if(mg.avgnum > 0 && new_baseb_flag == 0)
  {
  j=((last_point-baseb_pm+baseband_mask)&baseband_mask)/mg.avgnum;
  if(j > 0)
    {
    sellim_correction=1;  
    if(genparm[SECOND_FFT_ENABLE] != 0)
      {
      if(!swfloat && mix1_selfreq[0]>=0)
        {
        ia=mix1_selfreq[0]*fftx_points_per_hz;
        ia/=fft2_to_fft1_ratio;
        if(liminfo[ia]>0)
          {
          sellim_correction=1/pow(liminfo[ia],2.0);
          }
        }
      }
    fac=baseband_pwrfac*sellim_correction;  
    for(i=0; i<j; i++)
      {
      t1=0.000000001;
      t2=0.000000001;
      ia=(baseb_pm+mg.avgnum)&baseband_mask;
      if(sw_onechan)
        {
        while(baseb_pm != ia)
          {
          t1+=baseb_totpwr[baseb_pm];
          if(t2 < baseb_totpwr[baseb_pm])t2=baseb_totpwr[baseb_pm];
          baseb_pm=(baseb_pm+1)&baseband_mask;
          }
        mg_rms_meter[mg_pa]=log10(fac*t1/mg.avgnum);      
        mg_peak_meter[mg_pa]=log10(fac*t2);      
        }
      else
        {  
        t3=0.000000001;
        t4=0.000000001;
        while(baseb_pm != ia)
          {
          t1+=baseb_totpwr[baseb_pm];
          if(t2 < baseb_totpwr[baseb_pm])t2=baseb_totpwr[baseb_pm];
          r1=baseb_raw_orthog[2*baseb_pm  ]*baseb_raw_orthog[2*baseb_pm  ]+
             baseb_raw_orthog[2*baseb_pm+1]*baseb_raw_orthog[2*baseb_pm+1];
          t3+=r1;
          if(t4 < r1)t4=r1;
          baseb_pm=(baseb_pm+1)&baseband_mask;
          }
        mg_rms_meter[2*mg_pa  ]=log10(fac*t1/mg.avgnum);      
        mg_rms_meter[2*mg_pa+1]=log10(fac*t3/mg.avgnum);      
        mg_peak_meter[2*mg_pa  ]=log10(fac*t2);      
        mg_peak_meter[2*mg_pa+1]=log10(fac*t4);      
        }
      mg_pa=(mg_pa+1)&mg_mask;
      mg_valid++;    
      }
    if(cg.meter_graph_on != 0)sc[SC_UPDATE_METER_GRAPH]++;
    }
  if(mg_valid > mg_size)mg_valid=mg_size;  
  }  
// baseb_upthreshold is a fast attack, slow release signal follower 
// to baseb_totpwr. Make a fast attack, slow release signal
// follower in the reverse direction and store the largest of the 
// forward and backward peak detector -6dB in baseb_threshold.
ia=last_point;
ib=(baseb_pa+baseband_mask)&baseband_mask;
t2=0;
t5=0;
k=2;
ic=baseb_pb;
while(k!=0 && ia!=ic)
  {
  if(ia == ib)
    {
    k=1;
    }
  else
    {
    k|=1;
    }
  ia=(ia+baseband_mask)&baseband_mask;
  if(genparm[CW_DECODE_ENABLE] != 0 || bg.agc_flag == 1)
    {
    t2*=cg_decay_factor;
    if(t2 < baseb_totpwr[ia])
      {
      t2=baseb_totpwr[ia];
      }
    t1=t2;
    if(t1<baseb_upthreshold[ia])
      {
      t1=baseb_upthreshold[ia];
      }
    else
      {
      k&=2;
      }
    baseb_threshold[ia]=0.25*t1;
    }
  if(bg.agc_flag == 2)
    {
    t2*=cg_decay_factor;
    if(t2 < baseb[2*ia]*baseb[2*ia])
      {
      t2=baseb[2*ia]*baseb[2*ia];
      }
    t1=t2;
    if(t1<baseb_upthreshold[2*ia])
      {
      t1=baseb_upthreshold[2*ia];
      }
    else
      {
      k&=2;
      }
    baseb_threshold[2*ia]=0.25*t1;
    t5*=cg_decay_factor;
    if(t5 < baseb[2*ia+1]*baseb[2*ia+1])
      {
      t5=baseb[2*ia+1]*baseb[2*ia+1];
      }
    t1=t5;
    if(t1<baseb_upthreshold[2*ia+1])
      {
      t1=baseb_upthreshold[2*ia+1];
      }
    else
      {
      k&=2;
      }
    baseb_threshold[2*ia+1]=0.25*t1;
    }
  }
if(bg.agc_flag == 1)
  {
  while(ia != last_point)
    {
    rx_agc_sumpow1=agc_attack_factor1*rx_agc_sumpow1+
                           agc_attack_factor2*baseb_threshold[ia];
    rx_agc_sumpow2=agc_attack_factor1*rx_agc_sumpow2+
                                 agc_attack_factor2*rx_agc_sumpow1;
    rx_agc_factor1*=agc_release_factor;
    if(rx_agc_factor1 < rx_agc_sumpow2)
      {
      rx_agc_factor1=rx_agc_sumpow2;
      }
    baseb_agc_level[ia]=sqrt(rx_agc_factor1);
    ia=(ia+1)&baseband_mask;
    }
  }
if(bg.agc_flag == 2)
  {
  while(ia != last_point)
    {
    rx_agc_sumpow1=agc_attack_factor1*rx_agc_sumpow1+
                           agc_attack_factor2*baseb_threshold[2*ia];
    rx_agc_sumpow2=agc_attack_factor1*rx_agc_sumpow2+
                                 agc_attack_factor2*rx_agc_sumpow1;
    rx_agc_factor1*=agc_release_factor;
    rx_agc_factor1*=agc_release_factor;
    if(rx_agc_factor1 < rx_agc_sumpow2)
      {
      rx_agc_factor1=rx_agc_sumpow2;
      }
    baseb_agc_level[2*ia]=sqrt(rx_agc_factor1);
    rx_agc_sumpow3=agc_attack_factor1*rx_agc_sumpow3+
                           agc_attack_factor2*baseb_threshold[2*ia+1];
    rx_agc_sumpow4=agc_attack_factor1*rx_agc_sumpow4+
                               agc_attack_factor2*rx_agc_sumpow3;
    rx_agc_factor2*=agc_release_factor;
    if(rx_agc_factor2 < rx_agc_sumpow4)
      {
      rx_agc_factor2=rx_agc_sumpow4;
      }
    baseb_agc_level[2*ia+1]=sqrt(rx_agc_factor2);
    ia=(ia+1)&baseband_mask;
    }
  }
// Put whatever output the operator asked for in baseb_out    
p0=baseb_pa;
if(bg_delay != 0)
  {
// Synthetic stereo from mono by time delay of one channel is selected.  
// just send data direct and delayed. Do not worry about orthog
// if we have two channels.
  pb=(baseb_pa+bg.delay_points)&baseband_mask;
  if(use_bfo != 0)
    {
    while(p0 != last_point)
      {
      baseb_out[4*pb  ]=baseb_raw[2*p0];
      baseb_out[4*p0+2]=baseb_raw[2*p0];     
      baseb_out[4*pb+1]=baseb_raw[2*p0+1];
      baseb_out[4*p0+3]=baseb_raw[2*p0+1];
      pb=(pb+1)&baseband_mask;
      p0=(p0+1)&baseband_mask;
      }
    }
  else
    {    
    if(rx_mode == MODE_AM)
      {
      while(p0 != last_point)
        {
        t1=sqrt(baseb_totpwr[p0]);
        am_dclevel1=am_dclevel1*am_dclevel_factor1+t1*am_dclevel_factor2;
        baseb_out[4*pb  ]=t1-am_dclevel1;     
        baseb_out[4*p0+2]=t1-am_dclevel1;     
        baseb_out[4*pb+1]=0;
        baseb_out[4*p0+3]=0;      
        pb=(pb+1)&baseband_mask;
        p0=(p0+1)&baseband_mask;
        }
      }  
    else
      {
// The user selected FM. Make an FM detector.
// The user specified a delayed signal in the second channel.
// The delay is probably useless............
      while(p0 != last_point)
        {
        t2=am_dclevel1;
        t1=atan2(baseb_raw[2*p0+1],baseb_raw[2*p0]);
        am_dclevel1=t1;
        t1-=t2;
        if(t1 > PI_L)t1-=2*PI_L;                
        if(t1 < -PI_L)t1+=2*PI_L;                
        t1*=1000;
        baseb_out[4*pb  ]=t1;     
        baseb_out[4*p0+2]=t1;     
        baseb_out[4*pb+1]=0;
        baseb_out[4*p0+3]=0;      
        pb=(pb+1)&baseband_mask;
        p0=(p0+1)&baseband_mask;
        }
      }   
    }
  }
else  
  {
  switch (bg_coherent)
    {     
    case 0:
// No fancy processing at all. 
// Just store signal as it is or with a detector to fit the current mode.
    if(use_bfo != 0)
      {
      if(sw_onechan || bg_twopol == 0)
        {
        while(p0 != last_point)
          {
          baseb_out[2*p0  ]=baseb_raw[2*p0  ];     
          baseb_out[2*p0+1]=baseb_raw[2*p0+1];     
          p0=(p0+1)&baseband_mask;
          }
        }  
      else    
        {
        while(p0 != last_point)
          {
          baseb_out[4*p0  ]=baseb_raw[2*p0  ];     
          baseb_out[4*p0+1]=baseb_raw[2*p0+1];     
          baseb_out[4*p0+2]=baseb_raw_orthog[2*p0  ];     
          baseb_out[4*p0+3]=baseb_raw_orthog[2*p0+1];     
          p0=(p0+1)&baseband_mask;
          }
        }
      }
    else
      {  
      if(rx_mode == MODE_AM)
        {
// The user selected AM. Make amplitude detection.
        if(sw_onechan || bg_twopol == 0)
          {
          while(p0 != last_point)
            {
            t1=sqrt(baseb_totpwr[p0]);
            am_dclevel1=am_dclevel1*am_dclevel_factor1+t1*am_dclevel_factor2;
            baseb_out[2*p0  ]=t1-am_dclevel1;     
            baseb_out[2*p0+1]=0;     
            p0=(p0+1)&baseband_mask;
            }
          }  
        else    
          {
          while(p0 != last_point)
            {
            t1=sqrt(baseb_totpwr[p0]);
            am_dclevel1=am_dclevel1*am_dclevel_factor1+t1*am_dclevel_factor2;
            baseb_out[4*p0  ]=t1-am_dclevel1;     
            baseb_out[4*p0+1]=0;     
            t1=sqrt(baseb_raw_orthog[2*p0  ]*baseb_raw_orthog[2*p0  ]+
                    baseb_raw_orthog[2*p0+1]*baseb_raw_orthog[2*p0+1]);
            am_dclevel2=am_dclevel2*am_dclevel_factor1+t1*am_dclevel_factor2;
            baseb_out[4*p0+2]=t1-am_dclevel2;     
            baseb_out[4*p0+3]=0;     
            p0=(p0+1)&baseband_mask;
            }
          }
        }
      else
        {
// The user selected FM. Make an FM detector.
        if(sw_onechan || bg_twopol == 0)
          {
          while(p0 != last_point)
            {
            t2=am_dclevel1;
            t1=atan2(baseb_raw[2*p0+1],baseb_raw[2*p0]);
            am_dclevel1=t1;
            t1-=t2;
            if(t1 > PI_L)t1-=2*PI_L;                
            if(t1 < -PI_L)t1+=2*PI_L;                
//            fprintf(dmp,"\n%d   %f",p0, t1/PI_L);
            baseb_out[2*p0  ]=1000*t1;     
            baseb_out[2*p0+1]=0;      
            p0=(p0+1)&baseband_mask;
            }
          }
        else
          {
          while(p0 != last_point)
            {
            t2=am_dclevel1;
            t1=atan2(baseb_raw[2*p0+1],baseb_raw[2*p0]);
            am_dclevel1=t1;
            t1-=t2;
            if(t1 > PI_L)t1-=2*PI_L;                
            if(t1 < -PI_L)t1+=2*PI_L;                
            t1*=1000;
            baseb_out[4*p0  ]=t1;     
            baseb_out[4*p0+1]=0;      
            t2=am_dclevel2;
            t1=atan2(baseb_raw_orthog[2*p0+1],baseb_raw_orthog[2*p0]);
            am_dclevel2=t1;
            t1-=t2;
            if(t1 > PI_L)t1-=2*PI_L;                
            if(t1 < -PI_L)t1+=2*PI_L;                
            t1*=1000;
            baseb_out[4*p0+2]=t1;     
            baseb_out[4*p0+3]=0;      
            p0=(p0+1)&baseband_mask;
            }
          }
        }  
      }
    break;       

    case 1:
// Send the signal to one ear, the carrier to the other.
    if(use_bfo != 0)
      {
      while(p0 != last_point)
        {
        baseb_out[4*p0  ]=baseb_raw[2*p0  ];     
        baseb_out[4*p0+1]=baseb_raw[2*p0+1];  
        baseb_out[4*p0+2]=baseb_carrier[2*p0  ]*baseb_carrier_ampl[p0];     
        baseb_out[4*p0+3]=baseb_carrier[2*p0+1]*baseb_carrier_ampl[p0];     
        p0=(p0+1)&baseband_mask;
        }
      }
    else
      {
      if(rx_mode == MODE_AM)
        {
// The user selected AM. Make amplitude detection.
// Here it is possible to listen to AM in two separate filters.
        while(p0 != last_point)
          {
          t1=sqrt(baseb_totpwr[p0]);
          am_dclevel1=am_dclevel1*am_dclevel_factor1+t1*am_dclevel_factor2;
          baseb_out[4*p0  ]=t1-am_dclevel1;     
          baseb_out[4*p0+1]=0;     
          t1=sqrt(baseb_carrier_ampl[p0]);
          am_dclevel2=am_dclevel2*am_dclevel_factor1+t1*am_dclevel_factor2;
          baseb_out[4*p0+2]=t1-am_dclevel2;     
          baseb_out[4*p0+3]=0;     
          p0=(p0+1)&baseband_mask;
          }
        }
      else
        {
// The user selected FM. 
// Here it is possible to listen to FM in two separate filters.
        while(p0 != last_point)
          {
          t2=am_dclevel1;
          t1=atan2(baseb_raw[2*p0+1],baseb_raw[2*p0]);
          am_dclevel1=t1;
          t1-=t2;
          if(t1 > PI_L)t1-=2*PI_L;                
          if(t1 < -PI_L)t1+=2*PI_L;                
          t1*=1000;
          baseb_out[4*p0  ]=t1;     
          baseb_out[4*p0+1]=0;      
          t2=am_dclevel2;
          t1=atan2(baseb_carrier[2*p0+1],baseb_carrier[2*p0]);
          am_dclevel2=t1;
          t1-=t2;
          if(t1 > PI_L)t1-=2*PI_L;                
          if(t1 < -PI_L)t1+=2*PI_L;                
          t1*=1000;
          baseb_out[4*p0+2]=t1;     
          baseb_out[4*p0+3]=0;      
          p0=(p0+1)&baseband_mask;
          }
        }  
      }    
    break;

    case 2:
// Use the carrier phase to make I/Q demodulator.
// We already did it in the coherent function so just use the data.
// Send I to one ear, Q to the other.
// This means that we send AM modulation to one ear, FM modulation to
// the other ear.
// To detect FM this way, the carrier filter must be narrower than
// the modulation to be detected.
// The carrier also has to be stronger than the modulation sidebands
// which will typically not be the case!!.
    if(rx_mode == MODE_AM)
      {
      while(p0 != last_point)
        {
        am_dclevel1=am_dclevel1*am_dclevel_factor1+baseb[2*p0  ]*am_dclevel_factor2;
        baseb_out[4*p0  ]=baseb[2*p0  ]-am_dclevel1;     
        baseb_out[4*p0+1]=0;     
        baseb_out[4*p0+2]=baseb[2*p0+1];     
        baseb_out[4*p0+3]=0;     
        p0=(p0+1)&baseband_mask;
        }
      }  
    else
      {
      while(p0 != last_point)
        {
        baseb_out[4*p0  ]=baseb[2*p0  ];     
        baseb_out[4*p0+1]=0;     
        baseb_out[4*p0+2]=baseb[2*p0+1];     
        baseb_out[4*p0+3]=0;     
        p0=(p0+1)&baseband_mask;
        }
      }
    break;
       
    case 3:
// Use the carrier phase to make I/Q demodulator.
// We already did it in the coherent function so just use the data.
// Send I to both ears, but only if greater than zero. Skip Q.
    if(rx_mode == MODE_AM)
      {
      while(p0 != last_point)
        {
        baseb_out[2*p0]=baseb[2*p0];
        am_dclevel1=am_dclevel1*am_dclevel_factor1+baseb_out[2*p0]*am_dclevel_factor2;
        baseb_out[2*p0  ]-=am_dclevel1;     
        p0=(p0+1)&baseband_mask;
        }
      }
    else
      {
      while(p0 != last_point)
        {
        if(baseb[2*p0] > 0)
          {
          baseb_out[2*p0]=baseb[2*p0];
          }     
        else 
          {
          baseb_out[2*p0]=0;
          }     
        baseb_out[2*p0+1]=0;     
        p0=(p0+1)&baseband_mask;
        }
      }
    break;
       
    default:
    lirerr(82615);
    }
  }
// baseb_threshold is inaccurate since the backwards fast attack
// slow release peak detector can not have the decay corresponding
// to what will happen after baseb_pa.
// Set baseb_pb to point at a distance where the next key down
// has decayed backwards. 
baseb_pa=last_point;
baseb_pb=(baseb_pa-(int)(20*cg_code_unit)+baseband_size)&baseband_mask;
baseb_wts+=sizhalf;
fft3_px=(fft3_px+fft3_block)&fft3_mask;
timf3_py=(timf3_py+twice_rxchan*fft3_new_points)&timf3_mask;
}
