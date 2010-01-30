
#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif
#ifdef BSD
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <machine/cpufunc.h>
#include <machine/sysarch.h>
#else
#include <sys/io.h>
#endif
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <semaphore.h>
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <globdef.h>
#include <thrdef.h>
#include <uidef.h>
#include <xdef.h>
#include <ldef.h>
#include <hwaredef.h>
struct termios old_options;
#ifdef BSD
char serport_name[]="/dev/ttyd?";
extern int saved_euid;
#else
char serport_name[]="/dev/ttyS?";
#endif

void print_procerr(int xxprint)
{
int i;
if( (xxprint&1) != 0)
  {
  i=0;
  if( (xxprint&2)==0)
    {
    i=1;
    printf("\nMMX supported by hardware");
    }
  if( (xxprint&4)==0 && simd_present != 0)
    {
    i=1;
    printf("\nSIMD (=sse) supported by hardware");
    }
  if(i!=0)
    {
    printf("\n/proc/cpuinfo says LINUX core is not compatible.");
    printf(
           "\n\nLinrad will allow you to select routines that may be illegal");
    printf("\non your system.");
    printf("\nAny illegal instruction will cause a program crasch!!");
    printf("\nSeems just the sse flag in /proc/cpuinfo is missing.");
    printf("\nTo use fast routines with an older core, recompile LINUX");
    printf("\nwith appropriate patches.\n\n");
    }
  }
}

void mmxerr(void)
{
printf("\n\nCould not read file /proc/cpuinfo (flags:)");
printf("\nSetting MMX and SIMD flags from hardware");
printf("\nProgram may fail if kernel support is missing and modes"); 
printf("\nneeding MMX or SIMD are selected.");
printf("\n\n%s",press_enter);
fflush(stdout);
await_keyboard();
}

int investigate_cpu(void)
{
int i;
#ifndef BSD
int k;
FILE *file;
char s[256];
char *flags="flags";
char *fmmx=" mmx";
char *fsse=" sse";
#endif
int xxprint;
// If there is no mmx, do not use simd either.
tickspersec = sysconf(_SC_CLK_TCK);
i=check_mmx();
xxprint=0;
mmx_present=i&1;
if(mmx_present != 0)simd_present=i/2; else simd_present=0;
#ifdef BSD
{
size_t len=sizeof(no_of_processors);
int err;
if((err=sysctlbyname("hw.ncpu", &no_of_processors, &len, NULL, 0))<0)
  no_of_processors=1;
}
#else
no_of_processors=1;
if(i!=0)
  {
  file = fopen("/proc/cpuinfo", "r");
  if (file == NULL)
    {
    mmxerr();
    exit(0);
    }
  else
    {
no_of_processors=0;
nxline:;
    if(fgets(s,256,file)==NULL)
      {
      if(no_of_processors>0)goto cpuinfo_ok;
      mmxerr();
      exit(0);
      }
    else
      {  
      k=0;
      while(s[k] != flags[0] && k < 10)k++;
      for(i=0; i<5; i++) if(s[i+k]!=flags[i])goto nxline; 
      k+=5;  
      no_of_processors++;
nxbln:      
      if(no_of_processors>1)goto nxline;
      while(s[k]!=0 && s[k]!=' ')k++;
      while(s[k]!=0 && s[k]==' ')k++;
      if(s[k]!=0)
        {
        k--;
        for(i=0; i<4; i++)if(s[i+k]!=fmmx[i])goto notmmx;
        if(s[k+4] != ' ' && s[k+4] != 10)goto notmmx;
        xxprint|=2;
notmmx:;      
        for(i=0; i<4; i++)if(s[i+k]!=fsse[i])goto notsse;
        if(s[k+4] != ' ' && s[k+4] != 10)goto notsse;
        xxprint|=4;
notsse:;
        k++;
        goto nxbln;
        }      
      xxprint|=1;
      }
    goto nxline;
cpuinfo_ok:;      
    fclose(file);
    }
  }  
#endif
return xxprint;
}


void perseus_input(void){};

// Old versions of Linux have a special implementation of getrusage
// that returns the timing for the calling thread and not the timing for
// the process as prescribed by POSIX
// The configure script will set RESAGE_OLD=TRUE for those old linux
// kernels.
#if RUSAGE_OLD != TRUE
#include <sys/syscall.h>

void lir_system_times(double *cpu_time, double *total_time)
{
cpu_time[0]=lir_get_cpu_time()/no_of_processors;
total_time[0]=current_time();
}

void clear_thread_times(int no)
{
#ifdef BSD 
thread_pid[no]=no;
#else
thread_pid[no]=syscall(SYS_gettid);
#endif
thread_tottim1[no]=current_time();
thread_cputim1[no]=lir_get_cpu_time();
}

void make_thread_times(int no)
{
float t1,t2;
thread_tottim2[no]=current_time();
thread_cputim2[no]=lir_get_thread_time(no);
t1=100*(thread_cputim2[no]-thread_cputim1[no]);
t2=thread_tottim2[no]-thread_tottim1[no];
if(t1>0 && t2>0)
  {
  thread_workload[no]=t1/t2;
  }
else
  {
  thread_workload[no]=0;
  }
}

double lir_get_cpu_time(void)
{
double tm;
struct rusage rudat;
getrusage(RUSAGE_SELF,&rudat);
tm=0.000001*(rudat.ru_utime.tv_usec + rudat.ru_stime.tv_usec)+ 
                    rudat.ru_utime.tv_sec + rudat.ru_stime.tv_sec;
return tm;
}

double lir_get_thread_time(int no)
{
#ifdef BSD 
return 0*no;
#else
char fnam[80], info[512];
FILE *pidstat;
int j,k,m;
long long int ii1, ii2;

sprintf(fnam,"/proc/%d/task/%d/stat",thread_pid[no],thread_pid[no]);
pidstat=fopen(fnam,"r");
if(pidstat==NULL)return 0;
m=fread(info,1,512,pidstat);  
fclose(pidstat);  
j=0;
k=0;
while(k<13)
  {
  while(info[j]!= ' ' && j<m)j++;
  while(info[j]== ' ' && j<m)j++;
  k++;
  }
if(j>=m)return 0;
sscanf(&info[j], "%lld %lld", &ii1, &ii2);
return (double)(ii1+ii2)/tickspersec;
#endif
}
#endif

// Old versions of Linux have a special implementation of getrusage
// that returns the timing for the calling thread and not the timing for
// the process as prescribed by POSIX
// The configure script will set RESAGE_OLD=TRUE for those old linux
// kernels.
#if RUSAGE_OLD == TRUE

void make_thread_times(int no)
{
float t1,t2;
thread_tottim2[no]=current_time();
thread_cputim2[no]=lir_get_thread_time();
t1=100*(thread_cputim2[no]-thread_cputim1[no]);
t2=thread_tottim2[no]-thread_tottim1[no];
if(t1>0 && t2>0)
  {
  thread_workload[no]=t1/t2;
  }
else
  {
  thread_workload[no]=0;
  }
}

void clear_thread_times(int no)
{
thread_tottim1[no]=current_time();
thread_cputim1[no]=lir_get_cpu_time();
}

double lir_get_cpu_time(void)
{
int i;
float t1;
t1=0;
for(i=0; i<THREAD_MAX; i++)
  {
  if(thread_command_flag[i]!=THRFLAG_NOT_ACTIVE)
    {
    t1+=thread_workload[i];
    }
  }
return t1;
}

double lir_get_thread_time(void)
{
double tm;
struct rusage rudat;
getrusage(RUSAGE_SELF,&rudat);
tm=0.000001*(rudat.ru_utime.tv_usec + rudat.ru_stime.tv_usec)+ 
                    rudat.ru_utime.tv_sec + rudat.ru_stime.tv_sec;
return tm;
}

#endif






float lir_noisegen(int level) 
{
// Return a number distributed following a gaussian
// Mean value is 0 and sigma pow(2,level)
float x, y, z; 
y = (float)(random()+0.5)/RAND_MAX; 
z = (float)(random()+0.5)/RAND_MAX; 
x = z * 2*PI_L; 
return sin(x)*sqrt(-2*log(y))*pow(2.,level);
}

void lir_mutex_init(void)
{
pthread_mutex_init(&parport_lock,NULL);
}

void lir_mutex_destroy(void)
{
pthread_mutex_destroy(&parport_lock);
}

void lir_mutex_lock(void)
{
pthread_mutex_lock(&parport_lock);
}

void lir_mutex_unlock(void)
{
pthread_mutex_unlock(&parport_lock);
}



void lirerr(int errcod)
{
if(kill_all_flag) return;
DEB"\nlirerr(%d)",errcod);
if(dmp != 0)fflush(dmp);
lir_errcod=errcod;
sem_post(&sem_kill_all);
lir_sleep(100000);
}

int lir_open_serport(int serport_number, int baudrate,int stopbit_flag)
{
int rte;
int rc;
struct termios options;
rc=0;
if(serport != -1)return rc ;
if(serport_number < 1 || serport_number > 8)
  {
  rc=1279;
  return rc;
  }
if(serport_number <= 4)
  {
  sprintf(&serport_name[0],"%s","/dev/ttyS");
  sprintf(&serport_name[9],"%d",serport_number-1);
  }
else
  {
  sprintf(&serport_name[0],"%s","/dev/ttyUSB");
  sprintf(&serport_name[11],"%d",serport_number-5);
  }
serport=open(serport_name,O_RDWR  | O_NOCTTY | O_NDELAY);
if (serport == -1)
  {
  rc=1244;
  return rc;
  }
fcntl(serport,F_SETFL,0);           //blocking I/O
//fcntl(serport, F_SETFL, FNDELAY);     //non blocking I/O
if(tcgetattr(serport,&options) != 0)
  {
  rc=1277;
  goto close_x;
  }
switch ( baudrate )
  {
  case 110: 
  rte=B110;
  break;
  
  case 300: 
  rte=B300;
  break;
  
  case 600: 
  rte=B600;
  break;
  
  case 1200: 
  rte=B1200;
  break;
  
  case 2400: 
  rte=B2400;
  break;
  
  case 4800: 
  rte=B4800;
  break;
  
  case 9600: 
  rte=B9600;
  break;
  
  case 19200: 
  rte=B19200;
  break;
  
  case 38400: 
  rte=B38400;
  break;
  
  case 57600: 
  rte=B57600;
  break;

  default: 
  rc=1280;
  goto close_x;
  }
old_options=options;
cfsetispeed(&options,rte);
cfsetospeed(&options,rte);
//CLOCAL means don’t allow
//control of the port to be changed
//CREAD says to enable the receiver
options.c_cflag|= (CLOCAL | CREAD);
// no parity, 2 or 1 stopbits, 8 bits per word
options.c_cflag&= ~PARENB; //  no parity=>disable the "enable parity bit" PARENB
if(stopbit_flag)
  {
  options.c_cflag|= CSTOPB;// =>2 stopbits
  }
else
  {
  options.c_cflag&= ~CSTOPB;//=> 1 stopbit   
  }
options.c_cflag&= ~CSIZE;   // clear size-bit by anding with negation
options.c_cflag|= CS8;      // =>set size to 8 bits per word
// raw input /output
options.c_oflag &= ~OPOST; 
options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
//Set the timeouts:
//VMIN is the minimum amount characters to read
options.c_cc[VMIN]=0;
//The amount of time to wait
//for the amount of data
//specified by VMIN in tenths
//of a second.
options.c_cc[VTIME] = 1;
//  Select  flow control  mode
///* 
options.c_cflag &= ~CRTSCTS;       // no handshaking
options.c_iflag &= ~IXON;
//*/
/*
options.c_cflag &= ~CRTSCTS;       // Enable Xon/Xoff software handshaking 
options.c_iflag |= IXON;		
*/
/*
options.c_cflag |= CRTSCTS;        // Enable Hardware RTS handshaking 
options.c_iflag &= ~IXON;
*/
//flush I/O  buffers and apply the settings
if(tcsetattr(serport, TCSAFLUSH,&options) != 0) 
  {
  rc=1278;
close_x:  
  close(serport);
  serport=-1;
  }
return rc;
}

void lir_close_serport(void)
{
if(serport == -1)return;
if(tcsetattr(serport, TCSAFLUSH,&old_options) != 0)lirerr(1278);
close(serport);
serport=-1;
}

int lir_write_serport(void *s, int bytes)
{
return write(serport,s,bytes);
}

int lir_read_serport(void *s, int bytes)
{
int retnum, nread = 0;
while (bytes > 0) {
	retnum = read (serport, (char *)(s + nread), bytes);
	if (retnum <= 0) return -1;
	nread += retnum;
	bytes -= retnum;
	}	
return nread;
}

int lir_parport_permission(void)
{
int i;
// Get permission to write to the parallel port
#ifdef BSD
//  seteuid(saved_euid);
  i=i386_set_ioperm(ui.parport,4,1);
//  seteuid(getuid());
#else
if(ui.parport < 0x400-4)
  {
  i=ioperm(ui.parport,4,1);
  }
else
  {
  i=iopl(3);
  }
#endif  
if(i != 0)
  {
  return FALSE;
  }
else
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
}

void lir_sched_yield(void)
{
sched_yield();
}

void win_global_uiparms(int n)
{
// Do something with n to keep the compiler happy:
lir_inkey=n;
// Dummy routine. Not used under Linux.
}

void linrad_thread_create(int no)
{
thread_status_flag[no]=THRFLAG_INIT;
thread_command_flag[no]=THRFLAG_ACTIVE;
pthread_create(&thread_identifier[no],NULL,(void*)thread_routine[no], NULL);
threads_running=TRUE;
}

void thread_rx_output(void)
{
//int i;
//int policy;
//struct sched_param parms;
//policy=SCHED_RR;
//parms.sched_priority=sched_get_priority_max(policy);
//i=pthread_setschedparam(thread_identifier[THREAD_RX_OUTPUT],policy,&parms); 
//fprintf(dmp,"\nretcode %d ",i);
//i=pthread_getschedparam(thread_identifier[THREAD_RX_OUTPUT],&policy,&parms); 
//fprintf(dmp,"\nretcode %d prio %d (%d)",i,parms.sched_priority,EINVAL);

if(!kill_all_flag)rx_output();
thread_status_flag[THREAD_RX_OUTPUT]=THRFLAG_RETURNED;
}

void thread_kill_all(void)
{
int i;
// Wait until the semaphore is released.
// Then stop all processing threads so main can write any
// error code/message and exit.
sem_wait(&sem_kill_all);
kill_all();
i=0;
pthread_exit(&i);
}

double current_time(void)
{
struct timeval t;
gettimeofday(&t,NULL);
recent_time=0.000001*t.tv_usec+t.tv_sec;
return recent_time;
}

int ms_since_midnight(int set)
{
int i;
double dt1;
dt1=current_time();
if(set)netstart_time=dt1;
i=dt1/(24*3600);
dt1-=24*3600*i;
i=1000*dt1;
return i%(24*3600000);
}


void lir_sync(void)
{
// This routine is called to force a write to the hard disk
sync();
}

void lir_sem_post(int no)
{
sem_post(&lirsem[no]);
}

void lir_sem_wait(int no)
{
sem_wait(&lirsem[no]);
}

void lir_sem_free(int no)
{
if(lirsem_flag[no] == 0)lirerr(1203);
lirsem_flag[no]=0;
}


void lir_sem_init(int no)
{
if(lirsem_flag[no] != 0)lirerr(1204);
lirsem_flag[no]=1;
sem_init(&lirsem[no],0,0);
}

int lir_get_epoch_seconds(void)
{
struct timeval tim;
gettimeofday(&tim,NULL);
return tim.tv_sec;
}

void lir_join(int no)
{
lir_sched_yield();
if(thread_command_flag[no]==THRFLAG_NOT_ACTIVE)return;
pthread_join(thread_identifier[no],0);
thread_status_flag[no]=THRFLAG_NOT_ACTIVE;
}

void lir_sleep(int us)
{
usleep(us);
}

void lir_outb(char byte, int port)
{    
int i;
#ifdef BSD
//  seteuid(saved_euid);
  i=i386_set_ioperm(ui.parport,4,1);
//  seteuid(getuid());
#else
if(ui.parport < 0x400-4)
  {
  i=ioperm(ui.parport,4,1);
  }
else
  {
  i=iopl(3);
  }
#endif  
if(i!=0)lirerr(764921);
i=1000;
#ifdef BSD
{
int k;
k=byte;
outb(k,port);
}
#else
outb(byte,port);
#endif
while(i>0)i--;
}

char lir_inb(int port)
{
int i;
#ifdef BSD
//  seteuid(saved_euid);
  i=i386_set_ioperm(ui.parport,4,1);
//  seteuid(getuid());
#else
if(ui.parport < 0x400-4)
  {
  i=ioperm(ui.parport,4,1);
  }
else
  {
  i=iopl(3);
  }
#endif
if(i!=0)lirerr(764921);
return inb(port);
}

float lir_random(void)
{
return (float)(random())/RAND_MAX; 
}

void lir_srandom(void)
{
unsigned int seed;
seed=current_time();
srandom(seed);
}

// *********************************************************************
// Thread entries for Linux
// *********************************************************************

void thread_main_menu(void)
{
int i;
main_menu();
i=0;
pthread_exit(&i);
}

void thread_tune(void)
{
int i;
tune();
i=0;
pthread_exit(&i);
}

void thread_sdr14_input(void)
{
int i;
//int policy;
//struct sched_param parms;
//policy=SCHED_RR;
//parms.sched_priority=sched_get_priority_max(policy);
//i=pthread_setschedparam(thread_identifier[THREAD_RX_OUTPUT],policy,&parms); 
//fprintf(dmp,"\nretcode %d ",i);
//i=pthread_getschedparam(thread_identifier[THREAD_RX_OUTPUT],&policy,&parms); 
//fprintf(dmp,"\nretcode %d prio %d (%d)",i,parms.sched_priority,EINVAL);
sdr14_input();
i=0;
pthread_exit(&i);
}

void thread_radar(void)
{
int i;
run_radar();
i=0;
pthread_exit(&i);
}

void thread_perseus_input(void)
{
int i;
perseus_input();
i=0;
pthread_exit(&i);
}

void thread_cal_filtercorr(void)
{
int i;
cal_filtercorr();
i=0;
pthread_exit(&i);
}

void thread_cal_interval(void)
{
int i;
do_cal_interval();
i=0;
pthread_exit(&i);
}

void thread_user_command(void)
{
int i;
user_command();
i=0;
pthread_exit(&i);
}

void thread_narrowband_dsp(void)
{
int i;
narrowband_dsp();
i=0;
pthread_exit(&i);
}

void thread_wideband_dsp(void)
{
int i;
wideband_dsp();
i=0;
pthread_exit(&i);
}

void thread_second_fft(void)
{
int i;
second_fft();
i=0;
pthread_exit(&i);
}

void thread_timf2(void)
{
int i;
timf2_routine();
i=0;
pthread_exit(&i);
}

void thread_tx_input(void)
{
int i;
tx_input();
i=0;
pthread_exit(&i);
}

void thread_tx_output(void)
{
int i;
run_tx_output();
i=0;
pthread_exit(&i);
}

void thread_screen(void)
{
int i;
screen_routine();
i=0;
pthread_exit(&i);
}

void thread_rx_file_input(void)
{
int i;
rx_file_input();
i=0;
pthread_exit(&i);
}

void thread_cal_iqbalance(void)
{
int i;
cal_iqbalance();
i=0;
pthread_exit(&i);
}

void thread_rx_adtest(void)
{
int i;
rx_adtest();
i=0;
pthread_exit(&i);
}

void thread_powtim(void)
{
int i;
powtim();
i=0;
pthread_exit(&i);
}

void thread_txtest(void)
{
int i;
txtest();
i=0;
pthread_exit(&i);
}

