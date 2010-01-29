
#define YBO 8
#define YWF 4
#define MAX_FFT1_AVG1 6

#include <unistd.h>
#include <fcntl.h>
#include "globdef.h"
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "seldef.h"
#include "graphcal.h"
#include "screendef.h"
#include "sigdef.h"
#include "thrdef.h"
#include "options.h"

void update_meter_graph(void)
{
int i, j, k, m, n, pa, local_mg_pa;
int ya, yb;
int old_bar_y;
// mgw_p points to the screen position corresponding to mg_px
// in the mg_ arrays.
// Move the data backwards on screen if necessary and adjust 
// mgw_p accordingly.
hide_mouse(mg.xleft,mg.xright,mg.ytop,mg.ybottom);
local_mg_pa=mg_pa;
if(mg_bar)
  {
  old_bar_y=mg_bar_y;
  mg_bar_y=mg_y0;
  if(sw_onechan)
    {
    while(mg_px != local_mg_pa)
      {
      i=mg_ymin-(mg.yzero+mg.ygain*mg_peak_meter[mg_px]);
      if(i<mg_ymax)i=mg_ymax;
      if(i<mg_bar_y)mg_bar_y=i;
      mg_px=(mg_px+1)&mg_mask;
      }
    }  
  else
    {  
    while(mg_px != local_mg_pa)
      {
      i=mg_ymin-(mg.yzero+mg.ygain*mg_peak_meter[2*mg_px]);
      if(i<mg_ymax)i=mg_ymax;
      if(i<mg_bar_y)mg_bar_y=i;
      mg_px=(mg_px+1)&mg_mask;
      }
    }
  update_bar(mg_bar_x1,mg_bar_x2,mg_y0,mg_bar_y,old_bar_y,
                                                 MG_BAR_COLOR,mg_barbuf);
  }
else
  {  
  n=(local_mg_pa-mg_px+mg_size)&mg_mask;
  n+=mgw_p-mg_last_xpixel;
  yb=0;
  if(n > 0)
    {
    pa=mg_px;
    m=(mg_px+mg_mask)&mg_mask;
    if(sw_onechan)
      {
      for(j=mgw_p; j>=mg_first_xpixel; j--)
        {
        ya=-1;
        if( (mg.tracks&1)==0)
          {
          i=mg_rms_ypix[pa];
          ya=i;
          yb=i;
          k=mg_rms_ypix[m];
          if(k>ya)ya=k;
          if(k<yb)yb=k;
          lir_line(j-1,k,j,i,0);
          }
        if( (mg.tracks&2)==0)
          {
          i=mg_peak_ypix[pa];
          if(i<mg_ymax)i=mg_ymax;
          if(ya < 0)
            {
            ya=i;
            yb=i;
            }
          else
            {
            if(i>ya)ya=i;
            if(i<yb)yb=i;
            }
          k=mg_peak_ypix[m];
          if(k>ya)ya=k;
          if(k<yb)yb=k;
          lir_line(j-1,k,j,i,0);
          }
        i=yb;
        while(i <= ya)
          {
          if( mg_behind_meter[i] != 0)lir_setpixel(j,i,mg_behind_meter[i]);
          i++;
          }
        pa=m;
        m=(m+mg_mask)&mg_mask;
        }
      }  
    else
      {  
      for(j=mgw_p; j>=mg_first_xpixel; j--)
        {
        ya=-1;
        if( (mg.tracks&1)==0)
          {
          i=mg_rms_ypix[2*pa];
          ya=i;
          yb=i;
          k=mg_rms_ypix[2*m];
          if(k>ya)ya=k;
          if(k<yb)yb=k;
          lir_line(j-1,k,j,i,0);
          i=mg_rms_ypix[2*pa+1];
          if(i>ya)ya=i;
          if(i<yb)yb=i;
          k=mg_rms_ypix[2*m+1];
          if(k>ya)ya=k;
          if(k<yb)yb=k;
          lir_line(j-1,k,j,i,0);
          }
        if( (mg.tracks&2)==0)
          {
          i=mg_peak_ypix[2*pa];
          if(ya < 0)
            {
            ya=i;
            yb=i;
            }
          else
            {
            if(i>ya)ya=i;
            if(i<yb)yb=i;
            }
          k=mg_peak_ypix[2*m];
          if(k>ya)ya=k;
          if(k<yb)yb=k;
          lir_line(j-1,k,j,i,0);
          i=mg_peak_ypix[2*pa+1];
          if(i>ya)ya=i;
          if(i<yb)yb=i;
          k=mg_peak_ypix[2*m+1];
          if(k<mg_ymax)k=mg_ymax;
          if(k>ya)ya=k;
          if(k<yb)yb=k;
          lir_line(j-1,k,j,i,0);
          }
        i=yb;
        while(i <= ya)
          {
          if( mg_behind_meter[i] != 0)lir_setpixel(j,i,mg_behind_meter[i]);
          i++;
          }
        pa=m;
        m=(m+mg_mask)&mg_mask;
        }
      }
// We want to place the last point at mg_last_xpixel-n in the graph.
// The number of points is thus mg_last_xpixel-n-mg_first_xpixel.
// Set mg_px and mgw_p accordingly.
    if(n < 0.2*(mg_last_xpixel-mg_first_xpixel) )
      {
//  n=(local_mg_pa-mg_px+mg_size)&mg_mask;
//  n+=mgw_p-mg_last_xpixel;
      n=0.2*(mg_last_xpixel-mg_first_xpixel);
      }
    if(n >=mg_last_xpixel-mg_first_xpixel)
      {
      n=0.8*(mg_last_xpixel-mg_first_xpixel);
      }
    mgw_p=mg_first_xpixel;
    mg_px=(local_mg_pa-(mg_last_xpixel-n-mg_first_xpixel)+mg_size)&mg_mask;
    }
  m=(mg_px+mg_mask)&mg_mask;
  if(sw_onechan)
    {
    i=mg_ymin-(mg.yzero+mg.ygain*mg_rms_meter[m]);
    if(i<mg_ymax)i=mg_ymax;
    if(i>mg_ymin)i=mg_ymin;
    mg_rms_ypix[m]=i;
    i=mg_ymin-(mg.yzero+mg.ygain*mg_peak_meter[m]);
    if(i<mg_ymax)i=mg_ymax;
    if(i>mg_ymin)i=mg_ymin;
    mg_peak_ypix[m]=i;
    }
  else
    {
    i=mg_ymin-(mg.yzero+mg.ygain*mg_rms_meter[2*m]);
    if(i<mg_ymax)i=mg_ymax;
    if(i>mg_ymin)i=mg_ymin;
    mg_rms_ypix[2*m]=i;
    i=mg_ymin-(mg.yzero+mg.ygain*mg_peak_meter[2*m]);
    if(i<mg_ymax)i=mg_ymax;
    if(i>mg_ymin)i=mg_ymin;
    mg_peak_ypix[2*m]=i;
    i=mg_ymin-(mg.yzero+mg.ygain*mg_rms_meter[2*m+1]);
    if(i<mg_ymax)i=mg_ymax;
    if(i>mg_ymin)i=mg_ymin;
    mg_rms_ypix[2*m+1]=i;
    i=mg_ymin-(mg.yzero+mg.ygain*mg_peak_meter[2*m+1]);
    if(i<mg_ymax)i=mg_ymax;
    if(i>mg_ymin)i=mg_ymin;
    mg_peak_ypix[2*m+1]=i;
    }
  if(sw_onechan)
    {
    while(mg_px != local_mg_pa)
      {
      if( (mg.tracks&1)==0)
        {
        i=mg_ymin-(mg.yzero+mg.ygain*mg_rms_meter[mg_px]);
        if(i<mg_ymax)i=mg_ymax;
        if(i>mg_ymin)i=mg_ymin;
        mg_rms_ypix[mg_px]=i;
        k=mg_rms_ypix[m];
        lir_line(mgw_p-1,k,mgw_p,i,14);
        }
      if( (mg.tracks&2)==0)
        {
        i=mg_ymin-(mg.yzero+mg.ygain*mg_peak_meter[mg_px]);
        if(i<mg_ymax)i=mg_ymax;
        if(i>mg_ymin)i=mg_ymin;
        mg_peak_ypix[mg_px]=i;
        k=mg_peak_ypix[m];
        lir_line(mgw_p-1,k,mgw_p,i,15);
        }
      m=mg_px;
      mg_px=(mg_px+1)&mg_mask;
      mgw_p++;
      }
    }  
  else
    {  
    while(mg_px != local_mg_pa)
      {
      if( (mg.tracks&1)==0)
        {
        i=mg_ymin-(mg.yzero+mg.ygain*mg_rms_meter[2*mg_px]);
        if(i<mg_ymax)i=mg_ymax;
        if(i>mg_ymin)i=mg_ymin;
        mg_rms_ypix[2*mg_px]=i;
        k=mg_rms_ypix[2*m];
        lir_line(mgw_p-1,k,mgw_p,i,11);
        i=mg_ymin-(mg.yzero+mg.ygain*mg_rms_meter[2*mg_px+1]);
        if(i<mg_ymax)i=mg_ymax;
        if(i>mg_ymin)i=mg_ymin;
        mg_rms_ypix[2*mg_px+1]=i;
        k=mg_rms_ypix[2*m+1];
        lir_line(mgw_p-1,k,mgw_p,i,12);
        }
      if( (mg.tracks&2)==0)
        {
        i=mg_ymin-(mg.yzero+mg.ygain*mg_peak_meter[2*mg_px]);
        if(i<mg_ymax)i=mg_ymax;
        if(i>mg_ymin)i=mg_ymin;
        mg_peak_ypix[2*mg_px]=i;
        k=mg_peak_ypix[2*m];
        lir_line(mgw_p-1,k,mgw_p,i,10);
        i=mg_ymin-(mg.yzero+mg.ygain*mg_peak_meter[2*mg_px+1]);
        if(i<mg_ymax)i=mg_ymax;
        if(i>mg_ymin)i=mg_ymin;
        mg_peak_ypix[2*mg_px+1]=i;
        k=mg_peak_ypix[2*m+1];
        lir_line(mgw_p-1,k,mgw_p,i,13);
        }
      m=mg_px;
      mg_px=(mg_px+1)&mg_mask;
      mgw_p++;
      }
    }
  }    
}

void afc_cursor(void)
{
int i, j;
if(mix1_fq_mid[fftx_nx] < 0)return;
j=0.5+hg_first_xpixel+(mix1_fq_mid[fftx_nx]-hg_first_fq)/hg_hz_per_pixel;
if(j < hg_first_xpixel)j = hg_first_xpixel;
if(j > hg_last_xpixel)j = hg_last_xpixel;
if(afc_curx != j || afc_old_cursor_color != afc_cursor_color)
  {
  hide_mouse(j,j,hg_cury0,hg_cury2);
  if(afc_curx > 0)
    {
    hide_mouse(afc_curx,afc_curx,hg_cury0,hg_cury2);
    if(afc_curx == hg_center)
      {
      for(i=hg_cury0; i<hg_cury2; i++)
        {      
        lir_setpixel(afc_curx,i,8);
        lir_setpixel(j,i,afc_cursor_color);
        }
      }
    else
      {
      for(i=hg_cury0; i<hg_cury2; i++)
        {      
        lir_setpixel(afc_curx,i,hg_background[i]);
        lir_setpixel(j,i,afc_cursor_color);
        }
      }  
    }
  else
    {  
    for(i=hg_cury0; i<hg_cury2; i++)
      {      
      lir_setpixel(j,i,afc_cursor_color);
      }
    }
  }  
afc_curx=j;
afc_old_cursor_color = afc_cursor_color;
}



void show_coherent(void)
{    
char s[80];
int i, ia, ix, iy, n;
float t2, t4;
float sellim_correction;
sellim_correction=1;  
if(genparm[SECOND_FFT_ENABLE] != 0)
  {
  if(!swfloat)
    {
    ia=mix1_selfreq[0]*fftx_points_per_hz;
    if(ia > 0)
      {
      ia/=fft2_to_fft1_ratio;
      if(liminfo[ia]>0)
        {
        sellim_correction=1/pow(liminfo[ia],2.0);
        }
      }
    }
  }
hide_mouse(cg_old_x1, cg_old_x2, cg_old_y1, cg_old_y2);
for(iy=0; iy<cg_size; iy++)
  {
  n=iy*cg_size;
  for(ix=0; ix<cg_size; ix++)
    {
    i=1+cg_map[n+ix];
    cg_map[n+ix]*=0.8;
    if(i >= MAX_COLOR_SCALE)i=MAX_COLOR_SCALE-1;
    lir_fillbox(cg_old_x1+1+3*ix,cg_old_y1+1+3*iy,3,3,color_scale[i]);
    }
  }
lir_line(cg_x0,cg_old_y1,cg_x0,cg_y1,8);
if(kill_all_flag) return;
lir_hline(cg_old_x1+1,cg_y0,cg_old_x2-1,8);
if(kill_all_flag) return;
lir_line(cg_x0,cg_y0,cg_chirpx,cg_chirpy,14);
if(kill_all_flag) return;
t2=0;
t4=0;
ia=basblock_pa;
for(i=0; i<basblock_hold_points; i++)
  {
  t4+=basblock_avgpower[ia];
  if(t2<basblock_maxpower[ia])t2=basblock_maxpower[ia];
  ia=(ia+basblock_mask)&basblock_mask;
  }
t4/=basblock_hold_points;
s_meter_average_power=10*log10(t4*baseband_pwrfac*sellim_correction);
t2*=baseband_pwrfac;
sprintf(s,"%f  ",s_meter_average_power);
// Make sure we do not write outside our screen area.
s[COH_SIDE-2]=0;
lir_pixwrite(cg_old_x1+2*text_width,cg_old_y2-text_height,s);
if(s_meter_avgnum >=0)
  {
  s_meter_avg_filled_flag=TRUE;
  s_meter_avgnum++;
  s_meter_avg+=t4;
  t4=s_meter_avg/s_meter_avgnum;
  sprintf(s,"%f",10*log10(t4*baseband_pwrfac));
  s[COH_SIDE]=0;
  lir_pixwrite(cg_old_x1+1,cg_old_y2-4*text_height,s);
  sprintf(s,"%3d ",s_meter_avgnum);
  s[COH_SIDE]=0;
  lir_pixwrite(cg_old_x1+1,cg_old_y2-5*text_height,s);
  }
else
  {
  if(s_meter_avg_filled_flag==TRUE)
    {
    lir_fillbox(cg_old_x1+1,cg_old_y2-5*text_height,3*cg_size,2*text_height,0);
    s_meter_avg_filled_flag=FALSE;
    }
  }  
s_meter_fast_attack_slow_decay=10*log10(t2*sellim_correction);  
sprintf(s,"%.2f  ",s_meter_fast_attack_slow_decay);
s[COH_SIDE-2]=0;
lir_pixwrite(cg_old_x1+2*text_width,cg_old_y2-2*text_height,s);
t2*=sellim_correction;
if(s_meter_peak_hold < t2)
  {
  s_meter_peak_hold=t2;
  lir_pixwrite(cg_old_x1+2*text_width,cg_old_y2-3*text_height,s);
  }
}

void update_txtest(int xpix,int new_y2 )
{
int new_y1, old_y1, old_y2;
int ia, ib;
char bkg_color;
old_y2=fft1_spectrum[xpix];
fft1_old_spectrum[xpix]=old_y2;
if(new_y2 < 0)new_y2=0;
new_y2=wg.ybottom-1-new_y2;
if(new_y2 < wg.yborder+1)new_y2=wg.yborder+1;
if(xpix != wg_first_xpixel)
  {
  new_y1=fft1_spectrum[xpix-1];
  old_y1=fft1_old_spectrum[xpix-1];
  }
else
  {
  new_y1=new_y2;
  old_y1=new_y2;
  }
if( new_y2 != old_y2 || new_y1 != old_y1)
  {
  if(old_y1 > old_y2)
    {
    ia=old_y2;
    ib=old_y1;
    }
  else
    {
    ib=old_y2;
    ia=old_y1;
    }
  while(ia <= ib)
    {  
    bkg_color=wg_background[ia];
    lir_setpixel(xpix,ia,bkg_color);
    ia++;
    } 
  if(new_y1 > new_y2)
    {
    ia=new_y2;
    ib=new_y1;
    }
  else
    {
    ib=new_y2;
    ia=new_y1;
    }
  while(ia <= ib)
    {  
    lir_setpixel(xpix,ia,TXTEST_NARROW_COLOR);
    ia++;
    }
  fft1_spectrum[xpix]=new_y2;
  }
}

void update_txpeak(int xpix,int new_y2 )
{
int new_y1, old_y1, old_y2;
int ia, ib;
char bkg_color;
old_y2=txtest_ypeak[xpix];
txtest_old_ypeak[xpix]=old_y2;
if(new_y2 < 0)new_y2=0;
new_y2=wg.ybottom-1-new_y2;
if(new_y2 < wg.yborder+1)new_y2=wg.yborder+1;
if(old_y2 < new_y2)new_y2=old_y2;
if(xpix == txtest_first_xpix)
  {
  bkg_color=wg_background[old_y2];
  if(old_y2 == txtest_ypeak_decay[xpix])bkg_color=TXTEST_PEAK_DECAY_COLOR;
  lir_setpixel(xpix,old_y2,bkg_color);
  txtest_ypeak[xpix]=new_y2;
  lir_setpixel(xpix,new_y2,TXTEST_PEAK_POWER_COLOR);
  return;
  }
new_y1=txtest_ypeak[xpix-1];
old_y1=txtest_old_ypeak[xpix-1];
if( new_y2 != old_y2 || new_y1 != old_y1 || txtest_peak_redraw ==0)
  {
  if(old_y1 > old_y2)
    {
    ia=old_y2;
    ib=old_y1;
    }
  else
    {
    ib=old_y2;
    ia=old_y1;
    }
  while(ia <= ib)
    {  
    bkg_color=wg_background[ia];
    if(ia == txtest_ypeak_decay[xpix])bkg_color=TXTEST_PEAK_DECAY_COLOR;
    lir_setpixel(xpix,ia,bkg_color);
    ia++;
    } 
  if(new_y1 > new_y2)
    {
    ia=new_y2;
    ib=new_y1;
    }
  else
    {
    ib=new_y2;
    ia=new_y1;
    }
  while(ia <= ib)
    {  
    lir_setpixel(xpix,ia,TXTEST_PEAK_POWER_COLOR);
    ia++;
    }
  txtest_ypeak[xpix]=new_y2;
  }
txtest_peak_redraw++;
if(txtest_peak_redraw > TXTEST_PEAK_REDRAW_COUNT)
  {
  txtest_peak_redraw=0;
  }
}

void update_txpeak_decay(int xpix,int new_y2 )
{
int new_y1, old_y1, old_y2;
int ia, ib;
char bkg_color;
old_y2=txtest_ypeak_decay[xpix];
txtest_old_ypeak_decay[xpix]=old_y2;
if(new_y2 < 0)new_y2=0;
new_y2=wg.ybottom-1-new_y2;
if(new_y2 < wg.yborder+1)new_y2=wg.yborder+1;
if(xpix != txtest_first_xpix)
  {
  new_y1=txtest_ypeak_decay[xpix-1];
  old_y1=txtest_old_ypeak_decay[xpix-1];
  }
else
  {
  new_y1=new_y2;
  old_y1=new_y2;
  }
if( new_y2 != old_y2 || new_y1 != old_y1)
  {
  if(old_y1 > old_y2)
    {
    ia=old_y2;
    ib=old_y1;
    }
  else
    {
    ib=old_y2;
    ia=old_y1;
    }
  while(ia <= ib)
    {  
    bkg_color=wg_background[ia];
    if(ia == txtest_ypeak[xpix])bkg_color=TXTEST_PEAK_POWER_COLOR;
    lir_setpixel(xpix,ia,bkg_color);
    ia++;
    } 
  if(new_y1 > new_y2)
    {
    ia=new_y2;
    ib=new_y1;
    }
  else
    {
    ib=new_y2;
    ia=new_y1;
    }
  while(ia <= ib)
    {  
    lir_setpixel(xpix,ia,TXTEST_PEAK_DECAY_COLOR);
    ia++;
    }
  txtest_ypeak_decay[xpix]=new_y2;
  }
}

void update_txavg(int xpix,int new_y2 )
{
int new_y1, old_y1, old_y2;
int ia, ib;
char bkg_color;
old_y2=txtest_yavg[xpix];
txtest_old_yavg[xpix]=old_y2;
if(new_y2 < 0)new_y2=0;
new_y2=wg.ybottom-1-new_y2;
if(new_y2 < wg.yborder+1)new_y2=wg.yborder+1;
if(xpix != txtest_first_xpix)
  {
  new_y1=txtest_yavg[xpix-1];
  old_y1=txtest_old_yavg[xpix-1];
  }
else
  {
  new_y1=new_y2;
  old_y1=new_y2;
  }
if( new_y2 != old_y2 || new_y1 != old_y1)
  {
  if(old_y1 > old_y2)
    {
    ia=old_y2;
    ib=old_y1;
    }
  else
    {
    ib=old_y2;
    ia=old_y1;
    }
  while(ia <= ib)
    {  
    bkg_color=wg_background[ia];
    if(ia == txtest_ypeak_decay[xpix])bkg_color=TXTEST_PEAK_DECAY_COLOR;
    if(ia == txtest_ypeak[xpix])bkg_color=TXTEST_PEAK_POWER_COLOR;
    lir_setpixel(xpix,ia,bkg_color);
    ia++;
    } 
  if(new_y1 > new_y2)
    {
    ia=new_y2;
    ib=new_y1;
    }
  else
    {
    ib=new_y2;
    ia=new_y1;
    }
  while(ia <= ib)
    {  
    lir_setpixel(xpix,ia,TXTEST_WIDE_AVERAGE_COLOR);
    ia++;
    }
  txtest_yavg[xpix]=new_y2;
  }
}

void show_txtest_spectrum(void)
{
int j, ia;
float fxpix;
float yval_avg, der_avg, next_y, next_y_avg;
float yval_decay, der_decay, next_y_decay;
float *z;
int i,k,m,mm, new_y, xpix;
float yval,der,t1;
// Draw the normal fft1 spectrumn with  lines.
hide_mouse(wg_first_xpixel, wg_last_xpixel, wg.yborder, wg.ybottom);
if(wg.xpoints_per_pixel == 1 || wg.pixels_per_xpoint == 1)
  {
  i=wg.first_xpoint+wg_first_xpixel-wg_first_xpixel;
  for(xpix=wg_first_xpixel; xpix < wg_last_xpixel; xpix++)
    {
    update_txtest(xpix,(int)(wg_yfac_log*log10(fft1_slowsum[i]*wg_yfac_power)));
    i++;
    }
  }    
else
  {
  if(wg.xpoints_per_pixel == 0)
    {
// There are more pixels than data points so we must interpolate.
    i=wg.first_xpoint+(wg_first_xpixel-wg_first_xpixel)/wg.pixels_per_xpoint;
    new_y=wg_yfac_log*log10(fft1_slowsum[i]*wg_yfac_power);
    yval=new_y;
    if(i == wg.first_xpoint)
      {
      update_txtest(wg_first_xpixel,new_y);
      }
    i++;
    for(xpix=wg_first_xpixel; 
        xpix<wg_last_xpixel; xpix+=wg.pixels_per_xpoint)
      {
      t1=wg_yfac_log*log10(fft1_slowsum[i]*wg_yfac_power);
      der=(t1-yval)/wg.pixels_per_xpoint;
      m=xpix+wg.pixels_per_xpoint/2;  
      for(k=xpix+1; k<=m; k++)
        {
        yval=yval+der;
        new_y=yval;
        update_txtest(k,new_y);
        }  
      mm=xpix+wg.pixels_per_xpoint;  
      for(k=m+1; k<=mm; k++)
        {
        yval=yval+der;
        new_y=yval;
        update_txtest(k,new_y);
        }  
      i++;
      yval=t1;
      }      
    }
  else
    {
// There is more than one data point for each pixel.
// Slide a triangular filter across the spectrum to make it
// smoother before resampling.
    i=wg.first_xpoint+(wg_first_xpixel-wg_first_xpixel)*wg.xpoints_per_pixel;
    for(xpix=wg_first_xpixel; xpix<wg_last_xpixel; xpix++)
      {
      t1=fft1_slowsum[i]*wg.xpoints_per_pixel;
      for(k=1; k<wg.xpoints_per_pixel; k++)
        {
        t1+=(fft1_slowsum[i+k]+fft1_slowsum[i-k])*(wg.xpoints_per_pixel-k);
        }
      new_y=wg_yfac_log*log10(t1*wg_yfac_power);
      update_txtest(xpix,new_y);
      i+=wg.xpoints_per_pixel;
      }
    }  
  }
z=&txtest_power[txtest_spek_p0];
// Make the average spectrum.
if(txtest_no_of_segs==0 || txtest_pixinc==0)return;
txtest_last_xpix=txtest_first_xpix+(txtest_no_of_segs-1)*txtest_pixinc;  
xpix=txtest_first_xpix;
fxpix=xpix;
// There are typically more pixels than data points so we must interpolate.
ia=0;
yval=wg_yfac_log*log10(txtest_peak_power[0]);
yval_decay=wg_yfac_log*log10(txtest_peak_power_decay[0]);
yval_avg=wg_yfac_log*log10(txtest_powersum[0]);
if(txtest_decayfac1 < 0.5)
  {
  update_txpeak_decay(xpix,yval_decay);
  }
update_txavg(xpix,yval_avg);
update_txpeak(xpix,yval);
while( xpix < txtest_last_xpix)
  {
  ia++;
  m=fxpix+txtest_pixinc/2;  
  fxpix+=txtest_pixinc;
  j=fxpix;
  next_y=wg_yfac_log*log10(txtest_peak_power[ia]);
  der=(next_y-yval)/(j-xpix);
  next_y_decay=wg_yfac_log*log10(txtest_peak_power_decay[ia]);
  der_decay=(next_y_decay-yval_decay)/(j-xpix);
  next_y_avg=wg_yfac_log*log10(txtest_powersum[ia]);
  der_avg=(next_y_avg-yval_avg)/(j-xpix);
  for(k=xpix; k<m; k++)
    {
    yval=yval+der;
    yval_avg=yval_avg+der_avg;
    update_txavg(k,yval_avg);
    if(txtest_decayfac1 < 0.5)
      {
      yval_decay=yval_decay+der_decay;
      update_txpeak_decay(k,yval_decay);
      }
    update_txpeak(k,yval);
    }  
  xpix=fxpix;  
  yval=next_y-der*(xpix-m);
  yval_avg=next_y_avg-der_avg*(xpix-m);
  yval_decay=next_y_decay-der_decay*(xpix-m);
  for(k=m; k<xpix; k++)
    {
    yval=yval+der;
    yval_avg=yval_avg+der_avg;
    update_txavg(k,yval_avg);
    if(txtest_decayfac1 < 0.5)
      {
      yval_decay=yval_decay+der_decay;
      update_txpeak_decay(k,yval_decay);
      }
    update_txpeak(k,yval);
    }  
  yval=next_y;
  yval_avg=next_y_avg;
  yval_decay=next_y_decay;
  }  
}

void update_wg(int xpix,int new_y, char spk_color )
{
int i, old_y, sw;
char bkg_color;
old_y=fft1_spectrum[xpix];
if(new_y < 0)new_y=0;
new_y=wg.ybottom-1-new_y;
if(new_y < wg.yborder+1)new_y=wg.yborder+1;
if(old_y < 0)
  {
  old_y=-old_y;
  }
if(spk_color ==15)sw=0; else sw=1;
if(new_y!=old_y || sw!=0)
  {
  bkg_color=wg_background[old_y];
  if( (ui.network_flag&NET_RX_OUTPUT) != 0)
    {  
    for(i=0; i<MAX_FREQLIST; i++)
      {
      if(xpix == netfreq_curx[i])
        {
        bkg_color=MIX1_NETCUR_COLOR;
        }
      }
    }  
  if(xpix == mix1_curx[0])
    {
    bkg_color=MIX1_MAINCUR_COLOR;
    }
  else
    {
    for(i=1; i<genparm[MIX1_NO_OF_CHANNELS]; i++)
      {
      if(xpix == mix1_curx[i])
        {
        bkg_color=MIX1_SUBCUR_COLOR;
        }
      }  
    }
  lir_setpixel(xpix,old_y,bkg_color);
  lir_setpixel(xpix,new_y,spk_color);
  if(spk_color != 15)new_y=-new_y;
  fft1_spectrum[xpix]=new_y;
  }
}


void update_wg_spectrum(void)
{
int i,m,mm, new_y, xpix;
int k, num, xpos, color;
float yval,der,t1;
char cl;
// fft1_spectrum is the y-value currently on screen.
// behind_fft1_spectrum is the colour behind the current data point.
// Calculate updated values and move the corresponding bright 
// pixel in case the value has changed
hide_mouse(wg_first_xpixel, wg_last_xpixel, wg.yborder, wg.ybottom);
if(fft1_waterfall_flag)
  {  
  while(fft1_sumsq_pws != fft1_sumsq_pa ) 
    {
    update_fft1_slowsum();
    }
  }
if(rx_mode == MODE_TXTEST)
  {
  show_txtest_spectrum();
  return;
  }
for(num=0; num<genparm[MIX1_NO_OF_CHANNELS]; num++)
  {
  if(new_mix1_curx[num]!=mix1_curx[num])
    {
    xpos=new_mix1_curx[num];
    if(xpos!=-1)
      {
      if(num==0)
        {
        color=MIX1_MAINCUR_COLOR;
        }
      else
        {
        color=MIX1_SUBCUR_COLOR;
        }
      lir_line(xpos,wg.ybottom-1,xpos,wg.yborder+1,color);
      if(kill_all_flag) return;
      }
    xpos=mix1_curx[num];
    if(xpos!=-1)
      {
      for(i=wg.yborder+1; i<wg.ybottom; i++)
        {
        lir_setpixel(xpos,i, wg_background[i]);
        }
      }
    mix1_curx[num]=new_mix1_curx[num];
    }
  }
if( (ui.network_flag&NET_RX_OUTPUT) != 0)
  {  
  for(num=0; num<MAX_FREQLIST; num++)
    {
    if(new_netfreq_curx[num]!=netfreq_curx[num])
      {
      xpos=new_netfreq_curx[num];
      if(xpos!=-1)
        {
        lir_line(xpos,wg.ybottom-1,xpos,wg.yborder+1,MIX1_NETCUR_COLOR);
        if(kill_all_flag) return;
        }
     xpos=netfreq_curx[num];
     if(xpos!=-1)
        {
        for(i=wg.yborder+1; i<wg.ybottom; i++)
          {
          lir_setpixel(xpos,i, wg_background[i]);
          }
        }
      netfreq_curx[num]=new_netfreq_curx[num];
      }
    }
  }
if(wg.xpoints_per_pixel == 1 || wg.pixels_per_xpoint == 1)
  {
  i=wg.first_xpoint+wg_first_xpixel-wg_first_xpixel;
  for(xpix=wg_first_xpixel; xpix<=wg_last_xpixel; xpix++)
    {
    if(liminfo[i]==0)
      {
      cl=15;
      }
    else
      {
      cl=12;
      }
    update_wg(xpix,(int)(wg_yfac_log*log10(fft1_slowsum[i]*wg_yfac_power)),cl);
    i++;
    }
  }    
else
  {
  if(wg.xpoints_per_pixel == 0)
    {
// There are more pixels than data points so we must interpolate.
    i=wg.first_xpoint+(wg_first_xpixel-wg_first_xpixel)/wg.pixels_per_xpoint;
    new_y=wg_yfac_log*log10(fft1_slowsum[i]*wg_yfac_power);
    yval=new_y;
    if(liminfo[i]==0)
      {
      cl=15;
      }
    else
      {
      cl=12;
      }
    i++;  
    update_wg(wg_first_xpixel,new_y,cl);
    for(xpix=wg_first_xpixel; 
        xpix<=wg_last_xpixel; xpix+=wg.pixels_per_xpoint)
      {
      t1=wg_yfac_log*log10(fft1_slowsum[i]*wg_yfac_power);
      der=(t1-yval)/wg.pixels_per_xpoint;
      m=xpix+wg.pixels_per_xpoint/2;  
      for(k=xpix+1; k<=m; k++)
        {
        yval=yval+der;
        new_y=yval;
        update_wg(k,new_y,cl);
        }  
      if(liminfo[i]==0)
        {
        cl=15;
        }
      else
        {
        cl=12;
        }
      mm=xpix+wg.pixels_per_xpoint;  
      for(k=m+1; k<=mm; k++)
        {
        yval=yval+der;
        new_y=yval;
        update_wg(k,new_y,cl);
        }  
      i++;
      yval=t1;
      }      
    }
  else
    {
// There is more than one data point for each pixel.
// Slide a triangular filter across the spectrum to make it
// smoother before resampling.
    i=wg.first_xpoint+(wg_first_xpixel-wg_first_xpixel)*wg.xpoints_per_pixel;
    for(xpix=wg_first_xpixel; xpix<=wg_last_xpixel; xpix++)
      {
      if(liminfo[i]==0)
        {
        cl=15;
        }
      else
        {
        cl=12;
        }
      t1=fft1_slowsum[i]*wg.xpoints_per_pixel;
      for(k=1; k<wg.xpoints_per_pixel; k++)
        {
        t1+=(fft1_slowsum[i+k]+fft1_slowsum[i-k])*(wg.xpoints_per_pixel-k);
        if(liminfo[i+k]!=0||liminfo[i-k]!=0)cl=12;
        }
      new_y=wg_yfac_log*log10(t1*wg_yfac_power);
      update_wg(xpix,new_y,cl);
      i+=wg.xpoints_per_pixel;
      }
    }  
  }
}

void timf3_oscilloscope(void)
{
int i,j,k,m,mm,pa,pb,mask;
float t1,t2;
hide_mouse(0,screen_width>>1,0,screen_height);
mm=twice_rxchan;
mask=timf3_mask;
pa=timf3_ps;        
pb=(pa+timf3_osc_interval)&mask;
t1=0;
m=-1;
while(pa != pb)
  {
  t2=timf3_float[pa  ]*timf3_float[pa  ];
  for(j=1;j<mm; j++)
    {
    t2+=timf3_float[pa+j]*timf3_float[pa+j]; 
    } 
  if(t1<t2)
    {
    t1=t2;
    m=pa;
    }
  pa=(pa+mm)&mask;
  }
if(m >= 0)
  {
  pa=(m-ui.rx_rf_channels*screen_width/4)&timf3_mask;
  pa&=-mm;
  for(j=0; j<mm; j++)
    {  
    for(i=1; i<screen_width/2; i++)
      {
      lir_line(i-1,timf3_graph[mm*(i-1)+j],i,timf3_graph[mm*i+j],0);
      if(kill_all_flag) return;
      }
    }
  for(i=0; i<screen_width/2; i++)
    {
    for(j=0; j<mm; j++)
      {  
      k=timf3_y0[mm-j]-timf3_float[pa+j]*bg.oscill_gain;
      if(k<0)k=0;
      if(k>=screen_height)k=screen_height-1;
      timf3_graph[mm*i+j]=k;
      }
    pa=(pa+mm)&timf3_mask;
    }
  for(j=0; j<mm; j++)
    {  
    if(  (j&1)==0 )k=10; else k=13;
    for(i=1; i<screen_width/2; i++)
      {
      lir_line(i-1,timf3_graph[mm*(i-1)+j],i,timf3_graph[mm*i+j],k);
      if(kill_all_flag) return;
      }
    }
  }
timf3_ps=(timf3_ps+timf3_osc_interval)&timf3_mask;
}

void hg_cursor(void)
{
int i;
float mfq;
mfq=mix1_selfreq[0];
if(mfq < 0)return;
hg_center=0.5+hg_first_xpixel+(mfq-hg_first_fq)/hg_hz_per_pixel;
for(i=hg_cury1; i<hg_cury2; i++)
  {      
  if(hg_curx > 0)lir_setpixel(hg_curx,i,hg_background[i]);
  lir_setpixel(hg_center,i,8);
  }
hg_curx=hg_center;
}


void fq_scale(int x1, int x2, int y, int first_xpixel,  
             double first_frequency, float hz_per_pixel)
{
int j, k, digitnum;
int xa,xb,xc,xd,ya,yb;
char s[80];
char *fmt={"%.1f"};
float t1, fq_xstep, fq_x;
double fq_value, fq_valstep, fq_offset;
// Get the decimal fraction and convert it to Hz  
// Express frequencies in Hz and do not place numbers closer than
// 9 ascii characters.
fq_valstep=9*hz_per_pixel*text_width;
j=adjust_scale(&fq_valstep);
t1=1;
if(fq_valstep > 1)
  {
  fmt="%.0f";
  t1=10;
  }
digitnum=t1*100000;
fq_offset=(frequency_scale_offset/t1-
                                (int)(frequency_scale_offset/t1))*digitnum;
// Get the decimal fraction and convert it to Hz  
t1=first_frequency+fq_offset+rx_hware_fqshift;
fq_value=fq_valstep*(int)(0.5+t1/fq_valstep);
fq_x=first_xpixel+(fq_value-t1)/hz_per_pixel;
fq_xstep=fq_valstep/hz_per_pixel;
xb=-1;
ya=y+text_height-1;
yb=y+text_height+2;
hide_mouse(x1 ,x2, y+1, y+text_height+3);
lir_fillbox(x1+2, y+1, x2-x1-4,text_height+2,0);
lir_hline(x1,y+text_height+3,x2,7);
xd=fq_x;
while( xd < x2)
  {
  if(fq_value >=digitnum)fq_value-=digitnum;
  sprintf(s,fmt,fq_value);
  k=0;
  while(s[k]!=0)k++;
  xc=fq_x-k*text_width/2;
  xd=xc+k*text_width+2;
  if(xc > x1+2 && xd < x2)
    {
    lir_pixwrite(xc, y+2,s);
    if(xb > 0)
      {
      if(j <= 2 )
        {
        xa=(fq_x+xb)/2;
        lir_line(xa,ya,xa,yb,14);
        if(kill_all_flag) return;
        }
      else
        {  
        t1=0.2*(fq_x-xb)+0.5;
        xa=t1;
        lir_line(xb+xa,ya,xb+xa,yb,15);
        if(kill_all_flag) return;
        lir_line(fq_x-xa,ya,fq_x-xa,yb,15);
        if(kill_all_flag) return;
        t1=0.4*(fq_x-xb)+0.5;
        xa=t1;
        lir_line(xb+xa,ya,xb+xa,yb,15);
        if(kill_all_flag) return;
        lir_line(fq_x-xa,ya,fq_x-xa,yb,15);
        if(kill_all_flag) return;
        }
      }
    xb=fq_x;  
    lir_line(fq_x,ya,fq_x,yb,15);
    if(kill_all_flag) return;
    lir_line(fq_x+1,ya,fq_x+1,yb,15);
    if(kill_all_flag) return;
    lir_line(fq_x-1,ya,fq_x-1,yb,15);
    if(kill_all_flag) return;
    }
  fq_x+=fq_xstep;
  fq_value+=fq_valstep;
  }    
}


void hires_fq_scale(void)
{
int x1, x2, y;
float mfq;
double first_frequency; 
char s[80];
hide_mouse(hg.xleft,hg.xright, hg.ytop, hg.ytop+1.5*text_height);
sprintf(s,"%4d",hg.spek_avgnum);
show_button(&hgbutt[HG_FFT2_AVGNUM],s);
mfq=mix1_selfreq[0];
if(mfq < 0)return;
// Make a frequency scale on top of the high resolution spectrum.
// We will show mix1_selfreq[0] in this graph (if it exists)
hg_first_fq=mfq-hg_hz_per_pixel*hg_size/2;
hg_first_point=hg_first_fq/hg_hz_per_pixel;
if(hg_first_point<0)hg_first_point=0;
hg_last_point=hg_first_point+hg_size;
if(hg_last_point > fft2_size)hg_last_point=fft2_size;
hg_first_point=hg_last_point-hg_size;
if(hg_first_point<0)
  {
  lirerr(9998888);
  return;
  }
hg_recalc_pointer=hg_first_point;
hg_first_fq=hg_first_point*hg_hz_per_pixel;
hg_first_fq+=1;
first_frequency=hg_first_fq;
y=hg.ytop;
x1=hgbutt[HG_FFT2_AVGNUM].x2;
x2=hg.xright;
fq_scale(x1, x2, y, hg_first_xpixel, first_frequency, hg_hz_per_pixel);
hg_cursor();
}

void hg_stonbars_redraw(void)
{
update_bar(hg_ston1_x1,hg_ston1_x2,hg_ston_y0,hg_ston1_y,hg_ston1_yold,
                                            HG_STON1_RANGE_COLOR,hg_stonbuf);
hg_ston1_yold=hg_ston1_y;
update_bar(hg_ston2_x1,hg_ston2_x2,hg_ston_y0,hg_ston2_y,hg_ston2_yold,
                                            HG_STON2_RANGE_COLOR,hg_stonbuf);
hg_ston2_yold=hg_ston2_y;
}

void make_hg_yscale(void)
{
char s[80];
int i,k;
float t1;
int ypixels, scale_y;
double db_scalestep;
float scale_value, yrange,db_ref;
hide_mouse(hg.xleft,hg.xright, hg.ytop, hg_y0);
lir_fillbox(hg.xleft+1,hg.ytop+4+text_height,hg.xright-hg.xleft-2,
                                        hg_y0-hg.ytop-5-text_height,0);
for(i=0; i<screen_height; i++)hg_background[i]=0;
k=500-hg.spek_gain;
if(k>=0)k++;
t1=((float)(abs(k)));
if(k<0)t1=1/t1;
hg_db_per_pixel=sqrt(t1)*HG_GAIN/wg.waterfall_db_gain;
db_scalestep=1.3*hg_db_per_pixel*text_height;
adjust_scale(&db_scalestep);
ypixels=hg_y0-hg.ytop;
yrange=ypixels*hg_db_per_pixel;
// Make db_ref a dB value to put at the bottom of our graph.
// Select it for waterfall_db_zero to be at 8% of the y axis.
db_ref=(wg.waterfall_db_zero-0.08*yrange);
// Make db_ref a multiple of db_scalestep + 0.00001  
i=0.5+db_ref/db_scalestep-hg.spek_zero+500;
db_ref=i*db_scalestep+0.00001;
scale_value=db_ref-db_scalestep;
scale_y=hg_y0+db_scalestep/hg_db_per_pixel;
while( scale_y-2.5*text_height > hg.ytop+db_scalestep/hg_db_per_pixel)
  {
  scale_y-=db_scalestep/hg_db_per_pixel;
  scale_value+=db_scalestep;
  if(scale_y+2*text_height-1 < hg.ybottom)
    {
    if(scale_value < 0)
      {
      t1=scale_value-0.001;
      }
    else  
      {
      t1=scale_value+0.001;
      }
    i=t1;
    if(i>=-9 && i<100 && scale_value-i < db_scalestep/2)
      {
      sprintf(s,"%2d",i);
      lir_pixwrite(hg.xleft+2,scale_y-text_height/2+2,s);
      }
    }
  if(scale_y+2+text_height < hg.ybottom)
    {
    hg_background[scale_y]=HG_DBSCALE_COLOR;    
    lir_hline(hg_first_xpixel,scale_y,hg_last_xpixel,HG_DBSCALE_COLOR);
    if(kill_all_flag) return;
    }
  } 
hg_yfac_power=(float)(fft1_new_points)/fft1_size;
hg_yfac_power=(HG_ZERO*hg_yfac_power)/fft1_size;
hg_yfac_power/=(fft2_size*hg.spek_avgnum*pow(10.,0.1*db_ref));
if(fft_cntrl[FFT2_CURMODE].mmx != 0)
  {
  hg_yfac_power*=1<<(2*genparm[SECOND_FFT_ATT_N]);
  }
hg_yfac_power*=1<<(2*genparm[FIRST_BCKFFT_ATT_N]);
hg_yfac_log=10./hg_db_per_pixel;
// Place a colour scale at the right hand side
for(i=hg_y0; i>hg_ymax+text_height; i--)
  {
  scale_value=db_ref+(hg_y0-i)*hg_db_per_pixel;
  k=wg_waterf_cfac*((scale_value-wg.waterfall_db_zero)*100);
  if(k<0)k=0;
  if(k>=MAX_COLOR_SCALE)k=MAX_COLOR_SCALE-1;
  lir_hline(hg_last_xpixel+1,i,hg.xright-1,color_scale[k]);
  if(kill_all_flag) return;
  }
make_wg_yfac();
hg_stonbars_redraw();
sprintf(s,"%3d",hg.spek_gain);
show_button(&hgbutt[HG_SPECTRUM_GAIN],s);
sprintf(s,"%3d",hg.spek_zero);
show_button(&hgbutt[HG_SPECTRUM_ZERO],s);
hg_cursor();
afc_curx=-1;
}

void show_wtrfbutton(BUTTONS *butt, char *s)
{
int ix1, ix2, iy1, iy2;
ix1=butt[0].x1;
ix2=butt[0].x2;
iy1=butt[0].y1;
iy2=butt[0].y2;
if( ix1 < 0 ||
    ix1 >= screen_width ||
    iy1 < 0 ||
    iy2 >= screen_height )
  {
  return;
  }
hide_mouse(ix1-1,ix2+1,iy1-1,iy2+1);
lir_fillbox(ix1-1,iy1-1,ix2-ix1+2,iy2-iy1+2,0);
if(kill_all_flag) return;
lir_hline(ix1,iy1,ix2,button_color);
if(kill_all_flag) return;
lir_hline(ix1,iy2,ix2,button_color);
if(kill_all_flag) return;
lir_line(ix1,iy1,ix1,iy2,button_color);
if(kill_all_flag) return;
lir_line(ix2,iy1,ix2,iy2,button_color);
if(kill_all_flag) return;
lir_pixwrite(ix1+text_width/2-1,iy1+2,s);
}



void baseband_fq_scale(void)
{
int y, x1, x2;
float mfq;
double first_frequency; 
mfq=mix1_selfreq[0];
if(mfq < 0)return;
// Make a frequency scale on top of the baseband spectrum.
// We will show mix1_selfreq[0] in this graph (if it exists)
first_frequency=bg_hz_per_pixel*(bg_first_xpoint -fft3_size/2)+mfq;
y=bg.ytop;
x1=bgbutt[BG_PIX_PER_PNT_INC].x2;
x2=bgbutt[BG_RESOLUTION_DECREASE].x1;
fq_scale(x1, x2, y, bg_first_xpixel, first_frequency, 
bg_hz_per_pixel/bg.pixels_per_point);
}

void wide_fq_scale(void)
{
int ya;
double first_frequency;
if(lir_status == LIR_POWTIM)return;
first_frequency=wg_first_frequency;
ya=wg.ytop;
if(wg_freq_x1 <0)return;
fq_scale(wg_freq_x1, wg_freq_x2, ya, 
                          wg_first_xpixel, first_frequency, wg_hz_per_pixel);
}

void show_pol(void)
{
char s[80];
int angstep,i,x1,x2,y9,y2;
float major,minor,z1,z2,a1,a2,c1,c2,c3;
float pol_a,sina,pol_b;
hide_mouse(pg.xleft,pg.xright,pg.ytop,pg.ybottom);
// Make an ellipse to describe circular polarization
i=pgbutt[PG_ANGLE].x2-pgbutt[PG_ANGLE].x1;
lir_fillbox(pgbutt[PG_ANGLE].x1,pgbutt[PG_ANGLE].y1,i,i,PG_BACKGROUND_COLOR);
major=0.45*i;
// The complex amplitudes in our two channels are:
// re_x=cos(z)*cos(a)
// im_x=sin(z)*cos(a)
// re_y=sin(a)*(cos(z)*cos(w)-sin(z)*sin(w))
// im_y=sin(a)*(sin(z)*cos(w)+cos(z)*sin(w))
// z is the time function and a is the angle between main axis of
// the incoming wave and the x channel.
// If the antenna was twisted by the angle a, the signals would become:
// re_a=cos(a)*re_x+sin(a)*re_y
// im_a=cos(a)*im_x+sin(a)*im_y
// re_b=cos(a)*re_y-sin(a)*re_x
// im_b=cos(a)*im_y-sin(a)*im_x
// This would maximise the a signal and minimise the b signal.
// The power in the b channel is re_*re_b+im_b*im_b, and that is the
// function we want the minimum for over one cycle of z.
// re_b=cos(a)*sin(a)*(cos(z)*cos(w)-sin(z)*sin(w))-sin(a)*cos(z)*cos(a)
// im_b=cos(a)*sin(a)*(sin(z)*cos(w)+cos(z)*sin(w))-sin(a)*sin(z)*cos(a)
// re_b=sin(2*a)*(cos(z)*cos(w)-sin(z)*sin(w)-cos(z))/2
// im_b=sin(2*a)*(sin(z)*cos(w)+cos(z)*sin(w)-sin(z))/2
// The power in the b channel at minimum is:
// B=sin(2*a)^2*( [cos(z)*cos(w)-sin(z)*sin(w)-cos(z)]^2 +
//                [sin(z)*cos(w)+cos(z)*sin(w)-sin(z)]^2   )/4
// B=0.5*sin(2*a)^2*(1-cos(2*w) )
// For linear polarization B=0 and for circular B=0.5
// Illustrate the polarization with an ellipse.
// the axis power lengths are B and (1-B)
pol_a=acos(pg.c1);
if(pg.c3 == 0)
  {
  pg_b=0;
  }
else
  {
  pg_b=atan2(pg.c3,pg.c2); 
  }
if(fabs(1-pg.c1*pg.c1-pg.c2*pg.c2-pg.c3*pg.c3) >0.001)
  {
// Something went wrong.
DEB"pol error c1 %f  c2 %f  c3 %f",pg.c1, pg.c2, pg.c3);
  lirerr(887999);
  return;
  }
// Rotate the coordinate system by pg.angle
pol_b=pol_a-pg.angle*PI_L/180;
if(pol_b > PI_L)pol_b-=PI_L;
if(pol_b <0)pol_b+=PI_L;
c1=cos(pol_b);
c2=sin(pol_b)*cos(pg_b);
c3=sin(pol_b)*sin(pg_b);
z1=pow(sin(2*pol_a),2.)*0.5*(1-cos(2*pg_b));
minor=major*z1;
angstep=2*major/PI_L;
angstep*=sqrt(fabs(minor/major));
angstep++;
angstep&=0xfffffffe;
if(angstep < 2)angstep=2;
z2=2*PI_L/angstep;
z1=pol_a;
angstep++;
sina=sin(pol_b);
x1=pg_x0+major*c1;
y9=pg_y0-major*sina;
for(i=0; i<angstep; i++)
  {
  a1=major*cos(z1-pol_a);
  a2=minor*sin(z1-pol_a);
  x2=pg_x0+(a1*c1+a2*sina);
  y2=pg_y0+(a2*c1-a1*sina);
  lir_line(x1,y9,x2,y2,7);
  if(kill_all_flag) return;
  x1=x2;
  y9=y2;
  z1+=z2;
  }
a2=y2=x2=0;
pg_pol_angle=180*pol_b/PI_L;
sprintf(s,"%3d",(int)(pg_pol_angle));
lir_pixwrite(pgbutt[PG_CIRC].x1+text_width/2,pgbutt[PG_CIRC].y2+1,s);
lir_fillbox(pgbutt[PG_CIRC].x1,pgbutt[PG_CIRC].y1,
           pgbutt[PG_CIRC].x2-pgbutt[PG_CIRC].x1,
           pgbutt[PG_CIRC].y2-pgbutt[PG_CIRC].y1,PC_CONTROL_COLOR);
if(pol_a < 0.5*PI_L)
  {
  a1=1;
  }
else
  {
  a1=-1;
  }
if(a1*pg_b > 0)
  {
  s[0]='R';
  }
else
  {
  s[0]='L';
  }
s[1]=0;
lir_pixwrite(pg_x0-text_width/4,pg_y0-text_height/3,s); 
i=0.88*pg_b*(pgbutt[PG_CIRC].x2-pgbutt[PG_CIRC].x1)/PI_L;
lir_line(pg_x0+i,pgbutt[PG_CIRC].y1,pg_x0+i,pgbutt[PG_CIRC].y2-1,15);
}
