
// for Linux/svgalib.
// We set up the screen and mouse here and then we start
// the version independent main menu thread and the keyboard thread.
// We return here with lir_status=LIR_NEW_SCREEN in case the user
// wants a different screen resolution or LIR_UIPARMS if the
// user wants to change other system parameters.

//#define UNINIT_MEMDATA 0x57
#define UNINIT_MEMDATA 0
#define MAX_MOUSE_CURSIZE 50


#include <sys/io.h>
#include <vga.h>
#include <vgagl.h>
#include <vgamouse.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/mman.h>
#include <termios.h>
#include <sys/resource.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>


#include "globdef.h"
#include "lconf.h"
#include "thrdef.h"
#include "ldef.h"
#include "uidef.h"
#include "screendef.h"
#include "vernr.h"
#include "options.h"
#include "keyboard_def.h"

int mouse_wheel_flag;


int main(int argc, char *argv[])
{
struct MouseCaps caps;
char chr;
int i,ir,j,k;
char *parinfo;
int xxprint;
FILE *file;
int *uiparm;
// put something nonzero in scratch memory to make sure we not rely on 
// it being zero when starting (this may help us detect bugs).
if(UNINIT_MEMDATA != 0)
  {
  parinfo=(void*)(&uninit_mem_begin);
  k=&uninit_mem_end-parinfo;
  for(i=0; i<k; i++)parinfo[i]=UNINIT_MEMDATA;
  }
// Make silly use of parameters to suppress warning from compiler
chr=argv[0][0];
i=argc;
// Init svgalib and set it to text mode.
vga_init();
// Permissions to write to the hardware have to be set
// before any new threads are started. Reason unknown........
init_os_independent_globals();
serport=-1;
// Allocate buffers for keyboard and mouse, then start their threads.
keyboard_buffer=malloc(KEYBOARD_BUFFER_SIZE*sizeof(int)+
                     4*(MAX_MOUSE_CURSIZE+1)*sizeof(char));
behind_mouse = (void*)keyboard_buffer+KEYBOARD_BUFFER_SIZE*sizeof(int);
// Set the flags that tell whether daughter processes are running.
for(i=0; i<MAX_LIRSEM; i++)lirsem_flag[i]=0;
#ifdef THREAD_TIMERS
for(i=0; i<THREAD_MAX; i++)
  {
  thread_workload[i]=0;
  thread_tottim1[i]=0;
  thread_cputim1[i]=0;
  thread_tottim2[i]=0;
  thread_cputim2[i]=0;
  }
#endif
lir_sem_init(SEM_KEYBOARD);
pthread_create(&thread_identifier_keyboard,NULL,(void*)thread_keyboard, NULL);
pthread_create(&thread_identifier_mouse,NULL,(void*)thread_mouse, NULL);
// Create a thread that will close the entire process in a controlled way
// in case lirerr() is called or the ESC key is pressed.
sem_init(&sem_kill_all,0,0);
pthread_create(&thread_identifier_kill_all,NULL,(void*)thread_kill_all, NULL);
uiparm=(int*)(&ui);
if(DUMPFILE)
  {
  dmp = fopen("dmp", "w");
  DEB"\n******************************\n");
  }
else
  {  
  dmp=NULL;
  }
os_flag=OS_FLAG_LINUX;
xxprint=investigate_cpu();
file = fopen(userint_filename, "rb");
if (file == NULL)
  {
  vga_setmode(TEXT);
  print_procerr(xxprint);
  printf("\n\nSetup file %s missing.",userint_filename);
full_setup:;
  vga_setmode(TEXT);
  for(i=0; i<MAX_UIPARM; i++) uiparm[i]=0;
  printf("\nUse W to create a new %s file after setup.\n\n",userint_filename);
  printf(
    "Press S for setup routines in normal mode or press N for NEWCOMER mode");
  printf("\nAny other key to exit. (You might want to manually edit %s)",
                                                       userint_filename);
  printf("\nThen press enter\n\n=>");
  toupper_await_keyboard();
  if(lir_inkey != 'S' && lir_inkey != 'N') exit(0);
  if(lir_inkey == 'N')
    {
    ui.newcomer_mode=1;
    }
  else
    {
    ui.newcomer_mode=0;
    }
  lin_global_uiparms(0);
  if(kill_all_flag) goto exitmain;
  uiparm_change_flag=TRUE;
  ui.rx_input_mode=-1;
  ui.tx_dadev_no=-1;
  ui.rx_dadev_no=-1;
  }
else
  {
  parinfo=malloc(4096);
  if(parinfo == NULL)
    {
    lir_errcod=1078;
    goto exitmain;
    }
  for(i=0; i<4096; i++) parinfo[i]=0;
  ir=fread(parinfo,1,4095,file);
  fclose(file);
  file=NULL;
  if(ir >= 4095)
    {
    goto go_full_setup;
    }
  k=0;
  for(i=0; i<MAX_UIPARM; i++)
    {
    while(parinfo[k]==' ' ||
          parinfo[k]== '\n' )k++;
    j=0;
    while(parinfo[k]== uiparm_text[i][j])
      {
      k++;
      j++;
      } 
    if(uiparm_text[i][j] != 0)goto go_full_setup;
    while(parinfo[k]!='[' && k< ir)k++;
    sscanf(&parinfo[k],"[%d]",&uiparm[i]);
    while(parinfo[k]!='\n' && k< ir)k++;
    }
  if( ui.check != UI_VERNR ||
      ui.newcomer_mode <0 ||
      ui.newcomer_mode >1)
    {
go_full_setup:;    
    printf("\n\nSetup file %s has errors",userint_filename);
    parinfo=chk_free(parinfo);
    goto full_setup;
    }
  if (vga_hasmode(ui.vga_mode)==0)goto full_setup;
  graphics_init();
  if(lir_errcod != 0)goto text_exitmain;
  uiparm_change_flag=FALSE;
  }
vga_setmousesupport(1);
/*
mouse_init("/dev/mouse",vga_getmousetype(),MOUSE_DEFAULTSAMPLERATE);
mouse_setdefaulteventhandler();
*/
mouse_setxrange(0, WIDTH-1);
mouse_setyrange(0, HEIGHT-1);
mouse_setwrap(MOUSE_NOWRAP);
lir_status=LIR_OK;
// Set up mouse and system parameters in case it has not been done already.
mouse_cursize=HEIGHT/80;
if(mouse_cursize > MAX_MOUSE_CURSIZE)mouse_cursize=MAX_MOUSE_CURSIZE;
if(ui.mouse_speed < 1)ui.mouse_speed=1;
if(ui.mouse_speed > 999)ui.mouse_speed=999;
mouse_setscale(ui.mouse_speed);
mouse_x=screen_width/2;
mouse_y=screen_height/2;
new_mouse_x=screen_width/2;
new_mouse_y=screen_height/2;
mouse_setposition( mouse_x, mouse_y);
if(mouse_getcaps(&caps)) 
  {
// Failed! Old library version... Check the mouse type.
  i=vga_getmousetype() & MOUSE_TYPE_MASK;
  if(i == MOUSE_INTELLIMOUSE || i==MOUSE_IMPS2)
    {
    mouse_wheel_flag=1;
    }
  else
    {
    mouse_wheel_flag=0;
    }
  }
else
  {
// If this is a mouse_wheel_flag mouse, interpret rx as a wheel
  mouse_wheel_flag = ((caps.info & MOUSE_INFO_WHEEL) != 0);
  }
if(mouse_wheel_flag)
  {
  mouse_setrange_6d(0,0, 0,0, 0, 0, -180,180, 0,0, 0,0, MOUSE_RXDIM);
  }
users_open_devices();
if(kill_all_flag) goto text_exitmain;
pthread_create(&thread_identifier_main_menu,NULL,
                                              (void*)thread_main_menu, NULL);
usleep(50000);
pthread_join(thread_identifier_main_menu,0);
exitmain:;
clear_keyboard();
show_errmsg(1);
text_exitmain:;
vga_setmode(TEXT);
// Set text mode again. Helps lap top computers that otherwise
// fail to restore text mode.
vga_setmode(TEXT);
users_close_devices();
lir_close_serport();
show_errmsg(0);
printf("\n\nLeaving %s\n",PROGRAM_NAME);
pthread_cancel(thread_identifier_mouse);
pthread_join(thread_identifier_mouse,0);
lir_sleep(20000);
pthread_cancel(thread_identifier_keyboard);
pthread_join(thread_identifier_keyboard,0);
if(dmp!=NULL)fclose(dmp);
return lir_errcod;
}

void graphics_init(void)
{
int i;
vga_setmode(ui.vga_mode);
gl_setcontextvga(ui.vga_mode);
if(BYTESPERPIXEL != 1)
  {
  lirerr(1243);
  return;
  }
screen_width=WIDTH;
screen_height=HEIGHT;
if(screen_width < 640 || screen_height < 480)
  {
  lirerr(1242);
  return;
  }
for(i=0; i<256;i++)
  {
  gl_setpalettecolor(i, svga_palette[3*i+2],
                        svga_palette[3*i+1],
                        svga_palette[3*i  ]);
  }
init_font(ui.font_scale);
}


int force_getchar(void)
{
// Get a character from keyboard, but set a 0.1 s timeout
// and return -1 if no key was pressed.
struct termios term, oterm;
int c = 0;
// get the terminal settings 
tcgetattr(0, &oterm);
// get a copy of the settings, which we modify 
memcpy(&term, &oterm, sizeof(term));
// put the terminal in non-canonical mode, any 
// reads timeout after 0.1 seconds or when a 
// single character is read
term.c_lflag = term.c_lflag & (!ICANON);
term.c_cc[VMIN] = 0;
term.c_cc[VTIME] = 1;
tcsetattr(0, TCSANOW, &term);
// get input - timeout after 0.1 seconds or
// when one character is read. If timed out
// getchar() returns -1, otherwise it returns
// the character
c=getchar();
//   reset the terminal to original state 
tcsetattr(0, TCSANOW, &oterm);
return c;
}


void thread_keyboard(void)
{
int c, i, k;
char s[80];
pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,&i);
while(1)
  {
// It is silly to use force_getchar here. 
// We should set up stdin so it will return on each char of an ESC sequence.
// so we would not have to loop around at 10 Hz here.
  c=getchar();
  keyboard_buffer[keyboard_buffer_ptr]=c;
  if(c == 27)
    {
// Set the flag and wait a while to make it unlikely
// it will not be visible for the kill_all thread.
// We must not kill the keyboard thread while force_getchar() is
// executed because the changed terminal setting will survive
// after Linrad is finished and make the command line behave
// incorrectly.
      {
      c=force_getchar();
      if(c == -1)
        {
// The ESC key was pressed. Exit from Linrad NOW!    
        DEB"\nESC pressed");
        if(ui.newcomer_mode && !kill_all_flag)
          {
          newcomer_escpress(0);
newco_esc:;          
          while(c != 'Y' && c != 'N')c=toupper(getchar());
          if(c=='N')
            {
            while(c!=-1)c=force_getchar();
            newcomer_escpress(1);
            goto next;
            }
          if(c != 'Y')goto newco_esc;  
          }
        sem_post(&sem_kill_all);
        while(!kill_all_flag)lir_sleep(3000);
        keyboard_buffer[keyboard_buffer_ptr]=0;
        keyboard_buffer_ptr=(keyboard_buffer_ptr+1)&
                                                  (KEYBOARD_BUFFER_SIZE-1);
        sem_post(&lirsem[SEM_KEYBOARD]);
        while(1)
          {
          lir_sleep(30000);
          }
        return;
        }    
// The ESC sequences are all [27],[91],.....
      if(c == 91)
        {
        c=force_getchar();
// Arrow keys are:
// ARROW_UP=65
// ARROW_DOWN=66
// ARROW_RIGHT=67
// ARROW_LEFT=68
        if(c >= 65 && c <= 68)
          {
          c+=ARROW_UP_KEY-65;
          goto fkn_key_ok;
          }
        if(c >= 49 && c <= 54)
          {
          i=force_getchar();
// These keys have to be followed by 126
// HOME=49
// INSERT=50
// DELETE=51
// END=52
// PAGE_UP=53
// PAGE_DOWN=54
          if(i == 126)
            {
            c+=HOME_KEY-49;
            goto fkn_key_ok;
            }
          k=force_getchar();
          if(k == 126)
            {  
            if(c == 49)
              {
// F6=55
// F7=56
// F8=57
              if(i >= 55 && i <= 57)
                {
                c=i+F6_KEY-55;
                goto fkn_key_ok;
                }
              }  
            if(c == 50)
              {
// F9=48
// F10=49
// F10_PAD=50
// F11=51
// F12=52
// SHIFT_F1=53
// SHIFT_F2=54
// SHIFT_F2_PAD=55
// SHIFT_F3=56
// SHIFT_F4=57
              if(i >= 48 && i <= 57)
                { 
                c=i+F9_KEY-48;
                goto fkn_key_ok;
                }
              }    
            if(c == 51)
              {
// SHIFT_F5=49
// SHIFT_F6=50
// SHIFT_F7=51
// SHIFT_F8=52
             if(i >= 49 && i <= 52)
                { 
                c=i+SHIFT_F5_KEY-49;
                goto fkn_key_ok;
                }
              }
            }    
          sprintf(s,"ESC sequence):[27][91][%d][%d][%d]",c,i,k);
          goto skip;
          }
// PAUSE = 80
        if(c == 80)
          {
          c=PAUSE_KEY;
          goto fkn_key_ok;
          }
        if(c == 91)
          {

          c=force_getchar();
// F1=65
// F2=66
// F3=67
// F4=68
// F5=69
          if(c >= 65 && c <= 69)
            {
            c+=F1_KEY-65;
            goto fkn_key_ok;
            }
          sprintf(s,"ESC sequence:[27][91][91][%d]",c);
          goto skip;
          }
        sprintf(s,"ESC sequence:[27][91][%d]",c);
        goto skip;
        }
// ESC followed by something else than 91 (should never occur)
      sprintf(s,"ESC sequence:[27][%d]",c);
skip:;
      lir_text(1,5,s);
      c=force_getchar();
      if(c != -1)
        {
        i=0;
        while(s[i]!=0 && i<70)i++;
        sprintf(&s[i],"[%d]",c);
        goto skip;
        }
      if(c == -1) goto next;
fkn_key_ok:;
      keyboard_buffer[keyboard_buffer_ptr]=c;
      }
    }
  keyboard_buffer_ptr=(keyboard_buffer_ptr+1)&(KEYBOARD_BUFFER_SIZE-1);
  sem_post(&lirsem[SEM_KEYBOARD]);
next:;
  }
}

void thread_mouse(void)
{
int i, m, button, rx;
pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,&i);
while(TRUE)
  {
// **********************************************
// Wait for svgalib to report a mouse event
  mouse_waitforupdate();
// We do nothing unless mouse_flag is set.
  if(mouse_flag != 0)
    {
    button= mouse_getbutton();
    mouse_getposition_6d(&new_mouse_x, &new_mouse_y, NULL, &rx, NULL, NULL);
    if(rx!=0)
      {
      if (button & MOUSE_MIDDLEBUTTON) 
        {
        m=bg.wheel_stepn;
        if(rx > 0)
          {
          m++;
          if(m>30)m=30;
          }
        else
          {
          m--;
          if(m<-30)m=-30;
          if(genparm[AFC_ENABLE]==0 && m<0)m=0;
          }
        bg.wheel_stepn=m;
        sc[SC_SHOW_WHEEL]++;
        make_modepar_file(GRAPHTYPE_BG);
        }
      else
        {
        if(rx > 0)
          {
          step_rx_frequency(1);
          }
        else
          {
          step_rx_frequency(-1);
          }  
        }
      }
    if (button & MOUSE_LEFTBUTTON) 
      {
      new_lbutton_state=1;
      }
    else
      {
      new_lbutton_state=0;
      }
    if (button & MOUSE_RIGHTBUTTON) 
      {
      new_rbutton_state=1;
      }
    else
      {
      new_rbutton_state=0;
      }
    if( thread_status_flag[THREAD_SCREEN] == THRFLAG_SEM_WAIT ||
        thread_status_flag[THREAD_SCREEN] == THRFLAG_ACTIVE ||
        thread_status_flag[THREAD_SCREEN] == THRFLAG_IDLE) 
      {
      if( new_mouse_x!=mouse_x || new_mouse_y!=mouse_y)lir_sem_post(SEM_SCREEN);
      check_mouse_actions();
      }
    }
  }
}

