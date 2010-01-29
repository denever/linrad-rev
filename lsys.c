
#include <vga.h>
#include <vgagl.h>
#include <semaphore.h>
#include <pthread.h>
#include <ctype.h>
#include "globdef.h"
#include "lconf.h"
#include "thrdef.h"
#include "ldef.h"
#include "uidef.h"
#include "screendef.h"
#include "keyboard_def.h"

void lir_set_title(char *s)
{
char cc;
cc=s[0];
}

void lin_global_uiparms(int wn)
{
char ss[10];
int line;
char s[80];
int *modflag;
int i, grp;
vga_modeinfo *info;
// ************************************************
// *****   Select screen number for svgalib  ******
// ************************************************
modflag = malloc((GLASTMODE+1) * sizeof(int));
if(modflag == NULL)
  {
  if(wn==0)
    {
    exit(0);
    }
  else
    {  
    lirerr(1034);
    return;
    }
  }
modflag[0]=0;
line=0;
sprintf(s,"Choose one of the following video modes:");
if(wn==0)
  {
  for(i=0; i<32; i++)printf("\n");
  printf("%s",s);
  }
else
  {
  clear_screen();
  settextcolor(15);
  lir_text(0,line,s);
  settextcolor(7);
  line++;
  }  
grp=0;
for (i = 1; i <= GLASTMODE; i++)
  {
  if (vga_hasmode(i))
    {
    info = vga_getmodeinfo(i);
    if (info->colors == 256)
      {
      modflag[0]++;
      modflag[i]=1;
      sprintf(s,"%5d: %dx%d       ",i, info->width, info->height);
      s[20]=0;
      if(wn==0)
        { 
        if(grp==0)printf("\n");
        printf("%s",s); 
        }
      else  
        {
        lir_text(20*grp,line,s);
        }
      grp++;
      if(grp >= 3)
        {
        grp=0;
        line++;
        if(line > screen_last_line-1)line=screen_last_line-1;
        }
      }
    else
      {
      modflag[i]=0;
      }  
    }
  }
if(wn==0)printf("\n");
line++;
if(modflag[0] == 0)
  {
  sprintf(s,"No 256 color graphics found.");
  if(wn==0)
    {
    printf("%s\n",s);
    exit(0);
    }
  else
    {
    lirerr(352198);
    return;
    }
  }        
gtmode:;
sprintf(s,"Enter mode number.");
if(wn==0)
  {
  printf("%s Then %s\n",s,press_enter);
  fflush(stdout);
  lir_sleep(100000);
  if(lir_inkey==0)exit(0);
  clear_keyboard();
  i=0;
  ss[0]=' ';
  ss[1]=0;
  while(ss[i]!=10)
    {
    if(i<10)i++;
    await_keyboard();
    if(lir_inkey==0)exit(0);
    ss[i]=lir_inkey;
    ss[i+1]=0;
    }
  sscanf(ss,"%d", &ui.vga_mode);
  }  
else
  {
  lir_text(12,line,"=>");
  ui.vga_mode=lir_get_integer(15,line,3,0,GLASTMODE);
  }
sprintf(s,"Error: Mode number out of range");
if(ui.vga_mode <0 || ui.vga_mode > GLASTMODE)goto moderr;
if (modflag[ui.vga_mode] == 0)
  {
moderr:;  
  if(wn==0)
    {
    printf("%s\n\n",s);
    }
  goto gtmode;
  }     
free(modflag);
// *******************************************************
if(wn!=0)
  {
  line=0;
  clear_screen();
  }
if(ui.newcomer_mode != 0)
  {
  ui.font_scale=2;
  }
else
  {
  sprintf(s,"Enter font scale (1 to 5)"); 
fntc:;
  if(wn==0)
    {
    printf("\n%s\n=>",s); 
    await_keyboard();
    if(lir_inkey==0)exit(0);
    }
  else
    {
    lir_text(0,line,s);
    await_keyboard();
    if(kill_all_flag) return;
    line++;
    if(line>=screen_last_line)line=screen_last_line;
    }
  if(lir_inkey < '1' || lir_inkey > '5')goto fntc;
  ui.font_scale=lir_inkey-'0';
  if(lir_errcod)return;
  }
graphics_init();
if(lir_errcod)return;
clear_screen();
settextcolor(15);
lir_text(0,2,"Mouse speed reduction factor:");
ui.mouse_speed=lir_get_integer(30,2,3,1,999);
if(ui.newcomer_mode != 0)
  {
  ui.parport=0;
  ui.parport_pin=0;
  ui.max_blocked_cpus=0;
  }
else
  {  
  clear_screen();
  lir_text(0,3,"Parport address (lpt1=888, none=0):");
  ui.parport=lir_get_integer(36,3,5,0,99999);
  if(ui.parport != 0)
    {
    clear_screen();
    lir_text(0,4,"Parport read pin (ACK=10):");
gtpin:;
    ui.parport_pin=lir_get_integer(27,4,2,10,15);
    if(ui.parport_pin==14)goto gtpin;
    }
  else
    {
    ui.parport_pin=0;
    }
  if(no_of_processors > 1)
    {
    clear_screen();
    sprintf(s,"This system has % d processors.",no_of_processors);
    lir_text(0,3,s);
    lir_text(0,4,"How many do you allow Linrad to block?");
    lir_text(0,5,
        "If you run several instances of Linrad on one multiprocessor");
    lir_text(0,6,"platform it may be a bad idea to allow the total number");
    lir_text(0,7,"of blocked CPUs to be more that the total number less one.");        
    ui.max_blocked_cpus=lir_get_integer(27,9,2,0,no_of_processors-1);
    }    
  else
    {
    ui.max_blocked_cpus=0;
    }  
  }  
if(ui.screen_width_factor <= 0)ui.screen_width_factor=96;
if(ui.screen_height_factor <= 0)ui.screen_height_factor=92;
ui.process_priority=1;
if(wn!=0)
  {
  if( ui.newcomer_mode != 0)
    {
    clear_screen();
    lir_text(0,7,"You are now in newcomer mode.");
    lir_text(0,9,"Press 'Y' to change to normal mode or 'N' to");
    lir_text(0,10,"stay in newcomer mode.");
ask_newco:;
    await_processed_keyboard();
    if(lir_inkey == 'N')goto stay_newco;
    if(lir_inkey != 'Y')goto ask_newco;
    ui.newcomer_mode=0;
    }
stay_newco:;
  }
clear_screen();
lir_text(0,6,
          "Do not forget to save your parameters with 'W' in the main menu");
lir_text(5,8,press_any_key);
await_keyboard();
clear_screen();
settextcolor(7);
}

void x_global_uiparms(int n)
{
// Do something with n to keep the compiler happy:
lir_inkey=n;
// Dummy routine. Not used under Linux svgalib.
}

// *********************************************************
// Mouse routines for svgalib to draw a mouse cross on screen.
// *********************************************************

void restore_behind_mouse(void)
{
int i,imax;
imax=screen_width-mouse_x;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  lir_setpixel(mouse_x+i,mouse_y,behind_mouse[4*i  ]);
  }
imax=mouse_x;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  lir_setpixel(mouse_x-i,mouse_y,behind_mouse[4*i+1]);
  }
imax=screen_height-mouse_y;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  lir_setpixel(mouse_x,mouse_y+i,behind_mouse[4*i+2]);
  }
imax=mouse_y;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  lir_setpixel(mouse_x,mouse_y-i,behind_mouse[4*i+3]);
  }
lir_setpixel(mouse_x,mouse_y,behind_mouse[0]);
mouse_hide_flag=0;
}


void hide_mouse(int x1,int x2,int iy1,int y2)
{
if(mouse_hide_flag ==0)return;
if(mouse_xmax < x1)return;
if(mouse_xmin > x2)return;
if(mouse_ymax < iy1)return;
if(mouse_ymin > y2)return;
restore_behind_mouse();
}

void unconditional_hide_mouse(void)
{
if(mouse_hide_flag ==0)return;
restore_behind_mouse();
}

void show_mouse(void)
{
int i,imax;
unsigned char color;
if(mouse_hide_flag == 1)return;
mouse_hide_flag=1;
color=15-leftpressed;
imax=screen_width-mouse_x;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  behind_mouse[4*i  ]=gl_getpixel(mouse_x+i,mouse_y);
  lir_setpixel(mouse_x+i,mouse_y,color);
  }
imax=mouse_x;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  behind_mouse[4*i+1]=gl_getpixel(mouse_x-i,mouse_y);
  lir_setpixel(mouse_x-i,mouse_y,color);
  }
imax=screen_height-mouse_y;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  behind_mouse[4*i+2]=gl_getpixel(mouse_x,mouse_y+i);
  lir_setpixel(mouse_x,mouse_y+i,color);
  }
imax=mouse_y;
if(imax > mouse_cursize) imax=mouse_cursize;
for(i=1; i<imax; i++)
  {
  behind_mouse[4*i+3]=gl_getpixel(mouse_x,mouse_y-i);
  lir_setpixel(mouse_x,mouse_y-i,color);
  }
behind_mouse[0]=gl_getpixel(mouse_x,mouse_y);
lir_setpixel(mouse_x,mouse_y,color);
}

void lir_move_mouse_cursor(void)
{
if(mouse_hide_flag !=0)restore_behind_mouse();
set_button_coordinates();
mouse_xmax=mouse_x+mouse_cursize;
mouse_xmin=mouse_x-mouse_cursize;
mouse_ymax=mouse_y+mouse_cursize;
mouse_ymin=mouse_y-mouse_cursize;
}

// *********************************************************
// Graphics for svgalib
// *********************************************************

void lir_getpalettecolor(int j, int *r, int *g, int *b)
{
gl_getpalettecolor(j, r, g, b);
}

void lir_fillbox(int x, int y,int w, int h, unsigned char c)
{
if(x < 0 || 
   w < 0 || 
   x+w > WIDTH ||
   y < 0 ||
   h < 0 || 
   y+h > HEIGHT)lirerr(1213);
gl_fillbox(x,y,w,h,c);
}

void lir_getbox(int x, int y, int w, int h, void* dp)
{
gl_getbox(x,y,w,h,dp);
}

void lir_putbox(int x, int y, int w, int h, void* dp)
{
gl_putbox(x,y,w,h,dp);
}

void lir_hline(int x1, int y, int x2, unsigned char c)
{
if( x1 < 0 ||
    x2 < 0 ||
    y < 0 ||
    x1>=WIDTH ||
    x2>=WIDTH ||
    y >= HEIGHT)lirerr(1214);
gl_hline( x1, y, x2, c);
}

void lir_line(int x1, int yy1, int x2,int y2, unsigned char c)
{
if( x1 < 0 )  lirerr(99201);
if( x2 < 0 )lirerr(99202);
if( yy1 < 0 )lirerr(99203);
if( y2 < 0 )lirerr(99204);
if( x1>=WIDTH )lirerr(99205);
if( x2>=WIDTH )lirerr(99206);
if( yy1 >= HEIGHT)lirerr(99207);
if( y2 >= HEIGHT)lirerr(99208);
gl_line( x1, yy1, x2, y2, c);
}

void lir_setpixel(int x, int y, unsigned char c)
{
if( x < 0 )
  {
  lirerr(1206);
  }
else
  {  
  if( y < 0 )
    {
    lirerr(1207);
    }
  else
    {
    if( x >= WIDTH )
      {
      lirerr(1208);
      }
    else
      {
      if( y >= HEIGHT)
        {
        lirerr(1209);
        }
      else
        {
        gl_setpixel( x, y, (int)c);
        }
      }
    }
  }
}

void clear_screen(void)
{
vga_clear();
}

void lir_refresh_screen(void)
{
// This is used by Windows and X11 to force a copy from memory
// to the screen. svgalib is fast enough to allow
// direct screen updates. (SetPixel under Windows is extremely slow)
}

void lir_refresh_entire_screen(void)
{
// This is used by X11 to force a copy from memory
}

