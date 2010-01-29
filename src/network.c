
// ********************* sockaddr and sockaddr_in **********************
// struct sockaddr {
// unsigned short sa_family;    // address family, AF_xxx
// char sa_data[14];            // 14 bytes of protocol address
// sa_family can be a variety of things, 
// but it'll be AF_INET for everything we do 
// in this document. sa_data contains a destination 
// address and port number for the socket. 
// This is rather unwieldy since you don't 
// want to tediously pack the address in 
// the sa_data by hand.
//    
// To deal with struct sockaddr, programmers 
// created a parallel structure: 
// struct sockaddr_in ("in" for "Internet".)
// struct sockaddr_in {
// short int sin_family;         // Address family
// unsigned short int sin_port;  // Port number
// struct in_addr sin_addr;      // Internet address
// unsigned char sin_zero[8];    // Same size as struct sockaddr
// **********************************************************************                                                                

#include <ctype.h>
#include <unistd.h>
#include "globdef.h"

#if(OSNUM == OS_FLAG_WINDOWS)
#include <windows.h>
#include <winsock.h>
#define RECV_FLAG 0
typedef struct{
struct in_addr imr_multiaddr;   /* IP multicast address of group */
struct in_addr imr_interface;   /* local IP address of interface */
}IP_MREQ;
WSADATA wsadata;
#define IP_ADD_MEMBERSHIP 12
#define INVSOCK INVALID_SOCKET
#define CLOSE_FD closesocket
#endif

#if(OSNUM == OS_FLAG_LINUX)
#include <fcntl.h>
#include "rusage.h"
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#ifdef MSG_WAITALL
#define RECV_FLAG MSG_WAITALL
#else
#define RECV_FLAG 0
#endif
#define IP_MREQ struct ip_mreq
#define INVSOCK -1
#define CLOSE_FD close
#endif

#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "vernr.h"
#include "caldef.h"
#include "screendef.h"
#include "thrdef.h"
#include "keyboard_def.h"
#include "seldef.h"
#include "options.h"

// ********************  addresses and ports  ***************************
// These defininitions should be in agreement with current rules.
#define NET_MULTI_MINPORT 50000
#define NET_MULTI_MAXPORT 65000
#define CONNPORT 49812   // This is outside the range ..MINPORT to ..MAXPORT
#define DEFAULT_MULTI_GROUP "239.255.0.00"
#define MULTI_GROUP_OFFSET 10
char netsend_rx_multi_group[20]=DEFAULT_MULTI_GROUP;
char netrec_rx_multi_group[20]=DEFAULT_MULTI_GROUP;
char par_netsend_filename[]={"par_netsend_ip"};
char par_netrec_filename[]={"par_netrec_ip"};
// ***********************************************************************

// Line numbers for network parameters (for par_network)
#define NNLINE 4
#define NN_BASE 1
#define NN_SENDAD 2
#define NN_RECAD 3 
#define NN_OUT_ZERO 4  // Defines the port address offsets.
#define NN_RAWOUT16 4
#define NN_RAWOUT18 5
#define NN_RAWOUT24 6
#define NN_FFT1OUT 7 
#define NN_TIMF2OUT 8
#define NN_FFT2OUT 9 
#define NN_BASEBOUT 10
#define NN_RX 11

struct sockaddr_in netsend_addr_rxraw16;
struct sockaddr_in netsend_addr_rxraw18;
struct sockaddr_in netsend_addr_rxraw24;
struct sockaddr_in netsend_addr_rxfft1;
struct sockaddr_in netsend_addr_rxtimf2;
struct sockaddr_in netsend_addr_rxfft2;
struct sockaddr_in netsend_addr_rxbaseb;
struct sockaddr_in netrec_addr;
struct sockaddr_in netmaster_addr;
struct sockaddr_in slave_addr;

#define NETHEADER_SIZE 6
#define FFT1_INFOSIZ 3
fd_set master_readfds;
fd_set rx_input_fds;

void net_read( void *buf, int bytes);
void net_write(FD fd, void *buf, int size);

unsigned int netrd_18_ia;
unsigned int netrd_16_ib;
unsigned int netrd_16_ia;
unsigned int netrd_18_ib;
unsigned int netrd_24_ia;
unsigned int netrd_24_ib
;
int no_of_net_freq=0;
int no_of_subslaves=0;
double net_start_time;
int net_write_errors;


void chk_send(int fd, char *buf, int len, struct sockaddr *to,
                                                  int tolen)
{       
#if(OSNUM == OS_FLAG_WINDOWS)
int ierr;       
#endif
double dt1;
char s[80];
int kk, net_written;
float t1;
net_written=sendto(fd, buf ,len, 0, to, tolen);
if(net_written==len)return;
// There was an error.
// We do not expect short writes, but try to handle them anywaqy.
// The "normal" error would be that the call would block.
// Wait here for the time of maximum 2 blocks of our input data stream
// to see if the situation improves.
if(net_written == -1)
  {
#if(OSNUM == OS_FLAG_WINDOWS)
  ierr=WSAGetLastError();
  if(ierr != WSAEWOULDBLOCK)goto show_errors;  
#endif
#if(OSNUM == OS_FLAG_LINUX)
  if(errno != EAGAIN)goto show_errors;
#endif
  kk=0;
  }
else
  {
  kk=net_written;
  }
t1=0.5/interrupt_rate;  
dt1=current_time()-netstart_time-accumulated_netwait_time;
while(dt1 > 0)
  {
// sleep in time chunks that are at least half the time to fill
// our A/D input buffer.
// (It would be possible to actually monitor the buffer pointers,
// but since there are several sound systems there would be
// several routines to write.)  
  lir_sleep(1000*t1);
  net_written=sendto(fd, &buf[kk] ,len-kk, 0, to, tolen);  
  if(net_written == -1)
    {
#if(OSNUM == OS_FLAG_WINDOWS)
    ierr=WSAGetLastError();
    if(ierr != WSAEWOULDBLOCK)goto show_errors;  
#endif
#if(OSNUM == OS_FLAG_LINUX)
    if(errno != EAGAIN)goto show_errors;
#endif
    }
  else
    {
    kk+=net_written;
    }
  if(kk == len)return;  
  dt1=current_time()-netstart_time-accumulated_netwait_time;
  }
#if DUMPFILE == TRUE
DEB" short write %d (%d)",kk, len);
#endif
return;
show_errors:;
net_write_errors++;
sprintf(s,"NET_WRITE ERROR %d",net_write_errors);
wg_error(s,WGERR_NETWR);
#if DUMPFILE == TRUE
// ***********************************************
DEB"\nNetwork write error: ");
#if(OSNUM == OS_FLAG_WINDOWS)
DEB"sendto error code %d ",ierr);
switch (ierr)
  {
  case WSAEINVAL:
  DEB"WSAEINVAL");
  break;

  case WSAEINTR:
  DEB"WSAEINTR");
  break;

  case WSAEINPROGRESS:
  DEB"WSAEINPROGRESS");
  break;
  
  case WSAEFAULT:
  DEB"WSAEFAULT");
  break;

  case WSAENETRESET:
  DEB"WSAENETRESET");
  break;

  case WSAENOBUFS:
  DEB"WSAENOBUFS");
  break;

  case WSAENOTCONN:
  DEB"WSAENOTCONN");
  break;

  case WSAENOTSOCK:
  DEB"WSAENOTSOCK");
  break;

  case WSAEOPNOTSUPP:
  DEB"WSAEOPNOTSUPP");
  break;

  case WSAESHUTDOWN:
  DEB"WSAESHUTDOWN");
  break;

  case WSAEWOULDBLOCK:
  DEB"WSAEWOULDBLOCK");
  break;

  case WSAEMSGSIZE:
  DEB"WSAEMSGSIZE");
  break;

  case WSAECONNABORTED:
  DEB"WSAECONNABORTED");
  break;

  case WSAECONNRESET:
  DEB"WSAECONNRESET");
  break;

  case WSAEADDRNOTAVAIL:
  DEB"WSAEADDRNOTAVAIL");
  break;

  case WSAENETUNREACH:
  DEB"WSAENETUNREACH");
  break;

  case WSAEHOSTUNREACH:
  DEB"WSAEHOSTUNREACH");
  break;

  case WSAETIMEDOUT:
  DEB"WSAETIMEDOUT");
  break;

  case WSANOTINITIALISED:
  DEB"WSANOTINITIALISED");
  break;
  
  case WSAENETDOWN:
  DEB"WSAENETDOWN");
  break;
  
  case WSAEACCES:
  DEB"WSAEACCES");
  break;
  
  case WSAEAFNOSUPPORT:
  DEB"WSAEAFNOSUPPORT");
  break;
  
  case WSAEDESTADDRREQ:
  DEB"WSAEDESTADDRREQ");
  break;
  
  default:
  DEB"unknown");
  }
#endif
#if(OSNUM == OS_FLAG_LINUX)
switch (errno)
  {
  case ENETUNREACH:
  DEB"Network unreachable");
  break;
  
  case EAGAIN:
  DEB"Operation would block");
  break;
  
  case EINTR:
  DEB"Interrupted by a signal");
  break;

  default:
  DEB" errno=%d",errno);
  }
#endif
#endif
}

void lir_send_raw16(void)
{
chk_send(netfd.send_rx_raw16,(char*)&net_rxdata_16,sizeof(NET_RX_STRUCT),
                              (struct sockaddr *) &netsend_addr_rxraw16, 
                                               sizeof(netsend_addr_rxraw16));
}

void lir_send_raw18(void)
{
chk_send(netfd.send_rx_raw18,(char*)&net_rxdata_18,sizeof(NET_RX_STRUCT),
                              (struct sockaddr *) &netsend_addr_rxraw18, 
                                               sizeof(netsend_addr_rxraw18));
}

void lir_send_raw24(void)
{
chk_send(netfd.send_rx_raw24,(char*)&net_rxdata_24,sizeof(NET_RX_STRUCT),
                              (struct sockaddr *) &netsend_addr_rxraw24, 
                                               sizeof(netsend_addr_rxraw24));
}

void lir_send_fft1(void)
{
chk_send(netfd.send_rx_fft1,(char*)&net_rxdata_fft1,sizeof(NET_RX_STRUCT),
                              (struct sockaddr *) &netsend_addr_rxfft1, 
                                                 sizeof(netsend_addr_rxfft1));
}

void lir_send_timf2(void)
{
chk_send(netfd.send_rx_timf2,(char*)&net_rxdata_timf2,sizeof(NET_RX_STRUCT),
                              (struct sockaddr *) &netsend_addr_rxtimf2, 
                                               sizeof(netsend_addr_rxtimf2));
}

void lir_send_fft2(void)
{
chk_send(netfd.send_rx_fft2,(char*)&net_rxdata_fft2,sizeof(NET_RX_STRUCT),
                              (struct sockaddr *) &netsend_addr_rxfft2, 
                                                 sizeof(netsend_addr_rxfft2));
}

void lir_send_baseb(void)
{
chk_send(netfd.send_rx_baseb,(char*)&net_rxdata_baseb,sizeof(NET_RX_STRUCT),
                              (struct sockaddr *) &netsend_addr_rxbaseb, 
                                                 sizeof(netsend_addr_rxbaseb));
}

void close_network_sockets(void)
{
int i;
FD *net_fds;
if(ui.network_flag==0)return;
net_fds=(void*)(&netfd);
for(i=0; i<MAX_NET_FD; i++)
  {
  if(net_fds[i] != INVSOCK)
    {
    CLOSE_FD(net_fds[i]);
    net_fds[i]=INVSOCK;
    }
  }
#if(OSNUM == OS_FLAG_WINDOWS)
WSACleanup();
#endif      
}

void net_input_error(void)
{
char s[80];
net_no_of_errors++;
if(net_no_of_errors > 2)
  {
  sprintf(s,"Network error %d",net_no_of_errors-2);
  wg_error(s,WGERR_RXIN);
  }
}

#define MAX_FREQLIST_TIME 3
#define NETFREQ_TOL 10000

void update_freqlist(void)
{
float t1;
int i, j, k, m, xpos;
int tmp_curx[MAX_FREQLIST];
// Data from netfreq_list will be distributed on the network. 
// When we run as a master, netslaves_selfreq[] contains
// the frequencies reported to us by slaves.
// A value of -1 means that no frequency is selected.
// We may also have frequencies that belong to subslaves
// in netfreq_list.
// Start by removing old entries from both lists. 
t1=current_time()-net_start_time;
i=0;
while(i<no_of_netslaves)
  {
  if(t1-netslaves_time[i] > MAX_FREQLIST_TIME)
    {
    netslaves_selfreq[i]=-1;
    }
  i++;
  }  
i=0;
j=MAX_FREQLIST-1;
while(netfreq_list[i] >= 0)
  {
  if(t1-netfreq_time[i] > MAX_FREQLIST_TIME)
    {
    netfreq_list[i]=-1;
    if(j != MAX_FREQLIST-1)j=i;
    }
  i++;
  }
if(j != MAX_FREQLIST-1)
  {
  k=j+1;
  while(k<i)
    {
    if(t1-netfreq_time[i] <= MAX_FREQLIST_TIME)
      {
      netfreq_list[j]=netfreq_list[k];
      netfreq_time[j]=netfreq_time[k];
      j++;
      }
    k++;
    }
  }    
// Make sure that all frequencies that we have in netslaves_selfreq
// are present in netfreq_list. Update to the most recent value
// if we are within NETFREQ_TOL
i=0;
while(i<no_of_netslaves)
  {
  if( netslaves_selfreq[i]>=0 )
    {
    j=0;
    while(netfreq_list[j] >= 0)
      {
      if(abs(netfreq_list[j]-netslaves_selfreq[i]) < NETFREQ_TOL)
        {
        netfreq_list[j]=netslaves_selfreq[i];
        netfreq_time[j]=netslaves_time[i];
        goto updated;
        }
      j++;  
      }
    if(j >= MAX_FREQLIST)
      {
      lirerr(1254);
      return;
      }
    netfreq_list[j]=netslaves_selfreq[i];
    netfreq_time[j]=netslaves_time[i];
    }
updated:;
  i++;
  }
k=0;
m=0;
// Compute cursor locations.
while(netfreq_list[k] >= 0)
  {
  xpos=0.5+wg_first_xpixel+(0.001*netfreq_list[k]-wg_first_frequency)
                                                            /wg_hz_per_pixel;
  if(xpos > wg_first_xpixel && xpos < wg_last_xpixel)
    {
    tmp_curx[m]=xpos;
    m++;
    }
  k++;
  }
while(sc[SC_SHOW_FFT1]!=sd[SC_SHOW_FFT1])
  {
  if(kill_all_flag ||
            thread_command_flag[THREAD_LIR_SERVER] != THRFLAG_ACTIVE)return;
  lir_sleep(10000);
  }
// Remove cursors no longer present.  
for(i=0; i<MAX_FREQLIST; i++)
  {
  if(new_netfreq_curx[i] >= 0)
    {
    for(j=0; j<m; j++)
      {
      if( tmp_curx[j] == new_netfreq_curx[i])
        {
        k=1;
        tmp_curx[j]=-1;  
        goto nxt_i;
        }
      }
    new_netfreq_curx[i]=-1;
    }
nxt_i:; 
  }    
// Add new cursors.
j=0;
for(i=0; i<m; i++)
  {
  if(tmp_curx[i] >= 0)
    {
    while(new_netfreq_curx[j] >= 0)j++;
    new_netfreq_curx[j]=tmp_curx[i];
    j++;
    } 
  }
// Do not show slaves that listen to our own frequency.
for(j=0; j<MAX_FREQLIST; j++)
  {
  if(new_netfreq_curx[j] >= 0)
    {
    for(i=0; i<genparm[MIX1_NO_OF_CHANNELS]; i++)
      {
      if(new_mix1_curx[i]==new_netfreq_curx[j])
        {
        new_netfreq_curx[j]=-1;
        mix1_curx[i]=-1;
        }
      }
    }    
  }
sc[SC_SHOW_FFT1]++;
lir_sem_post(SEM_SCREEN);
}



void remove_from_freqlist(int ifr)
{
int j;
j=0;
while(netfreq_list[j] >= 0)
  {
  if(abs(netfreq_list[j]-ifr) < NETFREQ_TOL)
    {
    while(netfreq_list[j] >= 0)
      {
      netfreq_list[j]=netfreq_list[j+1];
      j++;
      }
    if( (ui.network_flag&NET_RX_INPUT)!=0)
      {
      slave_msg.type=NETMSG_REMOVE_SUBSLAVE;      
      slave_msg.frequency=ifr;
      net_write(netfd.master, &slave_msg, sizeof(slave_msg));
      }
    return;
    }
  j++;
  }
}

void net_send_slaves_freq(void)
{
int intbuf[MAX_NETSLAVES];
int i, k;
slave_msg.type=NETMSG_OWN_FREQ;
if(mix1_selfreq[0] >= 0)
  {
  slave_msg.frequency=1000*mix1_selfreq[0];
  }
else
  {
  slave_msg.frequency=-1;
  }
net_write(netfd.master, &slave_msg, sizeof(slave_msg));
if(kill_all_flag) return;
k=0;
for(i=0; i<no_of_netslaves; i++)
  {
  if( netslaves_selfreq[i]>=0 )
    {
    intbuf[k]=netslaves_selfreq[i];
    k++;
    }
  }  
if(k>0)
  {
  slave_msg.type=NETMSG_SUBSLAVE_FREQ;
  slave_msg.frequency=k;
  net_write(netfd.master, &slave_msg, sizeof(slave_msg));
  if(kill_all_flag) return;
  net_write(netfd.master, &intbuf, k*sizeof(int));
  }
}



void lir_netread_rxin(void *buf)
{
NET_RX_STRUCT *msg; 
char s[80];
fd_set testfds;
struct timeval timeout;
int nbytes;
msg=(void*)buf;
while( !kill_all_flag )
  {
  testfds=rx_input_fds;
  timeout.tv_sec=0;
  timeout.tv_usec=300000;
  if( select(FD_SETSIZE, &testfds, (fd_set *)0, 
                                         (fd_set *)0, &timeout) > 0)
    {   
    if(FD_ISSET(netfd.rec_rx,&testfds))
      {
      nbytes=recv(netfd.rec_rx,buf,sizeof(NET_RX_STRUCT), RECV_FLAG);
      if(nbytes != sizeof(NET_RX_STRUCT))
        {
        sprintf(s,"NET RX ERROR. Received %d  Expected %d",
                                      nbytes, sizeof(NET_RX_STRUCT));
        lir_text(0,10,s);
        lir_text(0,11,"Incompatible Linrad version?");
        }
      if(fg.passband_center != msg[0].passband_center ||
         fg.passband_direction != msg[0].passband_direction ||
         fft1_direction != fg.passband_direction)
        {
        fg.passband_center = msg[0].passband_center;
        fg.passband_direction = msg[0].passband_direction;
        fft1_direction=fg.passband_direction;
        }        
      return;
      }  
    }
  }  
}


void thread_rx_fft1_netinput(void)
{
#if RUSAGE_OLD == TRUE
int local_workload_counter;
#endif
int i, k;
int read_time;
float *net_floatbuf;
double dt1, read_start_time,total_reads;
double total_time1,total_time2;
struct sockaddr_in rx_addr;
unsigned int addrlen;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
unsigned int netrd_fft1_ia;
int netrd_fft1_ib;
unsigned short int block_cnt;
#if OSNUM == OS_FLAG_LINUX
clear_thread_times(THREAD_RX_FFT1_NETINPUT);
#endif
#if RUSAGE_OLD == TRUE
local_workload_counter=workload_counter;
#endif
while( all_threads_started != TRUE)
  {
  lir_sleep(30000);
  if(kill_all_flag) goto fft1net_rxin_error_exit;
  }   
net_floatbuf=(void*)&net_rxdata_fft1.buf[0];
total_time1=current_time();
read_start_time=total_time1;
rxin_local_workload_reset=workload_reset_flag;
timing_loop_counter_max=ui.rx_ad_speed*2*sizeof(float)
                          /(NET_MULTICAST_PAYLOAD*(1-fft1_interleave_ratio));
if( (ui.rx_input_mode&DWORD_INPUT) != 0)timing_loop_counter_max*=2;
timing_loop_counter=2.5*timing_loop_counter_max;
screen_loop_counter_max=0.1*timing_loop_counter_max;
if(screen_loop_counter_max==0)screen_loop_counter_max=1;
screen_loop_counter=timing_loop_counter_max;
total_reads=0;
initial_skip_flag=1;
addrlen=sizeof(rx_addr);
netrd_fft1_ia=0;        
lir_netread_rxin(&net_rxdata_fft1);
if(kill_all_flag) goto fft1net_rxin_error_exit;
netrd_fft1_ib=0;
block_cnt=net_rxdata_fft1.block_no;
thread_status_flag[THREAD_RX_FFT1_NETINPUT]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_RX_FFT1_NETINPUT] == THRFLAG_ACTIVE)
  {
readfft1:;      
#if RUSAGE_OLD == TRUE
  if(local_workload_counter != workload_counter)
    {
    local_workload_counter=workload_counter;
    make_thread_times(THREAD_RX_FFT1_NETINPUT);
    }
#endif
  timing_loop_counter--;
  if(timing_loop_counter == 0)
    {
    timing_loop_counter=timing_loop_counter_max;
    total_time2=current_time();
    if(initial_skip_flag != 0)
      {
      read_start_time=total_time2;
      total_reads=0;
      initial_skip_flag=0;
      }
    else
      {
      dt1=total_time2-read_start_time;
      measured_ad_speed=(1-fft1_interleave_ratio)*NET_MULTICAST_PAYLOAD*
                                 total_reads/(dt1*2*sizeof(float));
      }
    }
  while(netrd_fft1_ia<NET_MULTICAST_PAYLOAD/sizeof(float))
    {
    fft1_float[fft1_pa+netrd_fft1_ib  ]=net_floatbuf[netrd_fft1_ia  ];
    netrd_fft1_ib++;
    netrd_fft1_ia++;
    if(netrd_fft1_ib >= fft1_block)goto new_transform;
    }
  netrd_fft1_ia=0;
  lir_netread_rxin(&net_rxdata_fft1);
  total_reads++;
  if(kill_all_flag) goto fft1net_rxin_error_exit;
  block_cnt++;
  k=net_rxdata_fft1.ptr/(int)(sizeof(float));
  if( block_cnt != net_rxdata_fft1.block_no || netrd_fft1_ib!=k)
    {
// The block number (ot data pointer) is incorrect. Some data is lost!!
    net_input_error();
    i=net_rxdata_fft1.block_no-block_cnt;
    i=(i+0x10000)&0xffff;
    i*=NET_MULTICAST_PAYLOAD/sizeof(float);
    i+=k-netrd_fft1_ib;
    if(i>0 && i<2*fft1_block)
      {
// We have lost less than two transforms.
// Fill in zeroes and update pointers. The user may not notice.....      
      while(i>0)
        {
        i--;
        fft1_float[fft1_pa+netrd_fft1_ib  ]=0;
        netrd_fft1_ib++;
        netrd_fft1_ia++;
        if(netrd_fft1_ia>=NET_MULTICAST_PAYLOAD/sizeof(float))
          {
          netrd_fft1_ia=0;
          block_cnt++;
          }
        if(netrd_fft1_ib >= fft1_block)
          {
          netrd_fft1_ib=0;
          fft1_pa=(fft1_pa+fft1_block)&fft1_mask;  
          fft1_na=fft1_pa/fft1_block;
          if(fft1_nm != fft1n_mask)fft1_nm++;
          lir_sem_post(SEM_TIMF1);
          }
        }
      }
    else
      {  
      block_cnt=net_rxdata_fft1.block_no;
      netrd_fft1_ib=net_rxdata_fft1.ptr/sizeof(float);
      }
    }
  goto readfft1;
new_transform:;
  netrd_fft1_ib=0;
  fft1_pa=(fft1_pa+fft1_block)&fft1_mask;  
  fft1_na=fft1_pa/fft1_block;
  if(fft1_nm != fft1n_mask)fft1_nm++;
  lir_sem_post(SEM_TIMF1);
  if(kill_all_flag) goto fft1net_rxin_error_exit;
  read_time=ms_since_midnight(FALSE);
  if(abs(latest_listsend_time-read_time) > 1500)
    {
    latest_listsend_time=read_time;
    net_send_slaves_freq();
    }
  }
fft1net_rxin_error_exit:;
CLOSE_FD(netfd.rec_rx);
netfd.rec_rx=-1;
thread_status_flag[THREAD_RX_FFT1_NETINPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_RX_FFT1_NETINPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}



void thread_rx_raw_netinput(void)
{
#if RUSAGE_OLD == TRUE
int local_workload_counter;
#endif
int i;
unsigned short int block_cnt;
char *rxin_char;
double dt1, read_start_time,total_reads;
double total_time1,total_time2;
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
#if OSNUM == OS_FLAG_LINUX
clear_thread_times(THREAD_RX_RAW_NETINPUT);
#endif
#if RUSAGE_OLD == TRUE
local_workload_counter=workload_counter;
#endif
while( all_threads_started != TRUE)
  {
  lir_sleep(30000);
  if(kill_all_flag) goto rawnet_rxin_error_exit;
  }   
total_time1=current_time();
read_start_time=total_time1;
rxin_local_workload_reset=workload_reset_flag;
timing_loop_counter_max=interrupt_rate;
timing_loop_counter=2.5*timing_loop_counter_max;
screen_loop_counter_max=0.1*interrupt_rate;
if(screen_loop_counter_max==0)screen_loop_counter_max=1;
screen_loop_counter=interrupt_rate;
total_reads=0;
initial_skip_flag=1;
save_rw_bytes=(18*rxad.block_bytes)/32;
switch(ui.network_flag&NET_RX_INPUT)
  {
  case NET_RXIN_BASEB:
  case NET_RXIN_RAW16:
  lir_netread_rxin(&net_rxdata_16);
  netrd_16_ib=net_rxdata_16.ptr;
  netrd_16_ia=0;
  block_cnt=net_rxdata_16.block_no;
  break;
  
  case NET_RXIN_RAW18:
  lir_netread_rxin(&net_rxdata_18);
  netrd_18_ib=net_rxdata_18.ptr;
  netrd_18_ia=0;
  block_cnt=net_rxdata_18.block_no;
  break;
  
  case NET_RXIN_RAW24:
  lir_netread_rxin(&net_rxdata_24);
  netrd_24_ib=net_rxdata_24.ptr;
  netrd_24_ia=0;
  block_cnt=net_rxdata_24.block_no;
  break;
  
  default:
  lirerr(285613);
  goto rawnet_rxin_error_exit;
  }
thread_status_flag[THREAD_RX_RAW_NETINPUT]=THRFLAG_ACTIVE;
while(thread_command_flag[THREAD_RX_RAW_NETINPUT] == 
                                       THRFLAG_ACTIVE && !kill_all_flag)
  {
#if RUSAGE_OLD == TRUE
  if(local_workload_counter != workload_counter)
    {
    local_workload_counter=workload_counter;
    make_thread_times(THREAD_RX_RAW_NETINPUT);
    }
#endif
  timing_loop_counter--;
  if(timing_loop_counter == 0)
    {
    timing_loop_counter=timing_loop_counter_max;
    total_time2=current_time();
    if(initial_skip_flag != 0)
      {
      read_start_time=total_time2;
      total_reads=0;
      initial_skip_flag=0;
      }
    else
      {
      total_reads+=timing_loop_counter_max;
      dt1=total_time2-read_start_time;
      measured_ad_speed=total_reads*rxad.block_frames/dt1;
      }
    }
  rxin_isho=(void*)(&timf1_char[timf1p_pa]);
  rxin_char=(void*)(&timf1_char[timf1p_pa]);
  timf1p_pnb=timf1p_pa;
  timf1p_pa=(timf1p_pa+rxad.block_bytes)&timf1_bytemask;
  switch(ui.network_flag&NET_RX_INPUT)
    {
    case NET_RXIN_BASEB:
    case NET_RXIN_RAW16:
read16:;   
    while(netrd_16_ia<NET_MULTICAST_PAYLOAD)
      {
      rxin_char[netrd_16_ib  ]=net_rxdata_16.buf[netrd_16_ia  ];
      rxin_char[netrd_16_ib+1]=net_rxdata_16.buf[netrd_16_ia+1];
      netrd_16_ib+=2;
      netrd_16_ia+=2;
      if(netrd_16_ib >= rxad.block_bytes)goto read16x;
      }
    netrd_16_ia=0;        
    lir_netread_rxin(&net_rxdata_16);
    if(kill_all_flag) goto rawnet_rxin_error_exit;
    block_cnt++;
    if( block_cnt != net_rxdata_16.block_no || 
                                      (int)netrd_16_ib!=net_rxdata_16.ptr)
      {
// The block number (or data pointer) is incorrect. Some data is lost!!
      net_input_error();
      i=net_rxdata_16.block_no-block_cnt;
      i=(i+0x10000)&0xffff;
      i*=NET_MULTICAST_PAYLOAD;
      i+=net_rxdata_16.ptr-netrd_16_ib;
      if(i>0 && i < 2*fft1_blockbytes)
        {
// We have lost less than two transforms.
// Fill in zeroes and update pointers. The user will not notice.....      
        while( i>0)
          {
          i--;
          rawsave_tmp[netrd_16_ib  ]=0;
          rawsave_tmp[netrd_16_ib+1]=0;
          netrd_16_ib+=2;
          netrd_16_ia+=2;
          if(netrd_16_ia>=NET_MULTICAST_PAYLOAD)
            {
            netrd_16_ia=0;
            block_cnt++;
            }
          if(netrd_16_ib >= rxad.block_bytes)
            {
            netrd_16_ib=0;
            finish_rx_read(rxin_isho);
            rxin_isho=(void*)(&timf1_char[timf1p_pa]);
            rxin_char=(void*)(&timf1_char[timf1p_pa]);
            timf1p_pnb=timf1p_pa;
            timf1p_pa=(timf1p_pa+rxad.block_bytes)&timf1_bytemask;
            }
          }
        }  
      else
        {
        block_cnt=net_rxdata_16.block_no;
        netrd_16_ib=net_rxdata_16.ptr;
        }
      }
    goto read16;
read16x:;    
    netrd_16_ib=0;
    break;
    
    case NET_RXIN_RAW18:
read18:;      
    while(netrd_18_ia<NET_MULTICAST_PAYLOAD)
      {
      rawsave_tmp[netrd_18_ib  ]=net_rxdata_18.buf[netrd_18_ia  ];
      rawsave_tmp[netrd_18_ib+1]=net_rxdata_18.buf[netrd_18_ia+1];
      netrd_18_ib+=2;
      netrd_18_ia+=2;
      if(netrd_18_ib >= save_rw_bytes)goto new_blk_18;
      }
    netrd_18_ia=0;        
    lir_netread_rxin(&net_rxdata_18);
    if(kill_all_flag) goto rawnet_rxin_error_exit;
    block_cnt++;
    if( block_cnt != net_rxdata_18.block_no ||
                                     (int)netrd_18_ib!=net_rxdata_18.ptr)
// The block number (or data pointer) is incorrect. Some data is lost!!
      {
      net_input_error();
      i=net_rxdata_18.block_no-block_cnt;
      i=(i+0x10000)&0xffff;
      i*=NET_MULTICAST_PAYLOAD;
      i+=net_rxdata_16.ptr-netrd_16_ib;
      if(i>0 && i < 2*fft1_blockbytes*18/32)
        {
// We have lost less than two transforms.
// Fill in zeroes and update pointers. The user will not notice.....      
        while( i > 0)
          {
          i--;
          rawsave_tmp[netrd_18_ib  ]=0;
          rawsave_tmp[netrd_18_ib+1]=0;
          netrd_18_ib+=2;
          netrd_18_ia+=2;
          if(netrd_18_ia>=NET_MULTICAST_PAYLOAD)
            {
            netrd_18_ia=0;
            block_cnt++;
            }
          if(netrd_18_ib >= save_rw_bytes)
            {
            netrd_18_ib=0;
            expand_rawdat();
            finish_rx_read(rxin_isho);
            rxin_isho=(void*)(&timf1_char[timf1p_pa]);
            rxin_char=(void*)(&timf1_char[timf1p_pa]);
            timf1p_pnb=timf1p_pa;
            timf1p_pa=(timf1p_pa+rxad.block_bytes)&timf1_bytemask;
            }
          }
        }  
      else
        {
        block_cnt=net_rxdata_18.block_no;
        netrd_18_ib=net_rxdata_18.ptr;
        }
      }
    goto read18;
new_blk_18:;
    netrd_18_ib=0;
    expand_rawdat();
    break;

    case NET_RXIN_RAW24:
read24:;      
    while(netrd_24_ia<NET_MULTICAST_PAYLOAD)
      {
      rxin_char[netrd_24_ib  ]=0;
      rxin_char[netrd_24_ib+1]=net_rxdata_24.buf[netrd_24_ia  ];
      rxin_char[netrd_24_ib+2]=net_rxdata_24.buf[netrd_24_ia+1];
      rxin_char[netrd_24_ib+3]=net_rxdata_24.buf[netrd_24_ia+2];
      netrd_24_ib+=4;
      netrd_24_ia+=3;
      if(netrd_24_ib >= rxad.block_bytes)goto new_blk_24;
      }
    netrd_24_ia=0;        
    lir_netread_rxin(&net_rxdata_24);
    if(kill_all_flag) goto rawnet_rxin_error_exit;
    block_cnt++;
    if( block_cnt != net_rxdata_24.block_no ||
                                    (int)netrd_24_ib!=net_rxdata_24.ptr)
      {
// The block number (or data pointer) is incorrect. Some data is lost!!
      net_input_error();
      i=net_rxdata_24.block_no-block_cnt;
      i=(i+0x10000)&0xffff;
      i*=NET_MULTICAST_PAYLOAD;
      i+=net_rxdata_16.ptr-netrd_16_ib;
      if(i>0 && i < 2*fft1_blockbytes)
        {
// We have lost less than two transforms.
// Fill in zeroes and update pointers. The user will not notice.....      
        while( i>0 )  
          {
          rxin_char[netrd_24_ib  ]=0;
          rxin_char[netrd_24_ib+1]=0;
          rxin_char[netrd_24_ib+2]=0;
          rxin_char[netrd_24_ib+3]=0;
          netrd_24_ib+=4;
          netrd_24_ia+=3;
          if(netrd_24_ia>=NET_MULTICAST_PAYLOAD)
            {
            netrd_24_ia=0;
            block_cnt++;
            }
          if(netrd_24_ib >= rxad.block_bytes)
            {
            netrd_24_ib=0;
            finish_rx_read(rxin_isho);
            rxin_isho=(void*)(&timf1_char[timf1p_pa]);
            rxin_char=(void*)(&timf1_char[timf1p_pa]);
            timf1p_pnb=timf1p_pa;
            timf1p_pa=(timf1p_pa+rxad.block_bytes)&timf1_bytemask;
            }
          }
        }
      else
        {
        block_cnt=net_rxdata_24.block_no;
        netrd_24_ib=net_rxdata_24.ptr;
        }
      }
    goto read24;
new_blk_24:;
    netrd_24_ib=0;
    break;
    }
  finish_rx_read(rxin_isho);
  }
rawnet_rxin_error_exit:;
CLOSE_FD(netfd.rec_rx);
netfd.rec_rx=-1;
thread_status_flag[THREAD_RX_RAW_NETINPUT]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_RX_RAW_NETINPUT] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void thread_lir_server(void)
{
#if RUSAGE_OLD == TRUE
int local_workload_counter;
#endif
FILE *file;
unsigned char *ip;
int i, j, k, kk, ifr, ie;
char s[80];
float *tmpbuf;
float t1;
int intbuf[10+MAX_NETSLAVES];
struct timeval timeout;
FD client_sockfd;
fd_set testfds;
int nread; 
FD fd;
int client_len;
struct sockaddr_in client_address;
int netdat[FFT1_INFOSIZ];
int listupd_count;
#if OSNUM == OS_FLAG_LINUX
clear_thread_times(THREAD_LIR_SERVER);
#endif
#if RUSAGE_OLD == TRUE
local_workload_counter=workload_counter;
#endif
net_start_time=current_time();
listupd_count=0;
thread_status_flag[THREAD_LIR_SERVER]=THRFLAG_ACTIVE;
while(!kill_all_flag && 
               thread_command_flag[THREAD_LIR_SERVER] == THRFLAG_ACTIVE)
  {
  listupd_count++;
  if(listupd_count > 5)
    {
    listupd_count=0;
    update_freqlist();
    }
  testfds=master_readfds;
  timeout.tv_sec=0;
  timeout.tv_usec=300000;
  ie=select(FD_SETSIZE, &testfds, (fd_set *)0, (fd_set *)0, &timeout);
  if(ie > 0)
    {
#if RUSAGE_OLD == TRUE
    if(local_workload_counter != workload_counter)
      {
      local_workload_counter=workload_counter;
      make_thread_times(THREAD_LIR_SERVER);
      }
#endif
    if(FD_ISSET(netfd.any_slave,&testfds))
      {
      if(no_of_netslaves >= MAX_NETSLAVES)
        {
        lirerr(1270);
        goto netserver_error_exit;
        }
      client_len = sizeof(client_address);
      client_sockfd = accept(netfd.any_slave,
                    (struct sockaddr *)&client_address, (void*)&client_len);
      FD_SET(client_sockfd, &master_readfds);
      netfd.slaves[no_of_netslaves]=client_sockfd;
      lir_fillbox(5*no_of_netslaves,0,3,3,10);
      no_of_netslaves++;
      ip=(void*)&client_address.sin_addr.s_addr;
      }
    for(i = 0; i < no_of_netslaves; i++) 
      {
      if(FD_ISSET(netfd.slaves[i],&testfds))
        {
        fd=netfd.slaves[i];  
          {
          nread=recv(fd, (char*)&slave_msg, sizeof(slave_msg), MSG_PEEK);
          if(nread <= 0)
            {
// A slave is disconnected.
slave_cls:;
            if(netslaves_selfreq[i] >= 0)
              {
              remove_from_freqlist(netslaves_selfreq[i]);
              }
            j=i;
            while(j < no_of_netslaves-1)
              {
              netfd.slaves[j]=netfd.slaves[j+1];
              netslaves_selfreq[j]=netslaves_selfreq[j+1];
              netslaves_time[j]=netslaves_time[j+1];
              j++;
              }
            no_of_netslaves--;
            lir_fillbox(5*no_of_netslaves,0,3,3,0);
            CLOSE_FD(fd);
            FD_CLR(fd, &master_readfds);
            update_freqlist();
            listupd_count=0;
            }
          else 	
            {
// Check that we get the setup information correctly
// and exit if all seems ok.
            if(nread < (int)(sizeof(slave_msg)))
              {
              lirerr(886942);
              lir_sleep(10000);
              nread=recv(fd, (char*)&slave_msg, sizeof(slave_msg), MSG_PEEK);
              }
            if(nread == (int)(sizeof(slave_msg)))
              {
              recv(fd, (char*)&slave_msg, sizeof(slave_msg),0);
              switch (slave_msg.type)
                {
                case NETMSG_OWN_FREQ:
                for(i=0; i<no_of_netslaves; i++)
                  {
                  if(netfd.slaves[i] == fd)
                    {
                    ifr=slave_msg.frequency;
                    if( netslaves_selfreq[i] != ifr  &&
                                             netslaves_selfreq[i] >= 0)
                      {
                      remove_from_freqlist(netslaves_selfreq[i]);
                      }
                    netslaves_selfreq[i]=ifr;
                    netslaves_time[i]=current_time()-net_start_time;
                    }
                  }
                update_freqlist();
                listupd_count=0;
                break;

                case NETMSG_SUBSLAVE_FREQ:
                if(slave_msg.frequency <1)
                  {
                  lirerr(1256);
                  goto netserver_error_exit;
                  }
                if(slave_msg.frequency >MAX_NETSLAVES-1)
                  {
                  lirerr(1257);
                  goto netserver_error_exit;
                  }
                lir_sleep(30000);
                k=slave_msg.frequency;
                nread=recv(fd, (char*)&intbuf, k*sizeof(int), MSG_PEEK);
                if(nread == 0)goto slave_cls;
                if(nread == k*(int)(sizeof(int)))
                  {
                  recv(fd, (char*)&intbuf, k*sizeof(int),0);
// Check if the frequency is already on the list.
// And add it if it is missing.
                  t1=current_time()-net_start_time;
                  k--;
                  while(k>=0)
                    {
                    i=0;
                    while(netfreq_list[i] > 0)
                      {
                      if(abs(netfreq_list[i]-intbuf[k])<NETFREQ_TOL)
                        {
                        goto nextk;
                        }
                      i++;
                      }
                    while(netfreq_list[i] > 0)i++;
                    if(i >= MAX_FREQLIST)
                      {
                      lirerr(1254);
                      goto netserver_error_exit;
                      }
nextk:;   
                    netfreq_list[i]=intbuf[k];
                    netfreq_time[i]=t1;
                    k--;
                    }                    
                  update_freqlist();
                  listupd_count=0;
                  }
                else
                  {
                  recv(fd, (char*)&intbuf, nread,0);
                  }
                break;
                
                case NETMSG_REMOVE_SUBSLAVE:
                remove_from_freqlist(slave_msg.frequency);
                break;

                case NETMSG_MODE_REQUEST:
                intbuf[0]=ui.rx_ad_speed;
                intbuf[1]=ui.rx_ad_channels;
                intbuf[2]=ui.rx_rf_channels;
                intbuf[3]=ui.rx_input_mode;                
                intbuf[4]=rxad.block_bytes;
                intbuf[5]=fft1_size;
                intbuf[6]=fft1_n;
                intbuf[7]=genparm[FIRST_FFT_SINPOW];
                net_write(fd,intbuf,8*sizeof(int));
                if(kill_all_flag)goto netserver_error_exit;
                break;
                
                case NETMSG_BASEBMODE_REQUEST:
                intbuf[0]=genparm[DA_OUTPUT_SPEED];
                intbuf[1]=2*ui.rx_rf_channels;
                intbuf[2]=ui.rx_rf_channels;
                intbuf[3]=IQ_DATA;
                if(ui.rx_rf_channels == 2)intbuf[3]+=TWO_CHANNELS;                
                intbuf[4]=basebnet_block_bytes;
                intbuf[5]=0;
                intbuf[6]=0;
                intbuf[7]=0;
                net_write(fd,intbuf,8*sizeof(int));
                if(kill_all_flag)goto netserver_error_exit;
                break;
                
                case NETMSG_CAL_REQUEST:
                if( (fft1_calibrate_flag&CALAMP) == CALAMP &&
                                              diskread_flag < 2)
                  {
                  make_filfunc_filename(s);
                  file = fopen(s, "rb");
                  if(file == NULL)
                    {
                    lirerr(1062);
                    goto netserver_error_exit;
                    }  
// Send a flag so the client knows data will come.
                  s[0]=TRUE;
                  net_write(fd,s,sizeof(char));
                  if(kill_all_flag)goto netserver_error_exit;
// Read 10 integers. Send only [7], a flag that will tell the client 
// whether the data is in the time or frequency domain and [0] and [1]
// which is the size.
                  i=fread(intbuf, sizeof(int),10,file);
                  if(i != 10)
                    {
cal_error:;                    
                    fclose(file);
                    lirerr(1062);
                    goto netserver_error_exit;
                    }  
                  intbuf[2]=intbuf[7];   
                  net_write(fd,intbuf,3*sizeof(int));
                  if(kill_all_flag)goto netserver_error_exit;
                  if(intbuf[7] == 0)
                    {
                    k=intbuf[1];
                    }
                  else
                    {
                    k=intbuf[7];
                    }  
                  kk=twice_rxchan*sizeof(float);
                  tmpbuf=malloc(kk*k);
                  if(tmpbuf == NULL)
                    {
                    lirerr(1290);
                    goto netserver_error_exit;
                    }
                  i=fread(tmpbuf, kk, k, file);
                  if(i != k)goto cal_error;
                  lir_sched_yield();
                  net_write(fd, tmpbuf, kk*k);
                  if(kill_all_flag)goto netserver_error_exit;
                  lir_sched_yield();
                  i=fread(tmpbuf, sizeof(float),k, file);
                  if(i != k)goto cal_error;
                  lir_sched_yield();
                  net_write(fd, tmpbuf, sizeof(float)*k);
                  if(kill_all_flag)goto netserver_error_exit;
                  fclose(file);
                  free(tmpbuf);
                  lir_sched_yield();
                  }
                else
                  {
// Send a flag so the client knows there is no calibration function.
                  s[0]=FALSE;
                  net_write(fd,s,sizeof(char));
                  }
                break;

                case NETMSG_CALIQ_REQUEST:
                if( (fft1_calibrate_flag&CALIQ) == CALIQ &&
                     diskread_flag < 2)
                  {
                  make_iqcorr_filename(s);
                  file = fopen(s, "rb");
                  if(file == NULL)
                    {
                    lirerr(1062);
                    goto netserver_error_exit;
                    }
// Send a flag so the client knows data will come.
                  s[0]=TRUE;
                  net_write(fd,s,sizeof(char));
// Read 10 integers. Send only the first, the number of segments for         
// the calibration function.
                  i=fread(intbuf, sizeof(int),10,file);
                  if(i != 10)goto cal_error;
                  net_write(fd,intbuf,sizeof(int));
                  kk=twice_rxchan*sizeof(float)*4*intbuf[0];
                  tmpbuf=malloc(kk);
                  if(tmpbuf == NULL)
                    {
                    lirerr(1291);
                    goto netserver_error_exit;
                    }
                  i=fread(tmpbuf, 1, kk, file);  
                  if(i != kk)goto cal_error;
                  fclose(file);
                  lir_sched_yield();
                  net_write(fd,tmpbuf, kk);
                  lir_sched_yield();
                  }
                else
                  {
                  s[0]=FALSE;
                  net_write(fd,s,sizeof(char));
                  }
                break;

                case NETMSG_FFT1INFO_REQUEST:
                netdat[0]=genparm[FIRST_FFT_SINPOW];
                netdat[1]=genparm[FIRST_FFT_BANDWIDTH];
                netdat[2]=genparm[FIRST_FFT_VERNR];
                net_write(fd,netdat, FFT1_INFOSIZ*sizeof(int));
                break;
               
                default:
                DEB"ILLEGAL NET MESSAGE");
                break;
                }
              }  
            else
              {
              }
            }
          }
        }
      }
    }
  }
netserver_error_exit:;
for(i = 0; i < no_of_netslaves; i++) 
  {
  fd=netfd.slaves[i];  
    {
    CLOSE_FD(fd);
    }  
  }


thread_status_flag[THREAD_LIR_SERVER]=THRFLAG_RETURNED;
while(thread_command_flag[THREAD_LIR_SERVER] != THRFLAG_NOT_ACTIVE)
  {
  lir_sleep(1000);
  }
}

void read_netgroup(char *group, int group_offset, int verbose, char *filename)
{
FILE *netad;
char s[80];
int i, n, k;
sprintf(group,"%s",DEFAULT_MULTI_GROUP);
if(group_offset >=0)
  {
  sprintf(&group[MULTI_GROUP_OFFSET],"%d", group_offset);
  }
else
  {
  netad=fopen(filename, "r");
  if(netad==NULL)
    {
    sprintf(s,"%s.txt",filename);
    netad=fopen(s, "r");
    if(netad == NULL)goto file_error;
    }
  k=fread(group,1,20,netad);
  fclose(netad);
  if(k < 7)goto file_error;
  i=0;
  n=0;
  while(i<k && (group[i] == '.' || (group[i] >='0' && group[i] <= '9')))
    {
    if(group[i] == '.')
      {
      n++;
      if(n > 3)
        {
        group[i]=0;
        i=18;
        }
      }
    i++;
    }  
  if(i < 7)goto file_error;
  group[i]=0;
  }  
return;
file_error:;
if(verbose)
  {
  clear_screen();
  sprintf(s,"Could not read file %s",filename);
  lir_text(10,10,s);
  sprintf(s,"Create %s with a text editor.",filename);
  lir_text(10,11,s);
  lir_text(10,12,"One line only. Format:  10.0.0.35");
  sprintf(s,"Without the file Linrad will use %s",DEFAULT_MULTI_GROUP);
  lir_text(10,14,s);
  lir_text(20,16,press_any_key);
  await_keyboard();
  clear_screen();
  }
}

void net_addr_strings(int vrb)
{
read_netgroup(netsend_rx_multi_group, net.send_group, vrb, par_netsend_filename);
read_netgroup(netrec_rx_multi_group, net.rec_group, vrb, par_netrec_filename);
}

int read_netpar(void)
{
int i, j, k;
FILE *file;
int *netparm;
char *parinfo;
netparm=(int*)&net;
file = fopen(network_filename, "rb");
if (file == NULL)
  {
net_default:;  
  net.send_group=0;
  net.rec_group=0;
  net.port=NET_MULTI_MINPORT;
  net.send_raw=0;
  net.send_fft1=0;
  net.send_fft2=0;
  net.send_timf2=0;
  net.receive_raw=NET_RXIN_RAW16;
  net.receive_fft1=0;
  net.receive_baseb=0;
  net.check=NET_VERNR;
  return FALSE;
  }
else
  {
  parinfo=malloc(4096);
  if(parinfo == NULL)
    {
    lir_errcod=1078;
    return FALSE;
    }
  for(i=0; i<4096; i++) parinfo[i]=0;
  i=fread(parinfo,1,4095,file);
  fclose(file);
  file=NULL;
  if(i == 4095)
    {
    goto net_default;
    }
  k=0;
  for(i=0; i<MAX_NET_INTPAR; i++)
    {
    while(parinfo[k]==' ' ||
          parinfo[k]== '\n' )k++;
    j=0;
    while(parinfo[k]== net_intpar_text[i][j])
      {
      k++;
      j++;
      } 
    if(net_intpar_text[i][j] != 0)goto net_default;
    while(parinfo[k]!='[')k++;
    sscanf(&parinfo[k],"[%d]",&netparm[i]);
    while(parinfo[k]!='\n')k++;
    }
  }
if(net.send_group < -1 || net.send_group > 15)goto net_default;
if(net.rec_group < -1 || net.rec_group > 15)goto net_default;
if(net.port < NET_MULTI_MINPORT || net.port > NET_MULTI_MAXPORT)goto net_default;
if(net.send_raw < 0 || net.send_raw > 
  (NET_RXOUT_RAW16|NET_RXOUT_RAW18|NET_RXOUT_RAW24))goto net_default;
if(net.send_fft1 != 0 && net.send_fft1 != NET_RXOUT_FFT1)goto net_default;
if(net.send_fft2 != 0 && net.send_fft2 != NET_RXOUT_FFT2)goto net_default;
if(net.send_timf2 != 0 && net.send_timf2 != NET_RXOUT_TIMF2)goto net_default;
if(net.receive_raw != 0 && 
   net.receive_raw != NET_RXIN_RAW16 &&
   net.receive_raw != NET_RXIN_RAW18 &&
   net.receive_raw != NET_RXIN_RAW24 )goto net_default;
if(net.receive_fft1 != 0 && net.receive_fft1 != NET_RXIN_FFT1)goto net_default;
if(net.receive_baseb != 0 && net.receive_baseb != NET_RXIN_BASEB)goto net_default;
if(net.receive_raw != 0 && (net.receive_fft1+net.receive_baseb) != 0)goto net_default;
if(net.receive_baseb != 0 && net.receive_fft1 != 0)goto net_default;
if(net.send_raw != 0 && 
           (net.receive_raw+net.receive_fft1+net.receive_baseb) != 0)goto net_default;
if(net.send_fft1 != 0 && net.receive_fft1 != 0)goto net_default;
if(net.check != NET_VERNR)goto net_default;  
return TRUE;
}

void init_network(void)
{
fd_set fds, testfds;
struct timeval timeout;
int kk, mm, wtcnt;
float *tmpbuf;
int intbuf[10];
char s[80];
char *ss;
IP_MREQ mreq;
int i, j, k;
int server_flag;
unsigned char *ip;
int nbytes;
char msgx[sizeof(NET_RX_STRUCT)+2];
NET_RX_STRUCT *msg;
int *net_fds;
#if(OSNUM == OS_FLAG_LINUX)
unsigned int addrlen;
#endif
#if(OSNUM == OS_FLAG_WINDOWS)
int addrlen;
unsigned long int on;  
on=1;  
#endif
net_fds=(void*)(&netfd);
for(i=0; i<MAX_NET_FD; i++)
  {
  net_fds[i]=-1;
  }
net_write_errors=0;
no_of_netslaves=0;
netfft1_blknum=0;
netraw16_blknum=0;
netraw18_blknum=0;
netraw24_blknum=0;
nettimf2_blknum=0;
netfft2_blknum=0;
netbaseb_blknum=0;
next_blkptr_16=0;
next_blkptr_18=0;
next_blkptr_24=0;
next_blkptr_fft1=0;
next_blkptr_baseb=0;
netsend_ptr_16=0;
netsend_ptr_18=0;
netsend_ptr_24=0;
netsend_ptr_fft1=0;
netsend_ptr_timf2=0;
netsend_ptr_fft2=0;
netsend_ptr_baseb=0;
for(i=0; i<MAX_FREQLIST; i++)
  {
  netfreq_list[i]=-1;
  netfreq_curx[i]=-1;
  new_netfreq_curx[i]=-1;
  }
latest_listsend_time=0;
net_no_of_errors=0;
msg=(void*)msgx;
clear_screen();
server_flag=FALSE;
if(read_netpar()==FALSE)
  {
  lirerr(1265);
  return;
  }
net_addr_strings(FALSE);
#if(OSNUM == OS_FLAG_WINDOWS)
if(WSAStartup(2, &wsadata) != 0)
  {
  lirerr(1263);
  return;   
  }
#endif
if( (ui.network_flag & NET_RXOUT_RAW16) != 0)
  {
  if ((netfd.send_rx_raw16=socket(AF_INET,SOCK_DGRAM,0)) == INVSOCK) 
    {
    lirerr(1249);
    return;
    }
#if(OSNUM == OS_FLAG_WINDOWS)
  if (ioctlsocket(netfd.send_rx_raw16, FIONBIO, &on) < 0)lirerr(889666);  
#endif
#if(OSNUM == OS_FLAG_LINUX)
  fcntl(netfd.send_rx_raw16, F_SETFD, O_NONBLOCK);
#endif  
  memset(&netsend_addr_rxraw16,0,sizeof(netsend_addr_rxraw16));
  netsend_addr_rxraw16.sin_family=AF_INET;
  netsend_addr_rxraw16.sin_addr.s_addr=inet_addr(netsend_rx_multi_group);
  netsend_addr_rxraw16.sin_port=htons(net.port);
  server_flag=TRUE;
  }
if( (ui.network_flag & NET_RXOUT_RAW18) != 0)
  {
  if ((netfd.send_rx_raw18=socket(AF_INET,SOCK_DGRAM,0)) == INVSOCK) 
    {
    lirerr(1249);
    return;
    }
#if(OSNUM == OS_FLAG_WINDOWS)
  if (ioctlsocket(netfd.send_rx_raw18, FIONBIO, &on) < 0)lirerr(889666);  
#endif
#if(OSNUM == OS_FLAG_LINUX)
  fcntl(netfd.send_rx_raw18, F_SETFD, O_NONBLOCK);
#endif  
  memset(&netsend_addr_rxraw18,0,sizeof(netsend_addr_rxraw18));
  netsend_addr_rxraw18.sin_family=AF_INET;
  netsend_addr_rxraw18.sin_addr.s_addr=inet_addr(netsend_rx_multi_group);
  netsend_addr_rxraw18.sin_port=htons(net.port+1);
  server_flag=TRUE;
  }
if( (ui.network_flag & NET_RXOUT_RAW24) != 0)
  {
  if ((netfd.send_rx_raw24=socket(AF_INET,SOCK_DGRAM,0)) == INVSOCK) 
    {
    lirerr(1249);
    return;
    }
#if(OSNUM == OS_FLAG_WINDOWS)
  if (ioctlsocket(netfd.send_rx_raw24, FIONBIO, &on) < 0)lirerr(889666);  
#endif
#if(OSNUM == OS_FLAG_LINUX)
  fcntl(netfd.send_rx_raw24, F_SETFD, O_NONBLOCK);
#endif  
  memset(&netsend_addr_rxraw24,0,sizeof(netsend_addr_rxraw24));
  netsend_addr_rxraw24.sin_family=AF_INET;
  netsend_addr_rxraw24.sin_addr.s_addr=inet_addr(netsend_rx_multi_group);
  netsend_addr_rxraw24.sin_port=htons(net.port+2);
  server_flag=TRUE;
  }
if( (ui.network_flag & NET_RXOUT_FFT1) != 0)
  {
  if ((netfd.send_rx_fft1=socket(AF_INET,SOCK_DGRAM,0)) == INVSOCK) 
    {
    lirerr(1249);
    return;
    }
#if(OSNUM == OS_FLAG_WINDOWS)
  if (ioctlsocket(netfd.send_rx_fft1, FIONBIO, &on) < 0)lirerr(889666);  
#endif
#if(OSNUM == OS_FLAG_LINUX)
  fcntl(netfd.send_rx_fft1, F_SETFD, O_NONBLOCK);
#endif  
  memset(&netsend_addr_rxfft1,0,sizeof(netsend_addr_rxfft1));
  netsend_addr_rxfft1.sin_family=AF_INET;
  netsend_addr_rxfft1.sin_addr.s_addr=inet_addr(netsend_rx_multi_group);
  netsend_addr_rxfft1.sin_port=htons(net.port+3);
  server_flag=TRUE;
  }
if( (ui.network_flag & NET_RXOUT_TIMF2) != 0)
  {
  if ((netfd.send_rx_timf2=socket(AF_INET,SOCK_DGRAM,0)) == INVSOCK) 
    {
    lirerr(1249);
    return;
    }
#if(OSNUM == OS_FLAG_WINDOWS)
  if (ioctlsocket(netfd.send_rx_timf2, FIONBIO, &on) < 0)lirerr(889666);  
#endif
#if(OSNUM == OS_FLAG_LINUX)
  fcntl(netfd.send_rx_timf2, F_SETFD, O_NONBLOCK);
#endif  
  memset(&netsend_addr_rxtimf2,0,sizeof(netsend_addr_rxtimf2));
  netsend_addr_rxtimf2.sin_family=AF_INET;
  netsend_addr_rxtimf2.sin_addr.s_addr=inet_addr(netsend_rx_multi_group);
  netsend_addr_rxtimf2.sin_port=htons(net.port+4);
  }
if( (ui.network_flag & NET_RXOUT_FFT2) != 0)
  {
  if ((netfd.send_rx_fft2=socket(AF_INET,SOCK_DGRAM,0)) == INVSOCK) 
    {
    lirerr(1249);
    return;
    }
#if(OSNUM == OS_FLAG_WINDOWS)
  if (ioctlsocket(netfd.send_rx_fft2, FIONBIO, &on) < 0)lirerr(889666);  
#endif
#if(OSNUM == OS_FLAG_LINUX)
  i=fcntl(netfd.send_rx_fft2, F_SETFL, O_NONBLOCK);
#endif  
  memset(&netsend_addr_rxfft2,0,sizeof(netsend_addr_rxfft2));
  netsend_addr_rxfft2.sin_family=AF_INET;
  netsend_addr_rxfft2.sin_addr.s_addr=inet_addr(netsend_rx_multi_group);
  netsend_addr_rxfft2.sin_port=htons(net.port+5);
  }
if( (ui.network_flag & NET_RXOUT_BASEB) != 0)
  {
  if ((netfd.send_rx_baseb=socket(AF_INET,SOCK_DGRAM,0)) == INVSOCK) 
    {
    lirerr(1249);
    return;
    }
#if(OSNUM == OS_FLAG_WINDOWS)
  if (ioctlsocket(netfd.send_rx_baseb, FIONBIO, &on) < 0)lirerr(889666);  
#endif
#if(OSNUM == OS_FLAG_LINUX)
  i=fcntl(netfd.send_rx_baseb, F_SETFL, O_NONBLOCK);
#endif  
  memset(&netsend_addr_rxbaseb,0,sizeof(netsend_addr_rxbaseb));
  netsend_addr_rxbaseb.sin_family=AF_INET;
  netsend_addr_rxbaseb.sin_addr.s_addr=inet_addr(netsend_rx_multi_group);
  netsend_addr_rxbaseb.sin_port=htons(net.port+6);
  server_flag=TRUE;
  }
if(server_flag == TRUE)
  {
// We are sending input to another Linrad program.
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    if( (ui.network_flag&NET_RXOUT_RAW18)!=0)
      {
      lirerr(1250);
      return;
      }
    if( (ui.network_flag&NET_RXOUT_RAW24)!=0)
      {
      lirerr(1251);
      return;
      }
    }
// Set up the file descriptor for our server.
  netfd.any_slave=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(netfd.any_slave == INVSOCK)
    {
    lirerr(1249);
    return;
    }
// Set small buffers. We use TCP and will not loose data anyway.
// A small buffer will use up CPU time in small chunks so
// it will be harmless to the time-critical DSP processing
// and UDP network services.
  k=sizeof(int);
  i=8192;
  setsockopt(netfd.rec_rx,SOL_SOCKET,SO_RCVBUF,(char*)&i,k);
  setsockopt(netfd.rec_rx,SOL_SOCKET,SO_SNDBUF,(char*)&i,k);
  i=net.send_raw+net.send_fft1+net.send_fft2+net.send_timf2+net.send_baseb;
  if( ((i ^ ui.network_flag)&NET_RX_OUTPUT)!=0)
    {
    lirerr(1252);
    return;
    }
  memset(&slave_addr,0,sizeof(slave_addr));
  slave_addr.sin_family = AF_INET;
  slave_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  slave_addr.sin_port = htons(CONNPORT);
  i=bind(netfd.any_slave, (struct sockaddr *)&slave_addr, sizeof(slave_addr));
  if(i!=0)
    {
    lirerr(1275);
    return;
    }
  i=listen(netfd.any_slave, 5);
  if(i!=0)
    {
    lirerr(1276);
    return;
    }
  FD_ZERO(&master_readfds);
  FD_SET(netfd.any_slave, &master_readfds);
  }
if( (ui.network_flag & NET_RX_INPUT) != 0)
  {
  i=ui.network_flag|=net.receive_raw+net.receive_fft1;
  if( ((i ^ ui.network_flag)&NET_RX_INPUT)!=0)lirerr(1252);
  if ((netfd.rec_rx=socket(AF_INET,SOCK_DGRAM,0)) == INVSOCK) 
    {
    lirerr(1249);
    return;
    }
// Set a big RX buffer. Defaults: Linux=109568 and Windows=8192.
// Set the TX buffer small. We will not send on this socket.
  k=sizeof(int);
  i=131072;
  setsockopt(netfd.rec_rx,SOL_SOCKET,SO_RCVBUF,(char*)&i,k);
  i=8192;
  setsockopt(netfd.rec_rx,SOL_SOCKET,SO_SNDBUF,(char*)&i,k);
// allow multiple sockets to use the same PORT number 
  k=1;
  i=setsockopt(netfd.rec_rx,SOL_SOCKET,SO_REUSEADDR,(char*)&k,sizeof(int));
  if(i < 0) 
    {
    lirerr(1266);
    return;
    }
  memset(&netrec_addr,0,sizeof(netrec_addr));
  netrec_addr.sin_family=AF_INET;
  netrec_addr.sin_addr.s_addr=htonl(INADDR_ANY);
  rx_input_thread=THREAD_RX_RAW_NETINPUT;
  switch (ui.network_flag & NET_RX_INPUT)
    {
    case NET_RXIN_RAW16:
    ss="RAW16";
    i=0;
    break;

    case NET_RXIN_RAW18:
    ss="RAW18";
    i=1;
    break;
    
    case NET_RXIN_RAW24:
    ss="RAW24";
    i=2;
    break;

    case NET_RXIN_FFT1:
    ss="FFT1";
    i=3;
    rx_input_thread=THREAD_RX_FFT1_NETINPUT;
    break;

    case NET_RXIN_BASEB:
    ss="BASEBAND";
    i=6;
    break;

    default:
    lirerr(1253);
    return;
    }
  sprintf(s,"Listening to network for %s RX input",ss);
  lir_text(0,1,s);
  sprintf(s,"Group %s,  port %d", netrec_rx_multi_group, net.port+i);
  lir_text(0,2,s);
  lir_sched_yield();
  lir_refresh_screen();
  netrec_addr.sin_port=htons(net.port+i);
// bind to receive address
  if (bind(netfd.rec_rx,(struct sockaddr *) &netrec_addr,
                                                sizeof(netrec_addr)) < 0) 
    {
    lirerr(1267);
    return;
    }
// use setsockopt() to request that the kernel join a multicast group
  mreq.imr_multiaddr.s_addr=inet_addr(netrec_rx_multi_group);
  mreq.imr_interface.s_addr=htonl(INADDR_ANY);
  if (setsockopt(netfd.rec_rx,IPPROTO_IP,IP_ADD_MEMBERSHIP,
                                           (char*)&mreq,sizeof(mreq)) < 0) 
    {
    lirerr(1268);
    return;
    }
// Go into an endless loop waiting for multicast to be received
// from another Linrad system.
  nbytes=0;
  i=0;
  test_keyboard();
  FD_ZERO(&fds);
  FD_SET(netfd.rec_rx, &fds);
  wtcnt=0;
  while(nbytes <= 0 && lir_inkey == 0)
    {
    if(kill_all_flag)return;
    testfds=fds;
    timeout.tv_sec=0;
    timeout.tv_usec=30000;
    if( select(FD_SETSIZE, &testfds, (fd_set *)0,  (fd_set *)0, &timeout) > 0)
      {   
      if(FD_ISSET(netfd.rec_rx,&testfds))
        {
        memset(&netmaster_addr,0,sizeof(netmaster_addr));
        addrlen=sizeof(netmaster_addr);
        nbytes=recvfrom(netfd.rec_rx,(char*)msg,sizeof(NET_RX_STRUCT)+1,0, 
                         (struct sockaddr *) &netmaster_addr, &addrlen);
        if(nbytes != sizeof(NET_RX_STRUCT))
          {
          i++;
          sprintf(s,"NET RX ERROR %d    received %d expected %d",i,
                                                nbytes,sizeof(NET_RX_STRUCT));
          lir_text(1,4,s);
          nbytes=0;
          }
        lir_sched_yield();  
        lir_refresh_screen();
        }
      }  
    else
      {
      wtcnt++;
      if(wtcnt > 999)wtcnt=0;
      sprintf(s,"%d   ",wtcnt);
      lir_text(5,0,s);
      lir_refresh_screen();
      lir_sleep(100000);
      test_keyboard();
      }          
    }    
  if(lir_inkey != 0)
    {
    lir_inkey=0;
    lirerr(1269);
    return;
    }  
  ip=(void*)&netmaster_addr.sin_addr.s_addr;
  sprintf(s,"received %s data from   %d.%d.%d.%d", ss, 
                                                 ip[0], ip[1], ip[2], ip[3]);
  lir_text(1,4,s);
  lir_sched_yield();
  lir_refresh_screen();
  i=0;
  if ((netfd.master = socket(AF_INET, SOCK_STREAM, 0)) == INVSOCK) 
    {
    lirerr(1249);
    return;
    }
  netmaster_addr.sin_family=AF_INET;
  netmaster_addr.sin_port=htons(CONNPORT);
  j = connect(netfd.master, (struct sockaddr *)&netmaster_addr, 
                                               sizeof(netmaster_addr) );
  lir_sleep(100000);
  while(j == -1 && lir_inkey == 0)
    {
    sprintf(s,"Trying to connect %d,  errno=%d",i,errno);
    lir_text(1,5,s);
    lir_refresh_screen();
    j = connect(netfd.master, (struct sockaddr *)&netmaster_addr, 
                                               sizeof(netmaster_addr) );
    test_keyboard();
    if(kill_all_flag) return;
    i++;
    lir_sleep(100000);
    }
  if(lir_inkey != 0)
    {
    lir_inkey=0;
    lirerr(1269);
    return;
    }
  lir_sched_yield();    
  lir_refresh_screen();
  clear_lines(5,5);
  lir_text(1,5,"Connect sucessful !!");
  lir_text(1,6,"Waiting for mode info");
  lir_refresh_screen();
  if( (ui.network_flag&NET_RXIN_BASEB) !=0)
    {
    slave_msg.type=NETMSG_BASEBMODE_REQUEST;
    }
  else
    {  
    slave_msg.type=NETMSG_MODE_REQUEST;
    }
  slave_msg.frequency=-1;
  net_write(netfd.master, &slave_msg, sizeof(slave_msg));
  if(kill_all_flag)return;
  net_read(intbuf,8*sizeof(int));
  if(kill_all_flag)return;
  ui.rx_ad_speed=intbuf[0];
  ui.rx_ad_channels=intbuf[1];
  ui.rx_rf_channels=intbuf[2];
  ui.rx_input_mode=intbuf[3];
  rxad.block_bytes=intbuf[4];
  if( (ui.network_flag & NET_RXIN_FFT1) != 0)
    {
    fft1_size=intbuf[5];
    fft1_n=intbuf[6];
    }
  init_genparm(FALSE);
  if( (ui.network_flag & NET_RXIN_FFT1) != 0)
    {
    genparm[FIRST_FFT_SINPOW]=intbuf[7];
    }
  if( (ui.rx_input_mode&DWORD_INPUT) != 0 &&
                    (ui.network_flag&NET_RXIN_RAW16) !=0 )
    {
    ui.rx_input_mode&=DWORD_INPUT^0xffffffff;
    rxad.block_bytes/=2;
    }
  fg.passband_direction=0;
  fft1_direction=0;
  get_wideband_sizes();
  if(kill_all_flag) return;
  get_buffers(1);
  if(kill_all_flag) return;
  mm=2*ui.rx_rf_channels;
  fft1_calibrate_flag=0;
  clear_screen();
  lir_text(1,6,"Waiting for filter calibration info");
  lir_refresh_screen();
  slave_msg.type=NETMSG_CAL_REQUEST;
  slave_msg.frequency=-1;
  net_write(netfd.master, &slave_msg, sizeof(slave_msg));
  if(kill_all_flag)return;
  net_read(s,sizeof(char));
  lir_refresh_screen();
  if(s[0] == TRUE)
    {
// The master says there is a calibration function get it.
    net_read(intbuf,3*sizeof(int));
    if(kill_all_flag)return;
    if(intbuf[2] == 0)
      {
// The filtercorr function is saved in the frequency domain in 
// intbuf[1] points.
// This format is used during the initialisation procedures and
// it may be kept for normal operation in case the number of points
// will not be reduced by saving in the time domain.  
      tmpbuf=malloc(intbuf[1]*(mm+1)*sizeof(float));
      if( tmpbuf == NULL )
        {
        lirerr(1293);
        return;
        }
      net_read(tmpbuf, mm*sizeof(float)*intbuf[1]);
      if(kill_all_flag)return;
      net_read(&tmpbuf[mm*intbuf[1]], sizeof(float)*intbuf[1]);
      if(kill_all_flag)return;
      use_filtercorr_fd(intbuf[0], intbuf[1], tmpbuf, &tmpbuf[mm*intbuf[1]]);

      }
    else
      {
// The correction function was stored in the time domain in intbuf[2] points
// We have to reduce the number of points.
      tmpbuf=malloc(intbuf[2]*(mm+1)*sizeof(float));
      if( tmpbuf == NULL )
        {
        lirerr(1055);
        return;
        }
      net_read(tmpbuf, mm*sizeof(float)*intbuf[2]);
      if(kill_all_flag)return;
      net_read(&tmpbuf[mm*intbuf[2]], sizeof(float)*intbuf[2]);
      if(kill_all_flag)return;
      use_filtercorr_td(intbuf[2], tmpbuf, &tmpbuf[mm*intbuf[2]]);
      }
    }    
  else
    {
    clear_fft1_filtercorr();
    }
  lir_text(1,6,"Waiting for I/Q balance calibration info");
  slave_msg.type=NETMSG_CALIQ_REQUEST;
  slave_msg.frequency=-1;
  net_write(netfd.master, &slave_msg, sizeof(slave_msg));
  if(kill_all_flag)return;
  net_read(s,sizeof(char));
  if(kill_all_flag)return;
  if(s[0] == TRUE)
    {
    net_read(intbuf,sizeof(int));
    if(kill_all_flag)return;
    bal_segments=intbuf[0];
    kk=twice_rxchan*sizeof(float)*4*bal_segments;
    contracted_iq_foldcorr=malloc(kk);
    if(contracted_iq_foldcorr == NULL)
      {
      lirerr(1049);
      return;
      }
    net_read(contracted_iq_foldcorr, kk);  
    if(kill_all_flag)return;
    use_iqcorr();
    free(contracted_iq_foldcorr);
    }
  if( (ui.network_flag&NET_RXIN_BASEB) ==0)
    {
    fg.passband_center=msg[0].passband_center;
    fg.passband_direction=msg[0].passband_direction;
    freq_from_file=TRUE;
    }
  fft1_direction=fg.passband_direction;
  FD_ZERO(&rx_input_fds);
  FD_SET(netfd.rec_rx, &rx_input_fds);
  clear_lines(0,8);
  lir_refresh_screen();
  return;
  }
// We do not use the network for input.
// This means we run as a master. Prepare to listen for
// slaves that want to connect to us.  
get_wideband_sizes();
if(kill_all_flag) return;
get_buffers(1);
if(kill_all_flag) return;
}

void sendraw_text( int mask, int bits, int offset)
{
char ss[20], s[40];
sprintf(s,"%2d: Send raw data in %d bit format ",offset, bits);
if( (net.send_raw&mask) == 0)
  {
  strcat(s,"OFF");
  }
else
  {
  settextcolor(14);
  sprintf(ss,"ON  (port=%d)",net.port+offset-NN_OUT_ZERO);
  strcat(s,ss);
  }
lir_text(1,NNLINE+offset,s);
settextcolor(7);
}  

void sendfft_text( int flag, int no, int offset)
{
char ss[20], s[40];
sprintf(s,"%2d: Send FFT%d transforms ",offset, no);
if( flag == FALSE)
  {
  strcat(s,"OFF");
  }
else
  {
  settextcolor(14);
  sprintf(ss,"ON  (port=%d)",net.port+offset-NN_OUT_ZERO);
  strcat(s,ss);
  }
lir_text(1,NNLINE+offset,s);
settextcolor(7);
}

int get_ip_address(int *line, char *dir)
{
char s[80];
char *nam;
sprintf(s,"Default address is %s to",DEFAULT_MULTI_GROUP);
lir_text(10,line[0],s);
s[19+MULTI_GROUP_OFFSET]='1';
s[19+1+MULTI_GROUP_OFFSET]='5';
s[19+2+MULTI_GROUP_OFFSET]=0;
lir_text(10+19+MULTI_GROUP_OFFSET+6,line[0],&s[19]);
line[0]++;
if(dir[0]=='S')
  {
  nam=par_netsend_filename;
  }
else  
  {
  nam=par_netrec_filename;
  }
sprintf(s,"Set new %s address -1 to read from file %s", dir, nam); 
lir_text(10,line[0],s);
line[0]++;
lir_text(10,line[0],"or (0 to 15) to use default =>");
return lir_get_integer(40,line[0],2,-1,15);
}

void network_setup(char *msg)
{
int i, line;
char  s[80];
char ss[20];
FILE *file;
int *netparm;
netparm=(int*)&net;
sel:;
clear_screen();
lir_text(0,1,msg);
net_addr_strings(TRUE);
lir_text(3,3,"CURRENT NETWORK SETTINGS ARE:");
sprintf(s,"%2d:     Base port = %d  ", NN_BASE, net.port);
lir_text(1,NN_BASE+NNLINE,s);
sprintf(s,"%2d:    SEND address = %s  ",NN_SENDAD, netsend_rx_multi_group);
lir_text(1,NN_SENDAD+NNLINE,s);
sprintf(s,"%2d: RECEIVE address = %s  ",NN_RECAD, netrec_rx_multi_group);
lir_text(1,NN_RECAD+NNLINE,s);
sendraw_text(NET_RXOUT_RAW16,16,NN_RAWOUT16);
sendraw_text(NET_RXOUT_RAW18,18,NN_RAWOUT18);
sendraw_text(NET_RXOUT_RAW24,24,NN_RAWOUT24);
sendfft_text(net.send_fft1, 1, NN_FFT1OUT);
sprintf(s,"%2d: Send timf2 (blanker output) ",NN_TIMF2OUT);
if( net.send_timf2 == FALSE)
  {
  strcat(s,"OFF");
  }
else
  {
  settextcolor(14);
  sprintf(ss,"ON  (port=%d)",net.port+NN_TIMF2OUT-NN_OUT_ZERO);
  strcat(s,ss);
  }
lir_text(1,NNLINE+NN_TIMF2OUT,s);
settextcolor(7);
sprintf(s,"%2d: Send baseband (resampled) ",NN_BASEBOUT);
if( net.send_baseb == FALSE)
  {
  strcat(s,"OFF");
  }
else
  {
  settextcolor(14);
  sprintf(ss,"ON  (port=%d)",net.port+NN_BASEBOUT-NN_OUT_ZERO);
  strcat(s,ss);
  }
lir_text(1,NNLINE+NN_BASEBOUT,s);
settextcolor(7);
sendfft_text(net.send_fft2, 2, NN_FFT2OUT);
sprintf(s,"%2d: RX input from network ",NN_RX);
if( net.receive_raw+net.receive_fft1+net.receive_baseb == 0)
  {
  strcat(s,"OFF");
  }
else
  {
  strcat(s,"ON ");
  settextcolor(14);
  if( net.receive_fft1 == NET_RXIN_FFT1)
    {
    strcat(s,"(FFT1 transforms)");
    }
  else
    {
    if( net.receive_baseb == NET_RXIN_BASEB)
      {
      strcat(s,"(Baseband)");
      }
    else
      {  
      strcat(s,"(Raw data, ");  
      if( net.receive_raw == NET_RXIN_RAW16)
        {
        strcat(s,"16");  
        }
      else
        {
        if( net.receive_raw == NET_RXIN_RAW18)
          {
          strcat(s,"18");  
          }
        else
          {
          if( net.receive_raw == NET_RXIN_RAW24)
            {
            strcat(s,"24");  
            }
          else
            {
            lirerr(42962);
            return;
            }  
          }
        }
      strcat(s," bit)");  
      }
    }
  }
lir_text(1,NNLINE+NN_RX,s);
settextcolor(7);
line=1+NNLINE+NN_RX;
lir_text(1,line,"F1: Help");
line+=2;
lir_text(0,line,"On exit from this routine network will be OFF.");
line++;
lir_text(0,line,"The screen shows what will become enabled when send");
line++;
lir_text(0,line,
       "or receive is enabled (with ""R"" or ""T"" on the main menu)");
line++;
lir_text(0,line,remind_parsave);
line+=2;
lir_text(0,line,   "Enter a line number to change, 0 to exit =>");
i=lir_get_integer(44,line,2,0,NN_RX);
if(kill_all_flag) return;
if(lir_inkey == F1_KEY)
  {
  help_message(315);
  goto sel;
  }
line+=3;
switch (i)
  {
  case NN_RX:
  lir_text(10,line,"Set receive format.");
  lir_text(10,line+2,"A=Raw 16 bit");
  lir_text(10,line+3,"B=Raw 18 bit");
  lir_text(10,line+4,"C=Raw 24 bit");
  lir_text(10,line+5,"D=FFT1 transforms");
  lir_text(10,line+6,"E=Baseband");
  lir_text(10,line+7,"Z=None");
get_r:;
  toupper_await_keyboard();
  if(kill_all_flag) return;
  net.receive_raw=0;
  net.receive_fft1=0;
  net.receive_baseb=0;
  switch (lir_inkey)
    {
    case 'A':
    net.receive_raw=NET_RXIN_RAW16;
    break;

    case 'B':
    net.receive_raw=NET_RXIN_RAW18;
    break;

    case 'C':
    net.receive_raw=NET_RXIN_RAW24;
    break;

    case 'D':
    net.receive_fft1=NET_RXIN_FFT1;
    break;

    case 'E':
    net.receive_baseb=NET_RXIN_BASEB;
    break;

    case 'Z':
    break;

    default:
    goto get_r;
    }
  if(net.receive_raw != 0)net.send_raw=0;
  if(net.receive_fft1 != 0)
    {
    net.send_raw=0;
    net.send_fft1=0;
    }
  break;
    
  case NN_BASE:
  sprintf(s,"Base port (%d to %d) =>",NET_MULTI_MINPORT, NET_MULTI_MAXPORT);
  lir_text(10,line,s);
  net.port=lir_get_integer(40,line,5,NET_MULTI_MINPORT,NET_MULTI_MAXPORT);
  net.port=(net.port/10)*10;
  break;

  case NN_SENDAD:
  net.send_group=get_ip_address(&line, "SEND");
  break;

  case NN_RECAD:
  lir_text(10,line,"New RECEIVE address (0 to 15) =>");
  net.rec_group=get_ip_address(&line, "RECEIVE");
  break;

  case NN_RAWOUT16:
  net.send_raw^=NET_RXOUT_RAW16;
  break;
  
  case NN_RAWOUT18:
  net.send_raw^=NET_RXOUT_RAW18;
  break;

  case NN_RAWOUT24:
  net.send_raw^=NET_RXOUT_RAW24;
  break;
  
  case NN_FFT1OUT:
  net.send_fft1^=NET_RXOUT_FFT1;
  break;

  case NN_TIMF2OUT:
  net.send_timf2^=NET_RXOUT_TIMF2;
  break;

  case NN_FFT2OUT: 
  net.send_fft2^=NET_RXOUT_FFT2;
  break;

  case NN_BASEBOUT: 
  net.send_baseb^=NET_RXOUT_BASEB;
  break;

  case 0:
  goto netok;
  }
if(kill_all_flag)return;
if(net.send_fft1 != 0)net.receive_fft1=0;
if(net.send_raw != 0)
  {
  net.receive_raw=0;
  net.receive_fft1=0;
  }
goto sel;
netok:;
file = fopen(network_filename, "w");
if (file == NULL)
  {
  lirerr(1029);
  return;
  }
for(i=0; i<MAX_NET_INTPAR; i++)
  {
  fprintf(file,"%s [%d]\n",net_intpar_text[i],netparm[i]);
  }
parfile_end(file);
ui.network_flag=0;
}

void manage_network(unsigned int change)
{
int k, flag;
char *msg;
// Calling with change 0 means network setup.
// Call can be made with NET_RX_INPUT and NET_RX_OUTPUT to toggle
// the network activities.
// If the relevant part of ui.network_flag is zero we have to set it
// and we use the info in the parameter file to do so.
// If the parameter file is missing or incorrect we force a
// network setup before setting the flag.
// The strategy means that Linrad will start directly into the
// same network state as it had during the latest parameter save because
// ui.network_flag is one of them and it controls the network operations.
flag=1;
if(change != 0)
  {
  if( (change&ui.network_flag) !=0)
    {
    ui.network_flag&=(change^0xffffffff);
    return;
    }
  flag=0;  
  }  
if(read_netpar()==FALSE)flag=1;
if(flag == 1)
  {
  msg=" ";
setup:;  
  ui.network_flag=0;  
  network_setup(msg);
  }
else
  {  
  if(change == NET_RX_INPUT)
    {
    k=(net.receive_raw&(NET_RXIN_RAW16+NET_RXIN_RAW18+NET_RXIN_RAW24))+
      (net.receive_fft1&NET_RXIN_FFT1)+
      (net.receive_baseb&NET_RXIN_BASEB);
    msg="Select what format to use for input";
    }
  else  
    {
    k=(net.send_raw&(NET_RXOUT_RAW16|NET_RXOUT_RAW18|NET_RXOUT_RAW24))+
      (net.send_fft1&NET_RXOUT_FFT1)+
      (net.send_fft2&NET_RXOUT_FFT2)+
      (net.send_timf2&NET_RXOUT_TIMF2)+
      (net.send_baseb&NET_RXOUT_BASEB);
    msg="You have to enable at least one mode to start network transmit.";
    }
  if(k==0)goto setup; 
  ui.network_flag|=k;
  }
}

void net_read( void *buf, int bytes)
{
int i, network_nread; 
char s[80];
int nread, rdbytes;
int prtwait, netwait;
netwait=0;
prtwait=0;
#if(OSNUM == OS_FLAG_LINUX)
ioctl(netfd.master, FIONREAD, &network_nread);
#endif
#if(OSNUM == OS_FLAG_WINDOWS)
network_nread=recv(netfd.master, buf, bytes, MSG_PEEK);
#endif
rdbytes=0;
while(rdbytes < bytes)
  {
  nread=network_nread;
  if(nread > 0)
    {
    if(nread+rdbytes > bytes)nread=bytes-rdbytes;
    i=recv(netfd.master, buf+rdbytes, nread, 0);
    if(i==0)
      {
      lirerr(1273);
      return;
      }
    if(i==-1)
      {
      lirerr(1274);
      return;
      }
    rdbytes+=i;
    network_nread-=i;
    }
  else
    {
    if(prtwait==0)
      {
      sprintf(s,"WAITING FOR NETWORK %d   ",netwait);
      lir_text(5,8,s);
      prtwait=-5;
      }
    prtwait++;
    netwait++;
    lir_sleep(10000);
#if(OSNUM == OS_FLAG_LINUX)
    ioctl(netfd.master, FIONREAD, &network_nread);
#endif
#if(OSNUM == OS_FLAG_WINDOWS)
    network_nread=recv(netfd.master, buf+rdbytes, bytes-rdbytes, MSG_PEEK);
#endif
    lir_refresh_screen();
    test_keyboard();
    if(kill_all_flag) return;
    if(lir_inkey != 0)
      {
      process_current_lir_inkey();
      }
    if(kill_all_flag) return;
    } 
  }
}

void net_write(FD fd, void *buf, int size)
{
struct timeval timeout;
int i, j, ptr, remaining;
fd_set fds, testfds;
FD_ZERO(&fds);
FD_SET(fd, &fds);
ptr=0;
remaining=size;
while(remaining > 0)
  {
  lir_sched_yield();
  j=remaining;
  if(remaining > 1500)
    {
    j=1500;
    if(j>remaining/2)j=remaining/2;
    }
  testfds=fds;
  timeout.tv_sec=0;
  timeout.tv_usec=10000;
  if( select(FD_SETSIZE, (fd_set *)0, &testfds,  (fd_set *)0, &timeout) > 0)
    {   
    if(FD_ISSET(fd,&testfds))
      {
      i=send(fd, buf+ptr, j,0);
      if(i>0)
        {
        remaining-=i;
        ptr+=i;
        }
      else
        {
        if(i==0)
          {
          lirerr(1264);
          }
        else
          {
#if(OSNUM == OS_FLAG_WINDOWS)
          i=WSAGetLastError();
          DEB"\nerror code=%d",i);
#endif
#if(OSNUM == OS_FLAG_LINUX)
          DEB"\nerrno=%d",errno);
#endif
          lirerr(1101);
          }    
        return;
        }
      }
    if(kill_all_flag) return;
    }
  }  
}

   
