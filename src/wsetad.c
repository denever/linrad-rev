
// *********************************************************************
// *  This software uses the Perseus API which is defined through the  *
// *  following four files of the Microtelecom Software Defined Radio  *
// *  Developer Kit (SDRK):                                            *
// *  main.cpp, perseusdll.cpp, perseusdefs.h, perseusdll.h            *
// *  To use the Perseus hardware you need the perseususb.dll file     *
// *  You also need at least one of the following files:               *
// *  perseus125k24v21.sbs                                             *
// *  perseus250k24v21.sbs                                             *
// *  perseus500k24v21.sbs                                             *
// *  perseus1m24v21.sbs                                               *
// *  perseus2m24v21.sbs                                               *
// *  The dll and the sbs files are copyright protected and only       *
// *  available at the official Microtelecom Internet site:            *
// *  http://www.microtelecom.it                                       *
// *********************************************************************

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
#include "sdrdef.h"
#include "txdef.h"
#include "vernr.h"
#include "keyboard_def.h"
#include "thrdef.h"
#include "hwaredef.h"
#include "perseusdef.h"
#include "dlldef.h"
#include "options.h"

sdrWrite P_Write;
sdrRead P_Read;
sdrClose P_Close;
sdrSetTimeouts P_SetTimeouts;
FT_HANDLE sdr14_handle;

WAVEFORMATEXTENSIBLE fmt;
HINSTANCE h_sdr14;
HINSTANCE h_pers;

PtrToGetDLLVersion		m_pGetDLLVersion;
PtrToOpen			m_pOpen;
PtrToClose			m_pClose;
PtrToGetDeviceName		m_pGetDeviceName;
PtrToGetDeviceName		m_pGetFriendlyDeviceName;
PtrToEepromFunc			m_pEepromRead;
PtrToFwDownload			m_pFwDownload;
PtrToFpgaConfig			m_pFpgaConfig;
PtrToSetAttPresel		m_pSetAttenuator;
PtrToSetAttPresel		m_pSetPreselector;
PtrToSio                        m_pSio;
PtrToStartAsyncInput		m_pStartAsyncInput;
PtrToStopAsyncInput             m_pStopAsyncInput;

int timf1p_pers;
int perseus_bytesperbuf;

unsigned int lir_tx_output_samples(void)
{
return 0;
}

int lir_tx_input_samples(void)
{
return 0;
}

void lir_tx_adread(char *buf)
{    
buf[0]=0;
/*
int nread;
nread=read(tx_audio_in, buf, tx_read_bytes); 
if(nread != tx_read_bytes)lirerr(1283);
*/
}  


VOID CALLBACK tx_daout(HWAVEOUT hwi, UINT iMsg, DWORD dwInst, 
                         DWORD dwParam1, DWORD dwParam2)
{
DWORD i;
i=(DWORD)hwi;
i=dwInst;
i=dwParam2;
i=iMsg;
i=dwParam1;
if(iMsg == WIM_DATA)
  {
  }
else
  {
  if(iMsg == WIM_CLOSE)
    {
    i=(DWORD)hwi;
    i=dwInst;
    i=dwParam2;
    tx_audio_out=-1;
    }
  else
    {
    if(iMsg == WIM_OPEN)
      {
      tx_audio_out=0;
      }
    }  
  }
}                         

void open_tx_output(void)
{
int errcod;
WAVEFORMATEXTENSIBLE wfmt;
MMRESULT mmrs;
int i, k;
if(tx_audio_out != -1)return;
wfmt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
wfmt.Format.nSamplesPerSec=ui.tx_da_speed;
if(ui.tx_da_bytes == 2)
  {
  wfmt.Samples.wValidBitsPerSample=16;
  wfmt.Format.wBitsPerSample=16;
  }
else
  {
  wfmt.Samples.wValidBitsPerSample=24;
  wfmt.Format.wBitsPerSample=32;
  }
wfmt.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
wfmt.dwChannelMask = 7;
wfmt.Format.nChannels=ui.tx_da_channels;
wfmt.Format.nBlockAlign=(unsigned int)((wfmt.Format.nChannels *
                                         wfmt.Format.wBitsPerSample) / 8.0);
wfmt.Format.nAvgBytesPerSec=wfmt.Format.nBlockAlign*wfmt.Format.nSamplesPerSec;
wfmt.Format.cbSize=sizeof(WAVEFORMATEXTENSIBLE);;
mmrs=waveOutOpen(&hwav_txdaout, ui.tx_dadev_no, 
              (WAVEFORMATEX*)&wfmt, (DWORD)tx_daout, 0, CALLBACK_FUNCTION);
if(mmrs != MMSYSERR_NOERROR)
  {
  lirerr(1234);
  return;
  }
errcod=1237;
tx_waveout_buf=malloc(NO_OF_TX_WAVEOUT*tx_daout_block+wfmt.Format.nBlockAlign);
if(tx_waveout_buf==NULL)goto wave_close;
tx_wave_outhdr=malloc(NO_OF_TX_WAVEOUT*sizeof(WAVEHDR));
if(tx_wave_outhdr==NULL)goto free_waveout;
//öööööööö ??tx_da_wrbuf=malloc(tx_daout_block);
//ööööööö ??if(tx_da_wrbuf==NULL)
goto free_outhdr; //öööööööö


txdaout_newbuf_ptr=0;
for(i=0; i<NO_OF_TX_WAVEOUT; i++)
  {
  k=(int)(tx_waveout_buf+i*tx_daout_block+wfmt.Format.nBlockAlign);
  k&=(-1-wfmt.Format.nBlockAlign);
  tx_wave_outhdr[i].lpData=(char*)(k);
  tx_wave_outhdr[i].dwBufferLength=tx_daout_block;
  tx_wave_outhdr[i].dwBytesRecorded=0;
  tx_wave_outhdr[i].dwUser=1; 
  tx_wave_outhdr[i].dwFlags=0;
  tx_wave_outhdr[i].dwLoops=0;
  tx_wave_outhdr[i].lpNext=NULL;
  tx_wave_outhdr[i].reserved=0;
  mmrs=waveOutPrepareHeader(hwav_txdaout, &tx_wave_outhdr[i], sizeof(WAVEHDR));
  txdaout_newbuf[i]=&tx_wave_outhdr[i];
  if(mmrs != MMSYSERR_NOERROR)goto errfree;
  }
txda.buffer_bytes=NO_OF_TX_WAVEOUT*tx_daout_block;
tx_audio_out=0;
return;
errfree:;
//ÖÖfree(tx_da_wrbuf);
free_outhdr:;
free(tx_wave_outhdr);
free_waveout:;
free(tx_waveout_buf);
wave_close:;
waveOutClose(hwav_txdaout);
tx_audio_out=-1;
lirerr(errcod);
}



void close_tx_output(void)
{
/*
if(close(tx_audio_out)==-1)lirerr(231037);  
*/
tx_audio_out=-1;
}

void open_tx_input(void)
{
}

void close_tx_input(void)
{
tx_audio_in=-1;
}

int WINAPI ProcessInputCallback(char *pBuf, int nothing)
{
register char c1;
long old;
int i, j;
c1=0;
// This callback gets called whenever a buffer is ready 
// from the USB interface
j=timf1p_pers;
for(i=0; i<perseus_bytesperbuf; i+=6)
  {
  timf1_char[timf1p_pers+3]=pBuf[i+2]; 
  c1=pBuf[i+2]&0x80;
  timf1_char[timf1p_pers+2]=pBuf[i+1]; 
  timf1_char[timf1p_pers+1]=pBuf[i  ]; 
  c1|=pBuf[i];
  timf1_char[timf1p_pers  ]=0;
  timf1_char[timf1p_pers+7]=pBuf[i+5]; 
  timf1_char[timf1p_pers+6]=pBuf[i+4]; 
  timf1_char[timf1p_pers+5]=pBuf[i+3]; 
  timf1_char[timf1p_pers+4]=0;
  timf1p_pers=(timf1p_pers+8)&timf1_bytemask;
  }
if( (c1&1) != 0)
  {
  ad_maxamp[0]=0x7fff;
  ad_maxamp[1]=0x7fff;
  }
ReleaseSemaphore(rxin1_bufready,1,&old);
return nothing;
}

void lir_perseus_read(void)
{
wt:;
if( ((timf1p_pers-timf1p_pa+timf1_bytes)&timf1_bytemask) >= 
                                                      2*(int)rxad.block_bytes)
  {
  timf1p_pa=(timf1p_pa+rxad.block_bytes)&timf1_bytemask;
  return;
  }  
WaitForSingleObject(rxin1_bufready, INFINITE);
if(!kill_all_flag)goto wt;
}

int perseus_load_proprietary_code(int rate_no)
{
int i;
// Download the standard Microtelecom FX2 firmware to the Perseus receiver
// IMPORTANT NOTE:
// The parameter passed to PerseusFirmwareDownload() must be NULL
// for the Perseus hardware to be operated safely.
// Permanent damages to the receiver can result if a third-party firmware
// is downloaded to the Perseus FX2 controller. You're advised.
//
// During the execution of the PerseusFirmwareDownload() function
// the Perseus USB hardware is renumerated. This causes the function
// to be blocking for a while. It is suggested to put an hourglass cursor 
// before calling this function if the code has to be developed for a real
// Window application (and removing it when it returns)
i=(*m_pFwDownload)(NULL);
if(i != IHEX_DOWNLOAD_OK) 
  {
  switch (i)
    {
    case IHEX_DOWNLOAD_IOERROR:
    return 1083;

    case IHEX_DOWNLOAD_FILENOTFOUND:
    return 1084;
    
    case IHEX_DOWNLOAD_INVALIDHEXREC:
    return 1085;
    
    case IHEX_DOWNLOAD_INVALIDEXTREC:
    return 1089;
    
    default:
    return 1090;
    }
  }
// Download the FPGA bitstream to the Perseus' FPGA
// There are three bitstream provided with the current software version
// one for each of the output sample rate provided by the receiver
// (125, 250, 500, 1000 or 2000 kS/s)
// Each FPGA bistream (.sbs file) has an original Microtelecom 
// signature which must be provided as the second parameter to the 
// function PerseusFpgaConfig() to prevent the configuration of the 
// FPGA with non authorized, third-party, code which could damage 
// the receiver hardware.
// The bitstream files should be located in the same directory of 
// the executing software for things to work.
i=(*m_pFpgaConfig)( perseus_bitstream_names[rate_no], 
                    perseus_bitstream_signatures[rate_no]);
if(i != FPGA_CONFIG_OK)
  {
  switch (i) 
    {
    case FPGA_CONFIG_FILENOTFOUND:
    return 1092;
    
    case FPGA_CONFIG_IOERROR:
    return 1096;
    
    default:
    return 1097;
    }
  }
return 0;
}

int perseus_eeprom_read(unsigned char *buf, int addr, unsigned char count)
{
return (*m_pEepromRead)(buf, addr, count);
}

char *perseus_name(void)
{
return (*m_pGetDeviceName)();
}

char *perseus_frname(void)
{
return (*m_pGetFriendlyDeviceName)();
}

void perseus_store_att(unsigned char idAtt)
{
m_pSetAttenuator(idAtt);
}

void perseus_store_presel(unsigned char idPresel)
{
m_pSetPreselector(idPresel);
}

void perseus_store_sio(void)
{
m_pSio(&sioctl,sizeof(sioctl));
}

void open_perseus(void)
{
int j, k;
if(sdr != -1)return;
sdr=-1;
h_pers = LoadLibrary("perseususb.dll");
if(h_pers == NULL)
  {
  SNDLOG"\nUnable to load perseususb.dll");
  return;
  }
if(write_log)
  {
  lir_text(10,13,
         "If Linrad makes a silent exit, try again. If it does not work,");
  lir_text(10,14,
         "remove perseususb.dll if you do not want to use the Perseus.");
  lir_refresh_screen();
  lir_sleep(1500000);
  clear_lines(13,14);
  }
m_pGetDLLVersion	= (PtrToGetDLLVersion)GetProcAddress(h_pers,"GetDLLVersion");
m_pOpen                 = (PtrToOpen)GetProcAddress(h_pers,"Open");
m_pClose                = (PtrToClose)GetProcAddress(h_pers,"Close");
m_pGetDeviceName	= (PtrToGetDeviceName)GetProcAddress(h_pers,"GetDeviceName");
m_pGetFriendlyDeviceName= (PtrToGetDeviceName)GetProcAddress(h_pers,"GetFriendlyDeviceName");
m_pEepromRead		= (PtrToEepromFunc)GetProcAddress(h_pers,"EepromRead");
m_pFwDownload           = (PtrToFwDownload)GetProcAddress(h_pers,"FirmwareDownload");
m_pFpgaConfig		= (PtrToFpgaConfig)GetProcAddress(h_pers,"FpgaConfig");
m_pSetAttenuator	= (PtrToSetAttPresel)GetProcAddress(h_pers,"SetAttenuator");
m_pSetPreselector	= (PtrToSetAttPresel)GetProcAddress(h_pers,"SetPreselector");
m_pSio			= (PtrToSio)GetProcAddress(h_pers,"Sio");
m_pStartAsyncInput	= (PtrToStartAsyncInput)GetProcAddress(h_pers,"StartAsyncInput");
m_pStopAsyncInput	= (PtrToStopAsyncInput)GetProcAddress(h_pers,"StopAsyncInput");
SNDLOG"\nperseususb.dll loaded");
if( m_pGetDLLVersion == NULL ||
    m_pOpen == NULL ||
    m_pGetDeviceName == NULL ||
    m_pGetFriendlyDeviceName == NULL ||
    m_pEepromRead == NULL ||
    m_pFwDownload == NULL ||
    m_pFpgaConfig == NULL ||
    m_pSetAttenuator == NULL ||
    m_pSetPreselector == NULL ||
    m_pSio  == NULL ||
    m_pStartAsyncInput == NULL ||
    m_pStopAsyncInput == NULL
    )
  {
  SNDLOG"\nDLL function(s) missing");
free_pdll:;
  FreeLibrary(h_pers);
  return;
  }
j = m_pGetDLLVersion();
if (!j)
  {
  SNDLOG"\nDLLversion failed");
  goto free_pdll;
  }
k=j>>16;
j&=0xffff;
SNDLOG" vers %d.%d",k,j);  
j=m_pOpen();
if(j==FALSE)
  {
  SNDLOG"\nPerseus receiver not detected");
  if(write_log)fflush(sndlog);
  goto free_pdll;
  }
sdr=0;
}

void start_perseus_read(void)
{
int i;
sdr=-1;
rxin1_bufready=CreateSemaphore( NULL, 0, 1024, NULL);
if(rxin1_bufready== NULL)
  {
  close_perseus();
  lirerr(1223);
  return;
  }
perseus_bytesperbuf=rxad.block_bytes/PERSEUS_BYTESPERFRAME;
perseus_bytesperbuf*=PERSEUS_BYTESPERFRAME;
timf1p_pers=timf1p_pa;
lir_sched_yield();
i=m_pStartAsyncInput(perseus_bytesperbuf, ProcessInputCallback, 0);
if(i == FALSE)
  {
  close_perseus();
  lirerr(1098);
  return;
  }  
// Enable the Perseus data FIFO through its SIO interface
// setting the PERSEUS_SIO_FIFOEN bit in the ctl field of 
// the (global) sioctl variable.
sioctl.ctl |=PERSEUS_SIO_FIFOEN;
perseus_store_sio();
sdr=0;
}

void perseus_stop(void)
{
m_pStopAsyncInput();
sioctl.ctl &=~PERSEUS_SIO_FIFOEN;
perseus_store_sio();
lir_sleep(10000);
}

void close_perseus(void)
{
int i;
return;  // öööööööö There is a bug that prevents close and re-open.
i=m_pClose();
i=FreeLibrary(h_pers);
sdr=-1;
}


void lir_sdr14_write(void *s, int bytes)
{
DWORD written;
P_Write(sdr14_handle,s,bytes, &written);
}

int lir_sdr14_read(void *s,int bytes)
{
DWORD i;
P_Read(sdr14_handle, s, bytes, &i);
return i;
}

void close_sdr14(void)
{
P_Close(sdr14_handle);
sdr=-1;
FreeLibrary(h_sdr14);
}


void open_sdr14(void)
{
sdrOpenEx P_OpenEx;
h_sdr14 = LoadLibrary("ftd2xx.dll");
if (h_sdr14 == NULL)
  {
  SNDLOG"Failed to load ftd2xx.dll\n");
  goto open_sdr14_fail2;
  }
SNDLOG"File ftd2xx.dll loaded.\n");
P_OpenEx=(sdrOpenEx)GetProcAddress(h_sdr14, "FT_OpenEx");
if(P_OpenEx == NULL)
  {
  SNDLOG"Could not find	FT_OpenEx\n");
  goto open_sdr14_fail;
  }
if(P_OpenEx(sdr14_name_string,
                         FT_OPEN_BY_DESCRIPTION, &sdr14_handle) != FT_OK)
  {
  SNDLOG"Could not open %s\n",sdr14_name_string);
  if(P_OpenEx(sdriq_name_string,
                          FT_OPEN_BY_DESCRIPTION, &sdr14_handle) != FT_OK)
    {
    SNDLOG"Could not open %s\n",sdriq_name_string);
    goto open_sdr14_fail;
    }
  else
    {
    SNDLOG"%s open OK\n",sdriq_name_string);
    }
  }
else
  {
  SNDLOG"%s open OK\n",sdr14_name_string);
  }
P_Read=(sdrRead)GetProcAddress(h_sdr14, "FT_Read");
P_Write=(sdrWrite)GetProcAddress(h_sdr14, "FT_Write");
P_Close=(sdrClose)GetProcAddress(h_sdr14, "FT_Close");
P_SetTimeouts=(sdrSetTimeouts)GetProcAddress(h_sdr14, "FT_SetTimeouts");
if(P_Read == NULL ||
   P_Write == NULL ||
   P_Close == NULL ||
   P_SetTimeouts == NULL)goto open_sdr14_fail;
if(P_SetTimeouts(sdr14_handle,500,500) != FT_OK)goto open_sdr14_fail;
sdr=1;
SNDLOG"open_sdr14() sucessful.\n");
return;
open_sdr14_fail:;
FreeLibrary(h_sdr14);
open_sdr14_fail2:;
sdr=-1;
SNDLOG"open_sdr14() returned an error.\n");
}

DWORD WINAPI winthread_rx_adinput(PVOID arg)
{
char s[40];
int timing_loop_counter,timing_loop_counter_max,initial_skip_flag;
int local_rxadin1_newbuf_ptr;
int local_rxadin2_newbuf_ptr;
double dt1, read_start_time,total_reads;
double total_time2;
MMRESULT mmrs;
WAVEHDR *whdr1, *whdr2;
char *c1, *c2;
short int *c1_isho, *c2_isho;
int *c1_int, *c2_int, *rxin_int; 
int i, k;
c1=arg;
open_rx_sndin();
if(kill_all_flag) goto rxadin_init_error;
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
thread_status_flag[THREAD_RX_ADINPUT]=THRFLAG_ACTIVE;
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
local_rxadin1_newbuf_ptr=rxadin1_newbuf_ptr;
local_rxadin2_newbuf_ptr=rxadin2_newbuf_ptr;
screen_loop_counter_max=0.1*interrupt_rate;
if(screen_loop_counter_max==0)screen_loop_counter_max=1;
screen_loop_counter=interrupt_rate;
timing_loop_counter_max=screen_loop_counter_max;
timing_loop_counter=2.5*timing_loop_counter_max;
if(thread_command_flag[THREAD_SCREEN]!=THRFLAG_NOT_ACTIVE)
  {
  while(thread_status_flag[THREAD_SCREEN]!=THRFLAG_ACTIVE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_IDLE &&
        thread_status_flag[THREAD_SCREEN]!=THRFLAG_SEM_WAIT)
    {
    if(thread_command_flag[THREAD_RX_ADINPUT] ==
                                        THRFLAG_KILL)goto rxadin_init_error;
    lir_sleep(10000);
    }
  }
read_start_time=current_time();
total_reads=0;
initial_skip_flag=1;
while(thread_command_flag[THREAD_RX_ADINPUT] == THRFLAG_ACTIVE)
  {
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
  WaitForSingleObject(rxin1_bufready, INFINITE);
  rxin_isho=(void*)(&timf1_char[timf1p_pa]);
  timf1p_pnb=timf1p_pa;
  timf1p_pa=(timf1p_pa+rxad.block_bytes)&timf1_bytemask;
  whdr1=(WAVEHDR*)rxadin1_newbuf[local_rxadin1_newbuf_ptr];
  if( (whdr1[0].dwFlags & WHDR_DONE) == 0)
    {
    lirerr(1225);
    goto rxadin_error;
    }
  c1=whdr1[0].lpData;
  if(ui.rx_addev_no < 256)
    {
    memcpy(rxin_isho,c1,rxad.block_bytes);
    if(local_rxadin1_newbuf_ptr==rxadin1_newbuf_ptr)
      {
      sprintf(s,"RX%s%d",overrun_error_msg,no_of_rx_overrun_errors);
      wg_error(s,WGERR_RXIN);
      }
    local_rxadin1_newbuf_ptr++;
    if(local_rxadin1_newbuf_ptr>=no_of_rx_wavein)local_rxadin1_newbuf_ptr=0;
    mmrs=waveInAddBuffer(hwav_rxadin1,whdr1, sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      lirerr(1226);
      goto rxadin_error;
      }
    }
  else
    {
    WaitForSingleObject(rxin2_bufready, INFINITE);
    whdr2=(WAVEHDR*)rxadin2_newbuf[local_rxadin2_newbuf_ptr];
    if( (whdr2[0].dwFlags & WHDR_DONE) == 0)
      {
      lirerr(1231);
      goto rxadin_error;
      }
    c2=whdr2[0].lpData;
    if( (ui.rx_input_mode&DWORD_INPUT) == 0)
      {
      k=rxad.block_bytes>>3;
      c1_isho=(void*)whdr1[0].lpData;
      c2_isho=(void*)whdr2[0].lpData;
      for(i=0; i<k; i++)
        {
        rxin_isho[4*i  ]=c1_isho[2*i  ];
        rxin_isho[4*i+1]=c1_isho[2*i+1];
        rxin_isho[4*i+2]=c2_isho[2*i  ];
        rxin_isho[4*i+3]=c2_isho[2*i+1];
        }
      }
    else
      {
      k=rxad.block_bytes>>4;
      c1_int=(void*)whdr1[0].lpData;
      c2_int=(void*)whdr2[0].lpData;
      rxin_int=(void*)rxin_isho;
      for(i=0; i<k; i++)
        {
        rxin_int[4*i  ]=c1_int[2*i  ];
        rxin_int[4*i+1]=c1_int[2*i+1];
        rxin_int[4*i+2]=c2_int[2*i  ];
        rxin_int[4*i+3]=c2_int[2*i+1];
        }
      }
    if(local_rxadin1_newbuf_ptr==rxadin1_newbuf_ptr ||
       local_rxadin2_newbuf_ptr==rxadin2_newbuf_ptr)
      {
      sprintf(s,"RX%s%d",overrun_error_msg,no_of_rx_overrun_errors);
      wg_error(s,WGERR_RXIN);
      }
    local_rxadin1_newbuf_ptr++;
    local_rxadin2_newbuf_ptr++;
    if(local_rxadin1_newbuf_ptr>=no_of_rx_wavein)local_rxadin1_newbuf_ptr=0;
    if(local_rxadin2_newbuf_ptr>=no_of_rx_wavein)local_rxadin2_newbuf_ptr=0;
    mmrs=waveInAddBuffer(hwav_rxadin1,whdr1, sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      lirerr(1232);
      goto rxadin_error;
      }
    mmrs=waveInAddBuffer(hwav_rxadin2,whdr2, sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      lirerr(1233);
      goto rxadin_error;
      }
    }
  finish_rx_read(rxin_isho);
  if(kill_all_flag) goto rxadin_error;
  }
rxadin_error:;
close_rx_sndin();
rxadin_init_error:;
thread_status_flag[THREAD_RX_ADINPUT]=THRFLAG_RETURNED;
if(!kill_all_flag)
  {
  while(thread_command_flag[THREAD_RX_ADINPUT] != THRFLAG_NOT_ACTIVE)
    {
    lir_sleep(1000);
    }
  }
return 100;
}

int lir_rx_output_bytes(void)
{
// Find out how much empty space we have in the output buffers.
WAVEHDR *whdr;
int i,k;
if(rx_audio_out == 0)
  {
  k=0;
  for(i=0; i<NO_OF_RX_WAVEOUT; i++)
    {
    whdr=rxdaout_newbuf[i];
    if( (whdr[0].dwFlags&WHDR_INQUEUE) == 0)k++;
    }
  }
else
  {
  k=NO_OF_RX_WAVEOUT;
  }
return k*rx_daout_block;
}

int make_da_wts(void)
{
int i, k;
rxda.buffer_bytes=NO_OF_RX_WAVEOUT*rx_daout_block;
k=lir_rx_output_bytes();
if(k > rx_da_maxbytes)rx_da_maxbytes=k;
i=rxda.buffer_bytes-k;
i/=(rx_daout_channels*rx_daout_bytes);
return i;
}

void lir_empty_da_device_buffer(void)
{
if(rx_audio_out == 0)
  {
  waveOutReset(hwav_rxdaout);
  }
else
  {
  lirerr(745251);
  }
}

void close_rx_sndout(void)
{
MMRESULT mmrs;
WAVEHDR *whdr;
int i;
if(rx_audio_out == -1)return;
mmrs=waveOutReset(hwav_rxdaout);
if(mmrs != MMSYSERR_NOERROR)lirerr(25344);
for(i=0; i<NO_OF_RX_WAVEOUT; i++)
  {
  whdr=rxdaout_newbuf[i];
  while( (whdr[0].dwFlags&WHDR_INQUEUE) != 0)lir_sleep(3000);
  mmrs=waveOutUnprepareHeader(hwav_rxdaout, whdr, sizeof(WAVEHDR));
  if(mmrs != MMSYSERR_NOERROR)lirerr(25144);
  }
mmrs=waveOutClose(hwav_rxdaout);
if(mmrs != MMSYSERR_NOERROR)lirerr(25344);
free(rx_da_wrbuf);
free(rx_wave_outhdr);
free(rx_waveout_buf);
rx_audio_out=-1;
}

void lir_tx_dawrite(char * buf)
{
buf[0]=0;
}

void lir_rx_dawrite(void)
{
char *c;
WAVEHDR *whdr;
// We arrive here because one more block of data is available
// in rx_da_wrbuf.
// Wait until there is a buffer ready for us.
whdr=rxdaout_newbuf[rxdaout_newbuf_ptr];
while( (whdr[0].dwFlags&WHDR_INQUEUE) != 0)
  {
  lir_sleep(10000);
  }
c=whdr[0].lpData;
memcpy(c,rx_da_wrbuf,rx_daout_block);
waveOutWrite(hwav_rxdaout,whdr,sizeof(WAVEHDR));
rxdaout_newbuf_ptr++;
if(rxdaout_newbuf_ptr >= NO_OF_RX_WAVEOUT)rxdaout_newbuf_ptr=0;
}

DWORD WINAPI winthread_rx_output(PVOID arg)
{
char *c;
c=arg;
// The rx_output routine will call lir_rx_dawrite() every time there is
// enough data available to fill one buffer.
SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
if(!kill_all_flag)rx_output();
return 100;
}


void close_rx_sndin(void)
{
if(wave_in_open_flag1 == 0)return;
if(ui.rx_addev_no > 255)
  {
  waveInReset(hwav_rxadin2);
  waveInClose(hwav_rxadin2);
  }
waveInReset(hwav_rxadin1);
waveInClose(hwav_rxadin1);
while(wave_in_open_flag1!=0||wave_in_open_flag2!=0)
  {
  lir_sleep(1000);
  }
free(rx_wave_inhdr);
free(rx_wavein_buf);
CloseHandle(rxin1_bufready);
rxin1_bufready=NULL;
}

VOID CALLBACK rx_daout(HWAVEOUT hwi, UINT iMsg, DWORD dwInst,
                         DWORD dwParam1, DWORD dwParam2)
{
DWORD i;
i=(DWORD)hwi;
i=dwInst;
i=dwParam2;
i=iMsg;
i=dwParam1;
if(iMsg == WIM_DATA)
  {
  }
else
  {
  if(iMsg == WIM_CLOSE)
    {
    i=(DWORD)hwi;
    i=dwInst;
    i=dwParam2;
    rx_audio_in=-1;
    }
  else
    {
    if(iMsg == WIM_OPEN)
      {
      rx_audio_in=0;
      }
    }
  }
}

VOID CALLBACK rx_adin1(HWAVEIN hwi, UINT iMsg, DWORD dwInst,
                         DWORD dwParam1, DWORD dwParam2)
{
long old;
DWORD i;
if(iMsg == WIM_DATA)
  {
  rxadin1_newbuf[rxadin1_newbuf_ptr]=dwParam1;
  rxadin1_newbuf_ptr++;
  if(rxadin1_newbuf_ptr >= no_of_rx_wavein)rxadin1_newbuf_ptr=0;
  ReleaseSemaphore(rxin1_bufready,1,&old);
  }
else
  {
  if(iMsg == WIM_CLOSE)
    {
    i=(DWORD)hwi;
    i=dwInst;
    i=dwParam2;
    wave_in_open_flag1=0;
    }
  }
}

VOID CALLBACK rx_adin2(HWAVEIN hwi, UINT iMsg, DWORD dwInst, 
                         DWORD dwParam1, DWORD dwParam2)
{
long old;
DWORD i;
if(iMsg == WIM_DATA)
  {
  rxadin2_newbuf[rxadin2_newbuf_ptr]=dwParam1;
  rxadin2_newbuf_ptr++;
  if(rxadin2_newbuf_ptr >= no_of_rx_wavein)rxadin2_newbuf_ptr=0;
  ReleaseSemaphore(rxin2_bufready,1,&old);
  }
else
  {
  if(iMsg == WIM_CLOSE)
    {
    i=(DWORD)hwi;
    i=dwInst;
    i=dwParam2;
    wave_in_open_flag2=0;
    }
  }
}

void fill_wavefmt(int channels, int bits)
{
fmt.Format.nSamplesPerSec=ui.rx_ad_speed;
if(bits==16)
  {
  fmt.Samples.wValidBitsPerSample=16;
  fmt.Format.wBitsPerSample=16;
  if(ui.use_alsa == 0)
    {
    fmt.Format.wFormatTag=WAVE_FORMAT_PCM;
    fmt.Format.cbSize=0;
    }
  else
    {
    fmt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    fmt.Format.cbSize=sizeof(WAVEFORMATEXTENSIBLE);
    fmt.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }
  }
else
  {
  if(ui.use_alsa == 0)
    {
    lirerr(2654844);
    return;
    }
  fmt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
  fmt.Samples.wValidBitsPerSample=24;
  fmt.Format.wBitsPerSample=32;
  fmt.Format.cbSize=sizeof(WAVEFORMATEXTENSIBLE);
  fmt.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
  }
fmt.dwChannelMask = 7;
fmt.Format.nChannels=channels;
fmt.Format.nBlockAlign=fmt.Format.nChannels*fmt.Format.wBitsPerSample/8;
fmt.Format.nAvgBytesPerSec=fmt.Format.nBlockAlign*fmt.Format.nSamplesPerSec;
}

void open_rx_sndin(void)
{
int ch, bits, errcod;
MMRESULT mmrs;
int i, k, dev, devno;
if(wave_in_open_flag1 == 1)
  {
  lirerr(869233);
  return;
  }
dev=ui.rx_addev_no&255;
ch=ui.rx_ad_channels;
if(ui.rx_addev_no > 255)
  {
  devno=2;
  ch/=2;
  }
else
  {
  devno=1;
  }
if( (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  bits=16;
  }
else
  {
  bits=24;
  if(ui.use_alsa==0)
    {
    lirerr(1157);
    return;
    }
  }
fill_wavefmt(ch,bits);
if(fmt.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
  {
  mmrs=waveInOpen(&hwav_rxadin1, dev, (WAVEFORMATEX*)&fmt,
                                    (DWORD)rx_adin1, 0, CALLBACK_FUNCTION);
  }
else
  {
  mmrs=waveInOpen(&hwav_rxadin1, dev, &fmt.Format,
                                    (DWORD)rx_adin1, 0, CALLBACK_FUNCTION);
  }
if(mmrs != MMSYSERR_NOERROR)
  {
  switch (mmrs)
    {
    case MMSYSERR_ALLOCATED:
    lirerr(1192);
    break;
    
    case MMSYSERR_BADDEVICEID:
    lirerr(1146);
    break;
            
    case MMSYSERR_NODRIVER:
    lirerr(1147);
    break;

                
    case MMSYSERR_NOMEM:
    lirerr(1155);
    break;

    case WAVERR_BADFORMAT:
    lirerr(1156);
    break;

    default:
    lirerr(1227);
    break;
    }
  return;
  }
wave_in_open_flag1=1;
rxin1_bufready=CreateSemaphore( NULL, 0, 1024, NULL);
if(rxin1_bufready== NULL)
  {
  errcod=1223;
  goto errclose;
  }
if(devno==2)
  {
  dev=(ui.rx_addev_no>>8)-1;
  mmrs=waveInOpen(&hwav_rxadin2, dev, (WAVEFORMATEX*)&fmt,
                                     (DWORD)rx_adin2, 0, CALLBACK_FUNCTION);
  if(mmrs != MMSYSERR_NOERROR)
    {
    errcod=1228;
    goto errclose;
    }
  wave_in_open_flag2=1;
  rxin2_bufready=CreateSemaphore( NULL, 0, 1024, NULL);
  if(rxin2_bufready== NULL)
    {
    errcod=1229;
    goto errclose;
    }
  }
// We read ui.rx_ad_speed*rxad.frame bytes per second from hardware.
// The callback routine will give us the data in a block size of
// rxad.block_bytes/devno.
// Make the number of buffers large enough to hold 0.1 second of data.
no_of_rx_wavein=0.1*(float)(ui.rx_ad_speed*rxad.frame)/rxad.block_bytes;
if(no_of_rx_wavein<4)no_of_rx_wavein=4;
rx_wavein_buf=malloc(no_of_rx_wavein*rxad.block_bytes+fmt.Format.nBlockAlign);
if(rx_wavein_buf == NULL)
  {
  errcod=1236;
  goto errclose;
  }
rx_wave_inhdr=malloc(devno*no_of_rx_wavein*(sizeof(WAVEHDR)+sizeof(DWORD)));
if(rx_wave_inhdr == NULL)
  {
  errcod=1236;
  goto errfree_inbuf;
  }
rxadin1_newbuf=(void*)&rx_wave_inhdr[devno*no_of_rx_wavein];
rxadin2_newbuf=&rxadin1_newbuf[no_of_rx_wavein];
rxadin1_newbuf_ptr=0;
rxadin2_newbuf_ptr=0;
for(i=0; i<no_of_rx_wavein; i++)
  {
  if(devno==1)
    {
    k=(int)(rx_wavein_buf+i*rxad.block_bytes+fmt.Format.nBlockAlign);
    k&=(-1-fmt.Format.nBlockAlign);
    rx_wave_inhdr[i].lpData=(char*)(k);
    rx_wave_inhdr[i].dwBufferLength=rxad.block_bytes;
    rx_wave_inhdr[i].dwBytesRecorded=0;
    rx_wave_inhdr[i].dwUser=0;
    rx_wave_inhdr[i].dwFlags=0;
    rx_wave_inhdr[i].dwLoops=0;
    rx_wave_inhdr[i].lpNext=NULL;
    rx_wave_inhdr[i].reserved=0;
    mmrs=waveInPrepareHeader(hwav_rxadin1, &rx_wave_inhdr[i], sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      errcod=1245;
      goto errfree;
      }
    mmrs=waveInAddBuffer(hwav_rxadin1,&rx_wave_inhdr[i], sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      errcod=1247;
      goto errfree;
      }
    }
  else
    {
    k=(int)(rx_wavein_buf+(2*i  )*(rxad.block_bytes>>1)+fmt.Format.nBlockAlign);
    k&=(-1-fmt.Format.nBlockAlign);
    rx_wave_inhdr[2*i  ].lpData=(char*)(k);
    rx_wave_inhdr[2*i  ].dwBufferLength=rxad.block_bytes>>1;
    rx_wave_inhdr[2*i  ].dwBytesRecorded=0;
    rx_wave_inhdr[2*i  ].dwUser=0;
    rx_wave_inhdr[2*i  ].dwFlags=0;
    rx_wave_inhdr[2*i  ].dwLoops=0;
    rx_wave_inhdr[2*i  ].lpNext=NULL;
    rx_wave_inhdr[2*i  ].reserved=0;
    mmrs=waveInPrepareHeader(hwav_rxadin1, &rx_wave_inhdr[2*i],
                                                          sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      errcod=1245;
      goto errfree;
      }
    mmrs=waveInAddBuffer(hwav_rxadin1,&rx_wave_inhdr[2*i  ], sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      errcod=1247;
      goto errfree;
      }
    k=(int)(rx_wavein_buf+(2*i+1)*(rxad.block_bytes>>1)+fmt.Format.nBlockAlign);
    k&=(-1-fmt.Format.nBlockAlign);
    rx_wave_inhdr[2*i+1].lpData=(char*)(k);
    rx_wave_inhdr[2*i+1].dwBufferLength=rxad.block_bytes>>1;
    rx_wave_inhdr[2*i+1].dwBytesRecorded=0;
    rx_wave_inhdr[2*i+1].dwUser=0;
    rx_wave_inhdr[2*i+1].dwFlags=0;
    rx_wave_inhdr[2*i+1].dwLoops=0;
    rx_wave_inhdr[2*i+1].lpNext=NULL;
    rx_wave_inhdr[2*i+1].reserved=0;
    mmrs=waveInPrepareHeader(hwav_rxadin2, &rx_wave_inhdr[2*i+1],
                                                            sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      errcod=1245;
      goto errfree;
      }
    mmrs=waveInAddBuffer(hwav_rxadin2,&rx_wave_inhdr[2*i+1], sizeof(WAVEHDR));
    if(mmrs != MMSYSERR_NOERROR)
      {
      errcod=1247;
      goto errfree;
      }
    }
  }
waveInStart(hwav_rxadin1);
if(devno==2)waveInStart(hwav_rxadin2);
return;
errfree:;
free(rx_wave_inhdr);
errfree_inbuf:;
free(rx_wavein_buf);
errclose:;
if(devno==2)
  {
  waveInClose(hwav_rxadin2);
  }
waveInClose(hwav_rxadin1);
lirerr(errcod);
}

void set_rx_io(void)
{
char *ss;
char s[80];
char *logfile_name="soundboard_init.log";
int i, j, k, dev, line;
unsigned int device_id, no_of_addev, no_of_dadev;
MMRESULT mmrs;
WAVEINCAPS pwic;
WAVEOUTCAPS pwoc;
if(sdr != -1)return;
sndlog = fopen(logfile_name, "w");
if(sndlog == NULL)
  {
  lirerr(1016);
  return;
  }
write_log=TRUE;  
rxad.block_bytes=4096;
j=0;
line=1;
ui.rx_addev_no=-1;
ui.rx_dadev_no=-1;
clear_screen();
if((ui.network_flag&NET_RX_INPUT) != 0)goto set_sndout;
init_sdr14();
if(kill_all_flag)return;
if(ui.rx_addev_no == -1)
  {
  init_perseus();
  }
if(ui.rx_addev_no == -1)
  {
//    look for other hardwares
  }
if(ui.rx_addev_no != -1)goto set_sndout;
// If none of the supported SDR hardwares is found, use the
// sound cards for input and output.
// ----------------------------------------------------------
// Find out how many audio input devices there are on this computer.
// Allow the user to select one or two for the rx input.
get_dev2_q:;
settextcolor(14);
lir_text(20,0,"Available soundcards.");
settextcolor(7);
no_of_addev=waveInGetNumDevs();
if(no_of_addev == 0)
  {
  lir_text(5,2,"No soundcard detected.");
  lir_text(5,3,press_any_key);
  await_keyboard();
  fclose(sndlog);
  write_log=FALSE;
  return;
  }
line=2;
k=0;
if(no_of_addev > MAX_WAV_DEVICES)no_of_addev=MAX_WAV_DEVICES;
for(device_id=0; device_id<no_of_addev; device_id++)
  {
  mmrs=waveInGetDevCaps(device_id, &pwic, sizeof(WAVEINCAPS));
  if(mmrs==MMSYSERR_NOERROR)
    {
    sprintf(s,"%d  %s", device_id, pwic.szPname);
    settextcolor(7);
    k++;
    }
  else
    {
    sprintf(s,"%d ERROR (mmrs=%d) %s", device_id, mmrs, pwic.szPname);
    settextcolor(3);
    }  
  lir_text(10,line,s);
  line++;
  }
line+=2;
settextcolor(7);
if(k == 0)
  {
  lir_text(10,line,"No usable input device found.");
  line+=2;
  lir_text(14,line,press_any_key);
  await_keyboard();
  goto set_sndout;
  }
if(no_of_addev == 1)
  {
  ui.rx_addev_no=0;
  }
else
  {
  if(ui.rx_addev_no == -1)
    {
    if(ui.newcomer_mode == 0)
      {
      ss="(first)";
      }
    else
      {
      ss="";
      }  
    sprintf(s,"Select %s device for Rx input by line number:",ss);  
    lir_text(0,line,s);
    ui.rx_addev_no=lir_get_integer(51,line, 2, 0, no_of_addev-1);
    if(kill_all_flag) return;
    }
  else
    {
    sprintf(s,"Selected device is %d",ui.rx_addev_no);
    lir_text(0,line,s);
    }
  line++;
  if(ui.newcomer_mode == 0)
    {
    lir_text(0,line,
                 "Do you need more channels from the same soundcard ? (Y/N)");
    lir_text(0,line+1,"F1 for info/help");
get_dev2:;
    toupper_await_keyboard();
    if(lir_inkey == F1_KEY)
      {
      help_message(323);      
      clear_screen();
      goto get_dev2_q;
      }
    if(kill_all_flag) return;
    if(lir_inkey == 'Y')
      {
      line++;
      lir_text(0,line,"Select a second device for Rx input:");
dev2_n:;
      i=lir_get_integer(37,line, 2, 0, no_of_addev-1);
      if(kill_all_flag) return;
      if(i==ui.rx_addev_no)goto dev2_n;
      ui.rx_addev_no+=256*(i+1);
      }
    else
      {
      if(lir_inkey != 'N')goto get_dev2;
      }
    }
  }  
// Windows does not allow us to query the device for its capabilities.
// Ask the user to specify what he wants.
line+=2;
lir_text(0,line,"Linrad can not query hardware because Windows will report that");
line++;
lir_text(0,line,"everything is possible. Windows will silently resample and provide");
line++;
lir_text(0,line,"data that would be meaningless in an SDR context.");
line++;
lir_text(0,line,"Therefore, make sure you enter data that is compatible with the");
line++;
lir_text(0,line,"native capabilities of your soundcardhardware. (And make sure that");
line++;
lir_text(0,line,"the soundcard really is set to the speed you have selected.)");
line+=2;
if(ui.newcomer_mode == 0)
  {
get_wavfmt:;
  lir_text(10,line,"Use extended format (WAVEFORMATEXTENSIBLE) ? (Y/N)");
  lir_text(10,line+1,"F1 for info/help");
getfmt:;
  toupper_await_keyboard();
  if(lir_inkey == F1_KEY)
    {
    help_message(324);      
    clear_screen();
    goto get_wavfmt;
    }
  if(lir_inkey == 'N')
    {
    ui.use_alsa=0;
    }
  else
    {
    if(lir_inkey != 'Y')goto getfmt;
    ui.use_alsa=1;
    }
  line++;
  clear_lines(line,line);
  line++;
  }
else
  {
  ui.use_alsa=0;
  }
get_rxinrate:;
lir_text(10,line,"Sampling speed (Hz):");
ui.rx_ad_speed=lir_get_integer(31,line, 6, 5000, 999999);
if(kill_all_flag) return;
if(ui.use_alsa == 1)
  {
get_rxbits:;
  lir_text(40,line,"No of bits (16/24):");
  i=lir_get_integer(60,line, 2, 16, 24);
  if(kill_all_flag) return;
  clear_screen();
  if(i==16)
    {
    ui.rx_input_mode=0;
    }
  else
    {
    if(i==24)
      {
      ui.rx_input_mode=DWORD_INPUT;
      }
    else
      {
      goto get_rxbits;
      }
    }
  }
else
  {
  ui.rx_input_mode=0;
  }    
clear_screen();
// Check whether the device(s) can do whatever the user asked for.
// Try to open with four channels.
// Try again with two channels on error.
dev=ui.rx_addev_no&255;
j=8;
test_channels:;
j/=2;
fill_wavefmt(j,16);
mmrs=waveInOpen(&hwav_rxadin1, dev, (WAVEFORMATEX*)&fmt, 0, 0,
                                   WAVE_FORMAT_QUERY|WAVE_FORMAT_DIRECT);
if(mmrs != MMSYSERR_NOERROR)
  {
  if(j==4)goto test_channels;
  if(mmrs == MMSYSERR_ALLOCATED)lirerr(1218);
  if(mmrs == MMSYSERR_BADDEVICEID)lirerr(1219);
  if(mmrs == MMSYSERR_NODRIVER)lirerr(1220);
  if(mmrs == MMSYSERR_NOMEM)lirerr(1221);
  if(mmrs == WAVERR_BADFORMAT)lirerr(1222);
  lirerr(1238);
  return;
  }
if(ui.rx_addev_no>255)
  {
  dev=(ui.rx_addev_no>>8)-1;
  mmrs=waveInOpen(&hwav_rxadin1, dev, (WAVEFORMATEX*)&fmt, 0, 0,
                WAVE_FORMAT_QUERY|WAVE_FORMAT_DIRECT);
  if(mmrs != MMSYSERR_NOERROR)
    {
    lir_text(0,line+2,
           "ERROR  Format not supported by second device. (Press any key)");
    await_keyboard();
    if(kill_all_flag) return;
    clear_screen();
    waveInClose(hwav_rxadin1);
    goto get_rxinrate;
    }
  waveInClose(hwav_rxadin2);
  }
waveInClose(hwav_rxadin1);
if(ui.rx_input_mode==DWORD_INPUT)
  {
  if(ui.use_alsa == 0)lirerr(5823198);
  fill_wavefmt(j,24);
  mmrs=waveInOpen(&hwav_rxadin1, dev, (WAVEFORMATEX*)&fmt, 0, 0,
                                   WAVE_FORMAT_QUERY|WAVE_FORMAT_DIRECT);
  if(mmrs != MMSYSERR_NOERROR)
    {
    clear_screen();
    lir_text(5,5,"Can not use 24 bit A/D. Will use 16 bits instead.");
    lir_text(5,7,press_any_key);
    await_keyboard();
    ui.rx_input_mode=0;
    }
  waveInClose(hwav_rxadin1);
  }
if(ui.rx_addev_no>255 && j<4)j*=2;
// If we have reached this far, the user has given correct info about
// speed, channels and no of bits.
// Ask for the desired processing mode.
sel_radio:;
clear_screen();
lir_text(0,10,"Select radio interface:");
if(j == 2)j=3;
for(i=0; i<j; i++)
  {
  sprintf(s,"%d: %s",i+1,audiomode_text[i]);
  lir_text(5,12+i,s);
  }
lir_text(1,13+i,"F1 for help/info");  
line=15+i;
chsel:;
await_processed_keyboard();
if(lir_inkey == F1_KEY)
  {
  help_message(89);
  goto sel_radio;
  }
if(lir_inkey-'0'>j)goto chsel;
switch (lir_inkey)
  {
  case '1':
  ui.rx_rf_channels=1;
  ui.rx_ad_channels=1;
  break;

  case '2':
  ui.rx_input_mode|=IQ_DATA;
  ui.rx_rf_channels=1;
  ui.rx_ad_channels=2;
  break;

  case '3':
  ui.rx_input_mode|=TWO_CHANNELS;
  ui.rx_rf_channels=2;
  ui.rx_ad_channels=2;
  break;

  case '4':
  ui.rx_input_mode|=TWO_CHANNELS+IQ_DATA;
  ui.rx_rf_channels=2;
  ui.rx_ad_channels=4;
  break;

  default:
  goto chsel;
  }
// *********************************************************************
// Input seems OK.
// Open inputs, but do not start recording.
// This way we can (probably) avoid problems with old soundcards
// that do not have duplex capabilities.
open_rx_sndin();
if(kill_all_flag)return;
set_sndout:;
clear_screen();
no_of_dadev=waveOutGetNumDevs();
if(no_of_dadev == 0)
  {
  lir_text(5,2,"No output device detected");
  if(ui.rx_addev_no < SPECIFIC_DEVICE_CODES)
                                   lir_text(5,28,"while input is open.");
  lir_text(5,3,"PRESS ANY KEY");
  await_keyboard();
  fclose(sndlog);
  write_log=FALSE;
  return;
  }
settextcolor(14);
lir_text(10,0,"Select device for Rx output");
settextcolor(7);
line=2;
if(no_of_dadev > MAX_WAV_DEVICES)no_of_dadev=MAX_WAV_DEVICES;
for(device_id=0; device_id<no_of_dadev; device_id++)
  {
  mmrs=waveOutGetDevCaps(device_id, &pwoc, sizeof(WAVEOUTCAPS));
  if(mmrs==MMSYSERR_NOERROR)
    {
    sprintf(s,"%d  %s", device_id, pwoc.szPname);
    lir_text(10,line,s);
    line++;
    }
  }
line+=2;
if(no_of_dadev == 1)
  {
  ui.rx_dadev_no=0;
  }
else
  {
  lir_text(0,line,"Select device for Rx output by line number:");
  ui.rx_dadev_no=lir_get_integer(44,line, 2, 0, no_of_dadev-1);
  if(kill_all_flag) return;
  line+=2;
  }
ui.rx_max_da_channels=2;
ui.rx_min_da_channels=1;
mmrs=waveOutGetDevCaps(ui.rx_dadev_no, &pwoc, sizeof(WAVEOUTCAPS));
i=0;
if( (pwoc.dwFormats &
           (WAVE_FORMAT_1M08|WAVE_FORMAT_2M08|WAVE_FORMAT_4M08|
            WAVE_FORMAT_1S08|WAVE_FORMAT_2S08|WAVE_FORMAT_4S08)) != 0)i=1;
if( (pwoc.dwFormats &
           (WAVE_FORMAT_1M16|WAVE_FORMAT_2M16|WAVE_FORMAT_4M16|
            WAVE_FORMAT_1S16|WAVE_FORMAT_2S16|WAVE_FORMAT_4S16)) != 0)i|=2;
if(i==0 || i==3)
  {
  ui.rx_max_da_bytes=2;
  ui.rx_min_da_bytes=1;
  }
else
  {
  ui.rx_max_da_bytes=j;
  ui.rx_min_da_bytes=j;
  }
ui.rx_min_da_speed=5000;
ui.rx_max_da_speed=96000;
close_rx_sndin();
fclose(sndlog);
write_log=FALSE;
sprintf(s,"%s selected for Rx audio output.", pwoc.szPname);
lir_text(10,line,s);
line+=2;
if(ui.newcomer_mode == 0)
  {
  lir_text(10,line,"Press Y to use device, N to disable");
useq:;
  toupper_await_keyboard();
  if(lir_inkey == 'N')
    {
    ui.rx_dadev_no=-2;
    ui.rx_max_da_bytes=2;
    ui.rx_max_da_channels=2;
    ui.rx_min_da_bytes=1;
    ui.rx_min_da_channels=1;
    ui.rx_min_da_speed=5000;
    ui.rx_max_da_speed=192000;
    }
  else
    {
    if(lir_inkey != 'Y')
      {
      goto useq;
      }
    }
  line++;
  lir_text(10,line,"Set max DMA rate in Hz (10-999) >");
  ui.max_dma_rate=lir_get_integer(44, line, 3, 10,999);
  line++;
  }
else
  {
  ui.max_dma_rate=70;
  }
lir_text(10,line,remind_parsave);
line++;
lir_text(10,line,press_any_key);
await_keyboard();
}

void open_rx_sndout(void)
{
int errcod;
WAVEFORMATEX wfmt;
MMRESULT mmrs;
int i, k;
if(ui.rx_dadev_no == -2)return;
if(rx_audio_out == 0)return;
rx_daout_block=0.75*rxda.frame*min_delay_time*
                                               genparm[DA_OUTPUT_SPEED];
make_power_of_two(&rx_daout_block);
rx_output_blockrate=(float)(rxda.frame*
                               genparm[DA_OUTPUT_SPEED])/rx_daout_block;
while(rx_output_blockrate > ui.max_dma_rate)
  {
  rx_output_blockrate/=2;
  rx_daout_block*=2;
  }
DEB"\nRXout interrupt_rate: Desired %f",rx_output_blockrate);
wfmt.wFormatTag=WAVE_FORMAT_PCM;
wfmt.nSamplesPerSec=genparm[DA_OUTPUT_SPEED];
wfmt.wBitsPerSample=8*rx_daout_bytes;
wfmt.cbSize=0;
wfmt.nChannels=rx_daout_channels;
wfmt.nBlockAlign=wfmt.nChannels*wfmt.wBitsPerSample/8;
wfmt.nAvgBytesPerSec=wfmt.nBlockAlign*wfmt.nSamplesPerSec;
mmrs=waveOutOpen(&hwav_rxdaout, ui.rx_dadev_no,
                            &wfmt, (DWORD)rx_daout, 0, CALLBACK_FUNCTION);
if(mmrs != MMSYSERR_NOERROR)
  {
  lirerr(1234);
  return;
  }
errcod=1237;
rx_waveout_buf=malloc(NO_OF_RX_WAVEOUT*rx_daout_block+wfmt.nBlockAlign);
if(rx_waveout_buf==NULL)goto wave_close;
rx_wave_outhdr=malloc(NO_OF_RX_WAVEOUT*sizeof(WAVEHDR));
if(rx_wave_outhdr==NULL)goto free_waveout;
rx_da_wrbuf=malloc(rx_daout_block);
if(rx_da_wrbuf==NULL)goto free_outhdr;
rxdaout_newbuf_ptr=0;
for(i=0; i<NO_OF_RX_WAVEOUT; i++)
  {
  k=(int)(rx_waveout_buf+i*rx_daout_block+wfmt.nBlockAlign);
  k&=(-1-wfmt.nBlockAlign);
  rx_wave_outhdr[i].lpData=(char*)(k);
  rx_wave_outhdr[i].dwBufferLength=rx_daout_block;
  rx_wave_outhdr[i].dwBytesRecorded=0;
  rx_wave_outhdr[i].dwUser=1;
  rx_wave_outhdr[i].dwFlags=0;
  rx_wave_outhdr[i].dwLoops=0;
  rx_wave_outhdr[i].lpNext=NULL;
  rx_wave_outhdr[i].reserved=0;
  mmrs=waveOutPrepareHeader(hwav_rxdaout, &rx_wave_outhdr[i], sizeof(WAVEHDR));
  rxdaout_newbuf[i]=&rx_wave_outhdr[i];
  if(mmrs != MMSYSERR_NOERROR)goto errfree;
  }
rxda.buffer_bytes=NO_OF_RX_WAVEOUT*rx_daout_block;
rx_audio_out=0;
return;
errfree:;
free(rx_da_wrbuf);
free_outhdr:;
free(rx_wave_outhdr);
free_waveout:;
free(rx_waveout_buf);
wave_close:;
waveOutClose(hwav_rxdaout);
rx_audio_out=-1;
lirerr(errcod);
}

void set_tx_io(void){};
