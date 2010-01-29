	
#define YWF 4
#define YBO 4
#define DA_GAIN_RANGE 8.
#define DA_GAIN_REF 1.
#define BG_MINYCHAR 8
#define BG_MIN_WIDTH (36*text_width)

#include <unistd.h>
#include <string.h>
#include "globdef.h"
#include "uidef.h"
#include "fft3def.h"
#include "fft2def.h"
#include "fft1def.h"
#include "sigdef.h"
#include "seldef.h"
#include "vernr.h"
#include "screendef.h"
#include "graphcal.h"
#include "thrdef.h"
#include "options.h"

int binshape_points;
int binshape_total;

int bg_filter_points;
int baseband_graph_scro;
int bg_old_x1;
int bg_old_x2;
int bg_old_y1;
int bg_old_y2;
int daout_upsamp;
int new_bg_agc_flag;


float make_interleave_ratio(int nn);
void prepare_mixer(MIXER_VARIABLES *m, int nn);

void clear_baseb_arrays(int nn,int k)
{
memset(&baseb_raw[2*nn],0,2*k*sizeof(float));
memset(&baseb_raw_orthog[2*nn],0,2*k*sizeof(float));
memset(&baseb[2*nn],0,2*k*sizeof(float));
memset(&baseb_carrier[2*nn],0,2*k*sizeof(float));
memset(&baseb_carrier_ampl[nn],0,k*sizeof(float));
memset(&baseb_totpwr[nn],0,k*sizeof(float));
if(bg.agc_flag != 0)
  {
  memset(&baseb_agc_level[nn],0,k*sizeof(float));
  memset(&baseb_threshold[bg.agc_flag*nn],0,bg.agc_flag*k*sizeof(float));
  memset(&baseb_upthreshold[bg.agc_flag*nn],0,bg.agc_flag*k*sizeof(float));
  }
lir_sched_yield();
if(genparm[CW_DECODE_ENABLE] != 0)
  {
  if(bg.agc_flag == 0)
    {
    memset(&baseb_threshold[nn],0,k*sizeof(float));
    memset(&baseb_upthreshold[nn],0,k*sizeof(float));
    }
  memset(&baseb_ramp[nn],-cg_code_unit,k*sizeof(short int));
  memset(&baseb_wb_raw[2*nn],0,2*k*sizeof(float));
  memset(&baseb_fit[2*nn],0,2*k*sizeof(float));
  memset(&baseb_tmp[2*nn],0,2*k*sizeof(float));
  memset(&baseb_sho1[2*nn],0,2*k*sizeof(float));
  memset(&baseb_sho2[2*nn],0,2*k*sizeof(float));
  memset(&baseb_envelope[2*nn],0,2*k*sizeof(float));
  }
lir_sched_yield();
}

int filcur_pixel(int points)
{
float t1;
t1=(float)(2*points)/fft3_size;
t1=sqrt(t1);
return bg_first_xpixel+t1*bg_xpixels;
//return bg_first_xpixel+(bg_xpixels*2*points)/fft3_size;
}

int filcur_points(void)
{
float t1;
t1=(float)(mouse_x-bg_first_xpixel+1)/bg_xpixels;
t1=t1*t1;
return 0.5*fft3_size*t1;
}

void make_bg_waterf_cfac(void)
{
bg_waterf_cfac=bg.waterfall_gain*0.1;
bg_waterf_czer=10*bg.waterfall_zero;
}

void make_bg_yfac(void)
{
if(genparm[SECOND_FFT_ENABLE]==0)
  {
  bg.yfac_power=FFT1_BASEBAND_FACTOR;
  bg.yfac_power*=ui.rx_rf_channels*ui.rx_rf_channels;
  }
else
  {  
  bg.yfac_power=FFT2_BASEBAND_FACTOR*
                (float)(1<<genparm[FIRST_BCKFFT_ATT_N])/fft2_size;
  if(fft_cntrl[FFT2_CURMODE].mmx != 0)
    {
    bg.yfac_power*=1.4*(1<<(genparm[SECOND_FFT_ATT_N]));
    }
  bg.yfac_power*=bg.yfac_power;
  bg.yfac_power*=(float)(1<<genparm[MIX1_BANDWIDTH_REDUCTION_N]);
  }
bg.yfac_power*=(1 + 1/(0.5+genparm[FIRST_FFT_SINPOW]))/(float)(fft1_size);
bg.yfac_power/=fft3_size;
baseband_pwrfac=10*bg.yfac_power/fft3_size;
bg_waterf_yfac=20*baseband_pwrfac/bg.waterfall_avgnum;
bg.yfac_power/=bg.yzero*bg.yzero;
bg.yfac_log=10/bg.db_per_pixel;
}

void make_daout_gain(void)
{
float t1;
if(daout_gain_y>bg_y0)daout_gain_y=bg_y0;
if(daout_gain_y<bg_ymax)daout_gain_y=bg_ymax;
t1=(float)((bg_y0+bg_ymax)/2-daout_gain_y)/(bg_y0-bg_ymax);
bg.output_gain=pow(10.,t1*DA_GAIN_RANGE)/DA_GAIN_REF;    
if(rx_mode == MODE_FM)
  {
  daout_gain=0.005*bg.output_gain;
  }
else
  {
  daout_gain=0.5*bg.output_gain/fft3_size;
  if(genparm[SECOND_FFT_ENABLE]!=0)
    {
    daout_gain/=fft2_size;
    daout_gain*=1<<genparm[FIRST_BCKFFT_ATT_N];
    if(fft_cntrl[FFT2_CURMODE].mmx != 0)
      {
      daout_gain*=1<<(genparm[SECOND_FFT_ATT_N]);
      }
    }
  daout_gain*=sqrt((1 + 1/(0.5+genparm[FIRST_FFT_SINPOW]))/
                                                   (float)(fft1_size));
  }
if(rx_daout_bytes == 2)
  {
  daout_gain*=256;
  }
}

void make_daout_gainy(void)
{
int old;
float t1;
old=daout_gain_y;
t1=log10(bg.output_gain*DA_GAIN_REF)/DA_GAIN_RANGE;
daout_gain_y=(bg_y0+bg_ymax)/2-t1*(bg_y0-bg_ymax);
make_daout_gain();
update_bar(bg_vol_x1,bg_vol_x2,bg_y0,daout_gain_y,old,
                                                 BG_GAIN_COLOR,bg_volbuf);
}

void make_new_daout_upsamp(void)
{
int k, old;
float t1;
old=daout_upsamp;
t1=da_resample_ratio;
daout_upsamp=1;
if( (ui.network_flag & NET_RXOUT_BASEB) != 0)return;
k=genparm[DA_OUTPUT_SPEED];
while(t1 > 5 && k > 16000 && daout_upsamp < 8)
  {
  k/=2;
  t1/=2;
  daout_upsamp*=2;
  }
if(daout_upsamp != old)baseb_reset_counter++;
}

void make_bfo(void)
{
float bforef, bfooff, t1;
if(rx_mode == MODE_SSB)sc[SC_FREQ_READOUT]++;
if(use_bfo == 0)
  {
  rx_daout_cos=1;
  rx_daout_sin=0;
  rx_daout_phstep_sin=0;
  rx_daout_phstep_cos=1;
  bfo_xpixel=-1;
  bfo10_xpixel=-1;
  bfo100_xpixel=-1;
  return;
  }
if(bfo_xpixel > 0)
  {
  if(bg_filterfunc_y[bfo_xpixel] > 0)
                     lir_setpixel(bfo_xpixel,bg_filterfunc_y[bfo_xpixel],14);
  if(bg_filterfunc_y[bfo10_xpixel] > 0)
                 lir_setpixel(bfo10_xpixel,bg_filterfunc_y[bfo10_xpixel],14);
  if(bg_filterfunc_y[bfo100_xpixel] > 0)
               lir_setpixel(bfo100_xpixel,bg_filterfunc_y[bfo100_xpixel],14);
  if(bg_carrfilter_y[bfo_xpixel] > 0)
                     lir_setpixel(bfo_xpixel,bg_carrfilter_y[bfo_xpixel],14);
  if(bg_carrfilter_y[bfo10_xpixel] > 0)
                 lir_setpixel(bfo10_xpixel,bg_carrfilter_y[bfo10_xpixel],14);
  if(bg_carrfilter_y[bfo100_xpixel] > 0)
               lir_setpixel(bfo100_xpixel,bg_carrfilter_y[bfo100_xpixel],14);
  }
// When we arrive here only bg.bfo_freq is defined.
// Set up the other variables we need that depend on it. 
make_new_daout_upsamp();
t1=(2*PI_L*bg.bfo_freq*daout_upsamp)/genparm[DA_OUTPUT_SPEED];
while(t1 > 0.3*PI_L && daout_upsamp > 1)
  {
  daout_upsamp/=2;
  t1/=2;
  }
t1=(2*PI_L*bg.bfo_freq*daout_upsamp)/genparm[DA_OUTPUT_SPEED];
rx_daout_phstep_cos=cos(t1);
rx_daout_phstep_sin=sin(t1);
bforef=bg_first_xpixel+0.5+bg.pixels_per_point*(fft3_size/2-bg_first_xpoint);
bfooff=bg.pixels_per_point*bg.bfo_freq/bg_hz_per_pixel;
bfo_xpixel=bforef+bfooff;
if(bfo_xpixel < bg_first_xpixel)bfo_xpixel=bg_first_xpixel;
if(bfo_xpixel > bg_last_xpixel)bfo_xpixel=bg_last_xpixel;
bfo10_xpixel=bforef+0.1*bfooff;
if(bfo10_xpixel < bg_first_xpixel)bfo10_xpixel=bg_first_xpixel;
if(bfo10_xpixel > bg_last_xpixel)bfo10_xpixel=bg_last_xpixel;
bfo100_xpixel=bforef+0.01*bfooff;
if(bfo100_xpixel < bg_first_xpixel)bfo100_xpixel=bg_first_xpixel;
if(bfo100_xpixel > bg_last_xpixel)bfo100_xpixel=bg_last_xpixel;
lir_line(bfo100_xpixel, bg_y0,bfo100_xpixel,bg_y1,12);
if(kill_all_flag) return;
lir_line(bfo10_xpixel, bg_y1-1,bfo10_xpixel,bg_y2,12);
if(kill_all_flag) return;
lir_line(bfo_xpixel, bg_y2-1,bfo_xpixel,bg_y3,12);
}

void make_audio_signal(void)
{
int ptr, i,j,i1,i2,i3,i4,mm,nn;
float t1,t2,t3,t4,t5,t6,t7;
double dt1;
short int *intvar;
float r1,r2,a1,a2,b1,b2;
float rdiff, final_gain1, final_gain2;
// Resample the baseband signal so we get the correct sampling speed
// for the D/A converter.
// For each interval in baseb_out there is a non integer number of points
// in da_output.
dt1=1/sqrt(rx_daout_sin*rx_daout_sin+rx_daout_cos*rx_daout_cos);
rx_daout_sin*=dt1;
rx_daout_cos*=dt1;
final_gain1=final_gain2=0;
mm=rxda.frame;
nn=2*baseb_channels;
r1=daout_pa/(mm*da_resample_ratio);
resamp:;
r2=(daout_pa+mm)/(mm*da_resample_ratio);
i2=baseb_fx+r1+.5;
if(r2>r1)
  {
  i3=baseb_fx+r2+.5;
  }
else
  {
  i3=baseb_fx+r2+.5+daout_size/(mm*da_resample_ratio);
  }  
i2&=baseband_mask;
i3&=baseband_mask;
if(abs(i3-i2) > 1)
  {
  i2=baseb_fx+(r1+r2)/2;
  i3=i2+1;
  i2&=baseband_mask;
  i3&=baseband_mask;
  }
else
  {
  if(i3==i2)
    {
    i2=baseb_fx+r1;
    i2&=baseband_mask;
    if(i3==i2)
      {
      i3=i2+1;
      i3&=baseband_mask;
      }
    }
  }        
i4=(i3+1)&baseband_mask;
baseb_py=i4;
i1=(i2+baseband_mask)&baseband_mask;
baseb_wts=baseb_pa-r2-baseb_fx;
if(baseb_wts<0)baseb_wts+=baseband_size;
if( ((daout_px-daout_pa+daout_bufmask)&daout_bufmask) <= rx_daout_block)
  {
  return;
  }
if( baseb_wts > 2)
  {
  rdiff=r1+baseb_fx-i2;
  if(rdiff > baseband_size/2)
    {
    rdiff-=baseband_size;
    }
  dt1=rx_daout_cos;
  rx_daout_cos= rx_daout_cos*rx_daout_phstep_cos+rx_daout_sin*rx_daout_phstep_sin;
  rx_daout_sin= rx_daout_sin*rx_daout_phstep_cos-dt1*rx_daout_phstep_sin;
// Use Lagrange's interpolation formula to fit a third degree
// polynomial to 4 points:
//  a1=-rdiff *   (rdiff-1)*(rdiff-2)*baseb_out[nn*i1]/6
//     +(rdiff+1)*(rdiff-1)*(rdiff-2)*baseb_out[nn*i2]/2
//     -(rdiff+1)* rdiff   *(rdiff-2)*baseb_out[nn*i3]/2
//     +(rdiff+1)* rdiff   *(rdiff-1)*baseb_out[nn*i4]/6; 
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
  final_gain1=daout_gain;
  final_gain2=daout_gain;
  if(bg.agc_flag == 1)
    {
    if( daout_gain*baseb_agc_level[i2] > bg_agc_amplimit)
      {
      final_gain1=bg_agc_amplimit/baseb_agc_level[i2];
      final_gain2=final_gain1;
      }
    }    
  if(bg.agc_flag == 2)
    {
    if( daout_gain*baseb_agc_level[2*i2] > bg_agc_amplimit)
      {
      final_gain1=bg_agc_amplimit/baseb_agc_level[2*i2];
      }
    if( daout_gain*baseb_agc_level[2*i2+1] > bg_agc_amplimit)
      {
      final_gain2=bg_agc_amplimit/baseb_agc_level[2*i2+1];
      }
    }    
  if(baseb_channels == 1)
    {
    a1=final_gain1*(((t5*baseb_out[nn*i4  ]-t6*baseb_out[nn*i1  ])/3
                    +t4*baseb_out[nn*i2  ]-t7*baseb_out[nn*i3  ])/2);
    a2=final_gain1*(((t5*baseb_out[nn*i4+1]-t6*baseb_out[nn*i1+1])/3
                    +t4*baseb_out[nn*i2+1]-t7*baseb_out[nn*i3+1])/2);
    if( (ui.network_flag&NET_RXOUT_BASEB) != 0)
      {
      short int *ntbuf;
      ntbuf=(void*)&baseb_netsend_buffer[basebnet_pa];
      if(rx_daout_bytes == 1)
        {
        ntbuf[0]=floor(256*a2);
        ntbuf[1]=floor(256*a1);
        }
      else
        {
        ntbuf[0]=floor(a2);
        ntbuf[1]=floor(a1);        
        }
      basebnet_pa=(basebnet_pa+4)&basebnet_mask;
      }
    if(bg_expand != 2)
      {
      t1=sqrt(a1*a1+a2*a2);
      if(t1 > 0.5)
        {
        if(t1<bg_amplimit)t2=t1; else t2=bg_amplimit;
        if(bg_expand == 1)t2=bg_expand_a*(exp(bg_expand_b*t2)-1);
        if(bg_maxamp < t2)bg_maxamp=t2; 
        t2/=t1;
        t2*=a1*rx_daout_cos+a2*rx_daout_sin;
        }
      else
        {
        t2=0;
        }  
      if(rx_daout_bytes == 1)
        {
        daout[daout_pa]=0x80+t2;
        if(rx_daout_channels == 2)daout[daout_pa+1]=0x80+t2*da_ch2_sign;
        }
      else
        {
        i=t2;
        intvar=(void*)(&daout[daout_pa]);
        intvar[0]=i;
        if(rx_daout_channels == 2)intvar[1]=i*da_ch2_sign;
        }  
      }
    else
      {
      t2=a1*rx_daout_cos+a2*rx_daout_sin;
      if(t2 > bg_amplimit)t2=bg_amplimit;
      if(t2 < -bg_amplimit)t2=-bg_amplimit;
      if(bg_maxamp < t2)bg_maxamp=t2; 
      if(rx_daout_bytes == 1)
        {
        daout[daout_pa]=0x80+t2;
        if(rx_daout_channels == 2)daout[daout_pa+1]=0x80+t2*da_ch2_sign;
        }
      else
        {
        i=t2;
        intvar=(void*)(&daout[daout_pa]);
        intvar[0]=i;
        if(rx_daout_channels == 2)intvar[1]=i*da_ch2_sign;
        }  
      }
    }
  else
    {
    a1=final_gain1*(((t5*baseb_out[nn*i4  ]-t6*baseb_out[nn*i1  ])/3
                    +t4*baseb_out[nn*i2  ]-t7*baseb_out[nn*i3  ])/2);
    a2=final_gain1*(((t5*baseb_out[nn*i4+1]-t6*baseb_out[nn*i1+1])/3
                    +t4*baseb_out[nn*i2+1]-t7*baseb_out[nn*i3+1])/2);
    b1=final_gain2*(((t5*baseb_out[nn*i4+2]-t6*baseb_out[nn*i1+2])/3
                    +t4*baseb_out[nn*i2+2]-t7*baseb_out[nn*i3+2])/2);
    b2=final_gain2*(((t5*baseb_out[nn*i4+3]-t6*baseb_out[nn*i1+3])/3
                    +t4*baseb_out[nn*i2+3]-t7*baseb_out[nn*i3+3])/2);
    if( (ui.network_flag&NET_RXOUT_BASEB) != 0)
      {
      short int *ntbuf;
      ntbuf=(void*)&baseb_netsend_buffer[basebnet_pa];
      if(rx_daout_bytes == 1)
        {
        ntbuf[0]=floor(256*a2);
        ntbuf[1]=floor(256*a1);
        ntbuf[2]=floor(256*b2);
        ntbuf[3]=floor(256*b1);
        }
      else
        {
        ntbuf[0]=floor(a2);
        ntbuf[1]=floor(a1);        
        ntbuf[2]=floor(b2);
        ntbuf[3]=floor(b1);        
        }
      basebnet_pa=(basebnet_pa+8)&basebnet_mask;
      }
    if(bg_expand != 2 && bg.agc_flag != 2)
      {
      t1=a1*a1+a2*a2;
      t2=b1*b1+b2*b2;
      if(t1 < t2)t1=t2;
      t1=sqrt(t1);
      if(t1 > 0.5)
        {
        if(t1<bg_amplimit)t2=t1; else t2=bg_amplimit;
        if(bg_expand == 1)t2=bg_expand_a*(exp(bg_expand_b*t2)-1);
        if(bg_maxamp < t2)bg_maxamp=t2; 
        t2/=t1;
        t1=t2*(a1*rx_daout_cos+a2*rx_daout_sin);
        t2=t2*(b1*rx_daout_cos+b2*rx_daout_sin);
        }
      else
        {
        t1=0;
        t2=0;
        }  
      if(rx_daout_bytes == 1)
        {
        daout[daout_pa]=0x80+t1;
        daout[daout_pa+1]=0x80+t2;
        }
      else
        {
        intvar=(void*)(&daout[daout_pa]);
        intvar[0]=t1;
        intvar[1]=t2;
        }  
      }
    else
      {
      t1=a1*rx_daout_cos+a2*rx_daout_sin;
      t2=b1*rx_daout_cos+b2*rx_daout_sin;
      if(t1 > bg_amplimit)t1=bg_amplimit;
      if(t1 < -bg_amplimit)t1=-bg_amplimit;
      if(t2 > bg_amplimit)t2=bg_amplimit;
      if(t2 < -bg_amplimit)t2=-bg_amplimit;
      if(bg_maxamp < t1)bg_maxamp=t1; 
      if(bg_maxamp < t2)bg_maxamp=t2; 
      if(rx_daout_bytes == 1)
        {
        daout[daout_pa]=0x80+t1;
        daout[daout_pa+1]=0x80+t2;
        }
      else
        {
        intvar=(void*)(&daout[daout_pa]);
        intvar[0]=t1;
        intvar[1]=t2;
        }  
      }
    }
  r1=r2;
  ptr=(daout_pa-mm*(daout_upsamp+daout_size))&daout_bufmask;
  if(daout_upsamp==1)
    {
    daout_pa=(daout_pa+mm)&daout_bufmask;
    }
  else
    {
    if(rx_daout_channels == 2)
      {
      if(rx_daout_bytes == 1)
        {
// Two channels, one byte.        
        t1=(float)((char)(daout[daout_pa]-0x80)-(char)(daout[ptr]-0x80))/daout_upsamp;
        t2=(char)(daout[ptr]-0x80);
        t3=(float)((char)(daout[daout_pa+1]-0x80)-(char)(daout[ptr+1]-0x80))/daout_upsamp;
        t4=(char)(daout[ptr+1]-0x80);
        for(i=1; i<daout_upsamp; i++)
          {
          t2+=t1;
          t4+=t3;
          daout[ptr+2*i  ]=0x80+t2;
          daout[ptr+2*i+1]=0x80+t4;
          }
        daout_pa=(daout_pa+2*daout_upsamp)&daout_bufmask;
        }
      else
        {
// Two channels, two bytes.        
        intvar=(void*)(&daout[daout_pa]);
        i=intvar[0];
        j=intvar[1];
        intvar=(void*)(&daout[ptr]);
        t1=(float)(i-intvar[0])/daout_upsamp;
        t2=intvar[0];
        t3=(float)(j-intvar[1])/daout_upsamp;
        t4=intvar[1];
        for(i=1; i<daout_upsamp; i++)
          {
          t2+=t1;
          t4+=t3;
          intvar[2*i  ]=t2;
          intvar[2*i+1]=t4;
          }
        daout_pa=(daout_pa+4*daout_upsamp)&daout_bufmask;
        }  
      } 
    else
      {
      if(rx_daout_bytes == 1)
        {
// One channel, one byte.        
        t1=(float)((char)(daout[daout_pa]-0x80)-(char)(daout[ptr]-0x80))/daout_upsamp;
        t2=(char)(daout[ptr]-0x80);
        for(i=1; i<daout_upsamp; i++)
          {
          t2+=t1;
          daout[ptr+i]=0x80+t2;
          }
        daout_pa=(daout_pa+daout_upsamp)&daout_bufmask;
        }  
      else
        {
// One channel, two bytes.        
        intvar=(void*)(&daout[daout_pa]);
        i=intvar[0];
        intvar=(void*)(&daout[ptr]);
        t1=(float)(i-intvar[0])/daout_upsamp;
        t2=intvar[0];
        for(i=1; i<daout_upsamp; i++)
          {
          t2+=t1;
          intvar[i]=t2;
          }
        daout_pa=(daout_pa+2*daout_upsamp)&daout_bufmask;
        }  
      }
    }    
  if(daout_pa > daout_bufmask)lirerr(994578);
  if(daout_pa == 0)
    {
    baseb_fx+=r2;
    r1=0; 
    da_resample_ratio=new_da_resample_ratio;
    if(baseb_fx>baseband_size)baseb_fx-=baseband_size;
    if(use_bfo != 0)
      {
      t1=(2*PI_L*bg.bfo_freq*daout_upsamp)/genparm[DA_OUTPUT_SPEED];
      rx_daout_phstep_cos=cos(t1);
      rx_daout_phstep_sin=sin(t1);
      }
    }
  goto resamp;  
  }
}

void chk_bg_avgnum(void)
{
if(fft3_blocktime*bg.fft_avgnum > genparm[BASEBAND_STORAGE_TIME])
           bg.fft_avgnum=genparm[BASEBAND_STORAGE_TIME]/fft3_blocktime;
if(bg.fft_avgnum >9999)bg.fft_avgnum=9999;
if(bg.fft_avgnum <1)bg.fft_avgnum=1;
}

void clear_agc(void)
{
// The AGC attack is operated from two series connected low pass filters.
// followed by a peak detector.
// The AGC is in mix2.c
rx_agc_factor1=1;
rx_agc_factor2=1;
rx_agc_sumpow1=0;
rx_agc_sumpow2=0;
rx_agc_sumpow3=0;
rx_agc_sumpow4=0;
agc_attack_factor1=pow(0.5,5000./(baseband_sampling_speed*(1<<bg.agc_attack)));
agc_release_factor=pow(0.5,500./(baseband_sampling_speed*(1<<bg.agc_release)));
agc_attack_factor2=1-agc_attack_factor1;
}

void init_basebmem(void)
{
float t1;
int i, k, clr_size;
// Sampling rate for baseband has changed.
// Set flag to flush old data and reinit output.
lir_sched_yield();
if( (cg.oscill_on&6) != 0)
  {
  clear_cg_traces();
  cg.oscill_on=1;
  cg_osc_ptr=0;
  }
if(all_threads_started)
  {
  i=0;
  while(thread_status_flag[THREAD_RX_OUTPUT]!=THRFLAG_IDLE &&
     thread_status_flag[THREAD_RX_OUTPUT]!=THRFLAG_NOT_ACTIVE)
    {
    if(i>20)
      {
      lirerr(946253);
      return;
      }
    i++;
    lir_sleep(50000);  
    }
  }
if(baseband_handle != NULL)
  {
  if(kill_all_flag)return;
  baseband_handle=chk_free(baseband_handle);
  }
// Allow 0.2 s in the daout buffer.
daout_size=0.2*rxda.frame*genparm[DA_OUTPUT_SPEED];
daout_size/=rx_daout_block;
// Make sure the buffer will always hold at least four blocks.
if(daout_size < 4)daout_size=4;
daout_size*=rx_daout_block;
make_power_of_two(&daout_size);
t1=1.5*(float)(daout_size)/genparm[DA_OUTPUT_SPEED];
if(t1<genparm[BASEBAND_STORAGE_TIME])t1=genparm[BASEBAND_STORAGE_TIME];
// Allocate memory for transformation from fft3 to the baseband.
// We are already in the baseband but the filter in use may allow 
// a reduction of the sampling speed.
baseband_size=t1*baseband_sampling_speed;
make_power_of_two(&baseband_size);
daout_bufmask=daout_size*rxda.frame-1;
cg_size=2+(COH_SIDE*text_width)/3;
cg_size&=0xfffffffe;
k=2+bg_flatpoints+2*bg_curvpoints;
if(genparm[THIRD_FFT_SINPOW] > 3)k+=2;
if(genparm[THIRD_FFT_SINPOW] == 8)k+=2;
cg_code_unit=0.5*(float)(mix2.size)/k;
cg_decay_factor=pow(0.5,0.2/cg_code_unit);
cw_waveform_max=14*cg_code_unit;
cg_osc_offset=mix2.size+50*cg_code_unit;
while(baseband_size < 4*cg_osc_offset)baseband_size*=2;
reduce:;
if(baseband_size < 4*cg_osc_offset)
  {
  cg_osc_offset=baseband_size/4;
  }
cg_osc_offset_inc=cg_osc_offset/2;
max_cwdat=MAX_CW_PARTS*baseband_size/cg_code_unit;
baseband_mask=baseband_size-1;
baseband_neg=(7*baseband_size)>>3;
baseband_sizhalf=baseband_size>>1;
// The time stored in the baseband buffers is 
// baseband_size/baseband_sampling_speed, the same time is contained in
// the basblock buffers in mix2.size/2 fewer points.
// Find out the number of basblock points that correspond to 
// 5 seconds, the time constant for dB meter peak hold. 
basblock_size=2*baseband_size/mix2.size;
basblock_mask=basblock_size-1;
basblock_hold_points=5*basblock_size*baseband_sampling_speed/baseband_size;
if(basblock_hold_points<3)basblock_hold_points=3;
if(basblock_hold_points>basblock_mask)basblock_hold_points=basblock_mask;
// When listening to wideband signals, the time between basblock
// points may become short.
// Make sure we do not update the coherent graph too often, 
// graphics may be rather slow.
cg_update_interval=basblock_hold_points/20;
cg_update_count=0;
clear_agc();
// Jan 2009:
fft3_interleave_ratio=make_interleave_ratio(THIRD_FFT_SINPOW);
// We get signals from back transformation of fft3.
// We must use an interleave ratio that makes the interleave points
// go even up in mix2.size.     
mix2.interleave_points=fft3_interleave_ratio*mix2.size;
mix2.interleave_points&=0xfffffffe;  
mix2.new_points=mix2.size-mix2.interleave_points;
fft3_interleave_points=mix2.interleave_points*(fft3_size/mix2.size);
fft3_interleave_ratio=(float)(fft3_interleave_points)/fft1_size;
fft3_new_points=fft3_size-fft3_interleave_points;
fft3_blocktime=(float)(fft3_new_points)/timf3_sampling_speed;
// ********************************************************
init_memalloc(basebmem, MAX_BASEB_ARRAYS);
mem(1,&baseb_out,baseband_size*2*baseb_channels*sizeof(float),0);
mem(2,&baseb_carrier,baseband_size*2*sizeof(float),0);
mem(3,&baseb_raw,baseband_size*2*sizeof(float),0);
mem(4,&baseb_raw_orthog,baseband_size*2*sizeof(float),0);
mem(5,&baseb,baseband_size*2*sizeof(float),0);
mem(6,&baseb_totpwr,baseband_size*sizeof(float),0);
mem(7,&baseb_carrier_ampl,baseband_size*sizeof(float),0);
mem(8,&mix2.permute,mix2.size*sizeof(short int),0);
mem(9,&mix2.table,mix2.size*sizeof(COSIN_TABLE)/2,0);
// allocate space so we will not overflow even if channels
// and bytes become doubled.
mem(10,&daout,daout_size*rxda.frame,0);             
mem(11,&cg_map,cg_size*cg_size*sizeof(float),0);
mem(13,&cg_traces,CG_MAXTRACE*MAX_CG_OSCW*sizeof(short int),0);
mem(16,&basblock_maxpower,basblock_size*sizeof(float),0);
mem(17,&basblock_avgpower,basblock_size*sizeof(float),0);
mem(171,&mix2.window,(mix2.size/2+1)*sizeof(float),0);
mem(172,&mix2.cos2win,mix2.new_points*sizeof(float),0);
mem(173,&mix2.sin2win,mix2.new_points*sizeof(float),0);
if(bg.agc_flag != 0)
  {
  mem(18,&baseb_agc_level,bg.agc_flag*baseband_size*sizeof(float),0);
  mem(14,&baseb_upthreshold,bg.agc_flag*baseband_size*sizeof(float),0);
  mem(15,&baseb_threshold,bg.agc_flag*baseband_size*sizeof(float),0);
  }
if(genparm[CW_DECODE_ENABLE] != 0)
  {
  if(bg.agc_flag == 0)
    {
    mem(14,&baseb_upthreshold,baseband_size*sizeof(float),0);
    mem(15,&baseb_threshold,baseband_size*sizeof(float),0);
    }
  mem(27,&baseb_ramp,baseband_size*sizeof(short int),0);
  keying_spectrum_size=mix2.size/cg_code_unit;
  if(keying_spectrum_size > mix2.size/2)keying_spectrum_size=mix2.size/2;
  mem(19,&baseb_envelope,baseband_size*2*sizeof(float),0);
  mem(21,&dash_waveform,cw_waveform_max*2*sizeof(float),0);
  mem(215,&dash_wb_waveform,cw_waveform_max*2*sizeof(float),0);
  mem(225,&dot_wb_waveform,cw_waveform_max*sizeof(float),0);
  mem(23,&baseb_fit,baseband_size*2*sizeof(float),0);
  mem(24,&cw_carrier_window,mix2.size*sizeof(float),0);
  mem(25,&mix2_tmp,2*mix2.size*sizeof(float),0);
  mem(26,&mix2_pwr,mix2.size*sizeof(float),0);
  mem(28,&baseb_tmp,baseband_size*2*sizeof(float),0);
  mem(281,&baseb_sho1,baseband_size*2*sizeof(float),0);
  mem(282,&baseb_sho2,baseband_size*2*sizeof(float),0);
  mem(283,&baseb_wb_raw,baseband_size*2*sizeof(float),0);
  mem(30,&cw,max_cwdat*sizeof(MORSE_DECODE_DATA),0);
  mem(31,&keying_spectrum,keying_spectrum_size*sizeof(float),0);
  mem(285,&baseb_clock,baseband_size*sizeof(float),0);
  }
baseband_totmem=memalloc((int*)(&baseband_handle),"baseband");
if(baseband_totmem == 0 || lir_status==LIR_MEMERR)
  {
  lir_status=LIR_OK;
  baseband_size/=2;
  lir_pixwrite(bg.xleft+text_width,bg.ybottom-4*text_height,"BUFFERS REDUCED");
  goto reduce;
  }
k=fft3_size/mix2.size;
mix2.n=fft3_n;
while(k>1)
  {
  mix2.n--;
  k/=2;
  }
if(mix2.n > 12)
  {
  yieldflag_ndsp_mix2=TRUE;
  }
else
  {
  yieldflag_ndsp_mix2=FALSE;
  }
if(genparm[SECOND_FFT_ENABLE] == 0)
  {
  if(ui.max_blocked_cpus > 1)yieldflag_ndsp_mix2=FALSE;
  }
else  
  {
  if(ui.max_blocked_cpus > 3)yieldflag_ndsp_mix2=FALSE;
  }
prepare_mixer(&mix2, THIRD_FFT_SINPOW);
if(genparm[CW_DECODE_ENABLE] != 0)
  {
  make_window(4,mix2.size, genparm[THIRD_FFT_SINPOW], cw_carrier_window);
  }
memset(basblock_maxpower,0,basblock_size*sizeof(float));
memset(basblock_avgpower,0,basblock_size*sizeof(float));
clr_size=4*mix2.size;
if(clr_size > baseband_size)clr_size=baseband_size;  
lir_sched_yield();
clear_baseb_arrays(0,clr_size);
if(genparm[CW_DECODE_ENABLE] != 0)
  {
  for(i=0; i<max_cwdat; i++)
    {
    cw[i].tmp=-1;
    cw[i].unkn=-1;
    }
  lir_sched_yield();
  memset(keying_spectrum,0,keying_spectrum_size*sizeof(float));
  }
memset(baseb_out,0,clr_size*2*baseb_channels*sizeof(float));
baseb_pa=0;
rx_daout_cos=1;
rx_daout_sin=0;
if(kill_all_flag)return;
clear_coherent();
am_dclevel1=0;
am_dclevel2=0;
am_dclevel_factor1=pow(0.5,400./(baseband_sampling_speed*(1<<bg.agc_release)));
am_dclevel_factor2=1-am_dclevel_factor1;
baseb_indicator_block=(baseband_mask+1)/INDICATOR_SIZE;
daout_indicator_block=(daout_bufmask+1)/INDICATOR_SIZE;
DEB"\nbaseband_sampling_speed=%f",baseband_sampling_speed);
}

void construct_fir(int *points, float *filfunc, float *fir)
{
int i,j,k,ib,ic,ja,jb;
float t1,t2;
// Construct a FIR filter of size fft3 with a frequency response like the
// baseband filter we have set up.
// The array of FIR filter coefficients is the pulse response of a FIR 
// filter. Get it by taking the FFT.
// Note that a real to complex FFT would be more efficient. It is
// silly to yse the complex FFT with zeroes in the imaginary part!!
// (We might use the imaginary part to construct another FIR
// simultaneously - we use this function twice so that would be
// a very clever solution.)
fft3_tmp[0]=1;
fft3_tmp[1]=0;
i=fft3_size/2+1;
jb=fft3_size-1;
for(ja=1; ja<fft3_size/2; ja++)
  {
  t1=filfunc[i];
  fft3_tmp[2*ja  ]=t1;
  fft3_tmp[2*jb  ]=t1;
  fft3_tmp[2*ja+1]=0;
  fft3_tmp[2*jb+1]=0;
  i++;
  jb--;
  }
fft3_tmp[2*jb  ]=0;
fft3_tmp[2*jb+1]=0;
for( i=0; i<fft3_size/2; i++)
  {
  t1=fft3_tmp[2*i  ];
  t2=fft3_tmp[fft3_size+2*i  ];
  fft3_tmp[2*i  ]=t1+t2;
  fft3_tmp[fft3_size+2*i  ]=fft3_tab[i].cos*(t1-t2);
  fft3_tmp[fft3_size+2*i+1]=fft3_tab[i].sin*(t1-t2);
  } 
asmbulk_of_dif(fft3_size, fft3_n, fft3_tmp, fft3_tab, yieldflag_ndsp_fft3);
for(i=0; i < fft3_size; i+=2)
  {
  ib=fft3_permute[i];                               
  ic=fft3_permute[i+1];
  fir[ib]=fft3_tmp[2*i  ]+fft3_tmp[2*i+2];
  fir[ic]=fft3_tmp[2*i  ]-fft3_tmp[2*i+2];
  }
// Now take the effects of our window into account.
// Note the order in which fft3_window is stored.
for(i=0; i<fft3_size/2; i++)
  {
  fir[i]*=fft3_window[2*i];  
  fir[fft3_size/2+i]*=fft3_window[2*i+1];  
  }
// The FIR filter must be symmetric. Actually forcing symmetry
// might reduce rounding errors slightly.
for(i=1; i<fft3_size/2; i++)
  {
  fir[i]=0.5*(fir[i]+fir[fft3_size-i]);
  fir[fft3_size-i]=fir[i];
  }
// The fft1 algorithms use float variables with 24 bit accuracy.
// the associated spur level is -140 dB.
// There is no reason to make the FIR filter extend outside
// the range where the coefficients are below -150 dB.
t1=0.00000003*fir[fft3_size/2];
k=0;
while(fabs(fir[k]) < t1)k++;
j=k;
points[0]=0;
while(k<fft3_size-j)
  {
  fir[points[0]]=fir[k];
  k++;
  points[0]++;
  }
k=points[0];
memset(&fir[k],0,fft3_size-k);
// Normalize the FIR filter so it gives the same amplitude as we have
// with the back transformation of fft3 in mix2.
t1=0;
for(i=0; i<points[0]; i++)
  {
  t1+=fir[i];
  }
t1=2.65*fft3_size/t1;  
for(i=0; i<points[0]; i++)
  {
  fir[i]*=t1;
  }
}

void make_bg_filter(void)
{
int i,k,max,mm;
int j, iy;
int ja, jb, m;
float t1,t2,t3;
// Set up the filter function in the baseband for
// the main signal and show it on the screen.
// bg.filter_flat is the size of the flat region
// of the filter in Hz (divided by 2)
// bg.filter_curv is the curvature expressed as ten times the distance 
// in Hz to the 6dB point from the end of the flat region.
// The filter response is a flat center region with parabolic fall off.
//
// The filter is applied to the fft3 transforms with a symmetric
// function which is flat over 2*bg_flatpoints and falls off over
// bg_curvpoints at each end.
if(flat_xpixel > 0)
  {
  for(i=bg_ymax; i<=bg_y4; i++)lir_setpixel(flat_xpixel, i,bg_background[i]);
  }
if(curv_xpixel > 0)
  {
  for(i=bg_y4; i<bg_y3; i++)lir_setpixel(curv_xpixel, i,bg_background[i]);
  }
if(rx_mode != MODE_FM)
  {
// We should not generate frequencies above the Nyquist frequency 
// of our output.
  max=(float)(fft3_size)*genparm[DA_OUTPUT_SPEED]/(0.9*timf3_sampling_speed);
  if(use_bfo != 0 && (ui.network_flag & NET_RXOUT_BASEB) == 0)
    {
    max/=2;
    }
// We must make bg_filter_points smaller than fft3_size/2.
  if(max > fft3_size/2-2)max=fft3_size/2-2;
  }
else
  {
  max=fft3_size/2-2;
  }
bg_flatpoints=bg.filter_flat/bg_hz_per_pixel;
if(bg_flatpoints < 1)bg_flatpoints=1;
bg_curvpoints=0.1*bg.filter_curv/bg_hz_per_pixel;
if( genparm[CW_DECODE_ENABLE] != 0)
  {
  k=bg_xpixels/(2*bg.pixels_per_point)-NOISE_SEP-
                                NOISE_FILTERS*(NOISE_POINTS+NOISE_SEP)-1;
  if(max>k)max=k;
  k=bg_flatpoints+2*bg_curvpoints-max;  
  if(k>0)
    {
    k/=2;
    bg_flatpoints-=k;
    if(bg_flatpoints<1)bg_flatpoints=1;
    bg_curvpoints=(max-bg_flatpoints-1)/2;
    if(bg_curvpoints < 0)
      {
      k=1+2*bg_curvpoints;
      bg_flatpoints+=k;
      bg_curvpoints=0;
      }
    }
  }
else
  {
  k=bg_flatpoints+bg_curvpoints-max;  
  if(k>0)
    {
    k=(k+1)/2;
    bg_flatpoints-=k;
    if(bg_flatpoints<1)bg_flatpoints=1;
    bg_curvpoints=max-bg_flatpoints-1;
    if(bg_curvpoints < 0)
      {
      bg_flatpoints+=bg_curvpoints;
      bg_curvpoints=0;
      }
    }
  }
bg.filter_flat=bg_hz_per_pixel*bg_flatpoints;
bg.filter_curv=10*bg_hz_per_pixel*bg_curvpoints;
bg_curvpoints=0.1*bg.filter_curv/bg_hz_per_pixel;
bg_flatpoints=bg.filter_flat/bg_hz_per_pixel;
if(bg_flatpoints < 1)bg_flatpoints=1;
bgfil_weight=1;
bg_filterfunc[fft3_size/2-1]=1;
for(i=1; i<bg_flatpoints; i++)
  {
  bg_filterfunc[fft3_size/2-1+i]=1;
  bg_filterfunc[fft3_size/2-1-i]=1;
  bgfil_weight+=2;  
  }
bg_filter_points=bg_flatpoints;  
if(bg_curvpoints > 0)
  {
  t1=.5/bg_curvpoints;
  t2=1;
  t3=t1;
  t2=1-t3*t3;
  t3+=t1;
  while(t2 > 0 && bg_filter_points<fft3_size/2)
    {
    bg_filterfunc[fft3_size/2-1+bg_filter_points]=t2;
    bg_filterfunc[fft3_size/2-1-bg_filter_points]=t2;
    bgfil_weight+=2*t2*t2;  
    t2=1-t3*t3;
    t3+=t1;
    bg_filter_points++;
    }
  }  
for(i=bg_filter_points; i<fft3_size/2; i++)
  {
  bg_filterfunc[fft3_size/2-1+i]=0;
  bg_filterfunc[fft3_size/2-1-i]=0;
  }
bg_filterfunc[fft3_size-1]=0;
// The filter we just specified determines the bandwidth of
// the signal we recover when backtransforming from fft3.
// Find out by what factor we should reduce the sampling speed
// for further processing
k=bg_filter_points+binshape_points;
k=(fft3_size+k/2)/k;
make_power_of_two(&k);
if(genparm[CW_DECODE_ENABLE] != 0)k/=2;
k/=2;
if(k > 2)
  { 
  baseband_sampling_speed=2*timf3_sampling_speed/k;
  if(mix2.size != 2*fft3_size/k)
    {
    mix2.size=2*fft3_size/k;
    if(!all_threads_started)
      {
      init_basebmem();
      }
// öö   else
      {
      baseb_reset_counter++;
      }
    }
  }
else
  {
  baseband_sampling_speed=timf3_sampling_speed;   
  if(mix2.size != fft3_size)
    {
    mix2.size=fft3_size;
    if(!all_threads_started)
      {
      init_basebmem();
      }
//öö    else
      {
      baseb_reset_counter++;
      }
    }  
  }
if(mix2.size>fft3_size)
  {
  lirerr(88888);  
  return;
  }
carrfil_weight=1;
bg_carrfilter[fft3_size/2-1]=1;
mm=1;
k=bg.coh_factor;
while(mm < fft3_size/2)
  {
  if(k<fft3_size/2)
    {
    t2=bg_filterfunc[fft3_size/2-1+k];
    }
  else
    {
    t2=0;
    }  
  bg_carrfilter[fft3_size/2-1+mm]=t2;
  bg_carrfilter[fft3_size/2-1-mm]=t2;
  carrfil_weight+=2*t2*t2;
  mm++;
  k+=bg.coh_factor;
  }
bg_carrfilter[fft3_size-1]=0;
bg_filtershift_points=bg.filter_shift*(fft3_size/4+8)/999;
// Shift bg_filterfunc as specified by bg_filtershift_points
// But do it only if the mixer mode is back transformation
// and Rx mode is AM
if(bg.mixer_mode == 1)
  {
  if(rx_mode == MODE_AM && bg_filtershift_points > 0)
    {
    k=0;
    for(i=bg_filtershift_points; i< fft3_size; i++)
      {
      bg_filterfunc[k]=bg_filterfunc[i];
      k++;
      }
    while(k < fft3_size)
      {
      bg_filterfunc[k]=0;
      k++;
      }
    }
  else
    {
    k=fft3_size;
    for(i=fft3_size+bg_filtershift_points; i>=0; i--)
      {
      bg_filterfunc[i]=bg_filterfunc[k];
      k--;
      }
    while(k>=0)
      {
      bg_filterfunc[k]=0;
      k--;
      }
    }
  }  
  
  
// **************************************************************
// Place the current filter function on the screen.
// First remove any curve that may exist on the screen.
for(i=bg_first_xpixel; i<=bg_last_xpixel; i+=bg.pixels_per_point)
  {
  if(bg_filterfunc_y[i] < bg.ybottom && bg_filterfunc_y[i] >bg_ymax-2)
    {
    lir_setpixel(i,bg_filterfunc_y[i],bg_background[bg_filterfunc_y[i]]);
    }
  if(bg_carrfilter_y[i] < bg.ybottom && bg_carrfilter_y[i] >bg_ymax-2)
    {
    lir_setpixel(i,bg_carrfilter_y[i],bg_background[bg_carrfilter_y[i]]);
    }
  }
// Slide the bin filter shape over our carrier filter and accumulate
// to take the widening due to the fft window into account.
for(i=0; i<fft3_size; i++)
  {
  ja=i-fft3_size/2-1;
  jb=i+fft3_size/2-1;
  m=0;
  if(ja<0)
    {
    m=-ja;
    ja=0;
    }
  j=(fft3_size/2-binshape_total)-m;
  if(j>0)
    {
    ja+=j;
    m+=j;
    }
  if(jb > fft3_size)jb=fft3_size;    
  if(jb-ja > 2*binshape_total+1)jb=ja+2*binshape_total+1;
  t1=0;
  for(j=ja; j<jb; j++)
    {
    t1+=bg_carrfilter[j]*bg_binshape[m];
    m++;
    }
  bg_ytmp[i]=t1;
  }
if(fft3_size > 16384)lir_sched_yield();
t1=0;
for(i=0; i<fft3_size; i++)
  {
  if(bg_ytmp[i]>t1)t1=bg_ytmp[i];
  }
for(i=0; i<fft3_size; i++)
  {
  bg_ytmp[i]/=t1;
  }
j=bg_first_xpoint;
for(i=bg_first_xpixel; i<=bg_last_xpixel; i+=bg.pixels_per_point)
  {
  if(bg_ytmp[j] > 0.000000001)
    {
    iy=2*bg.yfac_log*log10(1./bg_ytmp[j]);  
    iy=bg_ymax-1+iy;
    if(iy>bg_y0)iy=bg_y0+1;  
    lir_setpixel(i,iy,58);
    }
  else
    {
    iy=-1;
    }
  bg_carrfilter_y[i]=iy;
  j++;
  }
if(bg.mixer_mode == 1)
  {
  for(i=0; i<bg_no_of_notches; i++)
    {
    j=fft3_size/2-bg_notch_pos[i]*bg_filter_points/1000;
    k=bg_notch_width[i]*bg_filter_points/1000;
    ja=j-k;
    jb=j+k;
    if(ja < 0)ja=0;
    if(jb >= fft3_size)jb=fft3_size-1;
    for(j=ja; j<=jb; j++)
      {
      bg_filterfunc[j]=0;
      }
    }
  }  
// Slide the bin filter shape over our filter function and accumulate
// to take the widening due to the fft window into account.
for(i=0; i<fft3_size; i++)
  {
  ja=i-fft3_size/2-1;
  jb=i+fft3_size/2-1;
  m=0;
  if(ja<0)
    {
    m=-ja;
    ja=0;
    }
  j=(fft3_size/2-binshape_total)-m;
  if(j>0)
    {
    ja+=j;
    m+=j;
    }
  if(jb > fft3_size)jb=fft3_size;    
  if(jb-ja > 2*binshape_total+1)jb=ja+2*binshape_total+1;
  t1=0;
  for(j=ja; j<jb; j++)
    {
    t1+=bg_filterfunc[j]*bg_binshape[m];
    m++;
    }
  bg_ytmp[i]=t1;
  }
t1=0;
for(i=0; i<fft3_size; i++)
  {
  if(bg_ytmp[i]>t1)t1=bg_ytmp[i];
  }
for(i=0; i<fft3_size; i++)
  {
  bg_ytmp[i]/=t1;
  }
bg_120db_points=fft3_size;
while(bg_ytmp[bg_120db_points-1] < 0.000001 && 
                                      bg_120db_points >0)bg_120db_points--;
bg_60db_points=bg_120db_points;
while(bg_ytmp[bg_60db_points-1] < 0.001 && 
                                        bg_60db_points >0)bg_60db_points--;
bg_6db_points=bg_60db_points;
while(bg_ytmp[bg_6db_points-1] < 0.5 && bg_6db_points >0)bg_6db_points--;
if(bg_6db_points == 0)
  {
  lirerr(287553);
  return;
  }
i=0;
while(bg_ytmp[i+1] < 0.000001)i++;
bg_120db_points-=i;
while(bg_ytmp[i+1] < 0.001)i++;
bg_60db_points-=i;
while(bg_ytmp[i+1] < 0.5)i++;
bg_6db_points-=i;
baseband_bw_hz=bg_6db_points*bg_hz_per_pixel;
baseband_bw_fftxpts=baseband_bw_hz*fftx_points_per_hz;
j=bg_first_xpoint;
for(i=bg_first_xpixel; i<=bg_last_xpixel; i+=bg.pixels_per_point)
  {
  if(j>0 && j<fft3_size && bg_ytmp[j] > 0.000000001)
    {
    iy=2*bg.yfac_log*log10(1./bg_ytmp[j]);  
    iy=bg_ymax-1+iy;
    if(iy>bg_y0)iy=bg_y0+1;  
    lir_setpixel(i,iy,14);
    }
  else
    {
    iy=-1;
    }
  bg_filterfunc_y[i]=iy;
  j++;
  }
construct_fir(&basebraw_fir_pts, bg_filterfunc, basebraw_fir);
if(fft3_size > 8192)lir_sched_yield();
construct_fir(&basebcarr_fir_pts, bg_carrfilter, basebcarr_fir);
lir_sched_yield();
flat_xpixel=filcur_pixel(bg_flatpoints);
lir_line(flat_xpixel, bg_ymax,flat_xpixel,bg_y4-1,14);
curv_xpixel=filcur_pixel(bg_curvpoints);
lir_line(curv_xpixel, bg_y4,curv_xpixel,bg_y3-1,14);
da_resample_ratio=genparm[DA_OUTPUT_SPEED]/baseband_sampling_speed;
make_new_daout_upsamp();
// Place the BFO line on the screen and make bfo related stuff.
make_bfo();
}

void check_bg_cohfac(void)
{
int j;
j=0.8*bg_6db_points;  
if(j>9999)j=9999;
if(bg.coh_factor > j)bg.coh_factor=j;
if(use_bfo != 0)
  {
  if(bg.coh_factor < 3)bg.coh_factor=3;
  }
else
  {
  if(bg.coh_factor < 1)bg.coh_factor=1;
  }
}


void new_bg_cohfac(void)
{
bg.coh_factor=numinput_int_data;
check_bg_cohfac();
init_baseband_sizes();
make_baseband_graph(TRUE);
}

void new_bg_filter_shift(void)
{
int i;
i=numinput_int_data;
// i has to be between -999 and 9999 because we allocate 4 characters
// for this parameter button.
if(i > 999)i=999;
bg.filter_shift=i;
make_baseband_graph(TRUE);
}

void new_bg_delpnts(void)
{
bg.delay_points=numinput_int_data;
if(bg.delay_points < 1)bg.delay_points=1;
if(bg.delay_points > 999)bg.delay_points=999;
sc[SC_BG_BUTTONS]++;
}


void new_bg_agc_attack(void)
{
bg.agc_attack=numinput_int_data;
sc[SC_BG_BUTTONS]++;
make_modepar_file(GRAPHTYPE_BG);
clear_agc();
}

void new_bg_agc_release(void)
{
bg.agc_release=numinput_int_data;
am_dclevel_factor1=pow(0.5,100./(baseband_sampling_speed*(1<<bg.agc_release)));
am_dclevel_factor2=1-am_dclevel_factor1;
sc[SC_BG_BUTTONS]++;
make_modepar_file(GRAPHTYPE_BG);
clear_agc();
}

void new_bg_waterfall_gain(void)
{
bg.waterfall_gain=numinput_float_data;
sc[SC_BG_BUTTONS]++;
make_bg_waterf_cfac();
make_modepar_file(GRAPHTYPE_BG);
sc[SC_BG_WATERF_REDRAW]++;
}

void new_bg_waterfall_zero(void)
{
bg.waterfall_zero=numinput_float_data;
sc[SC_BG_BUTTONS]++;
make_bg_waterf_cfac();
make_modepar_file(GRAPHTYPE_BG);
sc[SC_BG_WATERF_REDRAW]++;
}

void help_on_baseband_graph(void)
{
int msg_no;
int event_no;
// Set msg invalid in case we are not in any select area.
msg_no=-1;
if(mouse_y < bg_y0)
  {
  if(mouse_x < bg_first_xpixel)
    {
    if(mouse_x >= bg_vol_x1 && 
       mouse_x <= bg_vol_x2 &&
       mouse_y >= bg_ymax)
      {   
      msg_no=33;
      }
    }
  else
    { 
    if(mouse_x < bg.xright-text_width-6)
      { 
      msg_no=34;
      }
    }
  }
for(event_no=0; event_no<MAX_BGBUTT; event_no++)
  {
  if( bgbutt[event_no].x1 <= mouse_x && 
      bgbutt[event_no].x2 >= mouse_x &&      
      bgbutt[event_no].y1 <= mouse_y && 
      bgbutt[event_no].y2 >= mouse_y) 
    {
    switch (event_no)
      {
      case BG_TOP:
      case BG_BOTTOM:
      case BG_LEFT:
      case BG_RIGHT:
      case BG_YBORDER:
      msg_no=100;
      break;

      case BG_YSCALE_EXPAND:
      msg_no=35;
      break;

      case BG_YSCALE_CONTRACT:
      msg_no=36;
      break;

      case BG_YZERO_DECREASE:
      msg_no=37;
      break;

      case BG_YZERO_INCREASE:
      msg_no=38;
      break;

      case BG_RESOLUTION_DECREASE:
      msg_no=39;
      break;

      case BG_RESOLUTION_INCREASE:
      msg_no=40;
      break;

      case BG_OSCILLOSCOPE: 
      msg_no=41;
      break;

      case BG_OSC_INCREASE: 
      msg_no=42;
      break;

      case BG_OSC_DECREASE:
      msg_no=43;
      break;

      case BG_PIX_PER_PNT_INC:
      msg_no=44;
      break;    

      case BG_PIX_PER_PNT_DEC:
      msg_no=45;
      break;    

      case BG_TOGGLE_EXPANDER:
      msg_no=46;
      break;

      case  BG_TOGGLE_COHERENT:
      msg_no=47;
      break;

      case  BG_TOGGLE_PHASING:
      msg_no=48;
      break;

      case BG_TOGGLE_TWOPOL:
      msg_no=49;
      break;

      case  BG_TOGGLE_CHANNELS:
      msg_no=50;
      break;

      case  BG_TOGGLE_BYTES:
      msg_no=51;
      break;

      case BG_SEL_COHFAC:
      msg_no=52;
      break;

      case BG_SEL_DELPNTS:
      msg_no=53;
      break;

      case BG_SEL_FFT3AVGNUM:
      msg_no=63;
      break;

      case BG_TOGGLE_AGC:
      msg_no=79;
      break;

      
      case BG_SEL_AGC_ATTACK:
      msg_no=80;
      break;

      case BG_SEL_AGC_RELEASE:
      msg_no=81;
      break;

      case BG_WATERF_ZERO:
      msg_no=64;
      break;

      case BG_WATERF_GAIN:
      msg_no=66;
      break;

      case BG_WATERF_AVGNUM:
      msg_no=61;
      break;

      case BG_MIXER_MODE:
      msg_no=88;
      break;

      case BG_HORIZ_ARROW_MODE:
      msg_no=319;
      break;

      case BG_NOTCH_NO:
      msg_no=320;
      break;

      case BG_NOTCH_WIDTH:
      msg_no=321;
      break;  

      case BG_NOTCH_POS:
      msg_no=322;
      break;
      
      }
    }  
  }
help_message(msg_no);
}  

void change_fft3_avgnum(void)
{
int j,k;
bg.fft_avgnum=numinput_int_data;
chk_bg_avgnum();
make_modepar_file(GRAPHTYPE_BG);
if(sw_onechan)
  {  
  for(k=bg_first_xpixel; k<=bg_last_xpixel; k++)
    {
    lir_setpixel(k,fft3_spectrum[k],bg_background[fft3_spectrum[k]]);
    fft3_spectrum[k]=bg_y0;
    }
  }
else
  {
  for(k=bg_first_xpixel; k<=bg_last_xpixel; k++)
    {
    for(j=2*k; j<2*k+2; j++)
      {
      lir_setpixel(k,fft3_spectrum[j],bg_background[fft3_spectrum[j]]);
      fft3_spectrum[j]=bg_y0;
      }
    }
  }       
init_baseband_sizes();
make_baseband_graph(FALSE);
}



void change_bg_waterf_avgnum(void)
{
bg.waterfall_avgnum=numinput_int_data;
make_modepar_file(GRAPHTYPE_BG);
make_bg_yfac();
sc[SC_BG_BUTTONS]++;
}

void make_agc_amplimit(void)
{
bg_agc_amplimit=bg_amplimit*0.18;
if(rx_mode==MODE_AM)
  {
  bg_agc_amplimit*=1.2;
  if(bg_coherent != 0) bg_agc_amplimit*=.75;
  }
}

void new_bg_no_of_notches(void)
{
if(numinput_int_data > bg_no_of_notches)
  {
  if(bg_no_of_notches < MAX_BG_NOTCHES)
    {
    bg_notch_pos[bg_no_of_notches]=0;
    bg_notch_width[bg_no_of_notches]=0;
    bg_no_of_notches++;
    }
  }
bg_current_notch=numinput_int_data;
if(bg_current_notch > bg_no_of_notches)bg_current_notch=bg_no_of_notches;
if(bg_current_notch <= 0)
  {
  bg_current_notch=0;
  bg_no_of_notches=0;
  }
//sc[SC_BG_BUTTONS]++;
make_baseband_graph(TRUE);
}

void new_bg_notch_width(void)
{
int i;
if(bg_current_notch > 0)
  {
  if(numinput_int_data < 0)
    {
    if(bg_no_of_notches > 0)
      {
      i=bg_current_notch-1;
      while(i+1 < bg_no_of_notches)
        {
        bg_notch_pos[i]=bg_notch_pos[i+1];
        bg_notch_width[i]=bg_notch_width[i+1];
        i++;
        }
      bg_no_of_notches--;
      }
    }    
  else
    {
    bg_notch_width[bg_current_notch-1]=numinput_int_data;
    }
  }
//sc[SC_BG_BUTTONS]++;
make_baseband_graph(TRUE);
}


void new_bg_notch_pos(void)
{
if(bg_current_notch > 0)
  {
  bg_notch_pos[bg_current_notch-1]=numinput_int_data;
  if(bg_notch_pos[bg_current_notch-1] > 999)
                                  bg_notch_pos[bg_current_notch-1]=999;
  }
//sc[SC_BG_BUTTONS]++;
make_baseband_graph(TRUE);
}

void mouse_continue_baseband_graph(void)
{
int j;
switch (mouse_active_flag-1)
  {
  case BG_TOP:
  if(bg.ytop!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&bg,0);
    bg.ytop=mouse_y;
    j=bg.ybottom-BG_MINYCHAR*text_height;
    if(bg.ytop > j)bg.ytop=j;      
    if(bg_old_y1 > bg.ytop)bg_old_y1=bg.ytop;
    dual_graph_borders((void*)&bg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case BG_BOTTOM:
  if(bg.ybottom!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&bg,0);
    bg.ybottom=mouse_y;
    j=bg.yborder+BG_MINYCHAR*text_height;
    if(bg.ybottom < j)bg.ybottom=j;      
    if(bg.ybottom >= screen_height)bg.ybottom=screen_height-1; 
    if(bg_old_y2 < bg.ybottom)bg_old_y2=bg.ybottom;
    dual_graph_borders((void*)&bg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case BG_LEFT:
  if(bg.xleft!=mouse_x)
    {
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&bg,0);
    bg.xleft=mouse_x;
    j=bg.xright-BG_MIN_WIDTH;
    if(j<0)j=0;
    if(bg.xleft > j)bg.xleft=j;      
    if(bg_old_x1 > bg.xleft)bg_old_x1=bg.xleft;
    dual_graph_borders((void*)&bg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case BG_RIGHT:
  if(bg.xright!=mouse_x)
    {
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&bg,0);
    bg.xright=mouse_x;
    j=bg.xleft+BG_MIN_WIDTH;
    if(j>=screen_width)j=screen_width-1;
    if(bg.xright < j)bg.xright=j;      
    if(bg_old_x2 < bg.xright)bg_old_x2=bg.xright;
    dual_graph_borders((void*)&bg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  case BG_YBORDER:
  if(bg.yborder!=mouse_y)
    {
    pause_screen_and_hide_mouse();
    dual_graph_borders((void*)&bg,0);
    bg.yborder=mouse_y;
    if(bg.yborder < bg_yborder_min)bg.yborder = bg_yborder_min;
    if(bg.yborder > bg_yborder_max)bg.yborder = bg_yborder_max;
    dual_graph_borders((void*)&bg,15);
    resume_thread(THREAD_SCREEN);
    }
  break;

  default:
  goto await_release;
  }
if(leftpressed == BUTTON_RELEASED)goto finish;
return;
await_release:;
if(leftpressed != BUTTON_RELEASED) return;
switch (mouse_active_flag-1)
  {
  case BG_YSCALE_EXPAND:
  bg.yrange/=1.5;
  break;
      
  case BG_YSCALE_CONTRACT:
  bg.yrange*=1.5;
  break;
      
  case BG_YZERO_DECREASE:
  bg.yzero/=1.5;
  break;
            
  case BG_YZERO_INCREASE:
  bg.yzero*=1.5;
  break;

  case BG_RESOLUTION_DECREASE:
  bg.bandwidth/=2;
  break;

  case BG_RESOLUTION_INCREASE:
  bg.bandwidth*=2;
  break;

  case BG_OSCILLOSCOPE: 
  bg.oscill_on^=1;
  break;

  case BG_OSC_INCREASE: 
  if(bg.oscill_on != 0)
    {
    bg.oscill_gain*=5;
    }
  break;
    
  case BG_OSC_DECREASE:
  if(bg.oscill_on != 0)
    {
    bg.oscill_gain*=0.25;
    }
  break;

  case BG_PIX_PER_PNT_INC:
  bg.pixels_per_point++;
  if(bg.pixels_per_point > 16)bg.pixels_per_point=16;
  break;    

  case BG_PIX_PER_PNT_DEC:
  bg.pixels_per_point--;
  if(bg.pixels_per_point < 1)bg.pixels_per_point=1;
  break;    

  case BG_TOGGLE_EXPANDER:
  bg_expand++;
  if(bg_expand > 2)bg_expand=0;
  break;

  case  BG_TOGGLE_COHERENT:
  bg_coherent++;
  if(bg_coherent > 3)bg_coherent=0;
  if(rx_mode == MODE_FM && bg_coherent > 2)bg_coherent=0;
  if(bg_coherent == 0)bg_da_channels=1+((genparm[OUTPUT_MODE]>>1)&1);
  bg_twopol=0;
  bg_delay=0;
  make_agc_amplimit();
  break;

  case  BG_TOGGLE_PHASING:
  if(bg_coherent == 0)
    {
    bg_twopol=0;
    bg_delay^=1;
    if(bg_delay == 0)bg_da_channels=1+((genparm[OUTPUT_MODE]>>1)&1);
    }
  break;

  case BG_TOGGLE_TWOPOL:
  if(bg_coherent == 0)
    {
    bg_delay=0;
    bg_twopol^=1;
    if(bg_twopol == 0)bg_da_channels=1+((genparm[OUTPUT_MODE]>>1)&1);
    }
  break;

  case  BG_TOGGLE_CHANNELS:
  bg_da_channels=rx_daout_channels+1; 
  if(bg_da_channels>ui.rx_max_da_channels)
                                     bg_da_channels=ui.rx_min_da_channels;
  break;

  case  BG_TOGGLE_BYTES:
  bg_da_bytes=rx_daout_bytes+1;
  if(bg_da_bytes>ui.rx_max_da_bytes)bg_da_bytes=ui.rx_min_da_bytes;
  break;

  case BG_TOGGLE_AGC:
  if(genparm[CW_DECODE_ENABLE] == 0)
    {
    new_bg_agc_flag=bg.agc_flag+1;
    }
  break;
  
  case BG_SEL_COHFAC:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SEL_COHFAC].x1+7*text_width/2-1;
  numinput_ypix=bgbutt[BG_SEL_COHFAC].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_bg_cohfac;
  return;

  case BG_SEL_DELPNTS:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SEL_DELPNTS].x1+3*text_width/2-1;
  numinput_ypix=bgbutt[BG_SEL_DELPNTS].y1+2;
  numinput_chars=2;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_bg_delpnts;
  return;

  case BG_SEL_FFT3AVGNUM:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SEL_FFT3AVGNUM].x1+text_width/2-1;
  numinput_ypix=bgbutt[BG_SEL_FFT3AVGNUM].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=change_fft3_avgnum;
  return;

  case BG_WATERF_AVGNUM:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_WATERF_AVGNUM].x1+text_width/2-1;
  numinput_ypix=bgbutt[BG_WATERF_AVGNUM].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=change_bg_waterf_avgnum;
  return;

  case BG_SEL_AGC_ATTACK:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SEL_AGC_ATTACK].x1+3*text_width/2-1;
  numinput_ypix=bgbutt[BG_SEL_AGC_ATTACK].y1+2;
  numinput_chars=1;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_bg_agc_attack;
  return;

  case BG_SEL_AGC_RELEASE:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_SEL_AGC_RELEASE].x1+3*text_width/2-1;
  numinput_ypix=bgbutt[BG_SEL_AGC_RELEASE].y1+2;
  numinput_chars=1;    
  erase_numinput_txt();
  numinput_flag=FIXED_INT_PARM;
  par_from_keyboard_routine=new_bg_agc_release;
  return;

  case BG_WATERF_ZERO:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_WATERF_ZERO].x1+text_width/2-1;
  numinput_ypix=bgbutt[BG_WATERF_ZERO].y1+2;
  numinput_chars=5;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=new_bg_waterfall_zero;
  return;
 
  case BG_WATERF_GAIN:
  mouse_active_flag=1;
  numinput_xpix=bgbutt[BG_WATERF_GAIN].x1+text_width/2-1;
  numinput_ypix=bgbutt[BG_WATERF_GAIN].y1+2;
  numinput_chars=4;    
  erase_numinput_txt();
  numinput_flag=FIXED_FLOAT_PARM;
  par_from_keyboard_routine=new_bg_waterfall_gain;
  return;

  case BG_HORIZ_ARROW_MODE:
  bg.horiz_arrow_mode++;
  if(bg.horiz_arrow_mode > 2)bg.horiz_arrow_mode=0;
  break;

  case BG_MIXER_MODE:
  bg.mixer_mode++;
  if(bg.mixer_mode > 2)bg.mixer_mode=1;
  break;

  case BG_FILTER_SHIFT:
  if(rx_mode == MODE_AM && bg.mixer_mode == 1)
    {
    mouse_active_flag=1;
    numinput_xpix=bgbutt[BG_FILTER_SHIFT].x1+5*text_width/2;
    numinput_ypix=bgbutt[BG_FILTER_SHIFT].y1+2;
    numinput_chars=4;    
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_bg_filter_shift;
    }
  return;
  
  case BG_NOTCH_NO:
  if(bg.mixer_mode == 1)
    {
    mouse_active_flag=1;
    numinput_xpix=bgbutt[BG_NOTCH_NO].x1+2;
    numinput_ypix=bgbutt[BG_NOTCH_NO].y1+2;
    numinput_chars=1;    
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_bg_no_of_notches;
    return;
    }
  break;
        
  case BG_NOTCH_WIDTH:
  if(bg.mixer_mode == 1)
    {
    mouse_active_flag=1;
    numinput_xpix=bgbutt[BG_NOTCH_WIDTH].x1+3*text_width/2;
    numinput_ypix=bgbutt[BG_NOTCH_WIDTH].y1+2;
    numinput_chars=3;    
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_bg_notch_width;
    return;
    }
  break;  
    
  case BG_NOTCH_POS:
  if(bg.mixer_mode == 1)
    {
    mouse_active_flag=1;
    numinput_xpix=bgbutt[BG_NOTCH_POS].x1+7*text_width/2;
    numinput_ypix=bgbutt[BG_NOTCH_POS].y1+2;
    numinput_chars=4;    
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_bg_notch_pos;
    return;
    }
  break;
  }
finish:;
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
if(new_baseb_flag == -1)make_baseband_graph(TRUE);
baseb_reset_counter++;
}

void clear_bfo(void)
{
int i;
if(bfo_xpixel > 0)
  {
  for(i=bg_y1; i<=bg_y0; i++)lir_setpixel(bfo100_xpixel, i,bg_background[i]);
  for(i=bg_y2; i<bg_y1; i++)lir_setpixel(bfo10_xpixel, i,bg_background[i]);
  for(i=bg_y3; i<bg_y2; i++)lir_setpixel(bfo_xpixel, i,bg_background[i]);
  }
}


#define BG_BFO    1
#define BG_BFO10  2
#define BG_BFO100 3
#define BG_FLAT   4
#define BG_CURV   5
#define BG_VOLUME 6

void baseb_par_control(void)
{
int old;
unconditional_hide_mouse();
switch (bfo_flag)
  {
  case BG_BFO:  
  clear_bfo();
  bg.bfo_freq=((float)(mouse_x-bg_first_xpixel)/bg.pixels_per_point
                               -fft3_size/2+bg_first_xpoint)*bg_hz_per_pixel;
  make_bfo();
  break;

  case BG_BFO10:  
  clear_bfo();
  bg.bfo_freq=10*((float)(mouse_x-bg_first_xpixel)/bg.pixels_per_point
                               -fft3_size/2+bg_first_xpoint)*bg_hz_per_pixel;
  make_bfo();
  break;

  case BG_BFO100:  
  clear_bfo();
  bg.bfo_freq=100*((float)(mouse_x-bg_first_xpixel)/bg.pixels_per_point
                               -fft3_size/2+bg_first_xpoint)*bg_hz_per_pixel;
  make_bfo();
  break;

  case BG_FLAT:
  bg_flatpoints=filcur_points();
  if(bg_flatpoints < 1)bg_flatpoints=1;
  bg.filter_flat=bg_hz_per_pixel*bg_flatpoints;
  make_bg_filter(); 
  mg_clear_flag=TRUE;
  break;
  
  case BG_CURV:
  bg_curvpoints=filcur_points();
  if(bg_curvpoints < 0)bg_curvpoints=0;
  bg.filter_curv=bg_hz_per_pixel*10*bg_curvpoints;
  make_bg_filter(); 
  mg_clear_flag=TRUE;
  break;
    
  case BG_VOLUME:
  old=daout_gain_y;
  daout_gain_y=mouse_y;
  make_daout_gain();
  update_bar(bg_vol_x1,bg_vol_x2,bg_y0,daout_gain_y,old,
                                                 BG_GAIN_COLOR,bg_volbuf);
  break;
  }
if(kill_all_flag)return;  
lir_sched_yield();
if(leftpressed == BUTTON_RELEASED)
  {
  leftpressed=BUTTON_IDLE;
  make_modepar_file(GRAPHTYPE_BG);
// Restart output. It stops in case resampling_ratio was changed
//ööö  if((bfo_flag == BG_FLAT || bfo_flag == BG_CURV) && new_baseb_flag >= 0)
    {
    baseb_reset_counter++;
    }
  mouse_active_flag=0;
  baseb_control_flag=0;
  }
}

void mouse_on_baseband_graph(void)
{
int event_no;
// First find out if we are on a button or border line.
for(event_no=0; event_no<MAX_BGBUTT; event_no++)
  {
  if( bgbutt[event_no].x1 <= mouse_x && 
      bgbutt[event_no].x2 >= mouse_x &&      
      bgbutt[event_no].y1 <= mouse_y && 
      bgbutt[event_no].y2 >= mouse_y) 
    {
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_baseband_graph;
    return;
    }
  }
// Not button or border.
// User wants to change filter, bfo freq or gain.
// We use the upper part to change the flat region and
// the lower part to change steepness.
bfo_flag=0;
if(mouse_y < bg_y0 && mouse_y > bg_ymax)
  {
  if(mouse_x >= bg_first_xpixel)
    {
    if(mouse_y > bg_ymax && mouse_y <= bg_y4 && abs(mouse_x-flat_xpixel) < 5)
      {
      bfo_flag=BG_FLAT;
      }
    if(mouse_y > bg_y4 && mouse_y <= bg_y3 && abs(mouse_x-curv_xpixel) < 5)
      {
      bfo_flag=BG_CURV;
      }
    if(use_bfo != 0)
      {
      if(mouse_y > bg_y3 && mouse_y <= bg_y2 && abs(mouse_x-bfo_xpixel) < 5)
        {
        bfo_flag=BG_BFO;
        }
      if(mouse_y > bg_y2 && mouse_y <= bg_y1 && abs(mouse_x-bfo10_xpixel) < 5)
        {
        bfo_flag=BG_BFO10;
        }
      if(mouse_y > bg_y1 && abs(mouse_x-bfo100_xpixel) < 5)
        {
        bfo_flag=BG_BFO100;
        }
      } 
    }
  else
    {
    bfo_flag=BG_VOLUME;
    }
  }
if(bfo_flag != 0)
  {
  baseb_control_flag=1;
  current_mouse_activity=baseb_par_control;
  }  
else
  {
  current_mouse_activity=mouse_nothing;  
  }
mouse_active_flag=1;
}

void fft3_size_error(char *txt)
{  
int i;
settextcolor(15);
i=bg.yborder;
while(i<bg.ybottom-text_height)
  {
  lir_pixwrite(bg.xleft+5*text_width,i,"LIMIT");
  lir_pixwrite(bg.xleft+12*text_width,i,txt);
  i+=text_height;
  }
settextcolor(7);
}

void init_baseband_sizes(void)
{
bg.agc_flag=new_bg_agc_flag;
if(bg.agc_flag > 2)bg.agc_flag=0;
if(rx_mode != MODE_AM || bg_coherent!=2)bg.agc_flag&=1;
if(genparm[CW_DECODE_ENABLE] != 0)bg.agc_flag=0;
new_bg_agc_flag=bg.agc_flag;
if(bg_da_bytes>ui.rx_max_da_bytes ||
   bg_da_bytes<ui.rx_min_da_bytes)
  {
  bg_da_bytes=ui.rx_min_da_bytes;
  }
rx_daout_bytes=bg_da_bytes;
rx_daout_channels=bg_da_channels;
if(bg_da_channels>ui.rx_max_da_channels ||
   bg_da_channels<ui.rx_min_da_channels)
  {
  bg_da_channels=ui.rx_min_da_channels;
  }
rx_daout_channels=bg_da_channels; 
baseb_channels=1; 
if(bg_coherent > 0 && bg_coherent != 3)
  {
  if(ui.rx_max_da_channels == 1)
    {
    bg_coherent=0;
    } 
  else
    {
    baseb_channels=2;
    }    
  }
if(bg_delay != 0)
  {
  if(ui.rx_max_da_channels == 1)
    {
    bg_delay=0;
    } 
  else
    {
    baseb_channels=2;
    }    
  }
if(bg_twopol != 0)
  {
  if(ui.rx_max_da_channels == 1)
    {
    bg_twopol=0;
    } 
  else
    {
    baseb_channels=2;
    }    
  }
if(rx_daout_channels < baseb_channels)rx_daout_channels=baseb_channels;
if(rx_daout_channels > ui.rx_max_da_channels)
                           rx_daout_channels=ui.rx_max_da_channels;
if(rx_daout_channels < ui.rx_min_da_channels)
                           rx_daout_channels=ui.rx_min_da_channels;
bg_da_channels=rx_daout_channels; 
if(      baseb_channels == 1  && 
         rx_daout_channels == 2  &&
         bg_delay == 0 &&
         bg_twopol == 0 &&
         ui.rx_min_da_channels == 1)
  {
  da_ch2_sign=-1; 
  }
else
  {
  da_ch2_sign=1;
  }
rxda.frame=rx_daout_channels*rx_daout_bytes;
daout_samps=rx_daout_block/rxda.frame;
}

void make_baseband_graph(int clear_old)
{
char *stmp;
char s[80];
int volbuf_bytes;
int i,j,k,sizold,ypixels, fft3_totbytes;
int ix1,ix2,iy1,iy2,ib,ic;
double db_scalestep;
float t1, t2, t3, t4, x1, x2;
float scale_value, scale_y;
pause_thread(THREAD_SCREEN);
if(clear_old)
  {
  hide_mouse(bg_old_x1,bg_old_x2,bg_old_y1,bg_old_y2);
  lir_fillbox(bg_old_x1,bg_old_y1,bg_old_x2-bg_old_x1+1,
                                                    bg_old_y2-bg_old_y1+1,0);
  }
clear_agc();
current_graph_minh=BG_MINYCHAR*text_height;
current_graph_minw=BG_MIN_WIDTH;
check_graph_placement((void*)(&bg));
clear_button(bgbutt, MAX_BGBUTT);
hide_mouse(bg.xleft,bg.xright,bg.ytop,bg.ybottom);  
bg_yborder_min=bg.ytop+3*text_height/2+3;
bg_yborder_max=bg.ybottom-6*text_height;
if(bg.yborder < bg_yborder_min || bg.yborder > bg_yborder_max)
  {
  bg.yborder=(bg_yborder_max+bg_yborder_min) >> 1;
  }
bg_ymax=bg.yborder+text_height+4; 
bg_y0=bg.ybottom-3*text_height;
bg_y1=(4*bg_y0+bg_ymax)/5;
bg_y2=(3*bg_y0+2*bg_ymax)/5;
bg_y3=(2*bg_y0+3*bg_ymax)/5;
bg_y4=(bg_y0+4*bg_ymax)/5;
bg_avg_counter=0;
scro[baseband_graph_scro].no=BASEBAND_GRAPH;
scro[baseband_graph_scro].x1=bg.xleft;
scro[baseband_graph_scro].x2=bg.xright;
scro[baseband_graph_scro].y1=bg.ytop;
scro[baseband_graph_scro].y2=bg.ybottom;
bgbutt[BG_LEFT].x1=bg.xleft;
bgbutt[BG_LEFT].x2=bg.xleft+2;
bgbutt[BG_LEFT].y1=bg.ytop;
bgbutt[BG_LEFT].y2=bg.ybottom;
bgbutt[BG_RIGHT].x1=bg.xright-2;
bgbutt[BG_RIGHT].x2=bg.xright;
bgbutt[BG_RIGHT].y1=bg.ytop;
bgbutt[BG_RIGHT].y2=bg.ybottom;
bgbutt[BG_TOP].x1=bg.xleft;
bgbutt[BG_TOP].x2=bg.xright;
bgbutt[BG_TOP].y1=bg.ytop;
bgbutt[BG_TOP].y2=bg.ytop+2;
bgbutt[BG_BOTTOM].x1=bg.xleft;
bgbutt[BG_BOTTOM].x2=bg.xright;
bgbutt[BG_BOTTOM].y1=bg.ybottom-2;
bgbutt[BG_BOTTOM].y2=bg.ybottom;
bgbutt[BG_YBORDER].x1=bg.xleft;
bgbutt[BG_YBORDER].x2=bg.xright;
bgbutt[BG_YBORDER].y1=bg.yborder-1;
bgbutt[BG_YBORDER].y2=bg.yborder+1;
// Draw the border lines
dual_graph_borders((void*)&bg,7);
// Set variables that depend on output format.
if(rx_daout_bytes == 1)bg_amplimit=126; else bg_amplimit=32000;
// The expander (if enabled) expands the amplitude with an
// exponential function y = A* (exp(B*x)-1) 
// 0 <= x <= bg_amplimit
// y is always 0 for x=0.
// Make y=bg_amplimit/2 for x=bg_amplimit
bg_expand_b=genparm[AMPLITUDE_EXPAND_EXPONENT]/bg_amplimit;
bg_expand_a=bg_amplimit/(exp(bg_expand_b*bg_amplimit)-1);
bg_maxamp=0;
bg_amp_indicator_y=bg_y0-3;
// Set up all the variables that depend on the window parameters.
bg_first_xpixel=bg.xleft+4*text_width;
bg_last_xpixel=bg.xright-2*text_width;
bg_xpoints=2+(bg_last_xpixel-bg_first_xpixel)/bg.pixels_per_point;
bg_xpixels=bg_last_xpixel-bg_first_xpixel+1;
bg_vol_x1=bg.xleft+text_width;
bg_vol_x2=bg_vol_x1+5*text_width/2;
volbuf_bytes=(bg_y0-bg_ymax+2)*(bg_vol_x2-bg_vol_x1+2)*sizeof(char);
fft3_size=2*timf3_sampling_speed/bg.bandwidth;
if(fft3_size < 1)fft3_size=1;
i=fft3_size;
make_power_of_two(&fft3_size);
if(fft3_size > 1.5*i)fft3_size>>=1;
while(fft3_size < bg_xpoints)fft3_size<<=1;
if(fft3_size > 0x10000)
  {
  fft3_size=0x10000;
  fft3_size_error("Max N = 16");
  }
sizold=fft3_size;  
while(2*fft3_size*ui.rx_rf_channels+timf3_block > 0.8*timf3_size)fft3_size/=2;
if(sizold != fft3_size)
  {
  sizold=fft3_size;  
  if(genparm[SECOND_FFT_ENABLE] == 0)
    {
    fft3_size_error("fft1 storage time");
    }
  else
    {  
    fft3_size_error("fft2 storage time");
    }
  }
while(2*fft3_size/timf3_sampling_speed > 
                               genparm[BASEBAND_STORAGE_TIME])fft3_size/=2;
if(sizold != fft3_size)
  {
  fft3_size_error("baseband storage time");
  }
// fft3 uses sin squared window with interleave factor=2
new_fft3:;
while(fft3_size < bg_xpoints)
  {
  bg_xpoints/=2;
  bg.pixels_per_point*=2;
  }
fft3_n=0;
i=fft3_size;
while(i != 1)
  {
  i>>=1;
  fft3_n++;
  }
bg.bandwidth=1.9*timf3_sampling_speed/fft3_size;
bg_first_xpoint=(fft3_size-bg_xpoints)/2;
bg_hz_per_pixel=timf3_sampling_speed/fft3_size;  
bg_flatpoints=bg.filter_flat/bg_hz_per_pixel;
if(bg_flatpoints < 1)bg_flatpoints=1;
bg_curvpoints=0.1*bg.filter_curv/bg_hz_per_pixel;
fft3_block=fft3_size*2*ui.rx_rf_channels*MAX_MIX1;
fft3_blocktime=0.5*fft3_size/timf3_sampling_speed;
chk_bg_avgnum();
// find out how much memory we need for the fft3 buffer.
// First of all we need to hold the transforms that we use for averaging.
i=bg.fft_avgnum+2;
// And make sure it can hold two blocks of data from
// the source time function.
if(genparm[SECOND_FFT_ENABLE]!=0)
  {
  k=fft2_new_points;
  }
else
  {
  k=fft1_new_points;
  }
t1=2*k/timf1_sampling_speed;
if(t1<2)t1=2;
// make buffer big enough for t1 seconds of data
if(i < t1*bg_hz_per_pixel*2) i=t1*bg_hz_per_pixel*2; 
if(i<4)i=4;
make_power_of_two(&i);
t1=fft3_block*i;
if(t1*sizeof(float) > (float)(0x40000000))
  {
  fft3_size/=2;
  fft3_size_error("RAM memory");
  if(fft3_size >= bg_xpoints)
    {
    goto new_fft3;
    }
  bg.fft_avgnum/=2;
  goto new_fft3;
  }  
fft3_totsiz=fft3_block*i;
fft3_totbytes=fft3_totsiz*sizeof(float);
fft3_mask=fft3_totsiz-1;
fft3_show_size=bg_xpoints*bg.fft_avgnum;
bg_waterf_lines=bg.yborder-bg.ytop-text_height-YWF;
bg_waterf_y1=bg.ytop+text_height+YWF+1;
bg_waterf_y2=bg_waterf_y1+2+bg_waterf_lines/20;
if(bg_waterf_y2 > bg_waterf_y1+bg_waterf_lines-1)
                           bg_waterf_y2=bg_waterf_y1+bg_waterf_lines-1;
bg_waterf_y=bg_waterf_y2;
if(bg_waterf_y2 > bg.yborder-1)bg_waterf_y2=bg.yborder-1;
bg_waterf_yinc=bg_waterf_y2-bg_waterf_y1+1;
bg_waterf_size=bg_xpixels*bg_waterf_lines;
max_bg_waterf_times=2+0.5*bg_waterf_lines/text_height;
bg_waterf_ptr=0;
local_bg_waterf_ptr=0;
sc[SC_BG_WATERF_INIT]++;
// ***********************************************
if(fft3_handle != NULL)
  {
  fft3_handle=chk_free(fft3_handle);
  }
init_memalloc(fft3mem,MAX_FFT3_ARRAYS);
mem( 1,&fft3,fft3_totbytes,0);
mem( 2,&fft3_window,fft3_size*sizeof(float),0);
mem( 3,&fft3_tmp,2*fft3_size*(ui.rx_rf_channels+1)*sizeof(float),0);
mem( 4,&fft3_tab,fft3_size*sizeof(COSIN_TABLE)/2,0);
mem( 5,&fft3_permute,fft3_size*sizeof(short int),0);
mem( 6,&fft3_fqwin_inv,fft3_size*sizeof(float),0);
mem( 7,&fft3_power,fft3_show_size*ui.rx_rf_channels*sizeof(float),0);
mem( 8,&fft3_slowsum,bg_xpoints*ui.rx_rf_channels*sizeof(float),0);
mem( 9,&fft3_spectrum,screen_width*ui.rx_rf_channels*sizeof(short int),0);
mem(10,&bg_background,screen_height*sizeof(char),0);
mem(11,&bg_filterfunc,fft3_size*sizeof(float),0);
mem(12,&bg_filterfunc_y,screen_width*sizeof(short int),0);
mem(13,&bg_volbuf,volbuf_bytes,0);
mem(15,&bg_carrfilter,fft3_size*sizeof(float),0);
mem(16,&bg_carrfilter_y,screen_width*sizeof(short int),0);
mem(17,&bg_waterf_sum,bg_xpoints*sizeof(float),0);
mem(18,&bg_waterf,5000+bg_waterf_size*sizeof(short int),0);
mem(19,&bg_waterf_times,max_bg_waterf_times*sizeof(WATERF_TIMES),0);
mem(20,&bg_binshape,fft3_size*sizeof(float),0);
mem(21,&bg_ytmp,fft3_size*sizeof(float),0);
mem(22,&basebraw_fir,fft3_size*sizeof(float),0);
mem(23,&basebcarr_fir,fft3_size*sizeof(float),0);
// **********************************************
fft3_totmem=memalloc((int*)(&fft3_handle),"fft3");
if(fft3_totmem == 0)
  {
  fft3_size/=2;
  fft3_size_error("RAM memory");
  if(fft3_size >= bg_xpoints)
    {
    goto new_fft3;
    }
  lirerr(1056);
  return;
  }
lir_sched_yield();
memset(fft3,0,MAX_MIX1*fft3_size*4*ui.rx_rf_channels*sizeof(float));
memset(fft3_slowsum,0,bg_xpoints*ui.rx_rf_channels*sizeof(float));
memset(fft3_power,0,fft3_show_size*ui.rx_rf_channels*sizeof(float));
memset(bg_waterf_sum,0,bg_xpoints*sizeof(float));
if(fft3_size > 16384)lir_sched_yield();
fft3_slowsum_cnt=0;

for(i=0; i<max_bg_waterf_times; i++)
  {
  bg_waterf_times[i].line=5;
  bg_waterf_times[i].text[0]=0;
  }  
// Show the fft size in the upper right corner.
sprintf(s,"%2d",fft3_n);
// Clear the waterfall memory area
for(i=0;i<bg_waterf_size;i++)bg_waterf[i]=0x8000;
// Make sure we know these pixels are not on screen.
for(i=0;i<screen_width; i++)
  {
  bg_filterfunc_y[i]=-1;
  bg_carrfilter_y[i]=-1;
  }
lir_pixwrite(bg.xright-2*text_width,bg.yborder+1.5*text_height,s);
fft3_pa=0;
fft3_px=0;
fft3_indicator_block=(fft3_mask+1)/INDICATOR_SIZE;
stmp=(void*)(fft3);
sprintf(stmp,"Reserved for blanker");
i=(bg.xright-bg.xleft)/text_width-12;
if(i<0)i=0;
stmp[i]=0;
lir_pixwrite(bg.xleft+6*text_width,bg.ybottom-text_height-2,stmp);
init_fft(1,fft3_n, fft3_size, fft3_tab, fft3_permute);
make_window(1,fft3_size, genparm[THIRD_FFT_SINPOW], fft3_window);
// Find the average filter shape of an FFT bin.
// It depends on what window we have selected.
// Store a sine-wave with frequency from -0.5 bin to +0.5 bin
// in several steps. Produce the power spectrum in each case
// and collect the average.
lir_sched_yield();
for(j=-3; j<=3; j++)
  {
  t1=0;
  t2=j*PI_L/(fft3_size*3);
  for(i=0; i<fft3_size; i++)
    {
    fft3_tmp[2*i  ]=cos(t1);
    fft3_tmp[2*i+1]=sin(t1);
    t1+=t2;
    }
  for( i=0; i<fft3_size/2; i++)
    {
    t1=fft3_tmp[2*i  ]*fft3_window[2*i];
    t2=fft3_tmp[2*i+1]*fft3_window[2*i];      
    t3=fft3_tmp[fft3_size+2*i  ]*fft3_window[2*i+1];
    t4=fft3_tmp[fft3_size+2*i+1]*fft3_window[2*i+1];   
    x1=t1-t3;
    fft3_tmp[2*i  ]=t1+t3;
    x2=t4-t2;
    fft3_tmp[2*i+1]=t2+t4;
    fft3_tmp[fft3_size+2*i   ]=fft3_tab[i].cos*x1+fft3_tab[i].sin*x2;
    fft3_tmp[fft3_size+2*i +1]=fft3_tab[i].sin*x1-fft3_tab[i].cos*x2;
    } 
  asmbulk_of_dif(fft3_size, fft3_n, fft3_tmp, fft3_tab, yieldflag_ndsp_fft3);
  for(i=0; i < fft3_size; i+=2)
    {
    ib=fft3_permute[i];                               
    ic=fft3_permute[i+1];                             
    bg_binshape[ib  ]=pow(fft3_tmp[2*i  ]+fft3_tmp[2*i+2],2.0)+
                       pow(fft3_tmp[2*i+1]+fft3_tmp[2*i+3],2.0);                          
    bg_binshape[ic  ]=pow(fft3_tmp[2*i  ]-fft3_tmp[2*i+2],2.0)+
                       pow(fft3_tmp[2*i+1]-fft3_tmp[2*i+3],2.0);
    }
  if(fft3_size > 8192)lir_sched_yield();
  }
// The bin shape has to be symmetric around the midpoint so we
// use the average from both sides. Store in the lower half
// while accumulating errors in the upper half.
i=fft3_size/2-1;
k=fft3_size/2+1;
while(i>0)
  {
  t1=bg_binshape[i]-bg_binshape[k];
  bg_binshape[i]=0.5*(bg_binshape[i]+bg_binshape[k]);
  bg_binshape[k]=fabs(t1);
  i--;
  k++;
  }
t1=0;
for(i=3*fft3_size/4; i<fft3_size; i++)
  {
  t1+=bg_binshape[i];
  }
t1/=fft3_size/8;  
t2=0.5*t1;
// t1 is now twice the average error.
// Subtract it from our bg_binshape values.
for(i=0; i<=fft3_size/2; i++)
  {
  bg_binshape[i]-=t1;
  if(bg_binshape[i]<t2)bg_binshape[i]=t2;
  }
i=fft3_size/2-1;
k=fft3_size/2+1;
while(i > 0)
  {
  bg_binshape[i]=sqrt(bg_binshape[i]/bg_binshape[fft3_size/2]);
  bg_binshape[k]=bg_binshape[i];
  i--;
  k++;
  }
t2=sqrt(t2/bg_binshape[fft3_size/2]);
bg_binshape[fft3_size/2]=1;  
binshape_points=0;
i=fft3_size/2-1;
// Get the -100 dB point
while(i>1 && bg_binshape[i-1] > 0.00001)
  {
  i--;
  binshape_points++;
  }
binshape_total=binshape_points;  
while(i>0 && bg_binshape[i] > 1.5*t2)
  {
  i--;
  binshape_total++;
  }
// Now we have the spectral shape of a single FFT bin in
// bg_binshape with the maximum at position fft3_size/2.
// **************************************************************
//  *************************************************************
// When the spectrum is picked from fft1 in the mix1 process,
// mix1_fqwin is applied to suppress spurs.
// as a consequence the noise floor will not be flat in the
// baseband graph.
// Get the inverse of mix1_fqwin in fft3_size points so we can compensate.
make_window(6,fft3_size, 4, fft3_fqwin_inv);
// Write out the y scale for logarithmic spectrum graph.
// We want a point with amplitude 1<<(fft1_n/2) to be placed at the
// zero point of the dB scale. 
for(i=0; i<screen_height; i++)bg_background[i]=0;
ypixels=bg_y0-bg_ymax+1;
bg.db_per_pixel=20*log10(bg.yrange)/ypixels;
db_scalestep=1.3*bg.db_per_pixel*text_height;
adjust_scale(&db_scalestep);
if(db_scalestep < 1)
  {
  db_scalestep=1;
  bg.db_per_pixel=0.5/text_height;
  bg.yrange=pow(10.,ypixels*bg.db_per_pixel/20);
  }
t1=20*log10( bg.yzero);
i=(t1+0.5*db_scalestep)/db_scalestep;
scale_value=i*db_scalestep;
scale_y=bg_y0+(t1-scale_value)/bg.db_per_pixel;
while( scale_y > bg_ymax)
  {
  if(scale_y+text_height/2+1 < bg_y0)
    {
    i=(int)(scale_value);
    sprintf(s,"%3d",i);
    lir_pixwrite(bg.xleft+text_width/2,(int)scale_y-text_height/2+2,s);
    }
  if(scale_y+1 < bg_y0)
    {
    i=scale_y;
    bg_background[i]=BG_DBSCALE_COLOR;    
    lir_hline(bg_first_xpixel,i,bg_last_xpixel,BG_DBSCALE_COLOR);
    if(kill_all_flag) return;
    }
  scale_y-=db_scalestep/bg.db_per_pixel;
  scale_value+=db_scalestep;
  }
// Init fft3_spectrum at the zero level. 
for(i=0; i<screen_width*ui.rx_rf_channels; i++)fft3_spectrum[i]=bg_y0;
bg_show_pa=0;
fft3_slowsum_recalc=0;
make_button(bg.xleft+text_width,bg.ybottom-text_height/2-2,
                                         bgbutt,BG_YSCALE_EXPAND,24);
make_button(bg.xleft+3*text_width,bg.ybottom-text_height/2-2,
                                         bgbutt,BG_YSCALE_CONTRACT,25);
make_button(bg.xleft+5*text_width,bg.ybottom-text_height/2-2, bgbutt,
            BG_HORIZ_ARROW_MODE,arrow_mode_char[bg.horiz_arrow_mode]);

i=(bg.ybottom+bg.yborder)/2;
make_button(bg.xright-text_width,i-text_height/2-2,
                                         bgbutt,BG_YZERO_DECREASE,24);
make_button(bg.xright-text_width,i+text_height/2+2,
                                         bgbutt,BG_YZERO_INCREASE,25);
make_button(bg.xright-3*text_width,bg.ytop+text_height/2+3,
                                         bgbutt,BG_RESOLUTION_DECREASE,26);
make_button(bg.xright-text_width,bg.ytop+text_height/2+3,
                                         bgbutt,BG_RESOLUTION_INCREASE,27); 
make_button(bg.xright-text_width,bg.ybottom-text_height/2-2,
                                         bgbutt,BG_OSCILLOSCOPE,'o'); 
ix1=bg.xright-3*text_width+2;
if(bg.oscill_on != 0)
  {
  make_button(ix1, bg.ybottom-text_height/2-2, bgbutt,BG_OSC_INCREASE,'+'); 
  ix1-=2*text_width-1;
  make_button(ix1, bg.ybottom-text_height/2-2, bgbutt,BG_OSC_DECREASE,'-'); 
  ix1-=2*text_width-2;
  }
lir_sched_yield();
iy2=bg.ybottom-2;
iy1=iy2-text_height-1;
ix2=ix1-3+text_width;
ix1-=text_width;
bgbutt[BG_NOTCH_NO].x1=ix1;
bgbutt[BG_NOTCH_NO].x2=ix2;
bgbutt[BG_NOTCH_NO].y1=iy1;
bgbutt[BG_NOTCH_NO].y2=iy2;
ix2=ix1-3;
ix1-=7*text_width;
bgbutt[BG_NOTCH_WIDTH].x1=ix1;
bgbutt[BG_NOTCH_WIDTH].x2=ix2;
bgbutt[BG_NOTCH_WIDTH].y1=iy1;
bgbutt[BG_NOTCH_WIDTH].y2=iy2;
ix2=ix1-2;
ix1-=8*text_width;
//ö
bgbutt[BG_NOTCH_POS].x1=ix1;
bgbutt[BG_NOTCH_POS].x2=ix2;
bgbutt[BG_NOTCH_POS].y1=iy1;
bgbutt[BG_NOTCH_POS].y2=iy2;
make_button(bg.xleft+text_width,bg.ytop+text_height/2+3,
                                         bgbutt,BG_PIX_PER_PNT_DEC,26);
make_button(bg.xleft+3*text_width,bg.ytop+text_height/2+3,
                                         bgbutt,BG_PIX_PER_PNT_INC,27);
if(kill_all_flag) return;
bgbutt[BG_WATERF_AVGNUM].x1=2+bg.xleft;
bgbutt[BG_WATERF_AVGNUM].x2=2+bg.xleft+4.5*text_width;
bgbutt[BG_WATERF_AVGNUM].y1=bg.yborder-text_height-2;
bgbutt[BG_WATERF_AVGNUM].y2=bg.yborder-2;
bgbutt[BG_WATERF_GAIN].x1=bg.xright-4.5*text_width-2;
bgbutt[BG_WATERF_GAIN].x2=bg.xright-2;
bgbutt[BG_WATERF_GAIN].y1=bg.yborder-2*text_height-4;
bgbutt[BG_WATERF_GAIN].y2=bg.yborder-text_height-4;
bgbutt[BG_WATERF_ZERO].x1=bg.xright-5.5*text_width-2;
bgbutt[BG_WATERF_ZERO].x2=bg.xright-2;
bgbutt[BG_WATERF_ZERO].y1=bg.yborder-text_height-2;
bgbutt[BG_WATERF_ZERO].y2=bg.yborder-2;
iy1=bg.yborder+2;
iy2=bg.yborder+text_height+2;
bgbutt[BG_SEL_FFT3AVGNUM].x1=bg.xleft+2;
bgbutt[BG_SEL_FFT3AVGNUM].x2=bg.xleft+2+4.5*text_width;
bgbutt[BG_SEL_FFT3AVGNUM].y1=iy1;
bgbutt[BG_SEL_FFT3AVGNUM].y2=iy2;
ix1=bg.xleft+4+9*text_width/2;
if(rx_mode != MODE_FM)
  {
  ix2=ix1+7*text_width/2;
  bgbutt[BG_TOGGLE_AGC].x1=ix1;
  bgbutt[BG_TOGGLE_AGC].x2=ix2;
  bgbutt[BG_TOGGLE_AGC].y1=iy1;
  bgbutt[BG_TOGGLE_AGC].y2=iy2;
  ix1=ix2+2;
  ix2=ix1+5*text_width/2;
  if(bg.agc_flag == 0)
    {
    freq_readout_x1=ix1;
    bgbutt[BG_SEL_AGC_ATTACK].x1=screen_width+1;
    bgbutt[BG_SEL_AGC_ATTACK].x2=screen_width+1;
    }
  else
    {  
    bgbutt[BG_SEL_AGC_ATTACK].x1=ix1;
    bgbutt[BG_SEL_AGC_ATTACK].x2=ix2;
    }
  bgbutt[BG_SEL_AGC_ATTACK].y1=iy1;
  bgbutt[BG_SEL_AGC_ATTACK].y2=iy2;
  ix1=ix2+2;
  ix2=ix1+5*text_width/2;
  if(bg.agc_flag == 0)
    {
    bgbutt[BG_SEL_AGC_RELEASE].x1=screen_width+1;
    bgbutt[BG_SEL_AGC_RELEASE].x2=screen_width+1;
    }
  else  
    {
    bgbutt[BG_SEL_AGC_RELEASE].x1=ix1;
    bgbutt[BG_SEL_AGC_RELEASE].x2=ix2;
    freq_readout_x1=ix2;
    }
  bgbutt[BG_SEL_AGC_RELEASE].y1=iy1;
  bgbutt[BG_SEL_AGC_RELEASE].y2=iy2;
  }
else
  {
  freq_readout_x1=ix1;
  bg.agc_flag=0;
  }  
freq_readout_x1+=text_width;  
freq_readout_y1=iy1;
ix2=bg.xright-2;
ix1=ix2-1.5*text_width;
bgbutt[BG_MIXER_MODE].x1=ix1;
bgbutt[BG_MIXER_MODE].x2=ix2;
bgbutt[BG_MIXER_MODE].y1=iy1;
bgbutt[BG_MIXER_MODE].y2=iy2;
if(rx_mode == MODE_AM && bg.mixer_mode == 1)
  {
// ***********  make filter shift button ******************
  ix2=ix1-2;
  ix1-=8*text_width;
  bgbutt[BG_FILTER_SHIFT].x1=ix1;
  bgbutt[BG_FILTER_SHIFT].x2=ix2;
  bgbutt[BG_FILTER_SHIFT].y1=iy1;
  bgbutt[BG_FILTER_SHIFT].y2=iy2;
  }
freq_readout_x2=ix1-text_width;  
make_bg_yfac();
make_bg_filter();
if(kill_all_flag) return;
make_modepar_file(GRAPHTYPE_BG);
bg_flag=1;
daout_gain_y=-1;
make_daout_gainy();
// *************  Make button to select no of channels for mono modes
make_button(bg.xleft+text_width,bg.ybottom-2*text_height,
                       bgbutt,BG_TOGGLE_CHANNELS,48+bg_da_channels);
// ************** Make button to select number of bit's
ix1=bg.xleft+4+3*text_width/2;
ix2=ix1+5*text_width/2;
iy1=bg.ybottom-5*text_height/2-1;
iy2=iy1+text_height+1;
bgbutt[BG_TOGGLE_BYTES].x1=ix1;
bgbutt[BG_TOGGLE_BYTES].x2=ix2;
bgbutt[BG_TOGGLE_BYTES].y1=iy1;
bgbutt[BG_TOGGLE_BYTES].y2=iy2;
// **************   Make the button for the amplitude expander
if(use_bfo != 0)
  {
  ix1=ix2+2;
  ix2=ix1+7*text_width/2;
  bgbutt[BG_TOGGLE_EXPANDER].x1=ix1;
  bgbutt[BG_TOGGLE_EXPANDER].x2=ix2;
  bgbutt[BG_TOGGLE_EXPANDER].y1=iy1;
  bgbutt[BG_TOGGLE_EXPANDER].y2=iy2;
  }
// *********************     Make the button for coherent CW
ix1=ix2+2;
ix2=ix1+9*text_width/2;
bgbutt[BG_TOGGLE_COHERENT].x1=ix1;
bgbutt[BG_TOGGLE_COHERENT].x2=ix2;
bgbutt[BG_TOGGLE_COHERENT].y1=iy1;
bgbutt[BG_TOGGLE_COHERENT].y2=iy2;
// ***********  make coherent bandwidth factor button ******************
ix1=ix2+2;
ix2=ix1+15*text_width/2;
bgbutt[BG_SEL_COHFAC].x1=ix1;
bgbutt[BG_SEL_COHFAC].x2=ix2;
bgbutt[BG_SEL_COHFAC].y1=iy1;
bgbutt[BG_SEL_COHFAC].y2=iy2;
// ****************************************************************
// Make the button for frequency depending phase shift between ears
// This is the same as a time delay for one channel!
ix1=ix2+2;
ix2=ix1+7*text_width/2;
bgbutt[BG_TOGGLE_PHASING].x1=ix1;
bgbutt[BG_TOGGLE_PHASING].x2=ix2;
bgbutt[BG_TOGGLE_PHASING].y1=iy1;
bgbutt[BG_TOGGLE_PHASING].y2=iy2;
// ******  delay no of points button *************
ix1=ix2+2;
ix2=ix1+7*text_width/2;
bgbutt[BG_SEL_DELPNTS].x1=ix1;
bgbutt[BG_SEL_DELPNTS].x2=ix2;
bgbutt[BG_SEL_DELPNTS].y1=iy1;
bgbutt[BG_SEL_DELPNTS].y2=iy2;
// ****************************************************************
// Make the button for stereo reception of two channels
// if there are two channels.
if(ui.rx_rf_channels == 2)
  {
  ix1=ix2+2;
  ix2=ix1+7*text_width/2;
  bgbutt[BG_TOGGLE_TWOPOL].x1=ix1;
  bgbutt[BG_TOGGLE_TWOPOL].x2=ix2;
  bgbutt[BG_TOGGLE_TWOPOL].y1=iy1;
  bgbutt[BG_TOGGLE_TWOPOL].y2=iy2;
  }
else
  {
  bgbutt[BG_TOGGLE_TWOPOL].x1=screen_width+1;
  bgbutt[BG_TOGGLE_TWOPOL].x2=screen_width+1;
  }
i=bg_twopol;                 //bit 7
i=(i<<1)+bg_delay;           //bit 6
i=(i<<2)+bg_coherent;        //bits 4 and 5
i=(i<<2)+bg_expand;          //bits 2 and 3
i=(i<<1)+bg_da_channels-1;   //bit 1
i=(i<<1)+bg_da_bytes-1;      //bit 0
sprintf(s,"%3d",i);
lir_pixwrite(bg.xright-7*text_width/2,iy1+2,s);
bg_amp_indicator_x=bg.xleft+2;
show_bg_maxamp();
if(rx_mode == MODE_FM && bg_coherent > 2)bg_coherent=0;
check_bg_cohfac();
make_agc_amplimit();
bg_waterf_block=(bg_waterf_y2-bg_waterf_y1+1)*bg_xpixels;
make_bg_waterf_cfac();
if(genparm[AFC_ENABLE]==0 || rx_mode >= MODE_SSB)
  {
  sc[SC_SHOW_WHEEL]++;
  }
bg_old_y1=bg.ytop;
bg_old_y2=bg.ybottom;
bg_old_x1=bg.xleft;
bg_old_x2=bg.xright;
resume_thread(THREAD_SCREEN);
if(mix1_selfreq[0]>0)sc[SC_FREQ_READOUT]++;
sc[SC_BG_FQ_SCALE]++;
sc[SC_BG_BUTTONS]++;
}

void show_bg_maxamp(void)
{
unsigned char color;
float t1;
t1=bg_maxamp/bg_amplimit;
t1*=bg_y0-bg_ymax-2;
hide_mouse(bg_amp_indicator_x,bg_amp_indicator_x+text_width/2,
                                   bg_amp_indicator_y,bg_amp_indicator_y+3);
lir_fillbox(bg_amp_indicator_x,bg_amp_indicator_y,text_width/2,3,0);
bg_amp_indicator_y=bg_y0-t1-1;
if(t1<1)
  {
  color=10;
  }
else
  {
  if(bg_amp_indicator_y<=bg_ymax+3)
    {
    color=12;
    }
  else
    {
    color=7;
    }
  }
hide_mouse(bg_amp_indicator_x,bg_amp_indicator_x+text_width/2,
                                   bg_amp_indicator_y,bg_amp_indicator_y+3);
lir_fillbox(bg_amp_indicator_x,bg_amp_indicator_y,text_width/2,3,color);
bg_maxamp=0;
}

void init_baseband_graph(void)
{
int i,errcnt;
errcnt=0;
if (read_modepar_file(GRAPHTYPE_BG) == 0)
  {
bg_default:;  
// Make the default window for the high resolution graph.
  bg.xleft=hg.xright+2;
  bg.xright=bg.xleft+0.5*screen_width;
  if(bg.xright > screen_width)
    {
    bg.xright=screen_width-1;
    bg.xleft=bg.xright-BG_MIN_WIDTH;
    }
  bg.ytop=wg.ybottom+2;
  bg.yborder=bg.ytop+0.1*screen_height;
  bg.ybottom=bg.yborder+0.2*screen_height;
  if(bg.ybottom>screen_height-text_height-1)bg.ybottom=screen_height-text_height-1;
  bg.bandwidth=timf3_sampling_speed/256;
  bg.yrange=4096;
  bg.yzero=1;
  switch (rx_mode)
    {
    case MODE_WCW:
    bg.filter_flat=14;
    bg.filter_curv=0;
    bg.output_gain=1;
    bg.agc_flag=0;
    bg.waterfall_avgnum=5;
    bg.bandwidth/=4;
    break;
    
    case MODE_NCW:
    bg.filter_flat=250;
    bg.filter_curv=50;
    bg.output_gain=100;
    bg.agc_flag=1;
    bg.waterfall_avgnum=50;
    break;
    
    case MODE_SSB:
    bg.filter_flat=1200;
    bg.filter_curv=0;
    bg.output_gain=5000;
    bg.agc_flag=1;
    bg.waterfall_avgnum=50;
    break;
    
    case MODE_AM:
    bg.filter_flat=3000;
    bg.filter_curv=250;
    bg.output_gain=5000;
    bg.agc_flag=1;
    bg.waterfall_avgnum=200;
    break;
    
    case MODE_FM:
    bg.filter_flat=5000;
    bg.filter_curv=250;
    bg.output_gain=5;
    bg.agc_flag=0;
    bg.waterfall_avgnum=400;
    break;
    
    default:
    bg.filter_flat=150;
    bg.filter_curv=250;
    bg.output_gain=0.05;
    bg.agc_flag = 0;
    bg.waterfall_avgnum=100;
    break;
    }
  bg.bfo_freq=bg.filter_flat+0.1*bg.filter_curv+300;
  bg.check=BG_VERNR;
  bg.fft_avgnum=50;
  bg.pixels_per_point=1;
  bg.coh_factor=8;
  bg.delay_points=4;
  bg.agc_attack = 2;
  bg.agc_release = 4;
  bg.waterfall_gain=1;
  bg.waterfall_zero=20;
  bg.wheel_stepn=0;
  bg.oscill_on=0;
  bg.oscill_gain=0.1;
  bg.horiz_arrow_mode=0;
  bg.mixer_mode=1;
  bg.filter_shift=0;
  }
if(bg.check != BG_VERNR)goto bg_default;
errcnt++; 
if(errcnt < 2)
  {
  if( 
    bg.xleft < 0 || 
    bg.xleft > screen_last_xpixel ||
    bg.xright < 0 || 
    bg.xright > screen_last_xpixel ||
    bg.xright-bg.xleft < BG_MIN_WIDTH ||
    bg.ytop < 0 || 
    bg.ytop > screen_height-1 ||
    bg.ybottom < 0 || 
    bg.ybottom > screen_height-1 ||
    bg.ybottom-bg.ytop < 6*text_height ||
    bg.pixels_per_point > 16 ||
    bg.pixels_per_point < 1 ||
    bg.coh_factor < 1 || 
    bg.coh_factor > 9999 ||
    bg.delay_points < 0 || 
    bg.delay_points > 999 ||
    bg.waterfall_avgnum < 1 ||
    bg.agc_flag < 0 || 
    bg.agc_flag > 2 ||
    bg.horiz_arrow_mode<0 || 
    bg.horiz_arrow_mode > 2 ||
    bg.mixer_mode < 1 ||
    bg.mixer_mode > 2  ||
    bg.filter_shift < -999 ||
    bg.filter_shift > 999 
    )goto bg_default;
  }
bg_no_of_notches=0;
bg_current_notch=0;
new_bg_agc_flag=bg.agc_flag;
if(bg.wheel_stepn<-30 || bg.wheel_stepn >30)bg.wheel_stepn=0;
bfo_flag=0;  
mix2.size=-1;
bg_da_bytes=1+(genparm[OUTPUT_MODE]&1);                   //bit 0
bg_da_channels=1+((genparm[OUTPUT_MODE]>>1)&1);           //bit 1
bg_expand=(genparm[OUTPUT_MODE]>>2)&3;        //bit 2 and 3
bg_coherent=(genparm[OUTPUT_MODE]>>4)&3;      //bits 4 and 5
if(bg_coherent > 3)bg_coherent=3;
bg_delay=(genparm[OUTPUT_MODE]>>6)&1;       //bit 6
bg_twopol=(genparm[OUTPUT_MODE]>>7)&1;       //bit 7 
baseb_channels=0;
baseband_sampling_speed=0;
baseband_graph_scro=no_of_scro;
bfo_xpixel=-1;
init_baseband_sizes();
make_baseband_graph(FALSE);
no_of_scro++;
if(no_of_scro >= MAX_SCRO)lirerr(89);
for(i=0; i<screen_width*ui.rx_rf_channels; i++)fft3_spectrum[i]=bg_y0;
// Clear bg_timestamp_counter which is used to show time in the waterfall graph
// every time 2*text_height new waterfall lines have arrived.
bg_timestamp_counter=0;
}
