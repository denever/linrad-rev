
#include <ctype.h>
#include <unistd.h>
#include "globdef.h"
#include "uidef.h"
#include "screendef.h"
#include "vernr.h"
#include "sigdef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "thrdef.h"
#include "caldef.h"
#include "powtdef.h"
#include "keyboard_def.h"
#include "rusage.h"
#include "sdrdef.h"

#if(OSNUM == OS_FLAG_LINUX)
#include <pthread.h>
#include <semaphore.h>
#include "lconf.h"
#include "xdef.h"
#include "ldef.h"
#endif

#if(OSNUM == OS_FLAG_WINDOWS)
void win_semaphores(void);
#endif

double q_time;
void qt1(char *cc)
{
double dd;
dd=current_time();
DEB"\n%s %f",cc,dd-q_time);
q_time=dd;
}

void qt2(char *cc)
{
double dd;
dd=current_time();
DEB"\n%s %f",cc,dd-q_time);
}


void kill_all(void)
{
int i,j,k;
kill_all_flag=TRUE;
lir_sleep(10000);
// Sleep for a while to make sure that the first error
// code (if any) has been saved by lirerr.
// When we start to stop threads we may induce new errors
// and they must not overwrite the original error code
// which is ensured by lirerr watching kill_all_flag.
// *******************************************************
close_network_sockets();
k=0;
for(i=0; i<THREAD_MAX; i++)
  {
  if(thread_command_flag[i] != THRFLAG_NOT_ACTIVE)
    {
    k++;
    thread_command_flag[i]=THRFLAG_KILL;
    if(thread_waitsem[i] != -1)lir_sem_post(thread_waitsem[i]);
    }
  }
#if(OSNUM == OS_FLAG_WINDOWS)
win_semaphores();
#endif    
if(k==0)goto ok_exit;
j=0;
while(j<30)
  {
  j++;
  lir_sleep(20000);  
  k=0;
  for(i=0; i<THREAD_MAX; i++)
    {
    if(thread_status_flag[i] == THRFLAG_RETURNED  ||
       thread_status_flag[i] == THRFLAG_NOT_ACTIVE)
      {
      thread_command_flag[i]=THRFLAG_NOT_ACTIVE;
      }
    else
      {  
      if(thread_command_flag[i] != THRFLAG_NOT_ACTIVE)
        {
        k++;
        if(thread_waitsem[i] != -1)
          {
          lir_sem_post(thread_waitsem[i]);
          }
        thread_command_flag[i]=THRFLAG_KILL;
        }
      }
    }  
  if(k==0)
    {
ok_exit:;
    threads_running=FALSE;  
    return;
    }
  }
if(dmp != 0)
  {
  PERMDEB"\n\nTHREAD(S) FAIL TO STOP");
  for(i=0; i<THREAD_MAX; i++)
    {
    PERMDEB"\nThr=%d status=%d   cmd=%d",i,thread_status_flag[i],
                                         thread_command_flag[i]);
    }  
  PERMDEB"\n");
  fflush(dmp);
  }
// Stay here until ctrlC or kill button in Windows or X11
show_errmsg(2);
lir_refresh_screen();
zz:;
goto zz;
}

void pause_thread(int no)
{
int i;
lir_sched_yield();
if( thread_command_flag[no]==THRFLAG_NOT_ACTIVE ||
    thread_status_flag[no]==THRFLAG_RETURNED || kill_all_flag)return;
if(thread_command_flag[no]==THRFLAG_IDLE)
  {
  lirerr(1224);
  return;
  }
thread_command_flag[no]=THRFLAG_IDLE;
for(i=0; i<5; i++)
  {
  if( (ui.network_flag&NET_RXIN_FFT1) != 0 && no==THREAD_SCREEN &&
                    thread_status_flag[THREAD_SCREEN] == THRFLAG_SEM_WAIT)
    {
    lir_sem_post(SEM_SCREEN);
    }
  lir_sched_yield();
  if(thread_status_flag[no] == THRFLAG_IDLE)return;
  }
if(no == THREAD_RX_OUTPUT)lir_sem_post(SEM_RX_DASIG);  
while(thread_status_flag[no] != THRFLAG_IDLE  && !kill_all_flag)
  {
  if( no==THREAD_SCREEN &&
                    thread_status_flag[THREAD_SCREEN] == THRFLAG_SEM_WAIT)
    {
    lir_sem_post(SEM_SCREEN);
    }
  lir_sleep(5000);
  }
}


void resume_thread(int no)
{
// Make sure the input is allowed to take care of its data
// before heavy processing is released.
// This is occasionally needed for the sdr-14.
lir_sched_yield();
if(thread_command_flag[no]!=THRFLAG_IDLE || kill_all_flag)return;
thread_command_flag[no]=THRFLAG_ACTIVE;
}

void pause_screen_and_hide_mouse(void)
{
pause_thread(THREAD_SCREEN);
unconditional_hide_mouse();
}

void linrad_thread_stop_and_join(int no)
{
int k;
if(thread_command_flag[no] == THRFLAG_NOT_ACTIVE)return;
thread_command_flag[no]=THRFLAG_KILL;
if(thread_waitsem[no] != -1)lir_sem_post(thread_waitsem[no]);
lir_sched_yield();
k=0;
while(thread_status_flag[no] != THRFLAG_RETURNED)
  {
  if(thread_waitsem[no] != -1)lir_sem_post(thread_waitsem[no]);
  lir_sleep(3000);
  k++;
  if(no==THREAD_SDR14_INPUT)
    {
    if(k==100 && sdr!=-1)close_sdr14();
    }
  if(no==THREAD_PERSEUS_INPUT)
    {
    if(k==100 && sdr!=-1)close_perseus();
    }
  }
thread_command_flag[no]=THRFLAG_NOT_ACTIVE;
lir_join(no);
thread_status_flag[no]=THRFLAG_NOT_ACTIVE;
}

void lir_text(int x, int y, char *txt)
{
char s[512];
int i,j;
if(x<0 || y<0 || y > screen_last_line)return;
i=0;
while(txt[i] != 0)i++;
if(x+i <= screen_last_col)
  {
  lir_pixwrite(x*text_width, y*text_height+1,txt);
  }
else 
  {
  j=1+screen_last_col-x;
  if(j > 0)
    {
    for(i=0; i<j; i++)s[i]=txt[i];
    s[j]=0;
    lir_pixwrite(x*text_width, y*text_height,s);
    }
  }  
}

void lir_pixwrite(int x0, int y, char *txt)
{
int i, x;
unsigned int k;
i=-1;
if(x0>=0)
  {
  if(y>=0)
    {
    if(y+text_height-font_ysep < screen_height)
      {
      i=0;
      x=x0;
      while(txt[i] != 0)
        {
        k=(unsigned char)(txt[i]);
        if(k>160)k=1;
        if(x+text_width > screen_width)goto errx;
        lir_putbox(x,y,font_w,font_h,&vga_font[k*font_size]);
        i++;
        x+=text_width;
        } 
      return;
      }
    }
  }      
errx:;  
DEB"\nScreen write error");
DEB"\n[%s]   x=%d   y=%d   (%d  %d)",txt,x0,y,screen_width,screen_height);
if(dmp != NULL)lirerr(1258);
}


void aw_keyb(void)
{
while(keyboard_buffer_ptr == keyboard_buffer_used)
  {
  if(keyboard_buffer_ptr == keyboard_buffer_used)
    {
    lir_sem_wait(SEM_KEYBOARD);
    if(kill_all_flag) return;
    }
  }
lir_inkey = keyboard_buffer[keyboard_buffer_used];
keyboard_buffer_used=(keyboard_buffer_used+1)&(KEYBOARD_BUFFER_SIZE-1);
lir_sleep(10000);
}


void await_keyboard(void)
{
lir_refresh_screen();
aw_keyb();
}

void init_semaphores(void)
{
int i;
for(i=0; i<SEM_KEYBOARD; i++)lir_sem_init(i);
}

void free_semaphores(void)
{
int i;
for(i=0; i<SEM_KEYBOARD; i++)lir_sem_free(i);
}

void user_command(void)
{
#if RUSAGE_OLD == TRUE
int local_workload_counter;
#endif
int i, m, flag;
#if OSNUM == OS_FLAG_LINUX
clear_thread_times(THREAD_USER_COMMAND);
#endif
#if RUSAGE_OLD == TRUE
local_workload_counter=workload_counter;
#endif
thread_status_flag[THREAD_USER_COMMAND]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_USER_COMMAND] == THRFLAG_ACTIVE)
  {
#if RUSAGE_OLD == TRUE
  if(local_workload_counter != workload_counter)
    {
    local_workload_counter=workload_counter;
    make_thread_times(THREAD_USER_COMMAND);
    }
#endif
// **********************************************
// Get input from the user and do whatever.......
  if( usercontrol_mode != USR_NORMAL_RX)lir_refresh_screen();
  aw_keyb();
  process_current_lir_inkey();
  if(kill_all_flag) goto user_error_exit;
  if(numinput_flag != TEXT_PARM)
    {
    if(lir_inkey == 'X')
      {
      DEB"\n'X' Pressed");
      goto user_exit;
      }
    if(lir_inkey == 'Z') 
      {
      workload_reset_flag++;
      goto ignore;
      }
    }
  if(numinput_flag != 0)
    {
    get_numinput_chars();
    }
  else
    {  
    switch(usercontrol_mode)
      {
      case USR_NORMAL_RX:
      switch (lir_inkey)
        {
        case 'A':
        pause_thread(THREAD_SCREEN);
        ampinfo_flag^=1;
        amp_info_texts();
        timinfo_flag=0;
        resume_thread(THREAD_SCREEN);
        workload_reset_flag++;
        break;

        case '+':
        m=bg.wheel_stepn;
        m++;
        if(m>30)m=30;
        bg.wheel_stepn=m;
        make_modepar_file(GRAPHTYPE_BG);
        sc[SC_SHOW_WHEEL]++;
        break;

        case '-':
        m=bg.wheel_stepn;
        m--;
        if(m<-30)m=-30;
        if(genparm[AFC_ENABLE]==0 && m<0)m=0;
        bg.wheel_stepn=m;
        make_modepar_file(GRAPHTYPE_BG);
        sc[SC_SHOW_WHEEL]++;
        break;

        case 'E':
        if(genparm[AFC_ENABLE] == 1)spurinit_flag++;
        break;
      
        case 'C':
        if(genparm[AFC_ENABLE] != 0)spurcancel_flag++;
        break;

        case 'Q':
        userdefined_q();
        break;

        case 'S':
        if(abs(fg.passband_direction) != 1)break;
        disksave_start_stop();
        break;

        case 'T':
        timinfo_flag^=1;
        if(timinfo_flag!=0)time_info_time=current_time();
        pause_thread(THREAD_SCREEN);
        timing_info_texts();
        resume_thread(THREAD_SCREEN);
        workload_reset_flag++;
        break;

        case 'U':
        userdefined_u();
        break;

        case 'W':
        wavsave_start_stop(0);
        if( wav_write_flag < 0)wav_write_flag=0;
        break;      
    
        case F1_KEY:
        help_screen_objects();
        break;

        case F5_KEY:
        mailbox[1]++;
        break;

        case F6_KEY:
        mailbox[2]++;
        break;

        case F2_KEY:
        if(s_meter_avgnum <0)
          {
          s_meter_avg=0;
          s_meter_avgnum=0;
          }
        else
          {
          s_meter_avgnum=-1;
          }
        break;

        case F3_KEY:
        audio_dump_flag^=1;
        audio_dump_flag&=1;
        lir_fillbox(0,0,6,6,12*audio_dump_flag);
        lir_refresh_screen();
        break;

        case F9_KEY:
        truncate_flag<<=1;
        truncate_flag++;
        if( (ui.rx_input_mode&DWORD_INPUT) == 0)
          {
          if(truncate_flag > 0x3ff)truncate_flag=0;
          }
        else
          {
          if(truncate_flag > 0x3ffff)truncate_flag=0;
          }
        clear_lines(0,0);
        break;

        case F11_KEY:
        internal_generator_flag^=1;
        break;

        case F10_KEY:
        if(OSNUM == OS_FLAG_LINUX)
          {
          internal_generator_noise++;
          if( (ui.rx_input_mode&DWORD_INPUT) == 0)
            {
            if(internal_generator_noise > 8)internal_generator_noise=0;
            }
          else
            {
            if(internal_generator_noise <8)internal_generator_noise=8;
            if(internal_generator_noise > 24)internal_generator_noise=0;
            }
          clear_lines(0,0);
          }
        break;

        case SHIFT_F3_KEY:
        diskread_pause_flag^=1;
        diskread_pause_flag&=1;
        lir_fillbox(8,0,6,6,14*diskread_pause_flag);
        lir_refresh_screen();
        break;
      
        case ARROW_UP_KEY:
        if(genparm[CW_DECODE_ENABLE] != 0)
          {
          i=cg_osc_offset_inc;
          cg_osc_offset_inc*=1.3;
          if(i==cg_osc_offset_inc)cg_osc_offset_inc++;
          if(cg_osc_offset_inc>(baseband_size<<3) )
                                          cg_osc_offset_inc=baseband_size<<3;
          pause_thread(THREAD_SCREEN);
          shift_cg_osc_track(0);
          resume_thread(THREAD_SCREEN);
          }
        else
          {
          bg.filter_flat=bg_hz_per_pixel*(bg_flatpoints+1.5);
          make_bg_filter();
          make_modepar_file(GRAPHTYPE_BG);
          mg_clear_flag=TRUE;
          }
        break;
        
        case ARROW_DOWN_KEY:
        if(genparm[CW_DECODE_ENABLE] != 0)
          {
          i=cg_osc_offset_inc;
          cg_osc_offset_inc/=1.3;
          if(cg_osc_offset_inc==0)cg_osc_offset_inc=1;
          pause_thread(THREAD_SCREEN);
          shift_cg_osc_track(0);
          resume_thread(THREAD_SCREEN);
          }
        else
          {
          if(bg_flatpoints > 1)
            {
            bg.filter_flat=bg_hz_per_pixel*(bg_flatpoints-1.5);
            make_bg_filter();
            make_modepar_file(GRAPHTYPE_BG);
            mg_clear_flag=TRUE;
            }
          }
        break;

        case ARROW_RIGHT_KEY:
        if(genparm[CW_DECODE_ENABLE] != 0)
          {
          pause_thread(THREAD_SCREEN);
          shift_cg_osc_track(cg_osc_offset_inc);
          resume_thread(THREAD_SCREEN);
          }
        else
          {
          step_rx_frequency(1);
          }             
        break;
        
        case ARROW_LEFT_KEY:
        if(genparm[CW_DECODE_ENABLE] != 0)
          {
          pause_thread(THREAD_SCREEN);
          shift_cg_osc_track(-cg_osc_offset_inc);
          resume_thread(THREAD_SCREEN);
          }
        else
          {
          step_rx_frequency(-1);
          }
        break;

        case ' ':
        copy_rxfreq_to_tx();
        break;

        case 'V':
        case 'B':
        case 'N':
        copy_txfreq_to_rx();
        break;

        case 'M':
        bg.horiz_arrow_mode++;
        if(bg.horiz_arrow_mode > 2)bg.horiz_arrow_mode=0;
        make_modepar_file(GRAPHTYPE_BG);
        sc[SC_BG_BUTTONS]++;
        break;
        }
      break;  

      case USR_TXTEST:
      switch (lir_inkey)
        {
        case F4_KEY:
        lir_status=LIR_POWTIM;
        goto user_exit;      
        }
      break;

      case USR_POWTIM:
      switch (lir_inkey)
        {
        case '+':
        powtim_gain*=2;
        break;
      
        case '-':
        powtim_gain/=1.8;
        break;

        case ARROW_UP_KEY:
        powtim_fgain*=2;
        break;
      
        case ARROW_DOWN_KEY:
        powtim_fgain/=1.8;
        break;

        case 'D':
        if(powtim_xstep < 5)
          {
          powtim_xstep--;
          }
        else
          {
          powtim_xstep/=1.5;
          }
        if(powtim_xstep<1)powtim_xstep=1;     
        break;
      
        case 'I':
        if(powtim_xstep < 5)
          {
          powtim_xstep++;
          }
        else
          {
          powtim_xstep*=2;
          }
        if(powtim_xstep>timf2pow_mask/16)powtim_xstep=timf2pow_mask/16;  
        break;

        case 'L':
        powtim_displaymode^=1;
        break;

        case 'W':
        powtim_displaymode^=2;
        break;

        case 'M':
        powtim_trigmode++;
        if(powtim_trigmode >= POWTIM_TRIGMODES)powtim_trigmode=0;
        break;

        case 'P':
        powtim_pause_flag=1;
        break;
        
        case 'R':
        powtim_pause_flag=0;
        break;

        if(powtim_pause_flag != 0)
          {
          case ARROW_RIGHT_KEY:
          timf2_pn2=(timf2_pn2-screen_width*powtim_xstep/4+timf2_mask+1)&timf2_mask;
          break;

          case ARROW_LEFT_KEY:
          timf2_pn2=(timf2_pn2+screen_width*powtim_xstep/5)&timf2_mask;
          break;
          }        
        }
      if(kill_all_flag) goto user_error_exit;
      powtim_parmwrite();
      if(powtim_pause_flag != 0) powtim_screen();
      break;

      case USR_ADTEST:
      adtest_new++;
      switch (lir_inkey)
        {
        case '+':
        if(adtest_scale < BIG)adtest_scale*=2;
        break;

        case '-':
        if(adtest_scale > 1/BIG)adtest_scale/=2;
        break;

        case 'C':
        if(ui.rx_rf_channels == 2)adtest_channel^=1;
        break;

        case 'P':
        powtim_pause_flag=1;
        break;
        
        case 'R':
        powtim_pause_flag=0;
        break;

        case 'W':
        powtim_displaymode^=1;
        break;
        }
      break;
        
      case USR_IQ_BALANCE:
      pause_thread(THREAD_CAL_IQBALANCE);
      caliq_clear_flag=TRUE;
      switch (lir_inkey)
        {
        case '+':
        bal_segments*=2;
        break;
          
        case '-':
        bal_segments/=2;
        break;
          
        case 'S':
        write_iq_foldcorr();
        break;
          
        case 'U':
        update_iq_foldcorr();
        break;
          
        case 'C':
        clear_iq_foldcorr();
        break;

        case F1_KEY:
        help_message(311);
        caliq_clear_flag=FALSE;
        break;

        default:
        caliq_clear_flag=FALSE;
        break;
        }
      clear_keyboard();
      resume_thread(THREAD_CAL_IQBALANCE);
      break;

      case USR_CAL_INTERVAL:
      pause_thread(THREAD_CAL_INTERVAL);
      switch (lir_inkey)
        {
        case '+':
        cal_ygain*=2;
        break;
          
        case '-':
        cal_ygain/=2;
        break;

        case 'E':
        cal_xgain*=2;
        break;
          
        case 'C':
        cal_xgain/=2;
        break;
        
        case F1_KEY:
        help_message(304);
        break;

        case 10:
        DEB"\nENTER Pressed");
        usercontrol_mode=USR_CAL_FILTERCORR;
        goto user_exit;
        }
      clear_keyboard();
      resume_thread(THREAD_CAL_INTERVAL);
      break;

      case USR_CAL_FILTERCORR:
      if(thread_command_flag[THREAD_CAL_FILTERCORR]==THRFLAG_ACTIVE)
                                        pause_thread(THREAD_CAL_FILTERCORR);
      flag=THRFLAG_ACTIVE;  
      switch (lir_inkey)
        {
        case '+':
        cal_ygain*=2;
        break;
          
        case '-':
        cal_ygain/=2;
        break;

        case 'A':
        clear_fft1_filtercorr();
        make_cal_fft1_filtercorr();
        flag=THRFLAG_RESET;
        break;

        case 'E':
        cal_xgain*=2;
        break;
          
        case 'C':
        cal_xgain/=2;
        break;

        case 'S':
        write_filcorr(-1);
        break;
          
        case 'T':
        cal_domain^=1;
        break;

        case 'I':
        break;
          
        case 'U':
        if(cal_buf4[0]>=0)
          {
          if(cal_update_ram() == 0)
            {
            linrad_thread_stop_and_join(THREAD_CAL_FILTERCORR);
            goto user_exit;
            }
          flag=THRFLAG_RESET;
          }
        break;

        case F1_KEY:
        help_message(305);
        flag=THRFLAG_RESET;
        break;

        }
      thread_command_flag[THREAD_CAL_FILTERCORR]=flag;
      while(thread_status_flag[THREAD_CAL_FILTERCORR] != THRFLAG_ACTIVE)
        {
        lir_sleep(20000);
        if(kill_all_flag) goto user_error_exit;
        }
      break;

      case USR_TUNE:
      if(thread_command_flag[THREAD_TUNE]==THRFLAG_ACTIVE)
        {
        thread_command_flag[THREAD_TUNE]=THRFLAG_IDLE;
        while(thread_status_flag[THREAD_TUNE]!=THRFLAG_IDLE)
          {
          lir_sleep(5000);
          if(kill_all_flag) goto user_error_exit;
          }
        switch(lir_inkey)
          {
          case '+':
          tune_yzer-=100;
          break;

          case '-':
          tune_yzer+=100;
          break;
          } 
        thread_command_flag[THREAD_TUNE]=THRFLAG_ACTIVE;
        while(thread_status_flag[THREAD_TUNE]!=THRFLAG_ACTIVE)
          {
          lir_sleep(5000);
          if(kill_all_flag) goto user_error_exit;
          }
        }  
      break;
      }
    }
ignore:;
  if(kill_all_flag) goto user_error_exit;
  }
user_exit:;  
thread_status_flag[THREAD_USER_COMMAND]=THRFLAG_RETURNED;
return;
user_error_exit:;
if(dmp!=NULL)fflush(dmp);
thread_status_flag[THREAD_USER_COMMAND]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_USER_COMMAND] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void xz(char *s)
{
if(dmp == NULL)
  {
  lirerr(1074);
  lir_sleep(100000);
  return;
  }
PERMDEB" %s\n",s);
fflush(dmp);
lir_sync();
}


void timerr(int line, char*txt)
{
char s[80];
settextcolor(15);
sprintf(s,"Premature call to %s",txt);
lir_text(0,line,s);
settextcolor(7);
}

void test_keyboard(void)
{
if(kill_all_flag!=0)return;
if(keyboard_buffer_ptr != keyboard_buffer_used)
  {
  lir_sem_wait(SEM_KEYBOARD);
  lir_inkey = keyboard_buffer[keyboard_buffer_used];
  keyboard_buffer_used=(keyboard_buffer_used+1)&(KEYBOARD_BUFFER_SIZE-1);
  }
else
  {
  lir_inkey=0;  
  }
}


void clear_keyboard(void)
{
test_keyboard();
if(kill_all_flag) return;
while(lir_inkey != 0)
  {
  test_keyboard();
  if(kill_all_flag) return;
  }
}


void clear_await_keyboard(void)
{
clear_keyboard();
if(kill_all_flag) return;
await_keyboard();
}



int qnqcnt=0;
int qnqcnt1=0;
int qnqcnt2=0;
#define QQLINE 0
void qq(char *s)
{
char w[80];
sprintf(w,"%s%3d",s,qnqcnt);
lir_text(5,QQLINE,w);
lir_refresh_screen();
qnqcnt++;
if(qnqcnt > 999)qnqcnt=0;
}

void qq1(char *s)
{
char w[80];
sprintf(w,"%s%3d",s,qnqcnt1);
lir_text(15,QQLINE,w);
lir_refresh_screen();
qnqcnt1++;
if(qnqcnt1 > 999)qnqcnt1=0;
}

void qq2(char *s)
{
char w[80];
sprintf(w,"%s%3d",s,qnqcnt2);
lir_text(25,QQLINE,w);
lir_refresh_screen();
qnqcnt2++;
if(qnqcnt2 > 999)qnqcnt2=0;
}

void show_button(BUTTONS *butt, char *s)
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


void update_bar(int x1, int x2, int yzer, int newy, 
                int oldy, unsigned char color,char* buf)
{
int i, k, height, width;
if(newy == oldy)return;
if(oldy < 0)oldy=yzer;
width=x2-x1+1;
if(newy < oldy)
  {
  height=oldy-newy+1;
  lir_getbox(x1,newy,width,height,buf);
  k=width*height;
  for(i=0; i<k; i++)
    {
    if(buf[i] != 7) 
      {
      buf[i]=color;
      }
    }
  lir_putbox(x1,newy,width,height, buf);
  }
else
  {
  height=newy-oldy+1;
  lir_getbox(x1,oldy,width,height,buf);
  k=width*height;
  for(i=0; i<k; i++)
    {
    if(buf[i] != 7) 
      {
      buf[i]=0;
      }
    }
  lir_putbox(x1,oldy,width,height, buf);
  }
}

int make_power_of_two( int *i)
{
int k;
k=-1;
i[0]+=i[0]-1;
while(i[0] != 0)
  {
  i[0]/=2;
  k++;
  }
i[0]=1<<k;
return k;
}

void *chk_free(void *p)
{
if(p==NULL)return NULL;
free(p);
return NULL;
}

void process_current_lir_inkey(void)
{
if(lir_inkey == 13)lir_inkey = 10;
lir_inkey=toupper(lir_inkey);
if(lir_inkey == 'G')
  {
  if(fft1_handle==NULL)
    {
    save_screen_image();
    }
  else
    {  
    if( numinput_flag != TEXT_PARM)
      {
      save_screen_image();
      lir_inkey=0;
      }
    }
  }
}



int lir_get_filename(int  x, int y, char *name)
{
int len;
len=0;
next_char:;
name[len]=0;
lir_text(x,y,name);
await_keyboard();
if(kill_all_flag)return 0;
if(lir_inkey == 10) return len;
if(lir_inkey == 8 || 
   lir_inkey == ARROW_LEFT_KEY ||
   lir_inkey == 127)
  {
  if(len==0)goto next_char;
  name[len-1]=' ';
  lir_text(x,y,name);  
  len--;
  goto next_char;
  }
if(  (lir_inkey >=  '.' && lir_inkey <= '9') ||
     (lir_inkey >= 'A' && lir_inkey <= 'Z') ||
     lir_inkey == '_' || 
     lir_inkey == '-' || 
     lir_inkey == '+' || 
#if(OSNUM == OS_FLAG_WINDOWS)
     lir_inkey == ':' || 
     lir_inkey == '\\' || 
#endif
     (lir_inkey >= 'a' && lir_inkey <= 'z'))
  {
  name[len]=lir_inkey;
  if(len<79)len++;
  }
goto next_char;  
}

int lir_get_text(int  x, int y, char *txtbuf)
{
int len;
len=0;
next_char:;
txtbuf[len]='_';
txtbuf[len+1]=0;
lir_text(x,y,txtbuf);
await_keyboard();
if(kill_all_flag)return 0;
if(lir_inkey == 10)
  {
  txtbuf[len]=0;
  return len;
  }
if(lir_inkey == 8 || 
   lir_inkey == ARROW_LEFT_KEY ||
   lir_inkey == 127)
  {
  if(len==0)goto next_char;
  txtbuf[len]=' ';
  len--;
  txtbuf[len]='_';
  lir_text(x,y,txtbuf);  
  txtbuf[len+1]=0;
  goto next_char;
  }
if(lir_inkey >= ' ' && lir_inkey <= 'z')
  {
  txtbuf[len]=lir_inkey;
  if(len<78)len++;
  }
goto next_char;  
}


int lir_get_integer(int  x, int y, int len, int min, int max)
{
char s[16];
int i, j, pos;
if(len < 1 || len > 15)
  {
  lirerr(8);
  return min;
  }
for(i=1; i<len; i++)s[i]=' '; 
s[0]='_';
s[len]=0;
lir_text(x,y,s);
pos=0;
digin:;
await_processed_keyboard();
if(kill_all_flag)return min;
switch (lir_inkey)
  {
  case F1_KEY:
  return min;

  case 10: 
  if(pos != 0)
    {
    j=atoi(s);
    s[pos]=' ';
    if(j < min)
      {
      j=min;
      sprintf(s,"%d ",min);
      }
    lir_text(x,y,s);
    return j;
    }
  break;

  case 8:
  case 127:
  case ARROW_LEFT_KEY:
  if(pos > 0)
    {
    s[pos+1]=0;
    s[pos]=' ';
    pos--;
    s[pos]='_';
    lir_text(x,y,s);
    s[pos+1]=0;
    }
  break;
  
  case '-':
  if(min < 0)
    {
    if(pos == 0)
      {
      s[pos]='-';
      pos++;
      s[pos]=0;
      }
    lir_text(x,y,s);
    }  
  break;      


  default:
  if(lir_inkey>='0' && lir_inkey <='9')
    {
    if(pos < len)
      {
      s[pos]=lir_inkey;
      pos++;
      s[pos]=0;
      j=atoi(s);
      if(s[0] != '-')
        {
        if(j > max)sprintf(s,"%d",max);
        }
      else
        {  
        if(j < min)sprintf(s,"%d",min);
        }
      pos=0;
      while(s[pos] != 0 && s[pos] != ' ')pos++;
      s[pos]='_';
      j=pos+1;
      while( j< len)
        {
        s[j]=' ';
        j++;
        }
      s[len]=0;    
      lir_text(x,y,s);
      s[pos+1]=0;
      }
    }
  break;
  }
goto digin;
}

float lir_get_float(int  x, int y, int len, float min, float max)
{
char s[16];
int i, j, pos;
float t1;
if(len < 1 || len > 15)
  {
  lirerr(8);
  return min;
  }
for(i=1; i<len; i++)s[i]=' '; 
s[0]='_';
s[len]=0;
lir_text(x,y,s);
pos=0;
digin:;
await_processed_keyboard();
if(kill_all_flag)return min;
switch (lir_inkey)
  {
  case 10: 
  if(pos != 0)
    {
    t1=atof(s);
    if(t1 < min)t1=min;
    return t1;
    }
  break;
  
  case '-':
  if(pos == 0)
    {
    s[pos]='-';
    pos++;
    s[pos]=0;
    }
  break;      

  case 8:
  case 127:
  case ARROW_LEFT_KEY:
  if(pos > 0)
    {
    s[pos]=' ';
    pos--;
    s[pos]='_';
    lir_text(x,y,s);
    }
  break;
  
  case '.':
  for(j=0; j<pos; j++)
    {
    if(s[j]=='.')break;
    }
  s[pos]='.';
  pos++;
  s[pos]=0;
  break;      
  
  default:
  if(lir_inkey>='0' && lir_inkey <='9')
    {
    if(pos < len)
      {
      s[pos]=lir_inkey;
      pos++;
      s[pos]=0;
      t1=atof(s);
      if(t1 > max)sprintf(s,"%f",max);
      pos=0;
      while(s[pos] != 0 && s[pos] != ' ')pos++;
      s[pos]='_';
      j=pos+1;
      while( j< len)
        {
        s[j]=' ';
        j++;
        }  
      lir_text(x,y,s);
      }
    }
  break;
  }
goto digin;
}

void clear_lines(int i, int j)
{
int k;
if(i > j)
  {
  k=i;
  i=j;
  j=k;
  }
lir_fillbox(0,i*text_height,screen_width-1,(j-i+1)*text_height,0);
}

void await_processed_keyboard(void)
{
await_keyboard();
process_current_lir_inkey();
}


void toupper_await_keyboard(void)
{
await_keyboard();
lir_inkey=toupper(lir_inkey);
}

int adjust_scale(double *step)
{
int pot,i;
double t1;
// Make step the nearest larger of 1, 2 or 5 in the same power of 10 
t1=step[0];
pot=0;
while(t1 > 10)
  {
  t1/=10;
  pot++;
  }
if(t1<0.00001)t1=.00001;  
while(t1 < 1)
  {
  t1*=10;
  pot--;
  }
if(t1 <= 2)
  {
  t1=2;
  i=2;
  goto asx;
  }
if(t1 <= 5)
  {
  t1=5;
  i=5;
  goto asx;
  }
t1=10;
i=1;
asx:;
while(pot > 0)
  {
  t1*=10;
  pot--;
  }
while(pot < 0)
  {
  t1/=10;
  pot++;
  }
step[0]=t1;
return i;
}

void clear_button(BUTTONS *butt, int max)
{
int i;
for(i=0; i<max; i++)
  {
  butt[i].x1=-1;
  butt[i].x2=-1;
  butt[i].y1=-1;
  butt[i].y2=-1;
  }  
}

void make_button(int x, int y, BUTTONS *butt, int m, char chr)
{
char s[2];
s[0]=chr;
s[1]=0;
lir_pixwrite(x-text_width/2+1,y-text_height/2+1,s);
butt[m].x1=x-text_width/2-2;
butt[m].x2=x+text_width/2+2;
butt[m].y1=y-text_height/2-1;
butt[m].y2=y+text_height/2;
lir_hline(butt[m].x1,butt[m].y1,butt[m].x2,7);
if(kill_all_flag) return;
lir_hline(butt[m].x1,butt[m].y2,butt[m].x2,7);
if(kill_all_flag) return;
lir_line(butt[m].x1,butt[m].y1,butt[m].x1,butt[m].y2,7);
if(kill_all_flag) return;
lir_line(butt[m].x2,butt[m].y1,butt[m].x2,butt[m].y2,7);
}  

void set_graph_minwidth(WG_PARMS *a)
{
int i;
i=a[0].xright-a[0].xleft;
if(i!=current_graph_minw)
  {
  a[0].xleft=(a[0].xleft+a[0].xright-current_graph_minw)/2;
  a[0].xright=a[0].xleft+current_graph_minw;
  }
if(a[0].xright>screen_last_xpixel)
  {
  a[0].xright=screen_last_xpixel;
  a[0].xleft=a[0].xright-current_graph_minw;
  }
if(a[0].xleft<0)
  {
  a[0].xleft=0;
  a[0].xright=a[0].xleft+current_graph_minw;
  }
i=a[0].ybottom-a[0].ytop;
if(i!=current_graph_minh)
  {
  a[0].ybottom=(a[0].ybottom+a[0].ytop+current_graph_minh)/2;
  a[0].ytop=a[0].ybottom-current_graph_minh;
  }  
if(a[0].ybottom>=screen_height)
  {
  a[0].ybottom=screen_height-1;
  a[0].ytop=a[0].ybottom-current_graph_minh;
  }  
if(a[0].ytop <0)
  {
  a[0].ytop=0;
  a[0].ybottom=a[0].ytop+current_graph_minh;
  }
}


void graph_mincheck(WG_PARMS *a)
{
if(a[0].ybottom-a[0].ytop < current_graph_minh)
  {      
  a[0].ybottom=(a[0].ybottom+a[0].ytop+current_graph_minh)>>1;
  a[0].ytop=a[0].ybottom-current_graph_minh;
  }
if(a[0].xright-a[0].xleft < current_graph_minw)
  {
  a[0].xright=(a[0].xright+a[0].xleft+current_graph_minw)>>1;
  a[0].xleft=a[0].xright-current_graph_minw;
  }
if(a[0].xleft<0)
  {
  a[0].xleft=0;
  if(a[0].xright<current_graph_minw)a[0].xright=current_graph_minw;
  }
if(a[0].ytop<0)
  {
  a[0].ytop=0;
  if(a[0].ybottom<current_graph_minh)a[0].ybottom=current_graph_minh;
  }
if(a[0].xright>screen_last_xpixel)
  {
  a[0].xright=screen_last_xpixel;
  if(a[0].xright-a[0].xleft<current_graph_minw)
                                  a[0].xleft=a[0].xright-current_graph_minw;
  }
if(a[0].ybottom>screen_height-1)
  {
  a[0].ybottom=screen_height-1;
  if(a[0].ybottom-a[0].ytop<current_graph_minh)
                                  a[0].ytop=a[0].ybottom-current_graph_minh;
  }
}

void avoid_graph_collision(WG_PARMS *a, WG_PARMS *b)
{
int ix,jx,iy,jy;
int la,ra,ta,ba;
// One border line of graph a has been changed.
// Check if graph a now is on top of graph b.
// If there is a collission, find out what border line to move which
// will cause minimum loss of graph area.
// This routine is also called every time a graph is placed on the
// screen when the program is started.
// During start, this simple procedure may fail, therefore
// some security is added at the end of this routine. 
if(a==b)return;
ix=b[0].xright-a[0].xleft+1;
jx=b[0].xleft-a[0].xright+1;
if(ix*jx>0)goto check;
iy=b[0].ybottom-a[0].ytop+1;
jy=b[0].ytop-a[0].ybottom+1;
if(iy*jy>0)goto check;
la=ix*(a[0].ybottom-a[0].ytop);
ra=-jx*(a[0].ybottom-a[0].ytop);
ta=iy*(a[0].xright-a[0].xleft);
ba=-jy*(a[0].xright-a[0].xleft);
if(la<ra)
  {
  if(ta<ba)
    {
    if(la<ta)
      {
      a[0].xleft=b[0].xright+1;
      }
    else  
      {
      a[0].ytop=b[0].ybottom+1;
      }
    }
  else
    {
    if(la<ba)
      {
      a[0].xleft=b[0].xright+1;
      }
    else  
      {
      a[0].ybottom=b[0].ytop-1;
      }
    }  
  }
else
  {
  if(ta<ba)
    {
    if(ra<ta)
      {
      a[0].xright=b[0].xleft-1;
      }
    else  
      {
      a[0].ytop=b[0].ybottom+1;
      }
    }
  else
    {
    if(ra<ba)
      {
      a[0].xright=b[0].xleft-1;
      }
    else  
      {
      a[0].ybottom=b[0].ytop-1;
      }
    }  
  }
// Make sure we are always within the screen.
// Don't care if graphs overlay now.
check:;
graph_mincheck(a);
}  

void check_graph_placement(WG_PARMS *a)
{
graph_mincheck(a);
if(fg_flag)avoid_graph_collision(a,(void*)(&fg));
if(wg_flag)avoid_graph_collision(a,&wg);
if(hg_flag)avoid_graph_collision(a,(void*)(&hg));
if(bg_flag)avoid_graph_collision(a,(void*)(&bg));
if(ag_flag)avoid_graph_collision(a,(void*)(&ag));
if(cg_flag)avoid_graph_collision(a,(void*)(&cg));
if(pg_flag)avoid_graph_collision(a,(void*)(&pg));
if(eg_flag)avoid_graph_collision(a,(void*)(&eg));
if(mg_flag)avoid_graph_collision(a,(void*)(&mg));
if(tg_flag)avoid_graph_collision(a,(void*)(&tg));
if(rg_flag)avoid_graph_collision(a,(void*)(&rg));
}

void graph_borders(WG_PARMS *a,unsigned char color)
{
lir_hline(a[0].xleft,a[0].ytop,a[0].xright,color);
if(kill_all_flag) return;
lir_hline(a[0].xleft,a[0].ybottom,a[0].xright,color);
if(kill_all_flag) return;
lir_line(a[0].xleft,a[0].ytop,a[0].xleft,a[0].ybottom,color);
if(kill_all_flag) return;
lir_line(a[0].xright,a[0].ytop,a[0].xright,a[0].ybottom,color);
}

void dual_graph_borders(WG_PARMS *a,unsigned char color)
{
graph_borders(a,color);
if(kill_all_flag) return;
lir_hline(a[0].xleft,a[0].yborder,a[0].xright,color);
}
