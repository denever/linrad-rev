
#include <stdio.h>
#include "wdef.h"
#include "globdef.h"
#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "fft3def.h"
#include "screendef.h"
#include "sigdef.h"
#include "seldef.h"
#include "blnkdef.h"
#include "caldef.h"
#include "txdef.h"
#include "vernr.h"
#include "keyboard_def.h"
#include "thrdef.h"
#include "hwaredef.h"

void thread_rx_raw_netinput(void);
void thread_rx_fft1_netinput(void);
void thread_lir_server(void);

static DCB old_serDCB;
char serport_name[]="COM?";

// Parameters to generate quasi-random numbers using the
// The Linear Congruential Method, first introduced by D. Lehmer in 1951.
#define RANDOM_MOD 2147483647
#define RANDOM_FACT 16807
int win_random_seed=4711;

void lir_mutex_lock(void){};
void lir_mutex_unlock(void){};
void lir_mutex_init(void){};
void lir_mutex_destroy(void){};

float lir_noisegen(int level) 
{
return 0*level;
}

void lir_set_title(char *s)
{
char cc;
cc=s[0];
}

void lir_sync(void)
{
// This routine is called to force a write to the hard disk under Linux.
// Under Windows, some of the files are opened with the 'c' flag to ensure
// that data is really put to the disk when fflush is called.
}

float lir_random(void)
{
__int64 n;
n=(__int64)(RANDOM_FACT)*(__int64)(win_random_seed);
n=n%(__int64)(RANDOM_MOD);
win_random_seed=n;
return (float)(n)/RANDOM_MOD;
}

void lir_srandom(void)
{
win_random_seed=current_time();
}

double lir_get_thread_time(int no)
{
double t1, t2;
FILETIME t_create, t_exit, t_kernel, t_user;
GetThreadTimes(thread_identifier[no], &t_create, &t_exit, &t_kernel, &t_user);
t1=t_kernel.dwLowDateTime*(0.5/0x80000000)+t_kernel.dwHighDateTime;
t2=t_user.dwLowDateTime*(0.5/0x80000000)+t_user.dwHighDateTime;
// Convert filetime to seconds with the factor 429.5 
return 429.5*(t1+t2);
}

void lir_system_times(double *cpu_time, double *total_time)
{
__int64 crea, iexit, pkern, puser;
int res;
res=GetProcessTimes(CurrentProcess, (FILETIME*)&crea,
                    (FILETIME*)&iexit, (FILETIME*)&pkern, (FILETIME*)&puser);
if(res==0)
  {
  total_time[0]=0;
  cpu_time[0]=0;
  return;
  }
total_time[0]=no_of_processors*current_time();
cpu_time[0]=0.0000001*((double)((__int64)puser)+(double)(__int64)pkern);
}

int lir_get_epoch_seconds(void)
{
// Here we have to add a calendar to add the number
// of seconds from todays (year, month, day) to Jan 1 1970.
// The epoch time is needed for moon position computations.
SYSTEMTIME tim;
int k, days, secs;
unsigned char days_per_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
GetSystemTime(&tim);
secs=3600.*tim.wHour+60.*tim.wMinute+tim.wSecond;
days=tim.wDay-1;
k=tim.wMonth-2;
while( k >= 0 )
  {
  days+=days_per_month[k];
  k--;
  }
days+=(tim.wYear-1970)*365;  
// Leap years are every 4th year. Add the number of completed leap years.
days+=(tim.wYear-1969)/4;
// Do not worry about year 2100. This code will presumably
// not live until then......
// Add one more day if the current year is a leap year and if date is above
// February 28. 
if(tim.wYear%4 == 0)
  {
  if(tim.wMonth >= 3)
    {
    days++;
    }
  }  
return days*86400+secs;
}


int lir_open_serport(int serport_number, int baudrate,int stopbit_flag)
{
static COMSTAT Status;
static unsigned long lpErrors;
static DCB serDCB;
int rc;
COMMTIMEOUTS timeouts;
rc=0;
if(serport != INVALID_HANDLE_VALUE)return rc;
// Get a file handle to the comm port
if(serport_number < 1 || serport_number > 8)
  {
//  lirerr(1279);
  rc= 1279;
  return rc;
  }
sprintf(&serport_name[3],"%d",serport_number);
serport=CreateFile(serport_name,GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
// Check if valid
if (serport == INVALID_HANDLE_VALUE) 
  {
//  lirerr(1244);
  rc=1244;
  return rc;
  }
// Get serial port parameters - will overwrite baud rate, number of bits,
// parity and stop bits
if (!GetCommState(serport,&serDCB))
  {
//  lirerr(1277);
  rc=1277;
  goto close_x;
  }
switch ( baudrate )
  {
  case 110:
  serDCB.BaudRate= CBR_110; 
  break;
  
  case 300: 
  serDCB.BaudRate= CBR_300;
  break;
  
  case 600: 
  serDCB.BaudRate= CBR_600; 
  break; 
  
  case 1200: 
  serDCB.BaudRate= CBR_1200; 
  break;
  
  case 2400: 
  serDCB.BaudRate= CBR_2400; 
  break;
  
  case 4800: 
  serDCB.BaudRate= CBR_4800; 
  break;
  
  case 9600: 
  serDCB.BaudRate= CBR_9600; 
  break;
  
  case 19200: 
  serDCB.BaudRate= CBR_19200; 
  break;
  
  case 38400: 
  serDCB.BaudRate= CBR_38400; 
  break;
  
  case 57600: 
  serDCB.BaudRate= CBR_57600; 
  break;
  
  default: 
//  lirerr(1280);
  rc=1280;
  goto close_x;
  }
old_serDCB=serDCB;
serDCB.ByteSize=8;
serDCB.Parity=NOPARITY;
serDCB.StopBits=TWOSTOPBITS;
if(stopbit_flag)
  {
  serDCB.StopBits=TWOSTOPBITS;
  }
else
  {  
  serDCB.StopBits=ONESTOPBIT; 
  }
// Tell the device we want 4096 buffers in both directions.
SetupComm(serport,4096,4096);
// Write changes back
if (!SetCommState(serport,&serDCB))
  {
//  lirerr(1278);
  rc=1278;
  goto close_x;
  }
//Set timeouts
//how long (in mseconds) to wait between receiving characters before timing out.
timeouts.ReadIntervalTimeout=50;
//how long to wait (in mseconds) before returning.
timeouts.ReadTotalTimeoutConstant=50;
//how much additional time to wait (in mseconds) before
//returning for each byte that was requested in the read operation
timeouts.ReadTotalTimeoutMultiplier=10;
timeouts.WriteTotalTimeoutConstant=50;
timeouts.WriteTotalTimeoutMultiplier=10;
if(!SetCommTimeouts(serport, &timeouts)){
//  lirerr(1278);
  rc=1278;
  goto close_x;
}

// Set RTS or clear the RTS signal.
if (!EscapeCommFunction(serport, SETRTS))  // RTS
//if (!EscapeCommFunction(serport, CLRRTS))  //  Clear the RTS signal
  {
//  lirerr(1278);
  rc=1278;
close_x:
  CloseHandle(serport);
  serport=INVALID_HANDLE_VALUE;
  return rc ;
  }

ClearCommError(serport, &lpErrors, &Status);
return rc;
}

void lir_close_serport(void)
{
if(serport == INVALID_HANDLE_VALUE)return;
if(!SetCommState(serport,&old_serDCB))lirerr(1278);
CloseHandle(serport);
serport=INVALID_HANDLE_VALUE;
return ;
}

int lir_write_serport(void *s, int bytes)
{
unsigned long iBytesWritten;
if (WriteFile(serport,s,bytes,&iBytesWritten,NULL))
  {
  return 0;
  }
else
  {
  return -1;
  }
}

int lir_read_serport(void *s, int bytes)
{
unsigned long iBytesRead;
int nread = 0;
while (bytes > 0) {
if (ReadFile(serport,(char *)(s + nread),bytes,&iBytesRead,NULL))
  {
 if (iBytesRead == 0) return -1;
 nread += iBytesRead;
 bytes -= iBytesRead;
  }
else
  {
  return -1;
  }
}
return nread;
}

int ms_since_midnight(int set)
{
int i;
SYSTEMTIME tim;
if(set)netstart_time=current_time();
GetSystemTime(&tim);
i=3600000.*tim.wHour+60000.*tim.wMinute+1000.*tim.wSecond+tim.wMilliseconds;
return i;
}

void win_semaphores(void)
{
long old;
if(rxin1_bufready != NULL)
  {
  ReleaseSemaphore(rxin1_bufready,1,&old);
  }
}

void lir_sched_yield(void)
{
Sleep(0);
};


int lir_parport_permission(void)
{
if(parport_installed)
  {
  parport_ack_sign=0;
  switch (ui.parport_pin)
    {
    case 15:
    parport_ack=8;
    break;
    
    case 13:
    parport_ack=16;
    break;
    
    case 10:
    parport_ack=64;
    break;
    
    case 11:
    parport_ack=128;
    parport_ack_sign=128;
    break;
    }
  parport_status=ui.parport+1;
  parport_control=ui.parport+2;
  return TRUE;
  }
else
  {
  return FALSE;
  }  
}

void lir_sleep(int us)
{
int i;
i=us/1000;
if(i == 0 || i > 10)
  {
  Sleep(i);
  }
else
  {
  timeBeginPeriod(i);
  Sleep(i);
  timeEndPeriod(i);
  }
}

void lir_sem_post(int no)
{
long old;
ReleaseSemaphore(lirsem[no],1,&old);
}

void lir_sem_wait(int no)
{
WaitForSingleObject(lirsem[no],INFINITE);
}

void lir_sem_init(int no)
{
if(lirsem[no] != NULL)
  {
  lirerr(83456);
  return;
  }
lirsem[no]=CreateSemaphore( NULL, 0, 1024, NULL);
if(lirsem[no]==NULL)lirerr(729532);
}

void lir_sem_free(int no)
{
if(lirsem[no] == NULL)
  {
  lirerr(672319);
  return;
  }
CloseHandle(lirsem[no]);
lirsem[no]=NULL;
}

void linrad_thread_create(int no)
{
char *phony;
DWORD dummy;
phony=(void*)dummy;
thread_status_flag[no]=THRFLAG_INIT;
thread_command_flag[no]=THRFLAG_ACTIVE;
thread_identifier[no]=CreateThread(NULL, 0, thread_routines[no], 
                                           (PVOID)phony,0, &dummy);
if(thread_identifier[no] == NULL)lirerr(9871201);
threads_running=TRUE;
}

double current_time(void)
{
LARGE_INTEGER lrgi;
double dt1;
if(QueryPerformanceCounter(&lrgi)==0)
  {
  lirerr(10002);
  return 0;
  }
dt1=lrgi.HighPart-internal_clock_offset;
dt1*=(float)(0x10000)*(float)(0x10000);
dt1+=lrgi.LowPart;
recent_time=dt1/internal_clock_frequency;
return recent_time;
}

int toupper(int c)
{
if(c>96 && c<123)c-=32;
return c;
}

void lirerr(int errcod)
{
long old;
if(kill_all_flag) return;
DEB"\nlirerr(%d)",errcod);
if(dmp != 0)fflush(dmp);
lir_errcod=errcod;
ReleaseSemaphore(sem_kill_all,1,&old);
lir_sleep(100000);
}

char lir_inb(int port)
{
return (inp32)(port);
}

void lir_outb(char bytedat, int port)
{
(oup32)(port,bytedat);
}

void lir_join(int no)
{
if(thread_command_flag[no]==THRFLAG_NOT_ACTIVE)return;
WaitForSingleObject(thread_identifier[no],INFINITE);
thread_status_flag[no]=THRFLAG_NOT_ACTIVE;
}

// *************************************************************
// Set global parameters and the corresponding dummy routines.
// *************************************************************

void win_global_uiparms(int wn)
{
char s[80], ss[80], st[80], sr[80], su[80];
int line;
line=0;
ui.vga_mode=12;
ui.mouse_speed=8;
if(ui.newcomer_mode!=0)
  {
  ui.font_scale=2;
  ui.process_priority=1;
  ui.parport=0;
  ui.parport_pin=0;
  ui.max_blocked_cpus=0;
  }
else
  {    
  sprintf(s,"Enter font scale (1 to 5)"); 
fntc:;
  if(wn==0)
    {
    printf("\n%s, then press Enter: ",s); 
    fgets(ss,8,stdin);
    lir_inkey=toupper(ss[0]);
    }
  else
    {
    clear_screen();
    if(screen_width > 640)ui.vga_mode=11;
    if(screen_width > 800)ui.vga_mode=12;
    lir_text(0,line,s);
    toupper_await_keyboard();
    if(kill_all_flag) return;
    line++;
    if(line>=screen_last_line)line--;
    }
  if(lir_inkey < '1' || lir_inkey > '5')goto fntc;
  ui.font_scale=lir_inkey-'0';
  sprintf(s,"Set process priority (0=NORMAL to 3=REALTIME)"); 
prio:;
  if(wn==0)
    {
    printf("\n%s, then press Enter: ",s); 
    fgets(ss,8,stdin);
    lir_inkey=toupper(ss[0]);
    }  
  else
    {
    clear_screen();
    lir_text(0,7,s);
    toupper_await_keyboard();
    }
  if(lir_inkey >='0' && lir_inkey <= '3')
    {
    ui.process_priority=lir_inkey-'0';
    }
  else
    {
    goto prio;
    }
  sprintf(s,"Parport address (lpt1=888, none=0):"); 
parport_gtnum:;
  ui.vga_mode=10;
  if(wn==0)
    {
    printf("\n%s\n=>",s); 
    fgets(ss,8,stdin);
    sscanf(ss,"%d", &ui.parport);
    if(ui.parport < 0)goto parport_gtnum;
    }
  else
    {
    clear_screen();
    lir_text(0,3,s);
    ui.parport=lir_get_integer(36,3,5,0,99999);
    }
  if(ui.parport != 0)
    {
    sprintf (s,"Parport read pin (ACK=10):");
gtpin:;
    if(wn==0)
      {
      printf("\n%s\n=>",s); 
      fgets(ss,8,stdin);
      sscanf(ss,"%d", &ui.parport_pin);
      }
    else
      {
      clear_screen();
      lir_text(0,4,s);
      ui.parport_pin=lir_get_integer(27,4,2,10,15);
      }
    if( ui.parport_pin ==14 ||
            ui.parport_pin > 15 ||
                ui.parport_pin < 10) goto gtpin;
    }    
  else
    {
    ui.parport_pin=0;
    }
  if(no_of_processors > 1)
    {
    sprintf(s,"This system has % d processors.",no_of_processors);
    sprintf(ss,"How many do you allow Linrad to block?");
    sprintf(sr,
        "If you run several instances of Linrad on one multiprocessor");
    sprintf(st,"platform it may be a bad idea to allow the total number");
    sprintf(su,"of blocked CPUs to be more that the total number less one.");        
    if(wn==0)
      {
      printf("\n%s",s); 
      printf("\n%s",ss); 
      printf("\n%s",sr); 
      printf("\n%s",st); 
      printf("\n%s\n\n=>",su); 
      while(fgets(ss,8,stdin)==NULL);
      sscanf(ss,"%d", &ui.max_blocked_cpus);
      if(ui.max_blocked_cpus <0)ui.max_blocked_cpus=0;
      if(ui.max_blocked_cpus >=no_of_processors)
                                          ui.max_blocked_cpus=no_of_processors;
      }
    else
      {
      line++;
      lir_text(0,line,s);
      line++;
      lir_text(0,line,ss);
      line++;
      lir_text(0,line,sr);
      line++;
      lir_text(0,line,st);
      line++;
      lir_text(0,line,su);
      line+=2;
      lir_text(0,line,"=>");
      ui.max_blocked_cpus=lir_get_integer(3,line,2,0,no_of_processors-1);
      }
    }    
  else
    {
    ui.max_blocked_cpus=0;
    }  
  }
sprintf(s,"Percentage of screen width to use:"); 
parport_wfac:;
if(wn==0)
  {
  printf("\n%s\n=>",s); 
  fgets(ss,8,stdin);
  sscanf(ss,"%d", &ui.screen_width_factor);
  if(ui.screen_width_factor < 25 ||
     ui.screen_width_factor > 100)goto parport_wfac;
  }
else
  {
  clear_screen();
  lir_text(0,6,s);
  ui.screen_width_factor=lir_get_integer(36,6,3,0,100);
  }
sprintf(s,"Percentage of screen height to use:"); 
parport_hfac:;
if(wn==0)
  {
  printf("\n%s\n=>",s); 
  fgets(ss,8,stdin);
  sscanf(ss,"%d", &ui.screen_height_factor);
  if(ui.screen_height_factor < 25 ||
     ui.screen_height_factor > 100)goto parport_hfac;
  printf("\n\nLinrad will now open another window.");
  printf("\nMinimize this window and click on the new window to continue.");
  printf(
      "\n\nDo not forget to save your parameters with 'W' in the main menu");
  fflush(NULL);
  }
else
  {
  clear_screen();
  lir_text(0,7,s);
  ui.screen_height_factor=lir_get_integer(36,7,3,0,100);
  }
if(wn != 0)
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
  clear_screen();
  settextcolor(15);
  lir_text(0,3,"Save the new parameters with 'W' in the main menu.");
  lir_text(0,5,"Then exit and restart Linrad for any new");
  lir_text(0,6,"screen parameter to take effect.");
  lir_text(5,8,press_any_key);
  await_keyboard();
  clear_screen();
  settextcolor(7);
  }
}

void x_global_uiparms(int wn)
{
lir_inkey=wn;
}

void lin_global_uiparms(int wn)
{
lir_inkey=wn;
}

// *********************************************************
// Graphics for Windows
// *********************************************************

void lir_getpalettecolor(int j, int *r, int *g, int *b)
{
r[0]=(int)svga_palette[3*j]>>2;
g[0]=(int)svga_palette[3*j+1]>>2;
b[0]=(int)svga_palette[3*j+2]>>2;
}

void lir_fillbox(int x, int y,int w, int h, unsigned char c)
{
int i, j, k;
k=x+(screen_height-y)*screen_width;
if(k-h*screen_width<0 || k+w > screen_totpix)lirerr(1213);
if(last_mempix < k+h-1)last_mempix=k+h-1;
for(j=0; j<h; j++)
  {
  for(i=0; i<w; i++)mempix[k+i]=c;
  k-=screen_width;
  }
k+=screen_width;
if(first_mempix > k)first_mempix=k;
}

void lir_getbox(int x, int y, int w, int h, void* dp)
{
unsigned char *savmem;
int i, j, k, m;
k=x+(screen_height-y)*screen_width;
if(k-h*screen_width<0 || k+w > screen_totpix)lirerr(1212);
m=0;
savmem=(unsigned char*)dp;
for(j=0; j<h; j++)
  {
  for(i=0; i<w; i++)
    {
    savmem[m]=mempix[k+i];
    m++;
    }
  k-=screen_width;
  }
}

void lir_putbox(int x, int y, int w, int h, void* dp)
{
unsigned char *savmem;
int i, j, k, m;
k=x+(screen_height-y)*screen_width;
if(k-h*screen_width<0 || k+w > screen_totpix)lirerr(1211);
if(last_mempix < k+h-1)last_mempix=k+h-1;
m=0;
savmem=(unsigned char*)dp;
for(j=0; j<h; j++)
  {
  for(i=0; i<w; i++)
    {
    mempix[k+i]=savmem[m];
    m++;
    }
  k-=screen_width;
  }
k+=screen_width;
if(first_mempix > k)first_mempix=k;
}

void lir_hline(int x1, int y, int x2, unsigned char c)
{
int i, ia, ib;
ia=(screen_height-y)*screen_width;
if(x1 <= x2)
  {
  ib=ia+x2;
  ia+=x1;
  }
else
  {
  ib=ia+x1;
  ia+=x2;
  }
if(ia < 0 || ib >= screen_totpix)
  {
  lirerr(1214);
  return;
  }
for(i=ia; i<=ib; i++)mempix[i]=c;
if(first_mempix > ia)first_mempix=ia;
if(last_mempix < ib)last_mempix=ib;
}

void lir_line(int x1, int yy1, int x2,int y2, unsigned char c)
{
int ia;
int i,j,k;
int xd, yd;
float t1,t2,delt;
xd=x2-x1;
yd=y2-yy1;
if(yd==0)
  {
  if(xd==0)
    {
    lir_setpixel(x1,yy1,c);
    }
  else
    {
    lir_hline(x1,yy1,x2,c);
    }
  return;  
  }  
if(abs(xd)>=abs(yd))
  {
  if(xd>=0)
    {
    k=1;
    }
  else
    {
    k=-1;
    }  
  if(yd >= 0)
    {
    delt=0.5;
    }
  else
    {  
    delt=-0.5;
    }
  t1=yy1;
  t2=(float)(yd)/abs(xd);
  i=x1-k;
  while(i!=x2)
    {
    i+=k;
    j=t1+delt;
    ia=i+(screen_height-j)*(screen_width);
    if(ia < 0 || ia >= screen_totpix)
      {
      lirerr(10008);
      return;
      }
    mempix[ia]=c;
    if(first_mempix > ia)first_mempix=ia;
    if(last_mempix < ia)last_mempix=ia;
    t1+=t2;
    }
  }  
else
  {
  if(yd>=0)
    {
    k=1;
    }
  else
    {
    k=-1;
    } 
  if(xd >= 0)
    {
    delt=0.5;
    }
  else
    {  
    delt=-0.5;
    }
  t1=x1;
  t2=(float)(xd)/abs(yd);
  i=yy1-k;
  while(i!=y2)
    {
    i+=k;
    j=t1+delt;
    ia=j+(screen_height-i)*(screen_width);
    if(ia < 0 || ia >= screen_totpix)
      {
      lirerr(10007);
      return;
      }
    mempix[ia]=c;
    if(first_mempix > ia)first_mempix=ia;
    if(last_mempix < ia)last_mempix=ia;
    t1+=t2;
    }
  }  
}

void lir_setpixel(int x, int y, unsigned char c)
{
int ia;
ia=x+(screen_height-y)*screen_width;
if(ia < 0 || ia >= screen_totpix)lirerr(1210);
mempix[ia]=c;
if(first_mempix > ia)first_mempix=ia;
if(last_mempix < ia)last_mempix=ia;
}

void clear_screen(void)
{
BitBlt(memory_hdc, 0, 0, screen_width, screen_height, NULL, 0, 0, BLACKNESS);
lir_refresh_entire_screen();
lir_refresh_screen();
}

void lir_refresh_entire_screen(void)
{
first_mempix=0;
last_mempix=screen_totpix-1;
}

void lir_refresh_screen(void)
{
int l1, l2;
if(last_mempix >= first_mempix)
  {
  l1=screen_height-last_mempix/screen_width;
  l2=screen_height-first_mempix/screen_width+1;
  first_mempix=0x7fffffff;
  last_mempix=0;
  HDC hdc = GetDC(linrad_hwnd);
  BitBlt(hdc, 0, l1, screen_width, l2-l1, memory_hdc, 0, l1, SRCCOPY);
  ReleaseDC(linrad_hwnd, hdc);
  }
}

// *********************************************************************
// Thread entries for Windows
// *********************************************************************

DWORD WINAPI winthread_lir_server(PVOID arg)
{
char *c;
c=arg;
thread_lir_server();
return 100;
}

DWORD WINAPI winthread_main_menu(PVOID arg)
{
char *c;
c=arg;
main_menu();
return 100;
}

DWORD WINAPI winthread_kill_all(PVOID arg)
{
char *c;
c=arg;
WaitForSingleObject(sem_kill_all,INFINITE);
kill_all();
clear_keyboard();
show_errmsg(1);
return 100;
}

DWORD WINAPI winthread_tune(PVOID arg)
{
char *c;
c=arg;
tune();
return 100;
}

DWORD WINAPI winthread_sdr14_input(PVOID arg)
{
char *c;
c=arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
sdr14_input();
return 100;
}

DWORD WINAPI winthread_perseus_input(PVOID arg)
{
char *c;
c=arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
perseus_input();
return 100;
}

DWORD WINAPI winthread_radar(PVOID arg)
{
char *c;
c=arg;
run_radar();
return 100;
}

DWORD WINAPI winthread_cal_filtercorr(PVOID arg)
{
char *c;
c=arg;
cal_filtercorr();
return 100;
}

DWORD WINAPI winthread_cal_interval(PVOID arg)
{
char *c;
c=arg;
do_cal_interval();
return 100;
}

DWORD WINAPI winthread_user_command(PVOID arg)
{
char *c;
c=arg;
user_command();
return 100;
}

DWORD WINAPI winthread_narrowband_dsp(PVOID arg)
{
char *c;
c=arg;
narrowband_dsp();
return 100;
}

DWORD WINAPI winthread_wideband_dsp(PVOID arg)
{
char *c;
c=arg;
wideband_dsp();
return 100;
}

DWORD WINAPI winthread_second_fft(PVOID arg)
{
char *c;
c=arg;
second_fft();
return 100;
}

DWORD WINAPI winthread_timf2(PVOID arg)
{
char *c;
c=arg;
timf2_routine();
return 100;
}

DWORD WINAPI winthread_tx_input(PVOID arg)
{
char *c;
c=arg;
tx_input();
return 100;
}

DWORD WINAPI winthread_tx_output(PVOID arg)
{
char *c;
c=arg;
run_tx_output();
return 100;
}

DWORD WINAPI winthread_screen(PVOID arg)
{
char *c;
c=arg;
screen_routine();
return 100;
}

DWORD WINAPI winthread_rx_file_input(PVOID arg)
{
char *c;
c=arg;
rx_file_input();
return 100;
}

DWORD WINAPI winthread_rx_fft1_netinput(PVOID arg)
{
char *c;
c=arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
thread_rx_fft1_netinput();
return 100;
}

DWORD WINAPI winthread_rx_raw_netinput(PVOID arg)
{
char *c;
c=arg;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
thread_rx_raw_netinput();
return 100;
}

DWORD WINAPI winthread_cal_iqbalance(PVOID arg)
{
char *c;
c=arg;
cal_iqbalance();
return 100;
}

DWORD WINAPI winthread_rx_adtest(PVOID arg)
{
char *c;
c=arg;
rx_adtest();
return 100;
}

DWORD WINAPI winthread_powtim(PVOID arg)
{
char *c;
c=arg;
powtim();
return 100;
}

DWORD WINAPI winthread_txtest(PVOID arg)
{
char *c;
c=arg;
txtest();
return 100;
}

DWORD WINAPI winthread_mouse(PVOID arg)
{
char *c;
c=arg;
wxmouse();
return 100;
}



