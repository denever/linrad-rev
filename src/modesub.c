#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <globdef.h>
#include <uidef.h>
#include <fft1def.h>
#include <fft2def.h>
#include <fft3def.h>
#include <seldef.h>
#include <sigdef.h>
#include <screendef.h>
#include <vernr.h>
#include <thrdef.h>
#include <keyboard_def.h>
#include <blnkdef.h>
#include <caldef.h>
#include <txdef.h>
#include <options.h>
#include <rusage.h>


#if (USERS_EXTRA_PRESENT == 1)
extern float users_extra_update_interval;
extern double users_extra_time;
void users_extra(void);
#endif

#define MAX_FILES 256
#define MAX_NAMLEN 36
#define _FILE_OFFSET_BITS 64 

#define DISKSAVE_X_SIZE 48*text_width
#define DISKSAVE_Y_SIZE 7*text_height    
#define DISKSAVE_SCREEN_SIZE (DISKSAVE_X_SIZE*DISKSAVE_Y_SIZE)
#define DEBMEM 64


void write_wav_header(int totbytes);
int open_savefile(char *s);
fpos_t wav_wrpos;
int sel_parm;
int sel_line;


void init_os_independent_globals(void)
{
int i;
allow_parport=0;
keyboard_buffer_ptr=0;
keyboard_buffer_used=0;
mouse_flag=0;
lbutton_state=0;
rbutton_state=0;
new_lbutton_state=0;
new_rbutton_state=0;
// Make sure we know that no memory space is reserved yet.
wg_waterf=NULL;
bg_waterf=NULL;
fft3_handle=NULL;
hires_handle=NULL;
fft1_handle=NULL;
blanker_handle=NULL;
baseband_handle=NULL;
afc_handle=NULL;
calmem_handle=NULL;
txmem_handle=NULL;
rx_da_wrbuf=NULL;
vga_font=NULL;
dx=NULL;
// and that analog io is closed
rx_audio_in=-1;
rx_audio_out=-1;
sdr=-1;
rx_daout_block=0;
tx_daout_block=0;
tx_audio_in=-1;
tx_audio_out=-1;
tx_flag=0;
// Clear flags
workload_reset_flag=0;
kill_all_flag=0;
lir_errcod=0;
for(i=0; i<THREAD_MAX; i++)
  {
  thread_command_flag[i]=THRFLAG_NOT_ACTIVE;  
  thread_status_flag[i]=THRFLAG_NOT_ACTIVE;  
  }
threads_running=FALSE;
write_log=FALSE;
tx_hware_fqshift=0;
rx_hware_fqshift=0;
no_of_rx_overrun_errors=0;
no_of_rx_underrun_errors=0;
no_of_tx_overrun_errors=0;
no_of_tx_underrun_errors=0;
count_rx_underrun_flag=FALSE;
netstart_time=0;
}
 
int skip_calibration(void)
{
int rdbuf[10], chkbuf[10];
int i, mm;
if( (save_init_flag&1) == 1)
  {
// The raw file contains calibration data for fft1_filtercorr.
  i=fread(rdbuf, sizeof(int),10,save_rd_file);
  if(i != 10)goto exx;
  if(rdbuf[7] == 0)
    {
// The filtercorr function was saved in the frequency domain in 
// rdbuf[1] points.
    mm=rdbuf[1];
    }
  else
    {
// The correction function was stored in the time domain in rdbuf[7] points
    mm=rdbuf[7];
    }
  for(i=0; i<mm; i++)
    {
    if((int)fread(timf1_char, sizeof(float), 2*rdbuf[6]+1, save_rd_file)
                                                        != 2*rdbuf[6]+1)goto exx;
    }
  if(10 != fread(chkbuf, sizeof(int),10,save_rd_file))goto exx;
  for(i=0; i<10; i++)
    {
    if(rdbuf[i]!=chkbuf[i])
      {
exx:;
      lir_text(1,7,"ERROR. File corrupted");
      return FALSE;
      }
    }
  }
if( (save_init_flag&2) == 2)
  {
  i=fread(rdbuf, sizeof(int),10,save_rd_file);
  if(i != 10)goto exx;
  bal_segments=rdbuf[0];
  mm=rdbuf[3]*rdbuf[0];
  for(i=0; i<mm; i++)
    {
    if(fread(timf1_char, sizeof(float), 8, save_rd_file) !=8)goto exx;
    }
  if(fread(chkbuf, sizeof(int),10,save_rd_file)!=10)goto exx;
  for(i=0; i<10; i++)
    {
    if(rdbuf[i]!=chkbuf[i])goto exx;
    }
  }
return TRUE;
}
 

void raw2wav(void)
{
int amplitude_factor;
int j, k, bits;
unsigned int n;
int wav_wrbytes;
int line;
char fnam[256], s[80];
int *samp;
getin:;
clear_screen();
settextcolor(14);
lir_text(10,1,"RAW to WAV file converter.");
settextcolor(7);
lir_text(1,2,"Input file name:");
j=lir_get_filename(17,2,fnam);
if(j==0)return;
j=open_savefile(fnam);
if(j!=0)goto getin;
clear_screen();
if(ui.rx_ad_channels != 1 && ui.rx_ad_channels != 2)
  {
  fclose(save_rd_file);
  lir_text(0,0,"Can only write 1 or 2 audio channels.");
  sprintf(s,"This file has %d channels.",ui.rx_ad_channels);
  lir_text(0,2,s);
  lir_text(5,3,press_any_key);
  await_keyboard();
  return;
  }
rx_daout_channels=ui.rx_ad_channels;
genparm[DA_OUTPUT_SPEED]=ui.rx_ad_speed;
rxad.block_bytes=8192;
if((ui.rx_input_mode&DWORD_INPUT) != 0)
  {
  bits=18;
  rx_daout_bytes=3;
  wav_wrbytes=(3*rxad.block_bytes)/4;
  save_rw_bytes=18*rxad.block_bytes/32;
  }
else
  {
  bits=16;
  rx_daout_bytes=2;
  wav_wrbytes=rxad.block_bytes;
  save_rw_bytes=rxad.block_bytes;
  }
timf1p_pa=0;
rx_read_bytes=rxad.block_bytes;
timf1_char=malloc(rxad.block_bytes);
if(timf1_char == NULL)lirerr(34214);
rawsave_tmp=malloc(save_rw_bytes);
if(rawsave_tmp==NULL)lirerr(34124);
amplitude_factor=1;
sprintf(s,"Input file %s uses %d bits per sample",fnam,bits);
lir_text(0,0,s),
line=1;
if(fg.passband_direction <0 && (ui.rx_input_mode&IQ_DATA)!=0)
  {
  lir_text(0,line,"The passband direction flag is set, I and Q will be exchanged");
  line++;
  }
if((ui.rx_input_mode&DWORD_INPUT) != 0)
  {
// This file uses 18 bits per sample. Check whether more
// than 16 bits actually contain valid data.
  lir_text(0,line,"Scanning file for largest sample.");
  line++;
  if(skip_calibration() == FALSE)goto errx;
  k=0;
  while(diskread_flag != 4)
    {
    diskread_block_counter++;
    n=fread(rawsave_tmp,1,save_rw_bytes,save_rd_file);
    if(n != save_rw_bytes)
      {
      while(n != save_rw_bytes)
        {
        rawsave_tmp[n]=0;
        n++;
        }
      diskread_flag=4;
      }
    expand_rawdat();
    for(n=0; n<rxad.block_bytes; n+=4)
      {
      samp=(int*)(&timf1_char[n]);
      if(k<abs(samp[0]))k=abs(samp[0]);
      }
    sprintf(s,"Blk %.0f  (max ampl 0x%x)",diskread_block_counter,k);
    lir_text(1,line,s);
    }
  line++;
  amplitude_factor=0x7fffffff/k;
  sprintf(s,"Headroom to saturation is a factor %d",amplitude_factor);
  lir_text(1,line,s);
  line++;
  if(amplitude_factor >= 4)
    {
    amplitude_factor=4;
    lir_text(1,line,"The .WAV file will be written in 16 bit format (left shifted by 2)");
    line++;
    rx_daout_bytes=2;
    wav_wrbytes=rxad.block_bytes/2;
    }
  else
    {
    if(amplitude_factor==1)
      {
      j=18;
      }
    else
      {
      j=17;
      }
    sprintf(s,"The input file uses %d significant bits",j);
    lir_text(1,line,s);
    line++;  
    lir_text(1,line,"Do you wish to use 24 bit .WAV format (Y/N)?");
gtfmt:;
    toupper_await_keyboard();
    if(lir_inkey == 'N')
      {
      rx_daout_bytes=2;
      wav_wrbytes=rxad.block_bytes/2;
      }
    else
      {
      if(lir_inkey != 'Y')goto gtfmt;
      }
    }
  fclose(save_rd_file);
  j=open_savefile(fnam);
  if(j!=0)lirerr(954362);
  }
wavsave_start_stop(line+1);
if(wav_write_flag < 0)
  {
  wav_write_flag=0;
  return;
  }
if(wav_write_flag == 0)return;
diskread_block_counter=0;
if(skip_calibration() == FALSE)goto errx;
while(diskread_flag != 4)
  {
  diskread_block_counter++;
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    rxin_isho=(void*)(timf1_char);
    n=fread(rxin_isho,1,rxad.block_bytes,save_rd_file);
    if(n != rxad.block_bytes)
      {
      while(n != rxad.block_bytes)
        {
        timf1_char[n]=0;
        n++;
        }
      diskread_flag=4;
      }
    }
  else
    {  
    n=fread(rawsave_tmp,1,save_rw_bytes,save_rd_file);
    if(n != save_rw_bytes)
      {
      while(n != save_rw_bytes)
        {
        rawsave_tmp[n]=0;
        n++;
        }
      diskread_flag=4;
      }
    expand_rawdat();
    k=0;
    if(rx_daout_bytes==3)
      {
      k=0;
      for(n=0; n<rxad.block_bytes; n+=4)
        {
        timf1_char[k  ]=timf1_char[n+1];
        timf1_char[k+1]=timf1_char[n+2];
        timf1_char[k+2]=timf1_char[n+3];
        k+=3;
        }
      }
    else
      {
      for(n=0; n<rxad.block_bytes; n+=4)
        {
        samp=(int*)(&timf1_char[n]);
        samp[0]*=amplitude_factor;
        timf1_char[k  ]=timf1_char[n+2];
        timf1_char[k+1]=timf1_char[n+3];
        k+=2;
        }
      }
    }
  if(fg.passband_direction < 0 && (ui.rx_input_mode&IQ_DATA)!=0)
    {
    if(rx_daout_bytes==3)
      {
      for(j=0; j<wav_wrbytes; j+=6)
        {
        k=timf1_char[j];
        timf1_char[j]=timf1_char[j+3];
        timf1_char[j+3]=k;
        k=timf1_char[j+1];
        timf1_char[j+1]=timf1_char[j+4];
        timf1_char[j+4]=k;
        k=timf1_char[j+2];
        timf1_char[j+2]=timf1_char[j+5];
        timf1_char[j+5]=k;
        }
      }
    else  
      {
      for(j=0; j<wav_wrbytes; j+=4)
        {
        k=timf1_char[j];
        timf1_char[j]=timf1_char[j+2];
        timf1_char[j+2]=k;
        k=timf1_char[j+1];
        timf1_char[j+1]=timf1_char[j+3];
        timf1_char[j+3]=k;
        }
      }
    }
  if(fwrite(timf1_char,wav_wrbytes,1,wav_file)!=1 ||
    (unsigned int)(wavfile_bytes)+(unsigned int)(wav_wrbytes) > 0x7fffffff)
    {
    wavsave_start_stop(0);
    lir_text(1,7,"ERROR on write. File too big?");
    goto errx;
    }
  wavfile_bytes+=wav_wrbytes;
  sprintf(s,"%.0f ",diskread_block_counter);
  lir_text(1,7,s);
  }
wavsave_start_stop(0);
lir_text(1,7,"Conversion sucessful.");
errx:;
if(wavfile_bytes < 100000)
  {
  sprintf(s,"%d bytes written",wavfile_bytes);
  }
else
  {
  if(wavfile_bytes < 10000000)
    {
    sprintf(s,"%.2f kilobytes written",0.001*wavfile_bytes);
    }
  else
    {
    sprintf(s,"%.2f megabytes written",0.000001*wavfile_bytes);
    }
  }
lir_text(1,9,s);
lir_text(5,11,press_any_key);  
free(timf1_char);
free(rawsave_tmp);
fclose(save_rd_file);
await_keyboard();
}

int open_savefile(char *s)
{
int i;
i=0;
while(s[i] == ' ')i++;
if(s[i] == 0)
  {
  lir_text(5,4,"No file name given.");
  goto errfile_1;
  }
save_rd_file = fopen(s, "rb");
if (save_rd_file == NULL)
  {
  if(errno == EFBIG)
    {
    lir_text(5,3,"File too large.");
    }
errfile:;  
  lir_text(5,4,"Can not open file (Corrupted?)");
  lir_text(5,5,s);
errfile_1:;  
  await_keyboard();
  return 1;
  }
diskread_flag=2;
i=fread(&ui.rx_input_mode,sizeof(int),1,save_rd_file);
if(i != 1)goto errfile;
// In case ui.rx_input_mode is negative the file contains more data.
if(ui.rx_input_mode < 0)
  {
  i=fread(&diskread_time,sizeof(double),1,save_rd_file);
  if(i!=1)goto errfile;
  i=fread(&fg.passband_center,sizeof(double),1,save_rd_file);
  freq_from_file=TRUE;
  if(i!=1)goto errfile;
  i=fread(&fg.passband_direction,sizeof(int),1,save_rd_file);
  if(i!=1)goto errfile;
  if(abs(fg.passband_direction) != 1)goto errfile; 
  fft1_direction=fg.passband_direction;
  i=fread(&ui.rx_input_mode,sizeof(int),1,save_rd_file);
  if(i!=1)goto errfile;
  }
else
  {
  diskread_time=0;
  fg.passband_center=0;
  fg.passband_direction=1;
  fft1_direction=fg.passband_direction;
  }
if(ui.rx_input_mode >= MODEPARM_MAX)goto errfile;
i=fread(&ui.rx_rf_channels,sizeof(int),1,save_rd_file);
if(i!=1)goto errfile;
i=fread(&ui.rx_ad_channels,sizeof(int),1,save_rd_file);
if(i!=1)goto errfile;
if(ui.rx_ad_channels > 4 || ui.rx_ad_channels < 1)goto errfile;
if(ui.rx_ad_channels != ui.rx_rf_channels &&
               ui.rx_ad_channels != 2*ui.rx_rf_channels)goto errfile;
i=fread(&ui.rx_ad_speed,sizeof(int),1,save_rd_file);
if(i!=1)goto errfile;
save_init_flag=0;
i=fread(&save_init_flag,1,1,save_rd_file);
if(i!=1)goto errfile;
return 0;
}

void could_not_create(char *filename, int line)
{
lir_text(0,line,"Could not create file!!");
lir_text(0,line+1,filename);      
lir_text(0,line+2,"Make sure directory exists !!");
lir_text(0,line+3,press_any_key);
clear_await_keyboard();
}


FILE *open_parfile(int type, char *mode)
{
char s[80];
int i,j;
// Find out if there is a file with parameters for the current graph.
// Make the default graph if no file is found, else use old data.
i=0;
if(savefile_parname[0]!=0)
  {
  while(savefile_parname[i]!=0)
    {
    s[i]=savefile_parname[i];
    i++;
    }
  }  
else
  {  
  while(rxpar_filenames[rx_mode][i]!=0)
    {
    s[i]=rxpar_filenames[rx_mode][i];
    i++;
    }
  }
s[i  ]='_';
i++;
j=0;
while(graphtype_names[type][j] != 0)
  {
  s[i]=graphtype_names[type][j];
  i++;
  j++;
  }
s[i]=0;
return fopen(s, mode);
}


int read_modepar_file(int type)
{
int i, j, k, max_intpar, max_floatpar;
int *wgi;
float *wgf;
double *wgd;
char *parinfo;
FILE *wgfile;
wgfile = open_parfile(type,"r");
if (wgfile == NULL)return 0;
parinfo=malloc(4096);
if(parinfo == NULL)
  {
  lirerr(1082);
  return 0;
  }
for(i=0; i<4096; i++) parinfo[i]=0;
i=fread(parinfo,1,4095,wgfile);
fclose(wgfile);
if(i == 4095)goto txt_err;
wgi=(void*)(graphtype_parptr[type]);
k=0;
max_intpar = graphtype_max_intpar[type];
max_floatpar = graphtype_max_floatpar[type];
for(i=0; i<max_intpar; i++)
  {
  while( (parinfo[k]==' ' || parinfo[k]== '\n' ) && k<4095)k++;
  j=0;
  while(parinfo[k]== graphtype_partexts_int[type][i][j] && k<4095)
    {
    k++;
    j++;
    } 
  if(graphtype_partexts_int[type][i][j] != 0)
    {
txt_err:;    
    free(parinfo);
    return 0;
    }
  while(parinfo[k]!='[' && k<4095)k++;
  if(k>=4095)goto txt_err;
  sscanf(&parinfo[k],"[%d]",&wgi[i]);
  while(parinfo[k]!='\n' && k<4095)k++;
  if(k>=4095)goto txt_err;
  }
if(max_floatpar < 0)
  {
  wgd=(void*)(&wgi[max_intpar]);
  for(i=0; i<-max_floatpar; i++)
    {
    while(parinfo[k]==' ' ||
          parinfo[k]== '\n' )k++;
    j=0;
    while(parinfo[k]== graphtype_partexts_float[type][i][j]&&k<4095)
      {
      k++;
      j++;
      } 
    if(graphtype_partexts_float[type][i][j] != 0)goto txt_err;
    while(parinfo[k]!='[' && k<4095)k++;
    if(k>=4095)goto txt_err;
    sscanf(&parinfo[k],"[%lf]",&wgd[i]);
    if(k>=4095)goto txt_err;
    while(parinfo[k]!='\n' && k<4095)k++;
    }
  }
else
  {  
  wgf=(void*)(&wgi[max_intpar]);
  for(i=0; i<max_floatpar; i++)
    {
    while(parinfo[k]==' ' || parinfo[k]== '\n' )k++;
    j=0;
    while(parinfo[k]== graphtype_partexts_float[type][i][j])
      {
      k++;
      j++;
      } 
    if(graphtype_partexts_float[type][i][j] != 0)goto txt_err;
    while(parinfo[k]!='[')k++;
    sscanf(&parinfo[k],"[%f]",&wgf[i]);
    while(parinfo[k]!='\n')k++;
    }
  }
free(parinfo);
return 1;
}


void make_modepar_file(int type)
{
int i, max_intpar, max_floatpar;
int *wgi;
float *wgf;
double *wgd;
FILE *file;
file = open_parfile(type,"w");
if (file == NULL)
  {
  lirerr(1164);
  return;
  }
wgi=(void*)(graphtype_parptr[type]);
max_intpar = graphtype_max_intpar[type];
max_floatpar = graphtype_max_floatpar[type];
for(i=0; i<max_intpar; i++)
  {
  fprintf(file,"%s [%d]\n",graphtype_partexts_int[type][i],wgi[i]);
  }
if(max_floatpar < 0)
  {
  wgd=(void*)(&wgi[max_intpar]);
  for(i=0; i<-max_floatpar; i++)
    {
    fprintf(file,"%s [%.30f]\n",graphtype_partexts_float[type][i],wgd[i]);
    }
  }
else
  {  
  wgf=(void*)(&wgi[max_intpar]);
  for(i=0; i<max_floatpar; i++)
    {
    fprintf(file,"%s [%.15f]\n",graphtype_partexts_float[type][i],wgf[i]);
    }
  }  
parfile_end(file);
}

        
void select_namlin(char *s,FILE *file)
{
char ch;
int i,k,num, line, col;
fpos_t fileptr[MAX_FILES];
clear_screen();
if(sel_line == -1)lir_text(10,0,"SELECT A NUMBER FROM THE LIST:");
num=0;
line=2;
col=0;
get_name:;
k=1;
sprintf(s,"%2d:",num);
if(fgetpos(file, &fileptr[num]))
  {
  lirerr(1118);
  return;
  }
i=2;
while(k==1 && s[i] != 10 && s[i] != ' ' && i<MAX_NAMLEN)
  {
  i++;
  k=fread(&s[i],1,1,file);
  if(s[i]==13)s[i]=10;
  if(i==3 && s[i]==10)goto get_name;
  }
ch=s[i];
s[i]=0;
if(i>3)
  {
  if(sel_line == -1)lir_text(col,line,s);
  while(k==1 && ch != 10 )
    {
    k=fread(&ch,1,1,file);
    if(ch==13)ch=10;
    }
  num++;
  line++;
  }
if(line>screen_last_line)
  {
  line=2;
  col+=MAX_NAMLEN +5;
  if( col+MAX_NAMLEN+5 > screen_last_col)goto listfull;
  }
if(k == 1 && num < MAX_FILES)goto get_name;
listfull:;
if(sel_line == -1)sel_line=lir_get_integer(42, 0, 3, 0,num-1);
if(kill_all_flag) return;
if(fsetpos(file, &fileptr[sel_line]))lirerr(1119);
clear_screen();
}


void get_parfile_name(char *s)
{
int i, ptr, eq;
ptr=0;
while(s[ptr]!=0 && s[ptr]!=' ')
  {
  savefile_name[ptr]=s[ptr];
  if(s[ptr]==10 || s[ptr]==13 )
    {
    savefile_parname[0]=0;
    savefile_name[ptr]=0;
    s[ptr]=0;
    return;
    }
  ptr++;
  if(ptr > 40)lirerr(1104);
  }
savefile_name[ptr]=0;
s[ptr]=0;
ptr++;
while(s[ptr] == ' ')ptr++;
i=0;
eq=1;
while(s[ptr]!=0 && s[ptr]!=' ' && s[ptr] != 10 && s[ptr] != 13 && i<40)
  {
  savefile_parname[i]=s[ptr];
  if(s[i] != s[ptr])eq=0; 
  i++;
  ptr++;
  }
if(s[i] != ' ')eq=0;  
if(i!=0 && eq==1)lirerr(1120);  
if(i>= 40)lirerr(1115);  
savefile_parname[i]=0;
}

#define WORD unsigned short int
#define DWORD unsigned int

typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME;


typedef struct rcvr_hdr{
  char           chunkID[4];         // ="rcvr" (chunk perseus beta0.2)
  long           chunkSize;          // chunk length
  long           nCenterFrequencyHz; // center frequency
  long           SamplingRateIdx;    // 0=125K, 1=250K, 2=500K, 3=1M, 4=2M
  time_t         timeStart;          // start time of the recording (time(&timeStart))
  unsigned short wAttenId;           // 0=0dB, 1=10dB, 2=20dB, 3=30dB
  char           bAdcPresel;         // 0=Presel Off, 1=Presel On
  char           bAdcPreamp;         // 0=ADC Preamp Off, 1=ADC Preamp ON
  char           bAdcDither;         // 0=ADC Dither Off, 1=ADC Dither ON
  char           bSpare;             // for future use (default = 0)
  char           rsrvd[16];          // for future use (default = 000..0)
  }RCVR;

// "auxi" chunk as used in the SpectraVue WAV files
typedef struct auxi_hdr{
  char       chunkID[4];  // ="auxi" (chunk rfspace)
  long       chunkSize;   // chunk length
  SYSTEMTIME StartTime;
  SYSTEMTIME StopTime;
  DWORD      CenterFreq;  //receiver center frequency
  DWORD      ADFrequency; //A/D sample frequency before downsampling
  DWORD      IFFrequency; //IF freq if an external down converter is used
  DWORD      Bandwidth;   //displayable BW if you want to limit the display to less than Nyquist band
  DWORD      IQOffset;    //DC offset of the I and Q channels in 1/1000's of a count
  DWORD      Unused2;
  DWORD      Unused3;
  DWORD      Unused4;
  DWORD      Unused5;
  }AUXI;


int init_wavread(int sel_file)
{
RCVR perseus_hdr;
AUXI sdr14_hdr;
char s[256];
FILE *file;
int i, k, n, chunk_size;
short int ishort;
int errnr;
clear_screen();
file = fopen("adwav", "rb");
if (file == NULL)
  {
  file = fopen("adwav.txt", "rb");
  if (file == NULL)
    {
empty_error:;
    help_message(313);
    return 1;
    }
  }  
if(sel_file != 0)
  {  
  select_namlin(s,file);
  if(kill_all_flag) return 1;
  }  
for(i=0; i<256; i++)s[i]=0;
k=fread(&s , 1, 256, file);  
fclose(file);
if(k<2)goto empty_error;
i=0;
while( i<255 && (s[i] == ' ' || s[i] == 10 || s[i] == 13))i++;
get_parfile_name(&s[i]);
if(lir_errcod != 0)return 1;
for(n=i;n<256;n++)if(s[n]==10 || s[n]==13)s[n]=0;
save_rd_file = fopen(&s[i], "rb");
if (save_rd_file == NULL)
  {
  lir_text(5,4,"Can not open file");
  lir_text(5,5,s);
rdfile_x:;
  await_keyboard();
  return 1;
  }
diskread_time=0;
fg.passband_center=0;
// Read the WAV file header.
// First 4 bytes should be "RIFF"
k=fread(&i,sizeof(int),1,save_rd_file);
errnr=0;
if(k !=1)
  {
headerr:;  
  lir_text(5,5,s);
  sprintf(s,"Error in .wav file header [%d]",errnr);  
  lir_text(5,4,s);
  goto rdfile_x;
  }
if(i != 0x46464952)
  {
  errnr=1;
  goto headerr;
  }
// Read file size (we do not need it)
i=fread(&i,sizeof(int),1,save_rd_file);
if(i!=1)
  {
  errnr=2;
  goto headerr;
  }
// Now we should read "WAVE"
k=fread(&i,sizeof(int),1,save_rd_file);
errnr=2;
if(k !=1 || i != 0x45564157)goto headerr;
// Now we should read "fmt "
k=fread(&i,sizeof(int),1,save_rd_file);
errnr=3;
if(k !=1 || i != 0x20746d66)goto headerr;
// read the size of the format chunk.
k=fread(&chunk_size,sizeof(int),1,save_rd_file);
errnr=4;
if(k !=1 )goto headerr; 
// read the type of data (Format Tag). We only recognize PCM data!
k=fread(&ishort,sizeof(short int),1,save_rd_file);
errnr=5;
if(k !=1 || ishort != 1)
  {
  lir_text(5,3,"Unknown WAVE format. (PCM only accepted)");
  goto headerr;
  }
// Read no of channels
k=fread(&ishort,sizeof(short int),1,save_rd_file);
errnr=6;
if(k !=1 || ishort < 1 || ishort > 2)goto headerr;  
ui.rx_ad_channels=ishort;
ui.rx_input_mode=0;
// Read the sampling speed.
k=fread(&ui.rx_ad_speed,sizeof(int),1,save_rd_file);
errnr=7;
if(k !=1 )goto headerr; 
// Read average bytes per second (do not care what it is)
errnr=8;
k=fread(&i,sizeof(int),1,save_rd_file);
if(k !=1 )goto headerr; 
// Read block align to get 8 or 16 bit format.
k=fread(&ishort,sizeof(short int),1,save_rd_file);
errnr=9;
if(k !=1 )goto headerr; 
errnr=10;
ishort/=ui.rx_ad_channels;
switch (ishort)
  {
  case 1:
  ui.rx_input_mode+=BYTE_INPUT;
  break;
  
  case 2:
  break;
  
  case 3:
  ui.rx_input_mode+=BYTE_INPUT;
  ui.rx_input_mode+=DWORD_INPUT;
  break;
  
  default:
  goto headerr;
  }
// Skip extra bytes if present.
chunk_size-=14;
skip_chunk:;
errnr=11;
while(chunk_size != 0)
  {
  k=fread(&i,1,1,save_rd_file);
  if(k !=1 )goto headerr; 
  chunk_size--;
  }
// Read chunks until we encounter the "data" string (=0x61746164).
// Look for Perseus or SDR-14 headers.
errnr=12;
next_chunk:;
k=fread(&i,sizeof(int),1,save_rd_file);
if(k !=1 )goto headerr; 
errnr=13;
// test for "rcvr"
if(i==0x72766372)
  {
  k=fread(&chunk_size,sizeof(int),1,save_rd_file);
  if(k !=1 )goto headerr; 
  k=fread(&perseus_hdr.nCenterFrequencyHz,1,chunk_size,save_rd_file);
  diskread_time=perseus_hdr.timeStart;
  fg.passband_center=0.000001*perseus_hdr.nCenterFrequencyHz;
  freq_from_file=TRUE;
  goto next_chunk;
  }
errnr=14;  
// test for "auxi"
if(i==0x69787561)
  {
  k=fread(&chunk_size,sizeof(int),1,save_rd_file);
  if(k !=1 )goto headerr; 
  k=fread(&sdr14_hdr.StartTime,1,chunk_size,save_rd_file);
  diskread_time=sdr14_hdr.StartTime.wHour*3600.+
                sdr14_hdr.StartTime.wMinute*60.+
                sdr14_hdr.StartTime.wSecond;
  fg.passband_center=0.000001*sdr14_hdr.CenterFreq;
  freq_from_file=TRUE;
  goto next_chunk;
  }
errnr=25;
if(i != 0x61746164)
  {
// Unknown. Get the size and skip.  
  k=fread(&chunk_size,sizeof(int),1,save_rd_file);
  if(k !=1 )goto headerr; 
  goto skip_chunk;
  }
// Read how much data we have ( do not care)
i=fread(&i,sizeof(int),1,save_rd_file);
diskread_flag=2;
save_init_flag=0;
fg.passband_direction=0;
fft1_direction=fg.passband_direction;
return 0;
}  

void parfile_end(FILE *file)
{
fprintf(file,"\nChange only between brackets.");
fprintf(file,"\nIf file has errors, Linrad will ignore file and use defaults");
fprintf(file,"\nor prompt for a complete set of new parameters\n");
fprintf(file,"\n%s",PROGRAM_NAME);
fclose(file);
}


int init_diskread(int sel_file)
{
char s[256];
FILE *file;
int i, n, kk;
if(sel_file != -1)
  {
  sel_parm=sel_file;
  sel_line=-1;
  }
kk=sel_parm;
if( wav_read_flag != 0)
  {
  return init_wavread(kk);
  }       
clear_screen();
file = fopen("adfile", "rb");
if (file == NULL)
  {
  file = fopen("adfile.txt", "rb");
  if (file == NULL)
    {
emptyerror:;    
    help_message(314);
    return 1;
    }
  }
if(kk != 0)
  {  
  select_namlin(s,file);
  if(kill_all_flag) return 1;
  }  
for(i=0; i<256; i++)s[i]=0;
i=fread(&s , 1, 256, file);  
fclose(file);
if(i < 2)goto emptyerror;
i=0;
while( i<255 && (s[i] == ' ' || s[i] == 10 || s[i] == 13))i++;
get_parfile_name(&s[i]);
if(lir_errcod != 0)return 1;
for(n=i;n<256;n++)if(s[n]==10 || s[n]==13)s[n]=0;
return open_savefile(&s[i]);
}  

void complete_filename(int i, char *s, char *fxt, char *dir, char *fnm)
{
int j, k;
j=0;
k=0;
if( i <= 4 || (i > 4 && strcmp(&s[i-4],fxt) !=0))
  {
  strcpy(&s[i],fxt);
  }
if(s[0] != '/' && s[0]!= '.')
  {
  sprintf(fnm,"%s%s",dir,s);  
  }
else
  {
  strcpy(fnm,s);
  }
}



void disksave_start_stop(void)
{
// Open or close save_rd_file or save_wr_file.
// If mode=TRUE, we operate on save_wr_file.
FILE *fc_file, *iq_file;
int i;
int wrbuf[10];
double dt1;
char raw_filename[160];
char s[160];
char indicator[2];
char *disksave_screencopy; 
indicator[0]=' ';
if(diskwrite_flag == 0)
  {
  pause_thread(THREAD_SCREEN);
  indicator[0]='S';
  disksave_screencopy=malloc(DISKSAVE_SCREEN_SIZE);
  if(disksave_screencopy == NULL)
    {
    lirerr(1047);
    return;
    }
  lir_getbox(0,0,DISKSAVE_X_SIZE,DISKSAVE_Y_SIZE,disksave_screencopy);
  lir_fillbox(0,0,DISKSAVE_X_SIZE,DISKSAVE_Y_SIZE,10);
  lir_text(0,0,"Enter file name for raw data.");
  lir_text(0,1,"ENTER to skip.");
  lir_text(0,2,"=>");
  i=lir_get_filename(2,2,s);          
  if(kill_all_flag) return;
  if(i==0)
    {
    indicator[0]=' ';
    diskwrite_flag=FALSE;
    }
  else 
    {
    complete_filename(i, s, ".raw", RAWDIR, raw_filename);
    save_wr_file = fopen( raw_filename, "wb");
    if(save_wr_file == NULL)
      {
errx:;
      could_not_create(raw_filename,3);
      if(kill_all_flag) return;
      diskwrite_flag=FALSE;
      }
    else
      {
// Write -1 as a flag telling what version number of Linrad raw data file
// we are writing.        
      i=-1;
      i=fwrite(&i,sizeof(int),1,save_wr_file); 
      if(i != 1)
        {
wrerr:;
        fclose(save_wr_file);
        goto errx;
        }              
      if(diskread_flag < 2)
        {
        dt1=current_time();
        }
      else  
        {
        i=diskread_time+diskread_block_counter*rxad.block_frames/ui.rx_ad_speed;
        i%=24*3600;
        dt1=i;
        }
      i=fwrite(&dt1,sizeof(double),1,save_wr_file);
      if(i != 1)goto wrerr;
      i=fwrite(&fg.passband_center,sizeof(double),1,save_wr_file);
      if(i != 1)goto wrerr;
      i=fwrite(&fg.passband_direction,sizeof(int),1,save_wr_file);
      if(i != 1)goto wrerr;
// Write mode info so we know how to process data when reading 
      i=fwrite(&ui.rx_input_mode,sizeof(int),1,save_wr_file);
      if(i != 1)goto wrerr;
      i=fwrite(&ui.rx_rf_channels,sizeof(int),1,save_wr_file);
      if(i != 1)goto wrerr;
      i=fwrite(&ui.rx_ad_channels,sizeof(int),1,save_wr_file);
      if(i != 1)goto wrerr;
      i=fwrite(&ui.rx_ad_speed,sizeof(int),1,save_wr_file);
      if(i != 1)goto wrerr;
      save_init_flag=0;
      iq_file=NULL;
      fc_file=NULL;
      if( (fft1_calibrate_flag&CALAMP) == CALAMP)
        {
        make_filfunc_filename(s);
        fc_file = fopen(s, "rb");
        save_init_flag=1;
        }
      if(  (ui.rx_input_mode&IQ_DATA) != 0 && 
           (fft1_calibrate_flag&CALIQ) == CALIQ)
        {
        make_iqcorr_filename(s);
        iq_file = fopen(s, "rb");
        save_init_flag+=2;
        }
      i=fwrite(&save_init_flag,1,1,save_wr_file);
      if(i != 1)goto wrerr;

      if( (save_init_flag & 1) == 1)
        {
        if(fc_file != NULL)
          {
rd1:;
          i=fread(&s,1,1,fc_file);
          if(i != 0)
            {
            i=fwrite(&s,1,1,save_wr_file);
            if(i != 1)goto wrerr;
            goto rd1;
            }
          fclose(fc_file);
          fc_file=NULL;
          }
        else
          {
          wrbuf[0]=fft1_n;
          wrbuf[1]=fft1_size;
          wrbuf[2]=rx_mode;
          wrbuf[3]=ui.rx_input_mode;
          wrbuf[4]=genparm[FIRST_FFT_VERNR];
          wrbuf[5]=ui.rx_ad_speed;
          wrbuf[6]=ui.rx_rf_channels;
          wrbuf[7]=fft1_size;
          for(i=8; i<10; i++)wrbuf[i]=0;
          i=fwrite(wrbuf, sizeof(int),10,save_wr_file);
          if(i != 10)goto wrerr;
          i=fwrite(fft1_filtercorr, twice_rxchan*sizeof(float),
                                                    fft1_size, save_wr_file);  
          if(i != fft1_size)goto wrerr;
          i=fwrite(fft1_desired,sizeof(float),fft1_size,save_wr_file);
          if(i != fft1_size)goto wrerr;
          i=fwrite(wrbuf, sizeof(int),10,save_wr_file);
          if(i!=10)goto wrerr;
          }
        }    
      if( (save_init_flag & 2) == 2)
        {
        if(iq_file != NULL)
          {
rd2:;
          i=fread(&s,1,1,iq_file);
          if(i != 0)
            {
            i=fwrite(&s,1,1,save_wr_file);
            if(i != 1)goto wrerr;
            goto rd2;
            }
          fclose(iq_file);
          iq_file=NULL;
          }
        else  
          {
          wrbuf[0]=fft1_size;
          wrbuf[1]=ui.rx_input_mode&IQ_DATA;
          wrbuf[2]=ui.rx_ad_speed;
          wrbuf[3]=ui.rx_rf_channels;
          wrbuf[4]=FOLDCORR_VERNR;
          for(i=5; i<10; i++)wrbuf[i]=0;
          i=fwrite(wrbuf, sizeof(int),10,save_wr_file);
          if(i!=10)goto wrerr;
          i=fwrite(fft1_foldcorr, twice_rxchan*sizeof(float),
                                               4*fft1_size, save_wr_file);  
// We write four times too much data for foldcorr. ööööööööö
// make a new FOLDCORR_VERNR and write future files in the öööö
// proper size.  öööö
          if(i != 4*fft1_size)goto wrerr;
          i=fwrite(wrbuf, sizeof(int),10,save_wr_file);
          if(i!=10)goto wrerr;
          }
        }
      diskwrite_flag=TRUE;
      }
    }
  lir_putbox(0,0,DISKSAVE_X_SIZE,DISKSAVE_Y_SIZE,disksave_screencopy);
  free(disksave_screencopy);
  resume_thread(THREAD_SCREEN);
  }
else
  {
  diskwrite_flag=0;
  lir_sleep(200000);
  fclose(save_wr_file);
  save_wr_file=NULL;
  }
indicator[1]=0;  
settextcolor(14);
i=wg.xright-text_width-2;
if(i<2)i=2;
lir_pixwrite(i,wg.yborder+2,indicator);
settextcolor(7);
}

void wavsave_start_stop(int line)
{
int i;
char s[160],wav_filename[160];
char *wav_write_screencopy; 
char indicator[2];
indicator[0]=' ';
if( wav_write_flag == 0)
  {
  pause_thread(THREAD_SCREEN);
  wav_write_screencopy=malloc(DISKSAVE_SCREEN_SIZE);
  if(wav_write_screencopy == NULL)
    {
    lirerr(1031);
    return;
    }
  lir_getbox(0,line*text_height,DISKSAVE_X_SIZE,DISKSAVE_Y_SIZE,wav_write_screencopy);
  lir_fillbox(0,line*text_height,DISKSAVE_X_SIZE,DISKSAVE_Y_SIZE,10);
  lir_text(0,line,"Enter name for audio output file");
  lir_text(0,line+1,"ENTER to skip.");
  lir_text(0,line+2,"=>");
  i=lir_get_filename(2,line+2,s);          
  if(kill_all_flag) return;
  if(i==0)
    {
    indicator[0]=' ';
    }
  else 
    {
    complete_filename(i, s, ".wav", WAVDIR, wav_filename);
    wav_file = fopen( wav_filename, "wb");
    if(wav_file == NULL)
      {
      could_not_create(wav_filename, line+2);
      wav_write_flag = -1;
      }
    else
      {
// Write the .wav header, but make the file size gigantic
// We will write the correct size when closing, but if
// system crashes we can fix the header and recover data.
// Speed, channels and bits will be ok. Size will indicate loss of data. 
      write_wav_header(0x7fffffff);
      wavfile_bytes=0;
      fflush(wav_file);
      lir_sync();
      wav_write_flag=1;
      indicator[0]='W';
      }
    }
  lir_putbox(0,line*text_height,DISKSAVE_X_SIZE,DISKSAVE_Y_SIZE,wav_write_screencopy);
  free(wav_write_screencopy);
  resume_thread(THREAD_SCREEN);
  }
else
  {
  wav_write_flag = 0;
  lir_sleep(200000);
  write_wav_header(wavfile_bytes);
  fclose(wav_file);
  lir_sync();
  }
indicator[1]=0;  
settextcolor(14);
i=wg.xright-2*text_width-2;
if(i<2)i=2;
if(line == 0)lir_pixwrite(i,wg.yborder+2,indicator);
settextcolor(7);
lir_refresh_screen();
}

void write_wav_header(int totbytes)
{
int i;
// Write the header for a .wav file using the current output
// settings. 
// First point to start of file.
if(totbytes == 0x7fffffff)
  {
  if(fgetpos(wav_file, &wav_wrpos))
    {
    lirerr(1114);
    return;
    }
  }
else
  {
  if(fsetpos(wav_file, &wav_wrpos) != 0)
    {
    lirerr(1111);
    return;
    }
  }
// First 4 bytes should be "RIFF"
if(fwrite("RIFF",sizeof(int),1,wav_file)!= 1)
  {
headerr:;
  lirerr(1112);
  return;
  }
// Now file size in bytes -8
// The format chunk uses 16 bytes. 
i=totbytes+16;
if(fwrite(&i,sizeof(int),1,wav_file)!= 1)goto headerr;
// The next code word pair is "WAVEfmt "
if(fwrite("WAVEfmt ",sizeof(int),2,wav_file)!= 2)goto headerr;
// Write the size of the format chunk = 16
i=16;
if(fwrite(&i,sizeof(int),1,wav_file)!= 1)goto headerr;





// Write the type of data (Format Tag = 1 for PCM data)
i=1;
if(fwrite(&i,sizeof(short int),1,wav_file)!=1)goto headerr;
// Write no of channels
if(fwrite(&rx_daout_channels,sizeof(short int),1,wav_file)!=1)goto headerr;
// Write the output speed. 
if(fwrite(&genparm[DA_OUTPUT_SPEED],sizeof(int),1,wav_file)!=1)goto headerr;


i=genparm[DA_OUTPUT_SPEED]*rx_daout_bytes;
// Write average bytes per second.
if(fwrite(&i,sizeof(int),1,wav_file)!=1)goto headerr;


i=rx_daout_channels*rx_daout_bytes;
// Write block align.
if(fwrite(&i,sizeof(short int),1,wav_file)!=1)goto headerr;

// Write bits per sample
i=8*rx_daout_bytes;
if(fwrite(&i,sizeof(short int),1,wav_file)!=1)goto headerr;
// Now write the code word "data"
if(fwrite("data",sizeof(int),1,wav_file)!=1)goto headerr;
// And finally the size of the data block
if(fwrite(&totbytes,sizeof(int),1,wav_file)!=1)goto headerr;
}

void init_memalloc(MEM_INF *mm, int max)
{
memalloc_no=0;
memalloc_max=max;
memalloc_mem=mm;
}

void mem(int num, void *pointer, int size, int scratch_size)
{
if(lir_errcod !=0)return;
// Skip if outside array. Error code will come on return from memalloc.
if(memalloc_no<0 || memalloc_no>=memalloc_max)goto skip;
memalloc_mem[memalloc_no].pointer=pointer;
if(size<0)
  {
  lirerr(1125);
  return;
  }
memalloc_mem[memalloc_no].size=(size+15)&0xfffffff0;
memalloc_mem[memalloc_no].scratch_size=(scratch_size+15)&0xfffffff0;
memalloc_mem[memalloc_no].num=num;
skip:;
memalloc_no++;
}

int memalloc( int *handle, char *s)
{
int i,j,k,totbytes,kk;
char *x;
int *ptr;
if(memalloc_no<0 || memalloc_no >= memalloc_max)
  {
  DEB"memalloc_no=%d %s",memalloc_no,s);
  lirerr(1136);
  }
if(lir_errcod != 0)return 0;
memalloc_mem[memalloc_no].pointer=s;
memalloc_mem[memalloc_no].size=-1;
totbytes=16+DEBMEM;
for(i=0; i<memalloc_no; i++)
  {
  totbytes+=memalloc_mem[i].size+memalloc_mem[i].scratch_size+DEBMEM;
  }
DEB"%s: %3.1f Megabytes(%d arrays)\n",s,
                                totbytes*0.000001,memalloc_no);
handle[0]=(int)(malloc(totbytes+16));
if(handle[0] == 0)return 0;
k=((int)(handle[0])+15)&0xfffffff0;
kk=k;
for(i=0; i<memalloc_no; i++)
  {
  x=(void*)(k);
  k+=DEBMEM;
  for (j=0; j<DEBMEM; j++)x[j]=j&0xff;
  k+=memalloc_mem[i].scratch_size;
  ptr=memalloc_mem[i].pointer;
  ptr[0]=k;
  k+=memalloc_mem[i].size;
  }
x=(void*)(k);
k+=DEBMEM;
for (j=0; j<DEBMEM; j++)x[j]=j&0xff;
return totbytes;
}

void memcheck(int callno, MEM_INF *mm, int *handle)
{
char s[80];
int fl, i, j, k, errflag;
unsigned char *x;
k=(handle[0]+15)&0xfffffff0;
i=0;
errflag=0;
fl=mm[i].size;
while(fl != -1)
  {
  x=(void*)(k);
  k+=DEBMEM;
  for (j=0; j<DEBMEM; j++)
    {
    if( x[j] != (j&0xff) )
      {
      errflag=1;
      DEB"\nMEMORY ERROR mm=%d[%d]  data is%d(%d)  Call no %d",
                                            mm[i].num,j,x[j],j&0xff,callno);
      printf("\nMEMORY ERROR mm=%d[%d]  data is%d (%d)  Call no %d",
                                            mm[i].num,j,x[j],j&0xff,callno);
      } 
    }
  fl=mm[i].size;
  k+=mm[i].size;
  k+=mm[i].scratch_size;
  i++;
  }
if(errflag != 0)
  {
  if(fl!=-1)
    {
    sprintf(s,"\nUnknown");
    }
  else
    {
    sprintf(s,"%s",(char*)mm[i-1].pointer);
    }  
  DEB"\nError in %s\n",s);
  printf("\nError in %s\n",s);
  lirerr(1240);
  }
}

void show_name_and_size(void)
{
int i,imax;
char s[120];
float fftx_,base_,afc_,hires_,fft3_;
if(wg.yborder-3.5*text_height<wg.ytop)return;
fftx_=0.000001*fftx_totmem;
base_=0.000001*baseband_totmem;
afc_= 0.000001*afc_totmem;
fft3_=0.000001*fft3_totmem;
hires_=0.000001*hires_totmem;
sprintf(s,"%s %.1f Mbytes. fft1,fft2=%.1f  hires %.1f \
 fft3=%.1f  afc=%.1f  bas=%.1f)",
           PROGRAM_NAME,(fftx_+base_+afc_+hires_+fft3_),
           fftx_,hires_,fft3_,afc_,base_);
imax=(wg.xright-wg.xleft)/text_width-6;
if(imax > 120)imax=120;
i=0;
while(s[i]!=0  && i<imax)i++;
s[i]=0; 
settextcolor(15);
lir_pixwrite(wg.xleft+4*text_width,wg.yborder-2*text_height,s);
settextcolor(7);
}

