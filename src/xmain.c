// For help on X11 give command: "man X Interface"
// Event masks and event definitions are in /usr/X11R6/include/X11/X.h


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <ctype.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>
#include "globdef.h"
#include "thrdef.h"
#include "uidef.h"
#include "screendef.h"
#include "vernr.h"
#include "options.h"
#include "keyboard_def.h"
#include "lconf.h"
#include "xdef.h"
#include "ldef.h"

#if SHM_INSTALLED == 1
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
extern XShmSegmentInfo *shminfo;
int ShmMajor,ShmMinor;
Bool ShmPixmaps;
#endif

extern GC xgc;
extern XImage *ximage;
extern Display *xdis;
extern Window xwin;
extern Colormap lir_colormap;

int newcomer_escflag;
int saved_euid=-1;

typedef struct {
unsigned short int red;
unsigned short int green;
unsigned short int blue;
unsigned int pixel;
short int flag;
float total;
}PIXINFO;

// We want to know about the user clicking on the close window button
Atom wm_delete_window;

int main(int argc, char **argv)
{
int id,il,kd,kl;
int bitmap_pad;
float t1,t2;
PIXINFO *defpix, *lirpix;
float *pixdiff;
PIXINFO tmppix;
Colormap default_colormap;
char *hostname;
Visual *visual;
int i, k, m, screen_num;
Cursor cross_cursor;
unsigned short int *ipalette;
XColor xco;
for(i=0; i<MAX_LIRSEM; i++)lirsem_flag[i]=0;
XInitThreads();
if(DUMPFILE)
  {
  dmp = fopen("dmp", "w");
  DEB"\n******************************\n");
  }
else
  {  
  dmp=NULL;
  }
expose_event_done=FALSE;
first_mempix=0x7fffffff;
last_mempix=0;
shift_key_status=0;
i=argc;
os_flag=OS_FLAG_X;
init_os_independent_globals();
newcomer_escflag=FALSE;
serport=-1;
keyboard_buffer_ptr=0;
keyboard_buffer_used=0;
keyboard_buffer=malloc(KEYBOARD_BUFFER_SIZE*sizeof(int));
if (argv[1] == NULL)
  {
  hostname = NULL;
  }
else
  {
  hostname = argv[1];
  }
xdis = XOpenDisplay(hostname);
if (xdis == NULL)
  {
  fprintf(stderr, "\nCan't open display: %s\n", hostname);
  return(10);
  }
ui_setup();
X11_accesstype=X11_STANDARD;
#if SHM_INSTALLED == 1
// test if the X11 server supports MIT-SHM
if(ui.shm_mode != 0)
  {
  if(XShmQueryVersion(xdis,&ShmMajor,&ShmMinor,&ShmPixmaps))
    {
    X11_accesstype=X11_MIT_SHM;
    }
  else
    {
    printf("\nThe parameter shm_mode in par_userint specifies MIT-SHM");
    printf("\nbut the X11 server does not support MIT-SHM \n");
    printf("Check your X11 configuration with the xdpyinfo command \n");
    printf("and try to enable MIT-SHM  by adding following lines\n");
    printf("to the /etc/X11/xorg.conf file :\n\n");
    printf("Section ""Extensions"" \n");
    printf("       Option ""MIT-SHM"" ""enable""  \n");
    printf("EndSection \n\n");
    goto shm_error;
    }
  }
#endif
visual=DefaultVisual(xdis, 0);
screen_num = DefaultScreen(xdis);
screen_width = ui.screen_width_factor*DisplayWidth(xdis, screen_num)/100;
screen_width &= -4;
screen_height = ui.screen_height_factor*DisplayHeight(xdis, screen_num)/100;
screen_totpix=screen_width*(screen_height+1);
// *****************************************************************
// Set the variables Linrad uses to access the screen.
init_font(ui.font_scale);
if(lir_errcod != 0)goto exitmain;
xwin=XCreateSimpleWindow(xdis, RootWindow(xdis, 0), 
                                    0, 0, screen_width, screen_height, 1, 0, 0);
// We want to know about the user clicking on the close window button
wm_delete_window = XInternAtom(xdis, "WM_DELETE_WINDOW", 0);
XSetWMProtocols(xdis,xwin, &wm_delete_window, 1);
cross_cursor = XCreateFontCursor(xdis, XC_diamond_cross);
// attach the icon cursor to our window
XDefineCursor(xdis, xwin, cross_cursor);
xgc=DefaultGC(xdis, 0);
color_depth = DefaultDepth(xdis, screen_num );
if(visual->class!=TrueColor && color_depth != 8)
  {
  printf("Unknown color type\n");
  exit(1);
  }
bitmap_pad=color_depth;  
switch (color_depth)
  {
  case 24:
  bitmap_pad=32;
  mempix_char=(unsigned char *)malloc((screen_totpix+1)*4);
  for(i=0; i<(screen_totpix+1)*4;i++)mempix_char[i]=0;
// ******************************************************************
// Rearrange the palette. It was designed for svgalib under Linux
  for(i=0; i<3*256; i++)
    {
    svga_palette[i]<<=2;
    if(svga_palette[i] != 0) svga_palette[i]|=3;
    }
  break;
  
  case 16:
  mempix_shi=(unsigned short int*)malloc((screen_totpix+1)*sizeof(unsigned short int));
  mempix_char=(void*)mempix_shi;
  for(i=0; i<screen_totpix+1; i++)mempix_shi[i]=0;
// ******************************************************************
  ipalette=(void*)(&svga_palette[0]);
  for(i=0; i<256; i++)
    {
    k=svga_palette[3*i+2];
    k&=0xfffe;
    k<<=5;
    k|=svga_palette[3*i+1];
    k&=0xfffe;
    k<<=6;
    k|=svga_palette[3*i  ];
    k>>=1;
    ipalette[i]=k;
    }
  break;
  
  case 8:
  mempix_char=(unsigned char*)malloc((screen_totpix+1)+256);
  defpix=(PIXINFO*)malloc(sizeof(PIXINFO)*256);
  lirpix=(PIXINFO*)malloc(sizeof(PIXINFO)*256);
  pixdiff=(float*)malloc(256*256*sizeof(float));
  xpalette=&mempix_char[screen_totpix+1];
  for(i=0; i<(screen_totpix+1)+256; i++)mempix_char[i]=0;
  lir_colormap=XCreateColormap(xdis, xwin, visual, AllocAll);
  default_colormap = DefaultColormap(xdis, screen_num);
// Store the default colormap in defpix
  for(id=0; id<256; id++)
    {
    xco.pixel=id;
    k=XQueryColor (xdis,default_colormap,&xco);  
    defpix[id].red=xco.red;
    defpix[id].green=xco.green;
    defpix[id].blue=xco.blue;
    defpix[id].flag=0;
    defpix[id].pixel=id;
    defpix[id].total=    ( (float)((unsigned int)defpix[id].red)*
                                  (unsigned int)defpix[id].red+
                          (float)((unsigned int)defpix[id].green)*
                                  (unsigned int)defpix[id].green+
                          (float)((unsigned int)defpix[id].blue)*
                                  (unsigned int)defpix[id].blue);
    }
// svga_palette uses the six lowest bits for the colour intensities.
// shift left by 10 to move our data to occupy the six highest bits.
// Store the svgalib palette.
  for(il=0; il<MAX_SVGA_PALETTE; il++)
    {
    lirpix[il].red=svga_palette[3*il+2]<<2;
    lirpix[il].green=svga_palette[3*il+1]<<2;
    lirpix[il].blue=svga_palette[3*il  ]<<2;
    if(lirpix[il].red != 0)lirpix[il].red|=3;
    if(lirpix[il].green != 0)lirpix[il].green|=3;
    if(lirpix[il].blue != 0)lirpix[il].blue|=3;
    lirpix[il].red<<=8;
    lirpix[il].green<<=8;
    lirpix[il].blue<<=8;
    lirpix[il].pixel=il;
    lirpix[il].flag=1;
    lirpix[il].total=   ( (float)((unsigned int)lirpix[il].red)*
                                  (unsigned int)lirpix[il].red+
                          (float)((unsigned int)lirpix[il].green)*
                                  (unsigned int)lirpix[il].green+
                          (float)((unsigned int)lirpix[il].blue)*
                                  (unsigned int)lirpix[il].blue);
    }
  for(il=MAX_SVGA_PALETTE; il<256; il++)
    {
    lirpix[il].red=0;
    lirpix[il].green=0;
    lirpix[il].blue=0;
    lirpix[il].pixel=il;
    lirpix[il].flag=0;
    }
#define M 0.00000001
#define N 0x100
// Sort lirpix in order of ascending total intensity.
  for(il=0; il<MAX_SVGA_PALETTE-1; il++)
    {
    t1=0;
    m=il;
    for(kl=il; kl<MAX_SVGA_PALETTE; kl++)
      {
      if(lirpix[kl].total > t1)
        {
        t1=lirpix[kl].total;
        m=kl;
        }
      }  
    tmppix=lirpix[il];
    lirpix[il]=lirpix[m];
    lirpix[m]=tmppix;
    }
// Compute the similarity between lirpix and defpix and store in a matrix.
  for(il=0; il<MAX_SVGA_PALETTE; il++)
    {
    for(id=0; id<256; id++)
      {
      t2=pow((float)(int)(((unsigned int)lirpix[il].red-
                      (unsigned int)defpix[id].red)),2.0)+
         pow((float)(int)(((unsigned int)lirpix[il].green-
                      (unsigned int)defpix[id].green)),2.0)+
         pow((float)(int)(((unsigned int)lirpix[il].blue-
                      (unsigned int)defpix[id].blue)),2.0);
      pixdiff[id+il*256]=t2;
      }
    }  
// Reorder the default colormap for the diagonal elements
// of pixdiff (up to MAX_SVGA_PALETTE-1) to become as small
// as possible when stepping in the ascending order that
// lirpix currently is sorted in.
  for(il=0; il<MAX_SVGA_PALETTE; il++)
    {
    t1=BIG;
    kd=0;
    for(id=0; id<256; id++)
      {
      if(pixdiff[id+il*256] < t1)
        {
        t1=pixdiff[id+il*256];
        kd=id;
        }
      }
    tmppix=defpix[il];
    defpix[il]=defpix[kd];
    defpix[kd]=tmppix;
    for(kl=0; kl<MAX_SVGA_PALETTE; kl++)
      {
      t1=pixdiff[kd+kl*256];
      pixdiff[kd+kl*256]=pixdiff[il+kl*256];
      pixdiff[il+kl*256]=t1;
      }
    }
  for(i=0; i<MAX_SVGA_PALETTE; i++)
    {
    xco.pixel=defpix[i].pixel;
    xco.red=lirpix[i].red;
    xco.green=lirpix[i].green;
    xco.blue=lirpix[i].blue;
    xco.flags=DoRed|DoGreen|DoBlue;
    xco.pad=0;
    k=XStoreColor(xdis, lir_colormap, &xco);  
    if(k==0)
      {
      printf("\nPalette failed\n");
      goto exitmain;
      }
    xpalette[lirpix[i].pixel]=xco.pixel;
    }
  for(i=MAX_SVGA_PALETTE; i<256; i++)
    {
    xco.pixel=defpix[i].pixel;
    xco.red=defpix[i].red;
    xco.green=defpix[i].green;
    xco.blue=defpix[i].blue;
    xco.flags=DoRed|DoGreen|DoBlue;
    xco.pad=0;
    k=XStoreColor(xdis, lir_colormap, &xco);  
    if(k==0)
      {
      printf("\nPalette failed\n");
      goto exitmain;
      }
    xpalette[i]=lirpix[i].pixel;
    }
  XSetWindowColormap(xdis, xwin, lir_colormap);
  free(defpix);
  free(lirpix);
  free(pixdiff);
  break;
  
  default:
  printf("\nUnknown color depth: %d\n",color_depth);
  goto exitmain;
  } 
if(X11_accesstype==X11_STANDARD)
  {
  ximage=XCreateImage(xdis, visual, color_depth, ZPixmap, 0, 
          (char*)(mempix_char), screen_width, screen_height+1, bitmap_pad, 0);
  }
#if SHM_INSTALLED == 1
else
  { 
  shminfo=(XShmSegmentInfo *)malloc(sizeof(XShmSegmentInfo));
  memset(shminfo,0, sizeof(XShmSegmentInfo));
  ximage =XShmCreateImage(xdis, visual, color_depth, ZPixmap,NULL, 
                           shminfo, screen_width, screen_height+1 );
  shminfo->shmid=shmget(IPC_PRIVATE,
		     ximage->bytes_per_line*
		     ximage->height,IPC_CREAT|0777);
  shminfo->shmaddr = ximage->data = shmat (shminfo->shmid, 0, 0);
// replace  address in mempix_char and mempix_shi by shared memory address 
  switch (color_depth)
    {
    case 24:
    free(mempix_char);
    mempix_char=(unsigned char *)(ximage->data);
    break;
  
    case 16:
    free(mempix_char);
    mempix_char=(unsigned char *)(ximage->data);
    mempix_shi=(void *)(ximage->data);
    break;

    case 8:
    free(mempix_char);
    mempix_char=(unsigned char *)(ximage->data);
    defpix=(PIXINFO*)malloc(sizeof(PIXINFO)*256);
    lirpix=(PIXINFO*)malloc(sizeof(PIXINFO)*256);
    pixdiff=(float*)malloc(256*256*sizeof(float));
    xpalette=&mempix_char[screen_totpix+1];
    for(i=0; i<(screen_totpix+1)+256; i++)mempix_char[i]=0;
    lir_colormap=XCreateColormap(xdis, xwin, visual, AllocAll);
    default_colormap = DefaultColormap(xdis, screen_num);
// Store the default colormap in defpix
    for(id=0; id<256; id++)
      {
      xco.pixel=id;
      k=XQueryColor (xdis,default_colormap,&xco);  
      defpix[id].red=xco.red;
      defpix[id].green=xco.green;
      defpix[id].blue=xco.blue;
      defpix[id].flag=0;
      defpix[id].pixel=id;
      defpix[id].total=    ( (float)((unsigned int)defpix[id].red)*
                                    (unsigned int)defpix[id].red+
                            (float)((unsigned int)defpix[id].green)*
                                    (unsigned int)defpix[id].green+
                            (float)((unsigned int)defpix[id].blue)*
                                    (unsigned int)defpix[id].blue);
      } 
// svga_palette uses the six lowest bits for the colour intensities.
// shift left by 10 to move our data to occupy the six highest bits.
// Store the svgalib palette.
    for(il=0; il<MAX_SVGA_PALETTE; il++)
      {
      lirpix[il].red=svga_palette[3*il+2]<<2;
      lirpix[il].green=svga_palette[3*il+1]<<2;
      lirpix[il].blue=svga_palette[3*il  ]<<2;
      if(lirpix[il].red != 0)lirpix[il].red|=3;
      if(lirpix[il].green != 0)lirpix[il].green|=3;
      if(lirpix[il].blue != 0)lirpix[il].blue|=3;
      lirpix[il].red<<=8;
      lirpix[il].green<<=8;
      lirpix[il].blue<<=8;
      lirpix[il].pixel=il;
      lirpix[il].flag=1;
      lirpix[il].total=   ( (float)((unsigned int)lirpix[il].red)*
                                    (unsigned int)lirpix[il].red+
                            (float)((unsigned int)lirpix[il].green)*
                                    (unsigned int)lirpix[il].green+
                            (float)((unsigned int)lirpix[il].blue)*
                                    (unsigned int)lirpix[il].blue);
      }
    for(il=MAX_SVGA_PALETTE; il<256; il++)
      {
      lirpix[il].red=0;
      lirpix[il].green=0;
      lirpix[il].blue=0;
      lirpix[il].pixel=il;
      lirpix[il].flag=0;
      }
#define M 0.00000001
#define N 0x100
// Sort lirpix in order of ascending total intensity.
    for(il=0; il<MAX_SVGA_PALETTE-1; il++)
      {
      t1=0;
      m=il;
      for(kl=il; kl<MAX_SVGA_PALETTE; kl++)
        {
        if(lirpix[kl].total > t1)
          {
          t1=lirpix[kl].total;
          m=kl;
          }
        }  
      tmppix=lirpix[il];
      lirpix[il]=lirpix[m];
      lirpix[m]=tmppix;
      }
// Compute the similarity between lirpix and defpix and store in a matrix.
    for(il=0; il<MAX_SVGA_PALETTE; il++)
      {
      for(id=0; id<256; id++)
        {
        t2=pow((float)(int)(((unsigned int)lirpix[il].red-
                        (unsigned int)defpix[id].red)),2.0)+
           pow((float)(int)(((unsigned int)lirpix[il].green-
                        (unsigned int)defpix[id].green)),2.0)+
           pow((float)(int)(((unsigned int)lirpix[il].blue-
                        (unsigned int)defpix[id].blue)),2.0);
        pixdiff[id+il*256]=t2;
        }
      }  
// Reorder the default colormap for the diagonal elements
// of pixdiff (up to MAX_SVGA_PALETTE-1) to become as small
// as possible when stepping in the ascending order that
// lirpix currently is sorted in.
    for(il=0; il<MAX_SVGA_PALETTE; il++)
      {
      t1=BIG;
      kd=0;
      for(id=0; id<256; id++)
        {
        if(pixdiff[id+il*256] < t1)
          {
          t1=pixdiff[id+il*256];
          kd=id;
          }
        }
      tmppix=defpix[il];
      defpix[il]=defpix[kd];
      defpix[kd]=tmppix;
      for(kl=0; kl<MAX_SVGA_PALETTE; kl++)
        {
        t1=pixdiff[kd+kl*256];
        pixdiff[kd+kl*256]=pixdiff[il+kl*256];
        pixdiff[il+kl*256]=t1;
        }
      }
    for(i=0; i<MAX_SVGA_PALETTE; i++)
      {
      xco.pixel=defpix[i].pixel;
      xco.red=lirpix[i].red;
      xco.green=lirpix[i].green;
      xco.blue=lirpix[i].blue;
      xco.flags=DoRed|DoGreen|DoBlue;
      xco.pad=0;
      k=XStoreColor(xdis, lir_colormap, &xco);  
      if(k==0)
        {
        printf("\nPalette failed\n");
        goto exitmain;
        }
      xpalette[lirpix[i].pixel]=xco.pixel;
      }
    for(i=MAX_SVGA_PALETTE; i<256; i++)
      {
      xco.pixel=defpix[i].pixel;
      xco.red=defpix[i].red;
      xco.green=defpix[i].green;
      xco.blue=defpix[i].blue;
      xco.flags=DoRed|DoGreen|DoBlue;
      xco.pad=0;
      k=XStoreColor(xdis, lir_colormap, &xco);  
      if(k==0)
        {
        printf("\nPalette failed\n");
        goto exitmain;
        }
      xpalette[i]=lirpix[i].pixel;
      }
    XSetWindowColormap(xdis, xwin, lir_colormap);
    free(defpix);
    free(lirpix);
    free(pixdiff);
 
    break;
    }
  shminfo->readOnly = False;
  XShmAttach(xdis,shminfo);
  }
#endif
XInitImage(ximage);
XSelectInput(xdis, xwin, 
             ButtonPressMask|
             ExposureMask|
             KeyPressMask|
             KeyReleaseMask| 
             ButtonReleaseMask|
             PointerMotionMask);
XMapWindow(xdis, xwin);
// Create a thread that will close the entire process in a controlled way
// in case lirerr() is called or the ESC key is pressed.
sem_init(&sem_kill_all,0,0);
pthread_create(&thread_identifier_kill_all,NULL,(void*)thread_kill_all, NULL);
i=check_mmx();
mmx_present=i&1;
if(mmx_present != 0)simd_present=i/2; else simd_present=0;
lir_status=LIR_OK;
lir_sem_init(SEM_KEYBOARD);
process_event_flag=TRUE;
pthread_create(&thread_identifier_process_event,NULL,
                                        (void*)thread_process_event, NULL);
while(expose_event_done == FALSE)
  {
  lir_sleep(1000);
  }
lir_sem_init(SEM_MOUSE);
pthread_create(&thread_identifier_mouse,NULL, (void*)thread_mouse, NULL);
// Create a thread for the main menu.
users_open_devices();
if(kill_all_flag) goto skipmenu;
pthread_create(&thread_identifier_main_menu,NULL,
                                              (void*)thread_main_menu, NULL);
pthread_join(thread_identifier_main_menu,0);
skipmenu:;
sem_post(&sem_kill_all);
pthread_join(thread_identifier_kill_all,0);
lir_remove_mouse_thread();
pthread_join(thread_identifier_mouse,0);
show_errmsg(1);
lir_refresh_screen();
XSync(xdis,True);
process_event_flag=FALSE;
pthread_join(thread_identifier_process_event,0);
XFreeCursor(xdis, cross_cursor);
if(X11_accesstype==X11_STANDARD)
  {
  free(mempix_char);
  }
#if SHM_INSTALLED == 1
else
  { 
  XShmDetach(xdis,shminfo);
  shmdt(shminfo->shmaddr);
  shmctl(shminfo->shmid,IPC_RMID,NULL);
  free(shminfo);
  }
#endif 	
exitmain:;
users_close_devices();
lir_close_serport();
free(vga_font);
#if SHM_INSTALLED == 1
shm_error:;
#endif
XCloseDisplay(xdis);
free(keyboard_buffer);
if(dmp!=NULL)fclose(dmp);
return lir_errcod;
}

void thread_process_event(void)
{
int chr;
int cc, mx, my;
XEvent ev;
char key_buff[16];
int count, m;
KeySym ks;
int mbutton_state;
mbutton_state=0;
expose_time=current_time();
while(process_event_flag)
  {
  XNextEvent(xdis, &ev);
  switch(ev.type)
    {
// We want to know about the user clicking on the close window button
    case ClientMessage:
    if ( ev.xclient.data.l[0] == (int)(wm_delete_window) )
      {
      printf ( "Shutting down now!!!\n" );
      chr = X_ESC_SYM;
      sem_post(&sem_kill_all);
      store_in_kbdbuf(0);
      return;
      } 
    break;

    case Expose:
    expose_time=recent_time;
    if (ev.xexpose.count != 0)break;
    if( thread_status_flag[THREAD_SCREEN] != THRFLAG_SEM_WAIT &&
        thread_status_flag[THREAD_SCREEN] != THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN] != THRFLAG_IDLE)
      {
      if(X11_accesstype==X11_STANDARD)
        {
        XPutImage(xdis, xwin, xgc, ximage,0,0,0,0, screen_width, screen_height);
        }
#if SHM_INSTALLED == 1
      else
        { 
        XShmPutImage (xdis, xwin, xgc, ximage, 0, 0, 0, 0, screen_width, screen_height, FALSE);
        }
#endif
      }
    if(color_depth==8)XInstallColormap(xdis, lir_colormap);
    expose_event_done=TRUE;
    break;

    case ButtonPress:
    if ( (ev.xbutton.button == Button1) != 0)
      {
      new_lbutton_state=1;
      goto mousepost;
      }
    if ( (ev.xbutton.button == Button3) != 0) 
      {
      new_rbutton_state=1;
      goto mousepost;
      }
    if ( (ev.xbutton.button == Button2) != 0)
      {
      mbutton_state=1;
      }
    if(mbutton_state==0)
      {  
      if ( (ev.xbutton.button == Button5) != 0)
        {
        step_rx_frequency(1);
        }
      if ( (ev.xbutton.button == Button4) != 0)
        {
        step_rx_frequency(-1);
        }
      }
    else
      {
      m=bg.wheel_stepn;
      if ( (ev.xbutton.button == Button5) != 0)
        {
        m++;
        if(m>30)m=30;
        }
      if ( (ev.xbutton.button == Button4) != 0)
        {
        m=bg.wheel_stepn;
        m--;
        if(m<-30)m=-30;
        if(genparm[AFC_ENABLE]==0 && m<0)m=0;
        }
      bg.wheel_stepn=m;
      sc[SC_SHOW_WHEEL]++;
      make_modepar_file(GRAPHTYPE_BG);
      }
    break;

    case ButtonRelease:
    if ( (ev.xbutton.button == Button1) != 0)
      {
      new_lbutton_state=0;
      goto mousepost;
      }
    if ( (ev.xbutton.button == Button3) != 0) 
      {
      new_rbutton_state=0;
      goto mousepost;
      }
    if ( (ev.xbutton.button == Button2) != 0)
      {
      mbutton_state=0;
      }
    break;

    case MotionNotify:
    mx=new_mouse_x;
    my=new_mouse_y;
    new_mouse_x= ev.xbutton.x;
    if(new_mouse_x < 0)new_mouse_x=0;
    if(new_mouse_x >= screen_width)new_mouse_x=screen_width-1;
    new_mouse_y= ev.xbutton.y;
    if(new_mouse_y < 0)new_mouse_y=0;
    if(new_mouse_y >= screen_height)new_mouse_y=screen_height-1;
    if(  mx == new_mouse_x &&   my==new_mouse_y)break;
    if( (mx == new_mouse_x && new_mouse_x == screen_width-1) || 
        (my == new_mouse_y && new_mouse_y == screen_height-1)|| 
        (mx == new_mouse_x && new_mouse_x == 0) || 
        (my == new_mouse_y && new_mouse_y == 0) )break;
mousepost:;
    lir_sem_post(SEM_MOUSE);
    break;
  
    case KeyPress:
    chr = XKeycodeToKeysym(xdis, ev.xkey.keycode, 0);
    if(newcomer_escflag)
      {
      cc=toupper(chr);
      if(cc=='Y')goto escexit;
      if(cc!='N')break;
      newcomer_escpress(1);
      newcomer_escflag=FALSE;
      break;
      }
    if(chr == X_ESC_SYM)
      {
// The ESC key was pressed.
      if(ui.newcomer_mode != 0)
        {
        newcomer_escpress(0);
        newcomer_escflag=TRUE;
        break;
        }
escexit:;
      sem_post(&sem_kill_all);
      store_in_kbdbuf(0);
      return;
      }    
    if(chr == X_SHIFT_SYM_L || chr == X_SHIFT_SYM_R)
      {
      shift_key_status=1;
      break;
      }
// Get ASCII codes from Dec 32 to Dec 127
    count = XLookupString((XKeyEvent *)&ev, key_buff, 2, &ks,NULL);
    if ((count == 1) && ((int)key_buff[0]>31) )
      {
      store_in_kbdbuf((int)key_buff[0]);
      break;
      }
    cc=0;
    if(chr >= X_F1_SYM && chr <= X_F12_SYM)
      {
      if(chr <= X_F10_SYM)
        {
        if(shift_key_status == 0)
          {
          cc=F1_KEY+chr-X_F1_SYM;
          }
        else  
          {
          cc=SHIFT_F1_KEY+chr-X_F1_SYM;
          if(chr > X_F2_SYM)cc++;
          if(chr >= X_F9_SYM)cc=0;
          }
        }
      else
        {
        if(shift_key_status == 0)
          {
          cc=F11_KEY+chr-X_F11_SYM;
          }
        }  
      if(cc != 0)store_in_kbdbuf(cc);        
      break;
      }
    switch (chr)
      {
      case X_UP_SYM:
      cc=ARROW_UP_KEY;
      break;

      case X_DWN_SYM:
      cc=ARROW_DOWN_KEY;
      break;

      case X_RIGHT_SYM:
      cc=ARROW_RIGHT_KEY;
      break;

      case  X_BACKDEL_SYM:
      case X_LEFT_SYM:
      cc=ARROW_LEFT_KEY;
      break;
      
      case X_HOME_SYM:
      cc=HOME_KEY;
      break;
      
      case X_INSERT_SYM:
      cc=INSERT_KEY;
      break;
      
      case X_DELETE_SYM:
      cc=DELETE_KEY;
      break;
      
      case X_END_SYM:
      cc=END_KEY;
      break;
      
      case X_PGUP_SYM:
      cc=PAGE_UP_KEY;
      break;

      case X_PGDN_SYM:
      cc=PAGE_DOWN_KEY;
      break;
      
      case X_PAUSE_SYM:
      cc=PAUSE_KEY;
      break;
      
      case X_ENTER_SYM:
      cc=10;
      break;

      }
    if(cc != 0)store_in_kbdbuf(cc);        
    break;

    case KeyRelease:
    chr=XKeycodeToKeysym(xdis, ev.xkey.keycode, 0);
    if(chr == X_SHIFT_SYM_L || chr == X_SHIFT_SYM_R)
      {
      shift_key_status=0;
      }
    break;        
    }
  }
}

void ui_setup(void)
{
FILE *file;
int i, j, k;
int xxprint;
char s[10];
char chr;
int *uiparm;
char *parinfo;
uiparm=(int*)(&ui);
parinfo=NULL;
xxprint=investigate_cpu();
file = fopen(userint_filename, "rb");
if (file == NULL)
  {
  print_procerr(xxprint);
  printf("\n\nSetup file %s missing.",userint_filename);
full_setup:;
  for(i=0; i<MAX_UIPARM; i++) uiparm[i]=0;
  printf("\nUse W to create a new %s file after setup.\n\n",userint_filename);
  printf(
    "Press S for setup routines in normal mode or press N for NEWCOMER mode");
  printf("\nAny other key to exit. (You might want to manually edit %s)",
                                                       userint_filename);
  printf("\nThen press enter\n\n=>");
  while(fgets(s,8,stdin)==NULL);
  chr=toupper(s[0]);
  if(chr != 'S' && chr != 'N') exit(0);
  if(chr == 'N')
    {
    ui.newcomer_mode=1;
    }
  else
    {
    ui.newcomer_mode=0;
    }
  x_global_uiparms(0);
  ui.rx_input_mode=-1;
  ui.tx_dadev_no=-1;
  ui.rx_dadev_no=-1;
  uiparm_change_flag=TRUE;
  }
else
  {
  if(parinfo==NULL)
    {
    parinfo=malloc(4096);
    if(parinfo==NULL)
      {
      lirerr(1078);
      return;
      }
    }
  for(i=0; i<4096; i++) parinfo[i]=0;
  i=fread(parinfo,1,4095,file);
  fclose(file);
  file=NULL;
  if(i >= 4095)
    {
    goto go_full_setup;
    }
  k=0;
  for(i=0; i<MAX_UIPARM; i++)
    {
    while(parinfo[k]==' ' ||
          parinfo[k]== '\n' )k++;
    j=0;
    while(parinfo[k]== uiparm_text[i][j] && k<4096)
      {
      k++;
      j++;
      } 
    if(uiparm_text[i][j] != 0)goto go_full_setup;
    while(parinfo[k]!='[' && k<4096)k++;
    sscanf(&parinfo[k],"[%d]",&uiparm[i]);
    while(parinfo[k]!='\n')k++;
    }
  if( ui.font_scale < 1 || 
      ui.font_scale > 5 ||
      ui.check != UI_VERNR ||
      ui.screen_width_factor < 33 || 
      ui.screen_width_factor > 100 ||
      ui.screen_height_factor <33 || 
      ui.screen_height_factor > 100 ||
      ui.newcomer_mode <0 ||
      ui.newcomer_mode >1)
    {
go_full_setup:;
    printf("\n\nSetup file %s has errors",userint_filename);
    goto full_setup;
    }
  uiparm_change_flag=FALSE;
  free(parinfo);
  }
}

