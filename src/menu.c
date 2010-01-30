
#include <ctype.h>
#include <unistd.h>
#include <string.h>


#include <globdef.h>
#include <uidef.h> 
#include <fft1def.h>
#include <fft2def.h>
#include <screendef.h>
#include <powtdef.h>
#include <vernr.h>
#include <sigdef.h>
#include <seldef.h>
#include <thrdef.h>
#include <sdrdef.h>
#include <caldef.h>
#include <keyboard_def.h>
#include <txdef.h>
#include <options.h>

#define INVSOCK -1
extern int tune_bytes;
#if (USERS_EXTRA_PRESENT == 1)
extern void init_users_extra(void);
#endif

char *parfilnam;
int uiparm_save[MAX_UIPARM];
int dirflag, iqflag;

void prompt_reason(char *s)
{
clear_screen();
lir_text(5,8,"You are prompted to the parameter selection screens");
lir_text(15,10,"for the following reason:");
lir_text(5,13,s);
lir_text(20,16,press_any_key);
await_keyboard();
}

void check_output_no_of_channels(void)
{
int i;
i=1+((genparm[OUTPUT_MODE]>>1)&1);
if(i<ui.rx_min_da_channels)i=ui.rx_min_da_channels;
if(i>ui.rx_max_da_channels)i=ui.rx_max_da_channels;
i--;
genparm[OUTPUT_MODE]&=-3;
genparm[OUTPUT_MODE]|=i<<1;            
}

void show_bw(void)
{
char s[80];
clear_screen();
sprintf(s,"fft1 size %d (Bw=%fHz)",fft1_size,fft1_bandwidth);
lir_text(1,1,s);
sprintf(s,"fft2 size %d (Bw=%fHz)",fft2_size,fft2_bandwidth);
lir_text(1,2,s);
}

void modify_parms(char *line1, int first, int last)
{
char s[80];
int line1_len, line;
int i, j, k, m, no, mouse_line,parnum;
line1_len=strlen(line1)+8;
no=last-first+1;
start:;
hide_mouse(0,screen_width,0,screen_height);
clear_screen();
settextcolor(14);
lir_text(5,1, line1);
// Make sure fft1_n and fft2_n are defined and 
// in agreement with current parameters.
get_wideband_sizes();
if( first <= FIRST_FFT_SINPOW && last >= FIRST_FFT_SINPOW)
  {
  sprintf(s,"fft1 size=%d (Bw=%fHz)",fft1_size,fft1_bandwidth);
  lir_text(line1_len,1,s);
  }    
if( first <= SECOND_FFT_SINPOW && last >= SECOND_FFT_SINPOW)
  {
  sprintf(s,"fft2 size=%d (Bw=%fHz)",fft2_size,fft2_bandwidth);
  lir_text(line1_len,1,s);
  }    
if(kill_all_flag) return;
if( first <= DA_OUTPUT_SPEED && last >= DA_OUTPUT_SPEED)
  {
  if(genparm[DA_OUTPUT_SPEED] < ui.rx_min_da_speed)
                              genparm[DA_OUTPUT_SPEED]=ui.rx_min_da_speed;
  if(genparm[DA_OUTPUT_SPEED] > ui.rx_max_da_speed)
                              genparm[DA_OUTPUT_SPEED]=ui.rx_max_da_speed;
  }
if( first <= CW_DECODE_ENABLE && last >= CW_DECODE_ENABLE)
  {
  if(rx_mode==MODE_WCW || rx_mode==MODE_NCW || rx_mode==MODE_HSMS)
    {
    if(genparm[CW_DECODE_ENABLE] != 0)
      {
      settextcolor(12);
      lir_text(1,14,"WARNING: The Morse decode routines are incomplete.");
      lir_text(1,15,"They will not produce any useful output and may cause");
      lir_text(1,16,"a program crasch. Use only for development and perhaps");
      lir_text(1,17,"for some evaluation of chirp and other keying defects");
      lir_text(1,18,"with the coherent graph oscilloscope.");
      }
    }
  else
    {
    genparm[CW_DECODE_ENABLE]=0;
    }
  }
if( first <= FIRST_BCKFFT_ATT_N && last >= FIRST_BCKFFT_ATT_N)
  {
  k=(fft1_n-4)&0xfffe;
  if(genparm[FIRST_BCKFFT_ATT_N]>k)genparm[FIRST_BCKFFT_ATT_N]=k;
  if(genparm[FIRST_BCKFFT_ATT_N]<0)genparm[FIRST_BCKFFT_ATT_N]=
                                             genparm_min[FIRST_BCKFFT_ATT_N];
  }
if( first <= SECOND_FFT_ATT_N && last >= SECOND_FFT_ATT_N)
  {
  k=fft2_n-2;
  if(genparm[SECOND_FFT_ATT_N]>k)genparm[SECOND_FFT_ATT_N]=k;
  }
settextcolor(7);
line=0;
for(i=0; i<no; i++)
  {
  j=i+first;
  if(ui.newcomer_mode != 0 && newco_genparm[j] ==0)
    {
    settextcolor(8);
    }
  else
    {
    settextcolor(7);
    }
  sprintf(s,"%s [%d] ",genparm_text[j],genparm[j]);
  lir_text(1,3+i, s);
  }
lir_text(1,5+no,"Use left mouse button to select line");  
lir_text(1,3+no,"CONTINUE");  
settextcolor(15);
show_mouse();
modloop:;
if( new_mouse_x!=mouse_x || new_mouse_y!=mouse_y)
  {
  lir_move_mouse_cursor();
  show_mouse();
  }
lir_refresh_screen();
lir_sleep(10000);
if(new_lbutton_state==1)lbutton_state=1;
if(new_lbutton_state==0 && lbutton_state==1)
  {
  lbutton_state=0;
  mouse_line=mouse_y/text_height-3;
  if(mouse_line == no)goto loopx;
  if(mouse_line >= 0 && mouse_line <no)
    {
    parnum=mouse_line+first;
    clear_screen();
    settextcolor(14);
    lir_text(5,1, line1);
    line=3;
    settextcolor(15);
    sprintf(s,"Old value = %d",genparm[parnum]);
    lir_text(1,line, s);
    line++;
    lir_text(1,line,"Enter new value for:");
    line++;
    k=genparm_max[parnum];
    m=genparm_min[parnum];
    if(parnum == FIRST_FFT_VERNR)
      {
      k=0;
      fft1mode=(ui.rx_input_mode&(TWO_CHANNELS+IQ_DATA))/2;
      while( fft1_version[fft1mode][k+1] > 0 &&
                                  k < MAX_FFT1_VERNR-1)k++;
      if(simd_present == 0)
        {
        while( fft_cntrl[fft1_version[fft1mode][k]].simd != 0)k--;
        }
      if(mmx_present == 0)
        {
        while( fft_cntrl[fft1_version[fft1mode][k]].mmx != 0)k--;
        }
      }
    if(parnum == FIRST_BCKFFT_VERNR)
      {
      k=0;
      while( fft1_back_version[ui.rx_rf_channels-1][k+1] > 0 &&
                                  k < MAX_FFT1_BCKVERNR-1)k++;
      if(mmx_present == 0)
        {
        while( fft_cntrl[fft1_back_version[ui.rx_rf_channels-1][k]].mmx != 0)k--;
        }
      }
    if(parnum == FIRST_BCKFFT_ATT_N)
      {
      k=(fft1_n-4)&0xfffe;
      }
    if(parnum == SECOND_FFT_ATT_N)
      {
      k=fft2_n-2;
      }
    if(parnum == SECOND_FFT_VERNR)
      {
      k=0;
      while( fft2_version[ui.rx_rf_channels-1][k+1] > 0 &&
                                  k < MAX_FFT2_VERNR-1)k++;
      if(mmx_present == 0)
        {
        while( fft_cntrl[fft2_version[ui.rx_rf_channels-1][k]].mmx != 0)k--;
        }
      }
    if(parnum == MIX1_BANDWIDTH_REDUCTION_N)
      {
      if(genparm[SECOND_FFT_ENABLE] == 0)
        {
        k=fft1_n-3;
        }
      else
        {
        k=fft2_n-3;
        }  
      }
    if(parnum == DA_OUTPUT_SPEED)
      {
      m=ui.rx_min_da_speed;
      k=ui.rx_max_da_speed;
      }
    if(parnum == MAX_NO_OF_SPURS)
      {
      if(genparm[AFC_ENABLE]==0)
        {
        k=0;
        }
      else
        {  
        if(genparm[SECOND_FFT_ENABLE] == 0)
          {
          k=2*fft1_size/SPUR_WIDTH;
          }
        else
          {
          k=2*fft2_size/SPUR_WIDTH;
          }
        }  
      }
    if(parnum == SPUR_TIMECONSTANT)
      {
      if(genparm[SECOND_FFT_ENABLE] == 0)
        {
        k=5*genparm[FFT1_STORAGE_TIME];
        }
      else
        {
        k=5*genparm[FFT2_STORAGE_TIME];
        }
      }
// In case fft sizes are not set. 
    if(k < m || m<genparm_min[parnum] || k>genparm_max[parnum])
      { 
      k=genparm_max[parnum];
      m=genparm_min[parnum];
      }
    sprintf(s," %s (%d to %d)",genparm_text[parnum], m,k);
    lir_text(1,line, s);
    i=line+1;
    line+=4;
    msg_filename="help.lir";
    write_from_msg_file(&line, 201+mouse_line+first, TRUE, HELP_VERNR,FALSE);
    lir_text(7,i,"=>");
    genparm[parnum]=lir_get_integer(10, i, 8, m,k);
    if(kill_all_flag) return;
    if(parnum == OUTPUT_MODE)check_output_no_of_channels();
    goto start;
    }
  }
test_keyboard();
if(kill_all_flag) return;
if(lir_inkey != 0)
  {
  process_current_lir_inkey();
  if(kill_all_flag) return;
  }
if(lir_inkey == F1_KEY)
  {
  mouse_line=mouse_y/text_height-3;
  if(mouse_line >= 0 && mouse_line <no)
    {
    help_message(201+mouse_line+first);
    }
  else
    {
    help_message(200);
    }
  if(kill_all_flag) return;
  goto start;
  }
if(lir_inkey != 10 && lir_inkey!= 'X')goto modloop;
loopx:;
}

void set_general_parms(char *mode)
{
char s[80];
sprintf(s,"%s: Rx channels=%d",mode,ui.rx_rf_channels);
if(lir_status < LIR_OK)goto bufreduce;
setfft1:;
if(kill_all_flag) return;
modify_parms(s, 0, SECOND_FFT_ENABLE);
if(kill_all_flag) return;
if(lir_inkey == 'X')return;
// Make sure fft1_n and fft2_n are defined and that we can
// allocate memory.
get_wideband_sizes();
if(kill_all_flag) return;
get_buffers(0);
if(kill_all_flag) return;
if(fft1_handle != NULL)fft1_handle=chk_free(fft1_handle);
if(lir_status != LIR_OK)goto bufreduce;
if(genparm[SECOND_FFT_ENABLE]==1)
  {
  modify_parms(s, FIRST_BCKFFT_VERNR, FFT2_STORAGE_TIME);
  if(kill_all_flag) return;
  if(lir_inkey == 'X')return;
  get_wideband_sizes();
  if(kill_all_flag) return;
  get_buffers(0);
  if(kill_all_flag) return;
  if(fft1_handle != NULL)fft1_handle=chk_free(fft1_handle);
  if(lir_status != LIR_OK)
    {
bufreduce:;
    clear_screen();
    settextcolor(15);
    switch (lir_status)
      {
      case LIR_FFT1ERR:
      lir_text(5,5,"Out of memory !!!");
      lir_text(10,10,"Storage times are set to minimum.");
      settextcolor(14);
      lir_text(10,13,"Check memory allocations in waterfall window");
      lir_text(10,14,"to decide how much you may increase storage times.");
      genparm[FFT1_STORAGE_TIME]=genparm_min[FFT1_STORAGE_TIME];
      genparm[FFT2_STORAGE_TIME]=genparm_min[FFT2_STORAGE_TIME];
      genparm[BASEBAND_STORAGE_TIME]=genparm_min[BASEBAND_STORAGE_TIME];
      lir_status=LIR_OK;
      break;
      
      case LIR_SPURERR:
      sprintf(s,"fft1 storage time too short for spur removal");
      if(genparm[SECOND_FFT_ENABLE] != 0)s[3]='2';
      lir_text(7,7,s);
      lir_text(7,8,"Spur removal disabled");
      genparm[MAX_NO_OF_SPURS]=0;
      lir_status=LIR_OK;
      break;

      case LIR_NEW_SETTINGS:
      goto setfft1;
      }      
    settextcolor(7);
    lir_text(10,17,"Press ESC to quit, any other key to continue");
    await_processed_keyboard();
    if(kill_all_flag) return;
    goto setfft1;
    }
  }
if(ui.newcomer_mode == 0)
  {
  modify_parms(s, AFC_ENABLE, AFC_ENABLE);
  if(kill_all_flag) return;
  if(lir_inkey == 'X')return;
  if(genparm[AFC_ENABLE] != 0)modify_parms(s, AFC_ENABLE+1, SPUR_TIMECONSTANT);
  if(kill_all_flag) return;
  if(lir_inkey == 'X')return;
  }
modify_parms(s, MIX1_BANDWIDTH_REDUCTION_N, MAX_GENPARM-1);
if(kill_all_flag) return;
if(lir_inkey == 'X')return;
clear_screen();
}

void cal_package(void)
{
char s[80], ss[80];
int single_run;
int i, ia, ib, ic;
float t1,t2;
calibrate_flag = 1;
// **************************************************************
// Set fft1_direction positive.
// The calibration routine does not want to know if the fft1 routine
// will invert the frequency scale.
fft1_direction=1; 
single_run=0;
// Get normal buffers with minimum for all parameters.
// Everything selectable becomes deselected.
// Save fft1 version, window and bandwidth.
ia=genparm[FIRST_FFT_VERNR];
ib=genparm[FIRST_FFT_SINPOW];
ic=genparm[FIRST_FFT_BANDWIDTH];
for(i=0; i<MAX_GENPARM; i++)genparm[i]=genparm_min[i];
// Select the correct fft version for the current rx_mode
// This is not really needed for versions above Linrad-01.xx
// because the approximate fft has been removed. None of the
// fft implementations contains a filter any more. 
genparm[FIRST_FFT_VERNR]=ia;
// Force fft1_size to be 4 times larger than specified for the current
// rx_mode by dividing bandwidth by 4.
t2=ic/4;
// Compensate for not using the specified window
// We use sin power 4 to suppress wideband noise that otherwise would
// be produced by discontinuities in matching between transform ends.
t1=pow(0.5,1.0/ib);
t2*=(1-2*asin(t1)/PI_L);
t1=pow(0.5,1.0/4);
t2/=(1-2*asin(t1)/PI_L);
genparm[FIRST_FFT_SINPOW]=4;
i=t2+0.5;  
if(i < genparm_min[FIRST_FFT_BANDWIDTH])i=genparm_min[FIRST_FFT_BANDWIDTH];
genparm[FIRST_FFT_BANDWIDTH]=i;
clear_screen();
get_wideband_sizes();
if(kill_all_flag) return;
get_buffers(1);
if(kill_all_flag) return;
if(lir_status != LIR_OK)return;
wg.first_xpoint=0;
wg.xpoints=fft1_size;
set_fft1_endpoints();
if(fft1afc_flag == 0)fft1afc_flag=-1;
init_memalloc(calmem, MAX_CAL_ARRAYS);
mem( 1,&cal_graph,2*MAX_ADCHAN*screen_width*sizeof(short int),0);
mem( 2,&cal_table,fft1_size*sizeof(COSIN_TABLE )/2,0);
mem( 3,&cal_permute,fft1_size*sizeof(short int),0);
mem(22,&fft2_tab,fft1_size*sizeof(COSIN_TABLE )/2,0);
mem(23,&fft2_permute,fft1_size*sizeof(short int),0);
mem( 4,&cal_win,(1+fft1_size/2)*sizeof(float),0);
mem( 5,&cal_tmp,twice_rxchan*fft1_size*sizeof(float),0);
mem( 6,&cal_buf,twice_rxchan*fft1_size*sizeof(float),0);
mem( 7,&bal_flag,(BAL_MAX_SEG+1)*sizeof(int),0);
mem( 8,&bal_pos,BAL_AVGNUM*(BAL_MAX_SEG+1)*ui.rx_rf_channels*sizeof(int),0);
mem( 9,&bal_phsum,BAL_AVGNUM*(BAL_MAX_SEG+1)*ui.rx_rf_channels*sizeof(float),0);
mem(10,&bal_amprat,BAL_AVGNUM*(BAL_MAX_SEG+1)*ui.rx_rf_channels*sizeof(float),0);
mem(11,&contracted_iq_foldcorr,8*(BAL_MAX_SEG+1)*ui.rx_rf_channels*
                                                            sizeof(float),0);
mem(12,&cal_buf2,twice_rxchan*fft1_size*sizeof(float),0);
mem(13,&cal_buf3,twice_rxchan*fft1_size*sizeof(float),0);
mem(14,&cal_buf4,twice_rxchan*fft1_size*sizeof(float),0);
mem(15,&cal_buf5,twice_rxchan*fft1_size*sizeof(float),0);
mem(16,&cal_buf6,twice_rxchan*fft1_size*sizeof(float),0);
mem(17,&cal_buf7,twice_rxchan*fft1_size*sizeof(float),0);
mem(47,&cal_fft1_filtercorr,twice_rxchan*fft1_size*sizeof(float),0);
mem(48,&cal_fft1_desired,fft1_size*sizeof(float),0);
i=memalloc((int*)(&calmem_handle),"calmem");
if(i==0)
  {
  lirerr(1188);
  return;
  }
if(ui.rx_rf_channels == 1)
  {
  cal_ymax=.25;
  cal_yzer=.65;
  }
else
  {
  cal_ymax=.15;
  cal_yzer=.5;
  }
init_fft(0,fft1_n, fft1_size, cal_table, cal_permute);
make_window(2,fft1_size/2, 4, cal_win);
iqbeg:;
single_run++;
if(single_run >=2)goto cal_skip;
fft1_pa=fft1_block;
fft1_na=1;
fft1_pb=0;
fft1_px=0;
fft1_nx=0;
timf1p_pa=rxad.block_bytes;
timf1p_px=0;
if(kill_all_flag) goto cal_skip;
cal_initscreen();
if( (ui.rx_input_mode&IQ_DATA) != 0)
  {
  if( (ui.rx_input_mode&DIGITAL_IQ) == 0)
    {
    lir_text(0, 5,"Running in IQ mode (direct conversion receiver)");
    lir_text(0, 6,"The I/Q phase and amplitude should be calibrated before");
    lir_text(0, 7,"the total amplitude and phase response is calibrated");
    lir_text(5, 9,"A=> Calibrate I/Q phase and amplitude.");
    }
  lir_text(5,10,"B=> Calibrate total amplitude and phase");
  lir_text(5,11,"C=> Remove center discontinuity");
  lir_text(5,12,"D=> Refine amplitude and phase correction");
  lir_text(5,13,"X=> Skip");
  lir_text(5,14,"F1=> Help");
get_kbd:;
  await_processed_keyboard();
  if(kill_all_flag) goto cal_skip;
  switch (lir_inkey)
    {
    case 'X':
    goto cal_skip;

    case 'A':
    if( (ui.rx_input_mode&DIGITAL_IQ) != 0)break;
    if( (fft1_calibrate_flag&CALAMP)==CALAMP)
      {
      clear_screen();
      lir_text(5,5,"The amplitudes are already calibrated.");
      make_iqcorr_filename(s);
      sprintf(ss,"Exit from Linrad and remove the file %s",s);
      lir_text(1,6,ss);
      lir_text(5,8,press_any_key);
      await_keyboard();
      break;
      }
    usercontrol_mode=USR_IQ_BALANCE;
    init_semaphores();
    ampinfo_flag=1;
    linrad_thread_create(rx_input_thread);
    if(kill_all_flag) goto cal_skip_freesem;
    linrad_thread_create(THREAD_USER_COMMAND);
    if(kill_all_flag) goto iqbal;
    linrad_thread_create(THREAD_CAL_IQBALANCE);
    if(kill_all_flag) goto iqbal;
    linrad_thread_create(THREAD_WIDEBAND_DSP);
iqbal:;    
    lir_sleep(50000);
    lir_refresh_screen();
    lir_join(THREAD_USER_COMMAND);
    linrad_thread_stop_and_join(rx_input_thread);
    linrad_thread_stop_and_join(THREAD_CAL_IQBALANCE);
    linrad_thread_stop_and_join(THREAD_WIDEBAND_DSP);
    free_semaphores();
    if(kill_all_flag) goto cal_skip;
    break;

    case 'B':
    goto pulsecal;

    case 'C':
    if(remove_iq_notch() == 0)goto cal_skip;
    break;

    case 'D':
    final_filtercorr_init();
    goto cal_skip;

    case F1_KEY:
    help_message(302);
    break;

    default:
    goto get_kbd;
    }
  }    
else
  {
  lir_text(5, 9,"A=> Calibrate frequency response");
  lir_text(5,10,"B=> Refine amplitude and phase correction");
  lir_text(5,11,"X=> Skip");
  lir_text(5,12,"F1=> Help");
get_valid_keypress:;
  await_processed_keyboard();
  if(kill_all_flag) goto cal_skip;
  switch (lir_inkey)
    {
    case 'X':
    goto cal_skip;
        
    case 'A':
    goto pulsecal;

    case 'B':
    final_filtercorr_init();
    goto cal_skip;

    case F1_KEY:
    help_message(303);
    break;

    default:
    goto get_valid_keypress;
    }
  }  
goto iqbeg;
pulsecal:;
timf1p_pa=0;
usercontrol_mode=USR_CAL_INTERVAL;
init_semaphores();
ampinfo_flag=1;
linrad_thread_create(rx_input_thread);
if(kill_all_flag) goto cal_skip_freesem;
linrad_thread_create(THREAD_USER_COMMAND);
if(kill_all_flag) goto calint;
linrad_thread_create(THREAD_CAL_INTERVAL);
calint:;
lir_sleep(50000);
lir_refresh_screen();
lir_join(THREAD_USER_COMMAND);
linrad_thread_stop_and_join(rx_input_thread);
linrad_thread_stop_and_join(THREAD_CAL_INTERVAL);
free_semaphores();
if(kill_all_flag)goto cal_skip;
clear_screen();
lir_refresh_screen();
if(usercontrol_mode != USR_CAL_FILTERCORR)goto iqbeg;
if( (ui.rx_input_mode&IQ_DATA) == 0)cal_interval/=2;
#define ERRLINE 5
if(cal_interval > fft1_size/5)
  {
  lir_text(5,ERRLINE,"Pulse repetition frequency too low.");
  lir_text(5,ERRLINE+1,"Increase PRF or set fft1 bandwidth lower to");
  lir_text(5,ERRLINE+2,"increase transform size.");
err1:;
  lir_text(5,ERRLINE+5,press_any_key);
  await_keyboard();
  goto iqbeg;
  }  
if(cal_signal_level > 0.9)
  {
  lir_text(5,ERRLINE,"Pulses too strong (above 90%)");
  goto err1;
  }
timf1p_pa=0;
timf1p_px=0;
init_semaphores();
linrad_thread_create(rx_input_thread);
if(kill_all_flag) goto cal_skip_freesem;
linrad_thread_create(THREAD_WIDEBAND_DSP);
if(kill_all_flag) goto calfil;
linrad_thread_create(THREAD_CAL_FILTERCORR);
if(kill_all_flag) goto calfil;
linrad_thread_create(THREAD_USER_COMMAND);
calfil:;
lir_sleep(50000);
lir_refresh_screen();
lir_join(THREAD_USER_COMMAND);
linrad_thread_stop_and_join(rx_input_thread);
linrad_thread_stop_and_join(THREAD_CAL_FILTERCORR);
linrad_thread_stop_and_join(THREAD_WIDEBAND_DSP);
free_semaphores();
if(kill_all_flag)goto cal_skip;
goto iqbeg;
// ********************************
cal_skip_freesem:;
free_semaphores();
cal_skip:;
memcheck(199,calmem,(int*)&calmem_handle);
calmem_handle=chk_free(calmem_handle);
free_buffers();
}

void restore_uiparm(void)
{
int i;
int *uiparm;
uiparm=(void*)(&ui);
for(i=0; i<MAX_UIPARM; i++)uiparm[i]=uiparm_save[i];
}

void save_uiparm(void)
{
int i;
int *uiparm;
uiparm=(void*)(&ui);
for(i=0; i<MAX_UIPARM; i++)uiparm_save[i]=uiparm[i];
}

void init_genparm(int upd)
{
char s[256],ss[80];
FILE *file;
int  i, j, k, kmax;
int parwrite_flag, parfile_flag;
float t1;
int *uiparm;
char *parinfo;
parinfo=NULL;
uiparm=(void*)(&ui);
parwrite_flag=0;
if(upd == TRUE) goto updparm;
file=NULL;
// Start by reading general parameters for the selected rx mode.
// If no parameters found, go to the parameter select routine because
// we can not guess what hardware will be in use so default parameters
// are likely to be very far from what will be needed.
parfile_flag=0;
if(savefile_parname[0] != 0)
  {
// We use disk input and there is a parameter file name supplied
// for this particular recording. 
  file = fopen(savefile_parname, "r");
  if (file == NULL)goto saverr;
  parfilnam=savefile_parname;
  parfile_flag=1;
// First read the mode flag, an extra line in the userint par file
// placed above the normal lines.
// This line specifies receive mode and whether the file should be
// repeated endlessly
// Stereo .wav files have more flags. Whether it is two RF channels
// or I and Q from direct conversion. In the latter case what 
// direction to use for the frequency scale.
  i=fread(s,1,1,file);
  if(i != 1)goto saverr_close;
  rx_mode=toupper(s[0])-'A';  
  if(rx_mode < 0 || rx_mode >= MAX_RX_MODE)goto saverr_close;
  i=fread(s,1,1,file);
  if(i != 1)goto saverr_close;
  if(s[0]=='1')
    {
    savefile_repeat_flag=1;
    }
  else
    {
    if(s[0]!='0')goto saverr_close;
    savefile_repeat_flag=0;
    }
  if(wav_read_flag==0)goto skip_modlin;
  i=fread(s,1,1,file);
  if(i != 1)goto saverr_close;
  if(s[0]=='1')
    {
    iqflag=1;
    ui.rx_rf_channels=ui.rx_ad_channels/2;
    i=fread(s,1,1,file);
    if(i != 1)goto saverr_close;
    if(s[0]=='1')
      {
      dirflag=1;
      }
    else
      {
      if(s[0]!='0')goto saverr_close;
      dirflag=0;
      }
    }
  else
    {
    if(s[0]!='0')goto saverr_close;
    iqflag=0;
    ui.rx_rf_channels=ui.rx_ad_channels;
    }
skip_modlin:; 
// We use disk input and something was wrong with the first line.
// Read until end of line, then ask the user for the desired mode.
  i=fread(s,1,1,file);
  if(s[0]=='\n') goto fileok;
  if(i==1)goto skip_modlin;
saverr_close:;
  fclose(file);
saverr:;
  parwrite_flag=1;
  }
if(diskread_flag == 2)
  {
  clear_screen();
  for(i=0; i<MAX_RX_MODE; i++)
    {
    sprintf(s,"%c=%s",i+'A',rxmodes[i]);
    lir_text(0, i+2,s);
    }
  settextcolor(15);
  lir_text(5, MAX_RX_MODE+3, "Select Rx mode");
  rx_mode=-1;
  while(rx_mode < 0 || rx_mode > MAX_RX_MODE)
    {
    toupper_await_keyboard();
    if(kill_all_flag) return;
    rx_mode=lir_inkey-'A';
    }
  clear_screen();
  lir_text(5,5,"Repeat recording endlessly (Y/N)?");          
savrpt:;
  toupper_await_keyboard();
  if(kill_all_flag) return;
  if(lir_inkey=='Y')
    {
    savefile_repeat_flag=1;
    }
  else
    {
    if(lir_inkey != 'N')goto savrpt;  
    savefile_repeat_flag=0;
    }
  if( wav_read_flag != 0)
    {
    if(ui.rx_ad_channels == 2)
      {
      lir_text(5,6,"Stereo recording. Interpret as I/Q data (Y/N) ?");          
wawiq:;
      toupper_await_keyboard();
      if(kill_all_flag) return;
      if(lir_inkey=='Y')
        {
        iqflag=1;
        lir_text(5,7,"Invert frequency scale (Y/N) ?");
diriq:;
        toupper_await_keyboard();
        if(kill_all_flag) return;
        if(lir_inkey=='Y')
          {
          dirflag=1;
          }
        else
          {
          if(lir_inkey != 'N')goto diriq;  
          dirflag=0;
          }
        }
      else
        {
        if(lir_inkey != 'N')goto wawiq;  
        iqflag=0;
        }
      }
    }
  if(savefile_parname[0]!=0)
    {
    file = fopen( savefile_parname, "r");
    if(file == NULL) parwrite_flag=1;
    }              
  settextcolor(7);
  }
if( wav_read_flag != 0)
    {
    if(iqflag==1)
      {
      ui.rx_rf_channels=1;
      ui.rx_input_mode|=IQ_DATA;
      }
    else
      {
      ui.rx_input_mode|=TWO_CHANNELS;
      ui.rx_rf_channels=2;
      }
    if(dirflag == 1)
      {
      fg.passband_direction=-1;
      }
    else
      {
      fg.passband_direction=1;
      }
    fft1_direction=fg.passband_direction;
    }
if(parfile_flag == 0)parfilnam=rxpar_filenames[rx_mode];
file = fopen(rxpar_filenames[rx_mode], "r");
if(file == NULL)
  {
  sprintf(s,"%s file missing",rxpar_filenames[rx_mode]);
  prompt_reason(s);
iniparm:;
  if(kill_all_flag) return;
  for(i=0; i<MAX_GENPARM; i++)
    {
    genparm[i]=genparm_default[rx_mode][i];
    }
// The default mix1 bandwidth reduction will be fine when
// the sampling rate is 96 kHz I/Q.
  t1=96000;
  if( (ui.rx_input_mode&IQ_DATA) == 0)t1*=2;
  t1=ui.rx_ad_speed/t1;
  if(t1 > 1)
    {
    while(t1>1.5)
      {
      t1*=0.5;
      genparm[MIX1_BANDWIDTH_REDUCTION_N]++;
      }
    }
  if(t1 < 1)
    {
    while(t1<0.7)
      {
      t1*=2;
      genparm[MIX1_BANDWIDTH_REDUCTION_N]--;
      }
    genparm[MIX1_BANDWIDTH_REDUCTION_N]-=make_power_of_two(&i);
    if(genparm[MIX1_BANDWIDTH_REDUCTION_N] < 1)genparm[MIX1_BANDWIDTH_REDUCTION_N]=1;
    }
  genparm[MAX_GENPARM]=ui.rx_ad_speed;
updparm:;
  if(diskread_flag == 4)
    {
    if(init_diskread(-1) != 0)
      {
      lirerr(3641);
      return;
      }
    }
  set_general_parms(rxmodes[rx_mode]);
  if(kill_all_flag)return;
  genparm[MAX_GENPARM+1]=GENPARM_VERNR;
  for(i=0; i<MAX_UIPARM; i++)uiparm_save[i]=uiparm[i];
  if(savefile_parname[0]!=0)
    {
write_savefile_parms:;    
    parwrite_flag=0;
    parfilnam=savefile_parname;
    file = fopen(parfilnam, "w");
    if(file == NULL)goto wrerr;
    fprintf(file,"%c%c",rx_mode+'A',savefile_repeat_flag+'0');
    if( wav_read_flag != 0)
      {
      fprintf(file,"%c%c", iqflag+'0', dirflag+'0');
      }
    fprintf(file,"\n");
    goto wrok;   
    }
  file = fopen(parfilnam, "w");
  if(file == NULL)
    {
wrerr:;  
    clear_screen();
    could_not_create(parfilnam,0);
    return;
    }
wrok:;
  for(i=0; i<MAX_GENPARM+2; i++)
    {
    fprintf(file,"%s [%d]\n",genparm_text[i],genparm[i]);
    }
  parfile_end(file);
  file=NULL;
  }
else
  {
  parfilnam=rxpar_filenames[rx_mode];
fileok:;
  if( wav_read_flag != 0)
    {
    if(iqflag==1)
      {
      ui.rx_rf_channels=1;
      ui.rx_input_mode|=IQ_DATA;
      }
    else
      {
      ui.rx_input_mode|=TWO_CHANNELS;
      }
    if(dirflag == 1)
      {
      fg.passband_direction=-1;
      }
    else
      {
      fg.passband_direction=1;
      }
    fft1_direction=fg.passband_direction;
    }
  parinfo=malloc(4096);
  if(parinfo == NULL)
    {
    fclose(file);
    lirerr(1081);
    return;
    }
  for(i=0; i<4096; i++)parinfo[i]=0;
  kmax=fread(parinfo,1,4095,file);
  fclose(file);
  file=NULL;
  if(kmax >= 4095)
    {
    sprintf(ss,"Excessive file size");
daterr:;    
    sprintf(s,"%s Data error in file %s",ss,parfilnam);
    prompt_reason(s);
    goto iniparm;
    }
  k=0;
  for(i=0; i<MAX_GENPARM+2; i++)
    {
    while(k <= kmax && (parinfo[k]==' ' || parinfo[k]== '\n' ))k++;
    if(k > kmax)
      {
      sprintf(ss,"Premature end of file");
daterr_free:;
      parinfo=chk_free(parinfo);
      goto daterr;
      }
    j=0;
    while(parinfo[k]== genparm_text[i][j])
      {
      k++;
      j++;
      } 
    sprintf(ss,"Error: %s  ",genparm_text[i]);
    if(genparm_text[i][j] != 0 || k > kmax)
      {
      goto daterr_free;
      }
    while(k <= kmax && parinfo[k]!='[')k++;
    if(k > kmax)
      {
      goto daterr_free;
      }
    sscanf(&parinfo[k],"[%d]",&genparm[i]);
    while(k <= kmax && parinfo[k]!='\n')k++;
    if(k > kmax)
      {
      goto daterr_free;
      }
    }
  parinfo=chk_free(parinfo);
  if(diskread_flag == 4)
    {
    if(init_diskread(-1) != 0)
      {
      lirerr(3641);
      free(parinfo);
      return;
      }
    }
  else
    {    
    if( fabs( (float)(genparm[MAX_GENPARM] - ui.rx_ad_speed)/
             (genparm[MAX_GENPARM] + ui.rx_ad_speed)) > 0.0002)
      {
      sprintf(s,"Input sampling speed changed %d  (old=%d)",
                                       ui.rx_ad_speed, genparm[MAX_GENPARM]);
      prompt_reason(s);
      goto iniparm;
      }                                 
    }
  if(genparm[MAX_GENPARM+1] != (int)(GENPARM_VERNR))
    {
    prompt_reason("GENPARM version changed");
    goto iniparm;
    }
  if(rx_mode!=MODE_WCW && rx_mode!=MODE_NCW && rx_mode!=MODE_HSMS &&
                                         genparm[CW_DECODE_ENABLE] != 0)
    {
    prompt_reason("Mode not compatible with morse decode");
    goto iniparm;
    }
  for(i=0; i<MAX_GENPARM; i++)
    {
    if(genparm[i] < genparm_min[i] || genparm[i] > genparm_max[i])
      {
illegal_value:;
      sprintf(s,"Illegal value for %s: %d (%d to %d)",genparm_text[i],
                                   genparm[i],genparm_min[i], genparm_max[i]);
      prompt_reason(s);
      goto iniparm;
      }
    }
  fft1mode=(ui.rx_input_mode&(TWO_CHANNELS+IQ_DATA))/2;
  i=fft1_version[fft1mode][genparm[FIRST_FFT_VERNR]];
  if( i < 0 || i>=MAX_FFT_VERSIONS)
    {
    prompt_reason("FFT1 version incompatible with A/D mode");
    goto iniparm;
    }  
  if(simd_present == 0)
    {
    if( fft_cntrl[i].simd != 0)
      {
      prompt_reason("Parameters say use SIMD - not supported by computer!");
      goto iniparm;
      }
    }
  if(genparm[SECOND_FFT_ENABLE] != 0)
    {  
    if(mmx_present == 0)
      {
      if( fft_cntrl[i].mmx != 0)
        {
nommx:;      
        prompt_reason("Parameters say use MMX - not supported by computer!");
        goto iniparm;
        }
      }
    i=fft1_back_version[ui.rx_rf_channels-1][genparm[FIRST_BCKFFT_VERNR]];
    if( i < 0 || i>=MAX_FFT_VERSIONS)
      {
      prompt_reason("Backwards FFT1 version incompatible with no of channels");
      goto iniparm;
      }
    if(mmx_present == 0)
      {
      if( fft_cntrl[i].mmx != 0)goto nommx;
      }
    i=fft2_version[ui.rx_rf_channels-1][genparm[SECOND_FFT_VERNR]];
    if( i < 0 || i>=MAX_FFT_VERSIONS)
      {
      prompt_reason("FFT2 version incompatible with no of channels");
      goto iniparm;
      }
    if(mmx_present == 0)
      {
      if( fft_cntrl[i].mmx != 0)goto nommx;
      }
// Make sure fft1_n and fft2_n are defined.
    get_wideband_sizes();
    if(kill_all_flag) return;
    k=(fft1_n-4)&0xfffe;
    if(genparm[FIRST_BCKFFT_ATT_N]>k)
      {
      i=FIRST_BCKFFT_ATT_N;
      goto illegal_value;
      }
    k=fft2_n-2;
    if(genparm[SECOND_FFT_ATT_N]>k)
      {
      i=SECOND_FFT_ATT_N;
      goto illegal_value;
      }
    }
  if(genparm[DA_OUTPUT_SPEED] > ui.rx_max_da_speed||
               genparm[DA_OUTPUT_SPEED] < ui.rx_min_da_speed)
    {
    sprintf(s,"Output sampling speed out of range %d  (%d to %d)",
                              genparm[DA_OUTPUT_SPEED],ui.rx_min_da_speed,
                                                          ui.rx_max_da_speed);
    prompt_reason(s);
    goto iniparm;
    }
  check_output_no_of_channels();
  }
if( wav_read_flag != 0)
  {
  if(iqflag==1)
    {
    ui.rx_rf_channels=1;
    ui.rx_input_mode|=IQ_DATA;
    }
  else
    {
    ui.rx_input_mode|=TWO_CHANNELS;
    }
  if(dirflag == 1)
    {
    fg.passband_direction=-1;
    }
  else
    {
    fg.passband_direction=1;
    }
  fft1_direction=fg.passband_direction;
  }
if(genparm[AFC_ENABLE] == 0)
  {
  genparm[CW_DECODE_ENABLE]=0;
  }
if(parwrite_flag!=0)goto write_savefile_parms;    
}

void main_menu(void)
{
int local_workload_reset;
int first_txproc_no, use_tx;
int wlcnt, wlcnt_max, i, k, m;
double total_time1, total_time2;
double cpu_time1, cpu_time2;
int uiupd, line;
char s[256],ss[80],st[20];
char *sdrnam, *netin;
int ctlc_fix;
int x[4096];
int message_line;
FILE *file;
int *uiparm;
float t1, t2;
uiparm=(void*)(&ui);
eme_flag=0;
read_eme_database();
if(kill_all_flag) goto menu_x;
// Save the ui parameters. 
// The user has set up the sound cards for normal operation
// with his hardware but some routines, e.g. txtest may change
// the A/D parameters and/or other parameters.
// We will always start here with the initial ui parameters.
save_uiparm();
ctlc_fix=3;
menu_loop:;
lir_set_title("");
restore_uiparm();
clear_screen();
settextcolor(12);
switch(ui.rx_addev_no)
  {
  case SDR14_DEVICE_CODE:
  sdrnam=sdr14_name_string;
  break;
  
  case SDRIQ_DEVICE_CODE:
  sdrnam=sdriq_name_string;
  break;
  
  case PERSEUS_DEVICE_CODE:
  strncpy(st,perseus_name_string,7);
  st[7]=0;
  sdrnam=st;
  break;
  
  default:
  sdrnam=" ";
  break;
  }    
if((ui.network_flag&NET_RX_INPUT) != 0)
  {
  switch (ui.network_flag & NET_RX_INPUT)
    {
    case NET_RXIN_RAW16:
    netin="RAW16";
    break;

    case NET_RXIN_RAW18:
    netin="RAW18";
    break;
    
    case NET_RXIN_RAW24:
    netin="RAW24";
    break;

    case NET_RXIN_FFT1:
    netin="FFT1";
    break;

    case NET_RXIN_BASEB:
    netin="baseband";
    break;

    default:
    netin="ERROR";
    break;
    }
  sdrnam=" ";
  sprintf(ss,"NETWORK RX INPUT %s",netin);
  }
else
  {
  if(allow_parport)
    {
    sprintf(ss,"Parport %d:%d",ui.parport,ui.parport_pin);
    }
  else
    {
    ss[0]=0;
    }
  }
sprintf(s,"%s   %s    %s",PROGRAM_NAME, ss, sdrnam);    
lir_text(14,0,s);
if((ui.network_flag&NET_RX_OUTPUT) != 0)lir_text(0,0,"NETSEND");
lir_refresh_screen();
if(kill_all_flag)goto menu_x;
line=2;
if(ui.newcomer_mode != 0)
  {
  settextcolor(14);
  lir_text(14,1,"newcomer mode");
  line++;
  }
message_line=line;  
settextcolor(7);
button_color=7;
for(i=0; i<MAX_RX_MODE; i++)
  {
  if(ui.newcomer_mode == 0 || newcomer_rx_modes[i] != 0)
    {
    sprintf(s,"%c=%s",i+'A',rxmodes[i]);
    lir_text(0, line, s);
    line++;
    }
  }
if((ui.network_flag&NET_RX_INPUT) == 0)
  {
  lir_text(30,message_line,"1=Process first file named in 'adfile'");
  message_line++;
  lir_text(30,message_line,"2=Process first file named in 'adwav'");
  message_line++;
  lir_text(30,message_line,"3=Select file from 'adfile'");
  message_line++;
  lir_text(30,message_line,"4=Select file from 'adwav'");
  message_line++;
  if(ui.newcomer_mode == 0)
    {
    lir_text(30,message_line,"5=File converter .raw to .wav");
    message_line++;
    }
  }
if(ui.newcomer_mode == 0)
  {
  lir_text(30,message_line,"R=Toggle network input");
  message_line++;
  lir_text(30,message_line,"T=Toggle network output");
  message_line++;
  }
if(message_line > line)line=message_line;  
line++;  
settextcolor(3);
if(ui.newcomer_mode == 0)
  {
  lir_text(0, line  , "M=Init moon tracking and EME database");
  line++;
  lir_text(0, line, "N=Network set up");
  line++;
  }
lir_text(0, line, "S=Global parms set up");
line++;
lir_text(0, line, "U=A/D and D/A set up for RX");
line++;
if(ui.newcomer_mode == 0)
  {
  lir_text(0, line, "V=TX mode set up");
  line++;
  }
lir_text(0, line, "W=Save current parameters in par_userint");
line++;
if(ui.newcomer_mode == 0 && (ui.network_flag&NET_RX_INPUT) == 0)
  {
  lir_text(0, line, "Z=Hardware interface test");
  line++;
  }
lir_text(0, line, "F1=Show keyboard commands");
line++;
if(ui.newcomer_mode == 0)
  {
  lir_text(0, line, "F9=Emergency light");
  line++;
  }
lir_text(0, line, "ESC=Quit program");
if(kill_all_flag)goto menu_x;
message_line=line+1;
if(uiparm_change_flag==TRUE)
  {
  settextcolor(15);
  lir_text(5,message_line+1,"PARAMETERS NOT SAVED Press ""W"" to save");
  }
settextcolor(7);
if(ctlc_fix>0)
  {
// We have to write several times to the screen in order to
// not destroy the terminals Alt F1, Alt F2, Alt F3.... when entering
// the very first time.
// The videocard settings become corrupted if a ctrl C is pressed
// before we wrote enough to the screen. The reason is unknown
// but this fix helps svgalib-1.9.21 as well as svgalib-1.9.24
  ctlc_fix--;
  goto menu_loop;
  }
if(kill_all_flag) goto menu_x;
toupper_await_keyboard();
if(kill_all_flag) goto menu_x;
menu_loop_b:;
savefile_parname[0]=0;
calibrate_flag=0;
diskread_flag=0;
diskwrite_flag=FALSE;
freq_from_file=FALSE;
iqflag=0;
dirflag=0;
rx_input_thread=THREAD_RX_ADINPUT;
if(ui.rx_addev_no == SDR14_DEVICE_CODE ||
   ui.rx_addev_no == SDRIQ_DEVICE_CODE )rx_input_thread=THREAD_SDR14_INPUT;
if(ui.rx_addev_no == PERSEUS_DEVICE_CODE)rx_input_thread=THREAD_PERSEUS_INPUT;
t1=t2=0;
x[0]=0;
parfilnam=NULL;
lir_status=LIR_OK;
wg_freq_x1=-1;
switch ( lir_inkey )
  {
  case 'R':
  if(ui.newcomer_mode == 0)
    {
    manage_network(NET_RX_INPUT);
    save_uiparm();
    uiparm_change_flag=TRUE;
    }
  break;
  
  case 'T':
  if(ui.newcomer_mode == 0)
    {
    manage_network(NET_RX_OUTPUT);
    save_uiparm();
    uiparm_change_flag=TRUE;
    }
  break;
  
  case 'M':
  if(ui.newcomer_mode == 0)
    {
    init_eme_database();
    if(kill_all_flag) goto menu_x;
    }
  break;

  case F1_KEY:
  help_message(1);
  if(kill_all_flag) goto menu_x;
  break;

  case F9_KEY:
  if(ui.newcomer_mode == 0)
    {
    lir_fillbox(0,0,screen_width,screen_height,15);
    await_keyboard();
    }
  break;

  case '1':
  case '3':
  if((ui.network_flag&NET_RX_INPUT) == 0)
    {
    if(ui.rx_dadev_no == -1)goto setad;
    wav_read_flag=0;
    rx_input_thread=THREAD_RX_FILE_INPUT;
    if(init_diskread(lir_inkey-'1') == 0) goto do_pc_radio;
    }
  break;

  case '2':
  case '4':
  if((ui.network_flag&NET_RX_INPUT) == 0)
    {
    if(ui.rx_dadev_no == -1)goto setad;
    wav_read_flag=1;
    rx_input_thread=THREAD_RX_FILE_INPUT;
    if(init_diskread(lir_inkey-'2') == 0)
      {
      goto do_pc_radio;
      }
    }
  break;

  case '5':
  if(ui.newcomer_mode == 0)
    {
    wav_read_flag=0;
    raw2wav();
    }
  break;

  case 'U':
setad:;
  set_rx_io();
  if(kill_all_flag) goto menu_x;
  save_uiparm();
  uiparm_change_flag=TRUE;
  break; 

  case 'V':
  if(ui.newcomer_mode == 0)
    {
    i=tx_setup();
    if(kill_all_flag) goto menu_x;
    save_uiparm();
    uiparm_change_flag=TRUE;
    }
  break; 

  case 'Z':
  if(ui.newcomer_mode == 0)
    {
    hware_interface_test();
    if(kill_all_flag) goto menu_x;
    }
  break;
    
  case 'N':
  if(ui.newcomer_mode == 0)
    {
    manage_network(0);
    if(kill_all_flag) goto menu_x;
    save_uiparm();
    uiparm_change_flag=TRUE;
    }
  break;

  case 'S':
  switch (os_flag)
    {
    case OS_FLAG_LINUX:
    lin_global_uiparms(1);
    break;
      
    case OS_FLAG_WINDOWS:
    win_global_uiparms(1);
    break;
    
    case OS_FLAG_X:
    x_global_uiparms(1);
    break;
    }
  if(kill_all_flag) goto menu_x;
  save_uiparm();
  uiparm_change_flag=TRUE;
  break;

  case 'W':
  file = fopen(userint_filename, "w");
  if (file == NULL)
    {
    lirerr(1029);
    goto menu_x;
    }
  ui.check=UI_VERNR;  
  for(i=0; i<MAX_UIPARM; i++)
    {
    fprintf(file,"%s [%d]\n",uiparm_text[i],uiparm[i]);
    }
  parfile_end(file);
  settextcolor(15);
  clear_lines(message_line+1,message_line+1);
  lir_text(0, message_line, "User interface setup saved");
  for(i=0; i<MAX_UIPARM-1; i++)
    {
    sprintf(s,"%s [%d] ",uiparm_text[i],uiparm[i]);
    if( (message_line+i) <= screen_last_line)lir_text(29,message_line+i, s);
    }
  settextcolor(7);
  await_processed_keyboard();
  if(kill_all_flag) goto menu_x;
  uiparm_change_flag=FALSE;
  goto menu_loop_b;

  default:
  rx_mode=(lir_inkey-'A');
  if( rx_mode < 0 || rx_mode>=MAX_RX_MODE)goto menu_loop;
  if(ui.newcomer_mode != 0 && newcomer_rx_modes[rx_mode]==0)goto menu_loop;
  if( (ui.network_flag&NET_RX_INPUT)==0 && ui.rx_input_mode < 0)goto setad;
  if(ui.rx_max_da_channels==0)goto setad;
    {
do_pc_radio:;
    calibrate_flag=0;
    parfilnam=NULL;
    open_mouse();
    uiupd=FALSE;
    if((ui.network_flag&NET_RX_INPUT) == 0)
      {
updparm:;
      init_genparm(uiupd);
      if(kill_all_flag) goto menu_x;
      }
    clear_screen();
    settextcolor(7);
#if (USERS_EXTRA_PRESENT == 1)
    init_users_extra();
#endif
    lir_status=LIR_OK;
    fft1_waterfall_flag=0;
    all_threads_started=FALSE; 
    cg.oscill_on=0;
    first_txproc_no=tg.spproc_no;
    use_tx=ui.tx_enable;
    if(diskread_flag >= 2 ||
       ui.tx_addev_no == -1 ||
       ui.tx_dadev_no == -1)use_tx=0;
    if(read_txpar_file()==FALSE)use_tx=0;
    switch (rx_mode)
      {
      case MODE_WCW:
      case MODE_NCW:
      case MODE_HSMS:
      case MODE_SSB:
      case MODE_QRSS:
      use_bfo=1;
      goto do_normal_rx;

      case MODE_FM:
      case MODE_AM:
      use_bfo=0;
do_normal_rx:;
      lir_set_title(rxmodes[rx_mode]);
      workload_counter=0;
      workload=-1;
      usercontrol_mode=USR_NORMAL_RX;
#if RUSAGE_OLD != TRUE
      for(i=0; i<THREAD_MAX; i++)thread_pid[i]=0;
#endif
      if(ui.network_flag!=0)
        {
        init_network();
        if(kill_all_flag) goto menu_x;
        if( (ui.network_flag & (NET_RXOUT_FFT2+NET_RXOUT_TIMF2)) != 0 &&
                                            genparm[SECOND_FFT_ENABLE] == 0)
          {
          lirerr(1281);
          goto menu_x;
          }
        }
      else
        {
        get_wideband_sizes();
        if(kill_all_flag) goto menu_x;
        get_buffers(1);
        netfd.rec_rx=INVSOCK;
        }
      if(kill_all_flag) goto menu_x;
      if(lir_status != LIR_OK)goto rx_modes_exit;
      if(!freq_from_file)
        {
        read_freq_control_data();
        }
      check_filtercorr_direction();
      init_wide_graph();
      if( genparm[SECOND_FFT_ENABLE] != 0 )init_blanker();
      if(kill_all_flag) goto menu_x;
      if(lir_status != LIR_OK)goto rx_modes_exit;
      if(genparm[SECOND_FFT_ENABLE] != 0)
        {
        init_hires_graph();
        if(kill_all_flag) goto menu_x;
        if(lir_status != LIR_OK)goto rx_modes_exit;
        }
      if(genparm[AFC_ENABLE] != 0)
        {
        init_afc_graph();
        if(kill_all_flag) goto menu_x;
        if(lir_status != LIR_OK) goto rx_modes_exit;
        }
      if(ui.rx_rf_channels == 2)
        {
        init_pol_graph();
        if(kill_all_flag) goto menu_x;
        if(lir_status != LIR_OK) goto rx_modes_exit;
        }
      init_baseband_graph();
      if(kill_all_flag) goto menu_x;
      if(lir_status != LIR_OK) goto rx_modes_exit;
      init_coherent_graph();
      if(kill_all_flag) goto menu_x;
      if(lir_status != LIR_OK) goto rx_modes_exit;
      if(ui.newcomer_mode == 0 && eme_flag != 0)
        {
        init_eme_graph();
        if(kill_all_flag) goto menu_x;
        if(lir_status != LIR_OK) goto rx_modes_exit;
        }
      if(!freq_from_file)
        {
        init_freq_control();
        if(kill_all_flag) goto menu_x;
        if(lir_status != LIR_OK) goto rx_modes_exit;
        }
      if(use_tx != 0)
        {
// Get the xxproc file that the user stored in tg.spproc_no 
        init_tx_graph();
        if(read_txpar_file()==FALSE)
          {
// If it failed, read the one that was succesful before.          
          tg.spproc_no=first_txproc_no;
          read_txpar_file();
          }  
        }
// Make sure users_init_mode is the last window(s) we open.
// the avoid collission routine does not know where
// users windows are placed.
      users_init_mode();
      if(kill_all_flag) goto menu_x;
      if(lir_status != LIR_OK) goto rx_modes_exit;
      show_name_and_size();
      init_semaphores();
      linrad_thread_create(THREAD_RX_OUTPUT);
      if(kill_all_flag) goto normal_rx;
      linrad_thread_create(THREAD_USER_COMMAND);
      if(kill_all_flag) goto normal_rx;
      linrad_thread_create(THREAD_NARROWBAND_DSP);
      if(kill_all_flag) goto normal_rx;
      linrad_thread_create(THREAD_WIDEBAND_DSP);
      if(kill_all_flag) goto normal_rx;
      linrad_thread_create(THREAD_SCREEN);
      if(kill_all_flag) goto normal_rx;
      if((ui.network_flag&NET_RX_OUTPUT)!=0)
        {
        linrad_thread_create(THREAD_LIR_SERVER);
        if(kill_all_flag) goto normal_rx;
        }
      // Make sure that all the threads are running before
      // opening inputs in order to avoid overrun errors.
      // Some system calls they might do could cause overrun
      // errors if the input were open.
      while(thread_status_flag[THREAD_RX_OUTPUT]!=THRFLAG_IDLE &&
            thread_status_flag[THREAD_USER_COMMAND]!=THRFLAG_ACTIVE &&
            thread_status_flag[THREAD_NARROWBAND_DSP]!=THRFLAG_SEM_WAIT &&
            thread_status_flag[THREAD_WIDEBAND_DSP]!=THRFLAG_ACTIVE )

        {
        if(kill_all_flag) goto normal_rx;
        lir_sleep(3000);
        }
      if(kill_all_flag) goto normal_rx;
      if(use_tx != 0 )
        {
        switch (rx_mode)
          {
          case MODE_SSB:
          init_txmem_spproc();
          break;
          
          case MODE_WCW:
          case MODE_NCW:
          case MODE_HSMS:
          case MODE_QRSS:
          init_txmem_cwproc();
          break;

          case MODE_RADAR:
          init_txmem_cwproc();
          break;
          }
        if(kill_all_flag)goto normal_rx;
        lir_mutex_init();
        linrad_thread_create(THREAD_TX_OUTPUT);
        while(thread_status_flag[THREAD_TX_OUTPUT]!=THRFLAG_ACTIVE)
          {
          if(kill_all_flag) goto normal_rx;
          lir_sleep(3000);
          }
        if(kill_all_flag) goto normal_rx;
        lir_refresh_screen();
        linrad_thread_create(THREAD_TX_INPUT);
        while(thread_status_flag[THREAD_TX_INPUT]!=THRFLAG_ACTIVE)
          {
          if(kill_all_flag) goto normal_rx;
          lir_sleep(3000);
          }
        }
      lir_refresh_screen();
      linrad_thread_create(rx_input_thread);
      lir_sched_yield();
normal_rx:;
      fft1_waterfall_flag=1;
      lir_sleep(50000);
      lir_refresh_screen();
      all_threads_started=TRUE; 
      rxin_block_counter=0;
      i=0;
      k=0;
      t1=50000;
      if(t1<500000./interrupt_rate)t1=500000./interrupt_rate;
      wlcnt_max=1+interrupt_rate;
      wlcnt=wlcnt_max;
      local_workload_reset=workload_reset_flag;
#if RUSAGE_OLD != TRUE
      lir_system_times(&cpu_time1, &total_time1);
      current_time();
      for(i=0; i<THREAD_MAX; i++)
        {
        thread_tottim1[i]=recent_time;
        thread_cputim1[i]=0;
        }
#endif            
      while( !kill_all_flag &&
               thread_status_flag[THREAD_USER_COMMAND]==THRFLAG_ACTIVE &&
               thread_status_flag[rx_input_thread] != THRFLAG_RETURNED)
        {

        m=60;
        while(!kill_all_flag && m>0 && rxin_block_counter == i)
          {
          m--;
          if(local_workload_reset != workload_reset_flag)goto loadprt;
          lir_sleep(t1);
          }
        if( (diskread_flag < 2 || diskread_pause_flag==0) &&
                                   rxin_block_counter == i && i!=0)
          {
          wlcnt=0;
          sprintf(s,"No input %d",k);
          lir_text(0,screen_last_line,s);
          lir_refresh_screen();
          }
        wlcnt-=rxin_block_counter-i;
        if(wlcnt <= 0)
          {
loadprt:;
          wlcnt=wlcnt_max;
// *******************************************************************
// Compute the workload. We should arrive here at a rate of about 1 Hz.
// Modern Linux kernels (2.6.8 or 2.6.9 and later) as well as Windows
// allow us to compute the work load from here.
// For old Linux kernels with RUSAGE_OLD == TRUE we use get_rusage to 
// get the work load for each individual thread by calling from inside
// each one of the threads.  
#if RUSAGE_OLD != TRUE
          current_time();
          for(i=0; i<THREAD_MAX; i++)
            {
            if(thread_pid[i] != 0)
              {
              thread_tottim2[i]=recent_time;
              thread_cputim2[i]=lir_get_thread_time(i);
              thread_workload[i]=100*(thread_cputim2[i]-thread_cputim1[i])/
                                  (thread_tottim2[i]-thread_tottim1[i]);
              }  
            }
#endif
#if RUSAGE_OLD == TRUE
          workload=lir_get_cpu_time()/no_of_processors;
#else          
          lir_system_times(&cpu_time2, &total_time2);
          workload=100*(cpu_time2-cpu_time1)/(total_time2-total_time1);
#endif
          if(workload<0)workload=0;
          if(local_workload_reset != workload_reset_flag)
            {
            local_workload_reset=workload_reset_flag;
            total_time1=total_time2;
            cpu_time1=cpu_time2;
            for(i=0; i<THREAD_MAX; i++)
              {
              if(thread_command_flag[i]!=THRFLAG_NOT_ACTIVE)
                {
                thread_tottim1[i]=thread_tottim2[i];
                thread_cputim1[i]=thread_cputim2[i];
                }  
              }  
            }
          workload_counter++;
          lir_sem_post(SEM_SCREEN);
          }
        k++;  
        if(k==10000)k=1;
        i=rxin_block_counter;
        }
      lir_join(THREAD_USER_COMMAND);
      if(use_tx != 0)
        {
        linrad_thread_stop_and_join(THREAD_TX_INPUT);
        linrad_thread_stop_and_join(THREAD_TX_OUTPUT);
        close_tx_input();
        close_tx_output();
        if(txmem_handle != NULL)free(txmem_handle);
        txmem_handle=NULL;
        lir_sem_free(SEM_TXINPUT);
        }
      linrad_thread_stop_and_join(THREAD_RX_OUTPUT);
      linrad_thread_stop_and_join(rx_input_thread);
      linrad_thread_stop_and_join(THREAD_SCREEN);      
      linrad_thread_stop_and_join(THREAD_NARROWBAND_DSP);
      if((ui.network_flag&NET_RX_OUTPUT)!=0)
        {
        linrad_thread_stop_and_join(THREAD_LIR_SERVER);
        } 
      lir_mutex_destroy();
      linrad_thread_stop_and_join(THREAD_WIDEBAND_DSP);
      free_semaphores();
      if(kill_all_flag) goto menu_x;
      break;

      case MODE_TXTEST:
      usercontrol_mode=USR_TXTEST;
      if(ui.network_flag != 0 || diskread_flag != 0)
        {
        lirerr(1158);
        goto menu_x;
        }
      if(  (ui.rx_input_mode&IQ_DATA) != 0)
        {
        ui.rx_ad_channels=2;
        } 
      else
        {
        ui.rx_ad_channels=1;
        }
      ui.rx_input_mode&=-1-TWO_CHANNELS;
      ui.rx_rf_channels=1;
      genparm[AFC_ENABLE]=0;
      genparm[SECOND_FFT_ENABLE]=0;
      get_wideband_sizes();
      if(kill_all_flag) goto menu_x;
// Open the graph windows on the screen.
// Note that they must be opened in this order because
// Each window is placed outside the previous ones and the init
// routines assumes the order below and does not check for all possible
// conflicts.
      get_buffers(1);
      if(kill_all_flag) goto menu_x;
      if(lir_status != LIR_OK)goto rx_modes_exit;
      read_freq_control_data();
      init_wide_graph();
      if(kill_all_flag) goto menu_x;
      for(i=0; i<txtest_no_of_segs*wg.spek_avgnum; i++)txtest_power[i]=0;
      if(lir_status != LIR_OK)goto rx_modes_exit;
      init_freq_control();
// Make sure users_init_mode is the last window(s) we open.
// the avoid collission routine does not know where
// users windows are placed.
      users_init_mode();
      show_name_and_size();
      settextcolor(7);
      new_baseb_flag=-1;
      ampinfo_flag=1;
      init_semaphores();
      linrad_thread_create(rx_input_thread);
      if(kill_all_flag) goto txtest;
      linrad_thread_create(THREAD_USER_COMMAND);
      if(kill_all_flag) goto txtest;
      linrad_thread_create(THREAD_TXTEST);
      if(kill_all_flag) goto txtest;
      linrad_thread_create(THREAD_WIDEBAND_DSP);
      if(kill_all_flag) goto txtest;
      linrad_thread_create(THREAD_SCREEN);
txtest:;
      fft1_waterfall_flag=1;
      lir_refresh_screen();
      all_threads_started=TRUE; 
      lir_join(THREAD_USER_COMMAND);
      linrad_thread_stop_and_join(rx_input_thread);
      linrad_thread_stop_and_join(THREAD_WIDEBAND_DSP);
      linrad_thread_stop_and_join(THREAD_TXTEST);
      linrad_thread_stop_and_join(THREAD_SCREEN);
      free_semaphores();
      if(kill_all_flag)goto menu_x;
      if(lir_status == LIR_POWTIM)
        {
        usercontrol_mode=USR_POWTIM;
        free_buffers();
        if(kill_all_flag)goto menu_x;
        clear_screen();
        genparm[SECOND_FFT_ENABLE]=1;
        genparm[FIRST_BCKFFT_VERNR]=0;
        genparm[SECOND_FFT_VERNR]=0;
        genparm[FIRST_FFT_SINPOW] = 2;
        genparm[SECOND_FFT_SINPOW] = 2;
        genparm[AFC_ENABLE]=0;
        get_wideband_sizes();
        if(kill_all_flag) goto menu_x;
        get_buffers(1);
        if(kill_all_flag) goto menu_x;
        set_fft1_endpoints();
        if(kill_all_flag) goto menu_x;
        if(lir_status != LIR_POWTIM)goto txtest_exit;
        genparm[SECOND_FFT_ENABLE]=0;
        fft1_waterfall_flag=FALSE;
        init_semaphores();
        linrad_thread_create(rx_input_thread);
        if(kill_all_flag) goto powtim;
        linrad_thread_create(THREAD_USER_COMMAND);
        if(kill_all_flag) goto powtim;
        linrad_thread_create(THREAD_POWTIM);
        if(kill_all_flag) goto powtim;
        linrad_thread_create(THREAD_WIDEBAND_DSP);
powtim:;
        lir_sleep(50000);
        fft1_waterfall_flag=0;
        lir_refresh_screen();
        all_threads_started=TRUE; 
        lir_join(THREAD_USER_COMMAND);
        linrad_thread_stop_and_join(THREAD_POWTIM);
        linrad_thread_stop_and_join(rx_input_thread);
        linrad_thread_stop_and_join(THREAD_WIDEBAND_DSP);
        free_semaphores();
        clear_keyboard();
        }
      if(kill_all_flag) goto menu_x;
      goto txtest_exit;

// Soundcard test mode
      case MODE_RX_ADTEST:
      usercontrol_mode=USR_ADTEST;
      genparm[SECOND_FFT_ENABLE]=0;
      get_wideband_sizes();
      if(kill_all_flag) goto menu_x;
      get_buffers(1);
      if(kill_all_flag) goto menu_x;
      if(lir_status != LIR_OK)goto rx_modes_exit;
      init_semaphores();
      linrad_thread_create(rx_input_thread);
      if(kill_all_flag) goto adtest;
      linrad_thread_create(THREAD_USER_COMMAND);
      if(kill_all_flag) goto adtest;
      linrad_thread_create(THREAD_RX_ADTEST);
adtest:;
      lir_sleep(50000);
      lir_refresh_screen();
      all_threads_started=TRUE; 
      lir_join(THREAD_USER_COMMAND);
      linrad_thread_stop_and_join(rx_input_thread);
      linrad_thread_stop_and_join(THREAD_RX_ADTEST);
      free_semaphores();
      if(kill_all_flag)goto menu_x;
      break;

// Tune hardware.  
      case MODE_TUNE:
      usercontrol_mode=USR_TUNE;
      genparm[SECOND_FFT_ENABLE]=0;
      get_wideband_sizes();
      if(kill_all_flag) goto menu_x;
      clear_screen();
      if(     (ui.rx_input_mode&IQ_DATA) == 0 ||
                (ui.rx_rf_channels == 1) ||
                   ui.rx_ad_speed != 96000)
        {
        settextcolor(14);
        lir_text(10,10,"Incorrect soundcard settings");
        lir_text(5,12,"Set ""Two Rx channels I/Q data"", 96 kHz and 32 bit.");
        settextcolor(7);
        lir_text(13,14,press_any_key);
        await_processed_keyboard();
        if(kill_all_flag) goto menu_x;
        lir_status=LIR_TUNEERR;
        goto rx_modes_exit;
        } 
      get_buffers(1);
      if(kill_all_flag) goto menu_x;
      if(lir_status != LIR_OK)goto rx_modes_exit;
// Collect data during TUNETIME seconds.
#define TUNETIME 0.1
      tune_bytes=96000*TUNETIME;
      if(tune_bytes < 2*fft1_size)tune_bytes=2*fft1_size;
      tune_bytes*=rxad.frame;
      tune_bytes/=timf1_blockbytes;
      tune_bytes++;
      tune_bytes*=timf1_blockbytes;
      if(tune_bytes < 2*fft1_size*(int)rxad.frame)
        {
        lir_text(5,5,"Set fft1 bandwidth higher.");
        lir_text(5,7,press_any_key);
        await_processed_keyboard();
        if(kill_all_flag) goto menu_x;
        goto rx_modes_exit;
        }  
      if(fft1_desired[0] > .99 || bal_updflag == 1)
        {
        settextcolor(14);
        lir_text(3,10,"Calibrate the RX2500 (Read z_CALIBRATE.txt)");
        settextcolor(7);
        lir_text(5,13,press_any_key);
        await_processed_keyboard();
        if(kill_all_flag) goto menu_x;
        break;
        }
      lir_inkey=0;
      init_semaphores();
      linrad_thread_create(rx_input_thread);
      if(kill_all_flag) goto tune;
      linrad_thread_create(THREAD_TUNE);
      if(kill_all_flag) goto tune;
      linrad_thread_create(THREAD_USER_COMMAND);
tune:;
      lir_sleep(50000);
      lir_refresh_screen();
      all_threads_started=TRUE; 
      lir_join(THREAD_USER_COMMAND);
      linrad_thread_stop_and_join(rx_input_thread);
      linrad_thread_stop_and_join(THREAD_TUNE);
      free_semaphores();
      if(kill_all_flag)goto menu_x;
      break;

      case MODE_RADAR:
      use_bfo=1;
      usercontrol_mode=USR_NORMAL_RX;
      if(read_txpar_file()==FALSE)use_tx=0;
      get_wideband_sizes();
      if(kill_all_flag) goto menu_x;
      get_buffers(1);
      if(kill_all_flag) goto menu_x;
      if(lir_status != LIR_OK)goto rx_modes_exit;
      if(!freq_from_file)
        {
        read_freq_control_data();
        }
      check_filtercorr_direction();
      init_wide_graph();
      if( genparm[SECOND_FFT_ENABLE] != 0 )init_blanker();
      if(kill_all_flag) goto menu_x;
      if(lir_status != LIR_OK)goto rx_modes_exit;
      if(genparm[SECOND_FFT_ENABLE] != 0)
        {
        init_hires_graph();
        if(kill_all_flag) goto menu_x;
        if(lir_status != LIR_OK)goto rx_modes_exit;
        }
      if(genparm[AFC_ENABLE] != 0)
        {
        init_afc_graph();
        if(kill_all_flag) goto menu_x;
        if(lir_status != LIR_OK) goto rx_modes_exit;
        }
      if(ui.rx_rf_channels == 2)
        {
        init_pol_graph();
        if(kill_all_flag) goto menu_x;
        if(lir_status != LIR_OK) goto rx_modes_exit;
        }
      init_radar_graph();
      if(kill_all_flag) goto menu_x;
      if(lir_status != LIR_OK) goto rx_modes_exit;
      if(!freq_from_file)
        {
        init_freq_control();
        if(kill_all_flag) goto menu_x;
        if(lir_status != LIR_OK) goto rx_modes_exit;
        }
      if(use_tx != 0)
        {
// Get the xxproc file that the user stored in tg.spproc_no 
        init_tx_graph();
        if(read_txpar_file()==FALSE)
          {
// If it failed, read the one that was succesful before.          
          tg.spproc_no=first_txproc_no;
          read_txpar_file();
          }
        }
// Make sure users_init_mode is the last window(s) we open.
// the avoid collission routine does not know where
// users windows are placed.
      users_init_mode();
      if(kill_all_flag) goto menu_x;
      if(lir_status != LIR_OK) goto rx_modes_exit;
      show_name_and_size();
      init_semaphores();
      linrad_thread_create(rx_input_thread);
      if(kill_all_flag) goto radar;
      linrad_thread_create(THREAD_USER_COMMAND);
      if(kill_all_flag) goto radar;
      linrad_thread_create(THREAD_RADAR);
      if(kill_all_flag) goto radar;
      linrad_thread_create(THREAD_WIDEBAND_DSP);
      if(kill_all_flag) goto radar;
      linrad_thread_create(THREAD_SCREEN);
      if(kill_all_flag) goto radar;
      if((ui.network_flag&NET_RX_OUTPUT)!=0)
        {
        linrad_thread_create(THREAD_LIR_SERVER);
        if(kill_all_flag) goto radar;
        }
      if(use_tx != 0 )
        {
        init_txmem_cwproc();
        if(kill_all_flag)goto radar;
        linrad_thread_create(THREAD_TX_INPUT);
        if(kill_all_flag) goto radar;
        linrad_thread_create(THREAD_TX_OUTPUT);
        if(kill_all_flag) goto radar;
        lir_mutex_init();
        }
radar:;
      fft1_waterfall_flag=1;
      lir_sleep(50000);
      lir_refresh_screen();
      all_threads_started=TRUE; 
      lir_sleep(50000);
      rxin_block_counter=0;
      i=0;
      k=0;
      t1=50000+2000000./interrupt_rate;
      if(diskread_flag < 2)
        {         
        while(!kill_all_flag &&
                    thread_status_flag[THREAD_USER_COMMAND]==THRFLAG_ACTIVE)
          {
          m=30;
          while(!kill_all_flag && m>0 && rxin_block_counter == i)
            {
            m--;
            lir_sleep(t1);
            }
         if(rxin_block_counter == i && i!=0)
            {
            sprintf(s,"No input %d",k);
            lir_text(0,screen_last_line,s);
            lir_refresh_screen();
            }
          k++;  
          if(k==10000)k=1;
          i=rxin_block_counter;
          }
        m=30;
        while(!kill_all_flag && m>0 && rxin_block_counter == i)
          {
          m--;
          lir_sleep(t1);
          }
        if(rxin_block_counter == i && i!=0)  
          {
          sprintf(s,"No input %d",k);
          lir_text(0,screen_last_line,s);
          lir_refresh_screen();
          lirerr(1284);
          }
        }
      lir_join(THREAD_USER_COMMAND);
      if(use_tx != 0)
        {
        linrad_thread_stop_and_join(THREAD_TX_INPUT);
        linrad_thread_stop_and_join(THREAD_TX_OUTPUT);
        close_tx_input();
        close_tx_output();
        if(txmem_handle != NULL)free(txmem_handle);
        txmem_handle=NULL;
        lir_sem_free(SEM_TXINPUT);
        }
      linrad_thread_stop_and_join(rx_input_thread);
      linrad_thread_stop_and_join(THREAD_SCREEN);      
      linrad_thread_stop_and_join(THREAD_RADAR);
      if((ui.network_flag&NET_RX_OUTPUT)!=0)
        {
        linrad_thread_stop_and_join(THREAD_LIR_SERVER);
        } 
      lir_mutex_destroy();
      linrad_thread_stop_and_join(THREAD_WIDEBAND_DSP);
      free_semaphores();
      if(kill_all_flag) goto menu_x;
      break;

      default:
      lirerr(436);
      } 
rx_modes_exit:;
    close_network_sockets();
    if( (diskread_flag&(2+4+8)) != 0)
      {
      diskread_flag=4;
      fclose(save_rd_file);
      save_rd_file=NULL;
      }
txtest_exit:;
    free_buffers();
    if(kill_all_flag)goto menu_x;
    if(lir_status > LIR_OK)
      {
      if(lir_status == LIR_TUNEERR)goto menu_loop;
      clear_screen();
      lir_text(5,5,"Out of memory. Try less demanding parameters");
      lir_text(5,8,press_any_key);
      await_keyboard();
      if(kill_all_flag) goto menu_x;
go_updparm:;
      uiupd=TRUE;
      restore_uiparm();
      goto updparm;
      }
    settextcolor(7);
    if(lir_status == LIR_NEW_POL)
      {
      clear_screen();
      select_pol_default();
      if(kill_all_flag) goto menu_x;
      lir_status = LIR_OK;
      goto do_pc_radio;
      }
    if(lir_status == LIR_POWTIM)goto do_pc_radio;  
    if(lir_status < LIR_OK)
      {
      goto go_updparm;
      }  
    fft1_waterfall_flag=0;
wt_kbd:;
    clear_screen();
    line=0;
    settextcolor(12);
    lir_text(25,line,PROGRAM_NAME);
    if(diskread_flag >=2 )
      {
      settextcolor(14);
      lir_text(40,line,savefile_name);
      }
    line+=2;  
    settextcolor(15);
    sprintf(s,"F1 = Info about the %s mode",rxmodes[rx_mode]);  
    lir_text(11,line,s);
    line++;
    sprintf(s,"B = Back to %s without change",rxmodes[rx_mode]);
    lir_text(12,line,s);
    line++;
    lir_text(12,line,"P = Change parameters");
    line++;
    lir_text(12,line,"C = Calibrate");
    line++;
    if(ui.rx_rf_channels == 2)
      {
      lir_text(12,line,"D = Select Pol. default");
      line++;
      }
    line++;
    settextcolor(7);
    sprintf(s,"Current parameters (file: %s)",parfilnam);
    lir_text(2,line,s);
    line++;
    for(i=0; i<MAX_GENPARM+1; i++)
      {
      sprintf(s,"%s [%d] ",genparm_text[i],genparm[i]);
      if( line <= screen_last_line)lir_text(2,line, s);
      line++;
      }
    await_processed_keyboard();
    if(kill_all_flag) goto menu_x;
    switch (lir_inkey)
      {
      case 'B':
      goto do_pc_radio;
      
      case 'C':
      if((ui.network_flag&NET_RX_INPUT) == 0)
        {
        init_genparm(FALSE);
        if(kill_all_flag) goto menu_x;
        }
      cal_package();
      lir_status=LIR_OK;
      goto rx_modes_exit;

      case 'D':
      if(ui.rx_rf_channels == 2)select_pol_default();
      if(kill_all_flag) goto menu_x;
      break;
  
      case 'P':
      uiupd=TRUE;
      goto updparm;

      case F1_KEY:
      help_message(280+rx_mode);
      break;
      
      case 'X':
      goto menu_loop;
      }
    goto wt_kbd;
    }
  }
if(!kill_all_flag)goto menu_loop;
menu_x:;
close_mouse();
free_buffers();
close_network_sockets();
}


