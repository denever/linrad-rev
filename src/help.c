

#include <unistd.h>
#include "globdef.h"
#include "uidef.h"
#include "fft1def.h"
#include "screendef.h"
#include "seldef.h"
#include "sigdef.h"
#include "thrdef.h"
#include "vernr.h"
#include "rusage.h"

unsigned char show_scro_color;
FILE *msg_file;

void show_scro(int ix1,int iy1,int ix2,int iy2)
{
unsigned char cl;
if(ix1 < 0 || ix1 >= screen_width)return; 
if(ix2 < 0 || ix2 >= screen_width)return; 
if(show_scro_color==0)cl=12; else cl=10;
lir_hline(ix1,iy1,ix2,cl);
if(show_scro_color==2)cl=12; else cl=10;
lir_hline(ix1,iy2,ix2,cl);
if(show_scro_color==1)cl=12; else cl=10;
lir_line(ix1,iy1,ix1,iy2,cl);
if(show_scro_color==3)cl=12; else cl=10;
lir_line(ix2,iy1,ix2,iy2,cl);
}

char *screensave;

void init_screensave(void)
{
pause_thread(THREAD_SCREEN);
screensave=malloc(screen_width*screen_height);
lir_sched_yield();
if( screensave == NULL)
  {
  lirerr(1035);
  return;
  }
lir_getbox(0,0,screen_width,screen_height,screensave);
lir_sched_yield();
}

void close_screensave(void)
{
lir_putbox(0,0,screen_width,screen_height,screensave);
free(screensave);
lir_sched_yield();
resume_thread(THREAD_SCREEN);
}

void newcomer_escpress(int clear)
{
if(clear == 0)
  {
  init_screensave();
  if(kill_all_flag)return;
  clear_screen();
  settextcolor(15);
  lir_text(20,10,"ESC key pressed");
  settextcolor(7);
  lir_text(5,12,"Do you really want to exit from Linrad now ?  (Y/N)=>");
  }
else 
  {
  close_screensave();
  }
lir_refresh_screen();
}

void help_screen_objects(void)
{
int i, j;
// Make sure we are not in a mouse function.
mouse_time_wide=current_time();
mouse_time_narrow=mouse_time_wide;
if(mouse_task != -1)return;
lir_sched_yield();
if(mouse_task != -1)return;
mouse_time_wide=current_time();
mouse_time_narrow=mouse_time_wide;
lir_sched_yield();
if(mouse_task != -1)return;
mouse_inhibit=TRUE; 
init_screensave();
if(kill_all_flag)return;
for(i=0; i<no_of_scro; i++)
  {
  if( scro[i].x1 <= mouse_x && 
      scro[i].x2 >= mouse_x &&      
      scro[i].y1 <= mouse_y && 
      scro[i].y2 >= mouse_y) 
    {
    switch (scro[i].no)
      {
      case WIDE_GRAPH:
      help_on_wide_graph();
      break;

      case HIRES_GRAPH:
      help_on_hires_graph();
      break;

      case AFC_GRAPH:
      help_on_afc_graph();
      break;

      case BASEBAND_GRAPH:
      help_on_baseband_graph();
      break;

      case POL_GRAPH:
      help_on_pol_graph();
      break;

      case COH_GRAPH:
      help_on_coherent_graph();
      break;

      case EME_GRAPH:
      help_on_eme_graph();
      break;

      case FREQ_GRAPH:
      help_on_freq_graph();
      break;

      case METER_GRAPH:
      help_on_meter_graph();
      break;

      case TRANSMIT_GRAPH:
      help_on_tx_graph();
      break;

      case RADAR_GRAPH:
      help_on_radar_graph();
      break;
      }
    }
  }
if(kill_all_flag) return;  
if(lir_inkey != 0)
  {
  show_scro_color=0;
repeat:;
  for(i=0; i<no_of_scro; i++)
    {
    show_scro(scro[i].x1,scro[i].y1,scro[i].x2,scro[i].y2);
    switch (scro[i].no)
      {
      case WIDE_GRAPH:
      for(j=4; j<MAX_WGBUTT; j++)
        {
        show_scro(wgbutt[j].x1,wgbutt[j].y1,wgbutt[j].x2,wgbutt[j].y2);
        if(kill_all_flag) return;
        }
      break;

      case HIRES_GRAPH:
      for(j=4; j<MAX_HGBUTT; j++)
        {
        if(hgbutt[j].x1 > 0)
          { 
          show_scro(hgbutt[j].x1,hgbutt[j].y1,hgbutt[j].x2,hgbutt[j].y2);
          if(kill_all_flag) return;
          }
        }
// The control bars are separate from the buttons.
      show_scro(timf2_hg_xmin-2,timf2_hg_y[0],
                                 timf2_hg_xmax+1,timf2_hg_y[0]+text_height-1);
      if(kill_all_flag) return;
      show_scro(hg_ston1_x2+2,hg_ston_y0,hg_first_xpixel,hg_stonbars_ytop);
      if(kill_all_flag) return;
      show_scro(hg.xleft+1,hg_ston_y0, hg_ston1_x2,hg_stonbars_ytop);
      break;

      case AFC_GRAPH:
      for(j=4; j<MAX_AGBUTT; j++)
        {
        show_scro(agbutt[j].x1,agbutt[j].y1,agbutt[j].x2,agbutt[j].y2);
        if(kill_all_flag) return;
        } 
// The control bars are separate from the buttons.
      show_scro(ag_ston_x1,ag_fpar_y0,ag_ston_x2,ag_fpar_ytop);
      if(kill_all_flag) return;
      show_scro(ag_lock_x1,ag_fpar_y0,ag_lock_x2,ag_fpar_ytop);
      if(kill_all_flag) return;
      show_scro(ag_srch_x1,ag_fpar_y0,ag_srch_x2,ag_fpar_ytop);
      if(kill_all_flag) return;
      break;

      case BASEBAND_GRAPH:
      for(j=4; j<MAX_BGBUTT; j++)
        {
        show_scro(bgbutt[j].x1,bgbutt[j].y1,bgbutt[j].x2,bgbutt[j].y2);
        if(kill_all_flag) return;
        }
// The volume control bar:
      show_scro(bg_vol_x1,bg_y0,bg_vol_x2,bg_ymax);
      if(kill_all_flag) return;
      break;

      case POL_GRAPH:
      for(j=4; j<MAX_PGBUTT; j++)
        {
        show_scro(pgbutt[j].x1,pgbutt[j].y1,pgbutt[j].x2,pgbutt[j].y2);
        if(kill_all_flag) return;
        } 
      break;

      case COH_GRAPH:
      for(j=4; j<MAX_CGBUTT; j++)
        {
        show_scro(cgbutt[j].x1,cgbutt[j].y1,cgbutt[j].x2,cgbutt[j].y2);
        if(kill_all_flag) return;
        } 
      break;

      case EME_GRAPH:
      if(eme_active_flag != 0)
        {
        for(j=4; j<MAX_EGBUTT; j++)
          {
          show_scro(egbutt[j].x1,egbutt[j].y1,egbutt[j].x2,egbutt[j].y2);
          if(kill_all_flag) return;
          }
        }
      else
        {
        show_scro(egbutt[EG_MINIMISE].x1,egbutt[EG_MINIMISE].y1,
                  egbutt[EG_MINIMISE].x2,egbutt[EG_MINIMISE].y2);
        if(kill_all_flag) return;
        }           
      break;

      case FREQ_GRAPH:
      for(j=4; j<MAX_FGBUTT; j++)
        {
        show_scro(fgbutt[j].x1,fgbutt[j].y1,fgbutt[j].x2,fgbutt[j].y2);
        if(kill_all_flag) return;
        } 
      break;

      case METER_GRAPH:
      for(j=4; j<MAX_MGBUTT; j++)
        {
        show_scro(mgbutt[j].x1,mgbutt[j].y1,mgbutt[j].x2,mgbutt[j].y2);
        if(kill_all_flag) return;
        } 
      break;

      case TRANSMIT_GRAPH:
      for(j=4; j<MAX_TGBUTT; j++)
        {
        show_scro(tgbutt[j].x1,tgbutt[j].y1,tgbutt[j].x2,tgbutt[j].y2);
        if(kill_all_flag) return;
        } 
      break;
      }
    }
  show_scro_color++;
  if(show_scro_color > 3)show_scro_color=0;
  lir_refresh_screen();
  lir_sleep(400000);
  test_keyboard(); 
  if(kill_all_flag) goto help_scox; 
  process_current_lir_inkey();
  if(kill_all_flag) goto help_scox; 
  if(lir_inkey == 0)goto repeat;
  }
close_screensave();
lir_refresh_screen();
help_scox:;
resume_thread(THREAD_SCREEN);
mouse_inhibit=FALSE;
}

void write_from_msg_file(int *line, int msg_no, 
                                     int screen_mode, int vernr, int wait)
{
char s[512];
char chr;
int i,j,k;
msg_file=fopen(msg_filename, "r");
if(msg_file == NULL)
  {
  sprintf(s,"Could not find %s",msg_filename);
  goto finish_err;
  }
i=fread(&s,1,6,msg_file);
if(i!=6)
  {
  sprintf(s,"File empty");
  goto finish_err_close;
  }
s[6]=0;   
sscanf(s,"%d",&k);
if(k != vernr)
  {
  sprintf(s,"Error %d. Wrong version number in %s",msg_no, msg_filename);
  goto finish_err_close;
  }
gtnum:;
i=fread(&s,1,1,msg_file);
if(i==0)
  {
  sprintf(s,"Could not find info no %d",msg_no);
  goto finish_err_close;
  }
if(s[0] != '[')goto gtnum;
j=0;
while(fread(&s[j],1,1,msg_file)==1 && s[j] != ']')j++;
s[j]=0;
sscanf(s,"%d",&k);
if(k != msg_no)goto gtnum;
sprintf(s,"[%d]",msg_no);
j=0;
while(s[j]!=0)j++;
j--;
gtrow:; 
j++;   
i=fread(&s[j],1,1,msg_file);
if( i==1 && s[j] != 10 && s[j] != '[' && j<255 )goto gtrow;
chr=s[j];
if(chr== '[' || chr==10)
  {
  s[j]=0;
  }
else
  {  
  s[j+1]=0;
  }
if(screen_mode!=0)
  {
  lir_text(0,line[0],s);
  }
else
  {
  printf("\n%s",s);
  DEB"\n%s",s);
  }  
line[0]++;
if(chr != '[' && i==1)
  {
  j=-1;
  goto gtrow;
  }   
fclose(msg_file);
goto finish_ok;  
finish_err_close:;       
fclose(msg_file);
finish_err:;       
if(screen_mode!=0)
  {
  lir_text(0,line[0],s);
  }
else
  {
  printf("\n%s",s);
  DEB"\n%s",s);
  }  
line[0]++;
sprintf(s,"Error[%d] No info available in %s .",msg_no,msg_filename);
if(screen_mode!=0)
  {
  lir_text(0,line[0],s);
  }
else
  {
  printf("\n%s",s);
  DEB"\n%s",s);
  }  
line[0]++;
finish_ok:;
if(screen_mode == 2)
  {
  if(os_flag == OS_FLAG_LINUX)
    {
    lir_text(15,line[0]+2,"Exit with ctlC");
    }
  else
    {
    lir_text(15,line[0]+2,
       "Escape by clicking with the mouse mouse on the X in the title bar");
    }  
  if(os_flag == OS_FLAG_WINDOWS)
    {
    lir_text(15,line[0]+3,"or with ESC from keyboard.");
    }
  }
else
  {  
  if(screen_mode == 1 && wait == TRUE)
    {
    lir_text(15,line[0]+2,press_any_key);
    await_keyboard();
    settextcolor(7);
    lir_inkey=0;
    }
  else
    {
    DEB"\n");
    if(dmp != NULL)
      {
      fflush(dmp);
      lir_sync();
      }
    }
  }
}

void show_errmsg(int screen_mode)
{
char s[80];
int line;
if(lir_errcod == 0)return;
if(screen_mode != 0)
  {
  settextcolor(15);
  clear_screen();
  }
sprintf(s,"INTERNAL ERROR: %d",lir_errcod);
if(screen_mode != 0)
  {
  lir_text(0,2,s);
  }
else
  {
  printf("\n%s",s);
  DEB"\n%s",s);
  }  
msg_filename="errors.lir";
line=3;
if(screen_mode)
  {
  settextcolor(15);
  clear_screen();
  }
write_from_msg_file(&line, lir_errcod, screen_mode, ERROR_VERNR,TRUE);
}  


void help_message(int msg_no)
{
int line;
if(msg_no < 0)return;
msg_filename="help.lir";
line=0;
settextcolor(15);
clear_screen();
if(msg_no != 1)
  {
  write_from_msg_file(&line, msg_no, TRUE, HELP_VERNR,FALSE);
  line+=2;
  }
settextcolor(7);
write_from_msg_file(&line, 1, TRUE, HELP_VERNR,TRUE);
lir_inkey=0;
}
