#include <globdef.h>
#include <hwaredef.h>
#include <txdef.h>
#include <uidef.h>
#include <screendef.h>
#include <fft1def.h>
#include <sdrdef.h>

char hware_outdat;
char device_bits[256];  
int device_data[256];
unsigned char hware_updates[256];
char *device_name[256];
int hware_retry;
int hware_device_no;
int hware_data;
int hware_bits;
int hware_no_of_updates;


typedef struct {
double increment;
double lowest;
double highest;
int direction;
int label;
} FQ_INFO;




// **********************************************************
//                  WSE TX drive routines
// **********************************************************

// Associate a number to each frequency band that will
// need a separate case in set_hardware_tx_frequency.
#define TX_AUDIO_MODE 0
#define TX2500_TO_ANT 1
#define TX10700_TO_ANT 2
#define TX70_TO_ANT 3
#define TX144_TO_ANT 4
#define TXHFA_TO_ANT_1800 5
#define TXHFA_TO_ANT_3500 6
#define TXHFA_TO_ANT_7000 7
#define TXHFA_TO_ANT_10000 8
#define TXHFA_TO_ANT_14000 9
#define MAX_TX_BANDS 10
#define TX_BASEBAND_MINFREQ 0.013 //13 kHz
#define TX_BASEBAND_MAXFREQ 0.038 //38 kHz (13+25kHz)

FQ_INFO tx_fqinfo[MAX_TX_BANDS]={
            {0.0  ,   0.0  ,   0.0  , 1, TX_AUDIO_MODE},
            {0.025,   1.675,   2.150,-1, TXHFA_TO_ANT_1800},
            {0.025,   2.500,   2.500, 1, TX2500_TO_ANT},
            {0.025,   3.475,   3.950,-1, TXHFA_TO_ANT_3500},
            {0.025,   6.975,   7.450,-1, TXHFA_TO_ANT_7000},
            {0.025,   9.975,  10.450,-1, TXHFA_TO_ANT_10000},
            {0.025,  10.675,  10.750,-1, TX10700_TO_ANT},
            {0.025,  13.975,  14.450,-1, TXHFA_TO_ANT_14000},
            {0.025,  69.975,  70.450,-1, TX70_TO_ANT},
            {0.025, 143.975, 145.950,-1, TX144_TO_ANT}}; 
int tx2500_fqcorr=0;
int tx10700_fqcorr[4]={25,-70,-45,-30};
int tx70_fqcorr[5]={250,700,700,500,600};
int tx144_fqcorr[4]={2200,1300,1300,700};

char *device_name_tx10700={"TX10700"};
char *device_name_tx70={"TX70"};
char *device_name_txhfa144={"TX144 or TXHFA"};

void set_txhfa_data(int band)
{
if(device_data[TX144] != band)
  {
  device_data[TXHFA]=band;
  device_bits[TXHFA]=5;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=TXHFA;
  }
}

int set_tx144_data(int stepno)
{
int i;
i=8;
tx_hware_fqshift+=tx144_fqcorr[stepno/20];
while(stepno >= 20)
  {
  i>>=1;
  stepno-=20;
  }
if(device_data[TX144] != i)
  {
  device_data[TX144]=i;
  device_bits[TX144]=4;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=TX144;
  }
return stepno;  
}

void set_tx10700_data(int stepno)
{
int i;
i=8;
tx_hware_fqshift+=tx10700_fqcorr[stepno];
while(stepno != 0)
  {
  i>>=1;
  stepno--;
  }
if(device_data[TX10700] != i)
  {
  device_data[TX10700]=i;
  device_bits[TX10700]=4;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=TX10700;
  }
}

int set_tx70_data(int stepno)
{
int i;
tx_hware_fqshift+=tx70_fqcorr[stepno/4];
i=16;
while(stepno >= 4)
  {
  i>>=1;
  stepno-=4;
  }
if(device_data[TX70] != i)
  {
  device_data[TX70]=i;
  device_bits[TX70]=5;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=TX70;
  }
return stepno;
}

void set_hardware_tx_frequency(void)
{
int old_updates;
float t1;
int band_no, stepno;
if(ui.tx_da_channels==1)
  {
  tg.band_increment = 0.00001;
  tg.band_direction = 1;
  tg.band_center=0;
  t1=ui.tx_da_speed/8;
  if(tg.freq > t1)tg.freq=t1;
  if(-tg.freq < -t1)tg.freq=-t1;
  tg_basebfreq=tg.freq;
// Set up data to switch relays to connect units properly.
// and to select the correct LO frequency for all units in use.  
  make_tx_phstep();
  return;
  }    
if( ui.parport == 0 || !allow_parport)return;
if(hware_no_of_updates > 127)lirerr(213766);
band_no=0;
while(band_no < MAX_TX_BANDS-1 && tg.freq+TX_BASEBAND_MINFREQ >
                                       tx_fqinfo[band_no].highest)band_no++;
if(tg.freq+TX_BASEBAND_MAXFREQ  < tx_fqinfo[band_no].lowest && band_no > 0) 
  {
  t1=0.5*(tx_fqinfo[band_no].lowest+tx_fqinfo[band_no-1].highest);
  if(tg.freq < t1)band_no--;
  }
t1=tg.freq+0.5*(TX_BASEBAND_MINFREQ-TX_BASEBAND_MAXFREQ);
if(t1<0)t1=0;
stepno=(t1-tx_fqinfo[band_no].lowest)/tx_fqinfo[band_no].increment;
if(stepno < 0)stepno=0;
set_fq:;
tg.band_center=tx_fqinfo[band_no].lowest+stepno*tx_fqinfo[band_no].increment;  
if(tg.band_center > tx_fqinfo[band_no].highest+0.001)
  {
  stepno--;
  goto set_fq;
  } 
tg.band_increment = tx_fqinfo[band_no].increment;
tg.band_direction = tx_fqinfo[band_no].direction;
if(tg.band_direction > 0)
  {
  tg_basebfreq=tg.band_center-tg.freq;
  if(tg_basebfreq < TX_BASEBAND_MINFREQ)tg_basebfreq=TX_BASEBAND_MINFREQ;
  if(tg_basebfreq > TX_BASEBAND_MAXFREQ)tg_basebfreq=TX_BASEBAND_MAXFREQ;
  tg.freq=tg.band_center-tg_basebfreq;
  }
else
  {
  tg_basebfreq=tg.freq-tg.band_center;
  if(tg_basebfreq < TX_BASEBAND_MINFREQ)tg_basebfreq=TX_BASEBAND_MINFREQ;
  if(tg_basebfreq > TX_BASEBAND_MAXFREQ)tg_basebfreq=TX_BASEBAND_MAXFREQ;
  tg.freq=tg_basebfreq+tg.band_center;
  }
// Set up data to switch relays to connect units properly.
// and to select the correct LO frequency for all units in use.  
old_updates=hware_no_of_updates;
tx_hware_fqshift=0;
switch (tx_fqinfo[band_no].label)
  {   
  case TX_AUDIO_MODE:
// Send nothing to the hardware, just set frequency
// and direction to zero so frequency scale in waterfall and
// other graphs gives audio frequency.
  break;
     
  case TX2500_TO_ANT:
// Route the signal source to the TX2500 input. 
  tx_hware_fqshift+=tx2500_fqcorr;
  break;
  
  case TX10700_TO_ANT:
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TX10700 input. 
  goto set_10700;
  
  case TX70_TO_ANT:
// Route TX70 output to the TX10700 input. 
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TX10700 input. 
  goto set_70;

  case TXHFA_TO_ANT_1800:
// Route TXHFA output to the TX70 input. 
// Route TX70 output to the TX10700 input. 
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TXHFA input. 
  set_txhfa_data(16);
  goto set_70;

  case TXHFA_TO_ANT_3500:
// Route TXHFA output to the TX70 input. 
// Route TX70 output to the TX10700 input. 
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TXHFA input. 
  set_txhfa_data(8);
  goto set_70;

  case TXHFA_TO_ANT_7000:
// Route TXHFA output to the TX70 input. 
// Route TX70 output to the TX10700 input. 
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TXHFA input. 
  set_txhfa_data(4);
  goto set_70;

  case TXHFA_TO_ANT_10000:
// Route TXHFA output to the TX70 input. 
// Route TX70 output to the TX10700 input. 
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TXHFA input. 
  set_txhfa_data(2);
  goto set_70;

  case TXHFA_TO_ANT_14000:
// Route TXHFA output to the TX70 input. 
// Route TX70 output to the TX10700 input. 
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TXHFA input. 
  set_txhfa_data(1);
  goto set_70;

  case TX144_TO_ANT:
// Route TX144 output to the TX70 input. 
// Route TX70 output to the TX10700 input. 
// Route TX10700 output to the TX2500 input. 
// Route the signal source to the TX10700 input. 
  stepno=set_tx144_data(stepno);
set_70:;
  stepno=set_tx70_data(stepno);
set_10700:;
  set_tx10700_data(stepno);
  tx_hware_fqshift+=tx2500_fqcorr;
  break;
  }
make_tx_phstep();
if(hware_no_of_updates != old_updates)
  {
  hware_flag=1;
  hware_time=current_time();
  }
}

void hware_hand_key(void)
{
int i;
// This routine is called from the rx_input_thread.
// It is responsible for reading the hardware and setting
// hand_key to TRUE or FALSE depending on whether the hand key
// is depressed or not.
if( ui.parport != 0 && allow_parport)
  {
  i=lir_inb(parport_status)&HWARE_MORSE_KEY;
  if(i==0)
    {
    hand_key=TRUE;
    }
  else
    {
    hand_key=FALSE;
    }
  }
}

void hware_set_rxtx(int state)
{
if( ui.parport != 0 && allow_parport)
  {
  lir_mutex_lock();
  if(state == TRUE)
    {
    hware_outdat&=-1-HWARE_RXTX;
    }
  else
    {
    hware_outdat|=HWARE_RXTX;
    }
  lir_mutex_unlock();
  lir_outb(hware_outdat,parport_control);
  }
}


// **********************************************************
//                  WSE RX drive routines
// **********************************************************
// The radio hardware may be designed in many different ways.
// When the user selects a new frequency there may be several
// different hardware units that have to be changed.
// The routine supplied here is designed to work with the
// RXxxx -> RX70 -> RX10700 -> RX2500 converters.
// If you have some other hardware, you may replace or modify these 
// routines and move them into (w)users_hwaredriver.c   
//
// Associate a number to each frequency band that will
// need a separate case in set_hardware_rx_frequency.
#define RX_AUDIO_MODE 0
#define RX2500_TO_ANT 1
#define RX10700_TO_ANT 2
#define RX70_TO_ANT 3
#define RX144_TO_ANT 4
#define RXHFA_TO_ANT_1800 5
#define RXHFA_TO_ANT_3500 6
#define RXHFA_TO_ANT_7000 7
#define RXHFA_TO_ANT_10000 8
#define RXHFA_TO_ANT_14000 9
#define MAX_RX_BANDS 10

#define TEST_GROUP_Y1 4
#define TEST_GROUP_Y2 TEST_GROUP_Y1 +10
#define TEST_GROUP_Y3 TEST_GROUP_Y2 +8

char *undefmsg={"Undef"};
char *device_name_rx10700={"RX10700"};
char *device_name_rx70={"RX70"};
char *device_name_rx144={"RX144"};
char *device_name_rxhfa={"RXHFA"};
char *device_name_rxhfa_gain={"Gain(RXHFA)"};


FQ_INFO rx_fqinfo[MAX_RX_BANDS]={
             {0.0  ,   0.0  ,   0.0  , 1, RX_AUDIO_MODE},
             {0.025,   1.675,   2.150,-1, RXHFA_TO_ANT_1800},
             {0.025,   2.500,   2.500, 1, RX2500_TO_ANT},
             {0.025,   3.475,   3.950,-1, RXHFA_TO_ANT_3500},
             {0.025,   6.975,   7.450,-1, RXHFA_TO_ANT_7000},
             {0.025,   9.975,  10.450,-1, RXHFA_TO_ANT_10000},
             {0.025,  10.675,  10.750,-1, RX10700_TO_ANT},
             {0.025,  13.975,  14.450,-1, RXHFA_TO_ANT_14000},
             {0.025,  69.975,  70.450,-1, RX70_TO_ANT},
             {0.025, 143.975, 145.950,-1, RX144_TO_ANT}}; 

int rx2500_fqcorr=0;
int rx10700_fqcorr[4]={0,-100,180,-60};
int rx70_fqcorr[5]={-230,-100,-100,-100,-300};
int rx144_fqcorr[4]={0,-50,-160,-130};
int rxhfa_fqcorr[5]={0,0,0,0,0};


void hware_interface_test(void)
{
int i, pardat, j, k, xpos, bits[8];
int cntrldat;
char inpins[5]={15,13,12,10,11};
char s[80];
clear_screen();
if(allow_parport == FALSE)
  {
#if(OSNUM == OS_FLAG_LINUX)
  lirerr(1063);
#endif
#if(OSNUM == OS_FLAG_WINDOWS)
  lirerr(1064);
#endif
  return;
  }
clear_hware_data();
pardat=0;
cntrldat=0;
lir_text(3,TEST_GROUP_Y1,"DATA PORT (address, output only)");
lir_text(2,TEST_GROUP_Y1+2,"Enter 0 to 7 for bit to toggle");
lir_text(5,TEST_GROUP_Y1+4,"Bit:");
lir_text(2,TEST_GROUP_Y1+5,"Status:");
lir_text(5,TEST_GROUP_Y1+6,"Pin:");
lir_text(3,TEST_GROUP_Y3,"CONTROL PORT input pins");
lir_text(5,TEST_GROUP_Y3+3,"Bit:");
lir_text(2,TEST_GROUP_Y3+4,"Status:");
lir_text(5,TEST_GROUP_Y3+5,"Pin:");
lir_text(5,TEST_GROUP_Y2,"Enter D to toggle data (pin 17)");
lir_text(5,TEST_GROUP_Y2+1,"Enter C to toggle clock (pin 1)");
goto show;
loop:;
lir_refresh_screen();
lir_sleep(10000);
test_keyboard();
if(kill_all_flag) return;
if(lir_inkey != 0)
  {
  process_current_lir_inkey();
  if(kill_all_flag) return;
  if(lir_inkey >= '0' && lir_inkey <= '7')
    {
    k=1;
    j=lir_inkey-'0';
    while(j != 0)
      {
      k<<=1;
      j--;
      }
    pardat^=k;
    }
  if(lir_inkey=='D')
    {
    cntrldat ^= HWARE_DATA;
    }
  if(lir_inkey=='C')
    {
    cntrldat ^= HWARE_CLOCK;
    }
show:;  
  j=pardat;
  k=0;
  while(k<8)
    {
    bits[k]=j&1;
    if(bits[k] >0)bits[k]=1;
    j>>=1;
    k++;
    }
  xpos=11;
  for(k=0; k<8; k++)
    {
    sprintf(s,"%1d",k);
    lir_text(xpos,TEST_GROUP_Y1+4,s);
    sprintf(s,"%1d",bits[k]);
    lir_text(xpos,TEST_GROUP_Y1+5,s);
    sprintf(s,"%1d",k+2);
    lir_text(xpos,TEST_GROUP_Y1+6,s);
    xpos+=3;
    }
  for(i=0; i<20;i++)s[i]=' ';
  s[20]=0;  
  lir_text(xpos+5,TEST_GROUP_Y1+5,s);
  lir_text(xpos+5,TEST_GROUP_Y1+5,device_name[pardat]);
  s[1]=0;
  if( (cntrldat&HWARE_DATA) == 0)
    {
    s[0]='H';
    }
  else
    {
    s[0]='L';
    }  
  lir_text(3,TEST_GROUP_Y2+3,"Data=");
  lir_text(8,TEST_GROUP_Y2+3,s);
  if( (cntrldat&HWARE_CLOCK) == 0)
    {
    s[0]='H';
    }
  else
    {
    s[0]='L';
    }  
  lir_text(23,TEST_GROUP_Y2+3,"Clock=");
  lir_text(29,TEST_GROUP_Y2+3,s);
  lir_outb(pardat,ui.parport);
  lir_outb(cntrldat,parport_control);
  }
i=lir_inb(parport_status);
xpos=11;
i >>= 3;
j=8;
for(k=0; k<5; k++)
  {
  settextcolor(7);
  if( (j & parport_ack) !=0)settextcolor(12);
  sprintf(s,"%1d",k+3);
  lir_text(xpos+1,TEST_GROUP_Y3+3,s);
  sprintf(s,"%1d",i&1);
  lir_text(xpos+1,TEST_GROUP_Y3+4,s);
  sprintf(s,"%1d",inpins[k]);
  lir_text(xpos,TEST_GROUP_Y3+5,s);
  xpos+=3;
  i >>= 1;
  j <<= 1;
  }
settextcolor(7);
if(lir_inkey != 'X')goto loop;
}

void set_rxhfa_data(int band)
{
// band   freq
//  16    1.8MHz
//   8    3.5MHz
//   4     7MHz
//   2    10MHz
//   1    14MHz
int i, k;
i=band;
k=0;
while(i!=16)
  {
  i*=2;
  k++;
  }
rx_hware_fqshift+=rxhfa_fqcorr[k];
if(device_data[RXHFA] != band)
  {
  device_data[RXHFA]=band;
  device_bits[RXHFA]=5;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=RXHFA;
  }
}

int set_rx144_data(int stepno)
{
int i;
rx_hware_fqshift+=rx144_fqcorr[stepno/20];
i=8;
while(stepno >= 20)
  {
  i>>=1;
  stepno-=20;
  }
if(device_data[RX144] != i)
  {
  device_data[RX144]=i;
  device_bits[RX144]=4;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=RX144;
  }
return stepno;  
}

void set_rx10700_data(int stepno)
{
int i;
i=8;
rx_hware_fqshift+=rx10700_fqcorr[stepno];
while(stepno != 0)
  {
  i>>=1;
  stepno--;
  }
if(device_data[RX10700] != i)
  {
  device_data[RX10700]=i;
  device_bits[RX10700]=4;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=RX10700;
  }
}

int set_rx70_data(int stepno)
{
int i;
i=16;
rx_hware_fqshift+=rx70_fqcorr[stepno/4];
while(stepno >= 4)
  {
  i>>=1;
  stepno-=4;
  }
if(device_data[RX70] != i)
  {
  device_data[RX70]=i;
  device_bits[RX70]=5;
  hware_no_of_updates++;
  hware_updates[hware_no_of_updates]=RX70;
  }
return stepno;
}

void set_hardware_rx_gain(void)
{
// bit0 to bit 2 for amplifier/bypass/-10 dB.
// bit0-bit2 = 6 => -10dB
// bit0-bit2 = 3 => 0dB
// bit0-bit2 = 5 => +10dB
//
// bit3 for 5 dB attenuator.
// bit3 = 0 => -5dB
// bit3 = 1 =>  0dB
int gaindat[6]={6,14,3,11,5,13};
int i;
if(diskread_flag >= 2)return;
if(ui.rx_addev_no == SDR14_DEVICE_CODE)
  {
  fg.gain_increment=10;
  fg.gain/=10;
  fg.gain*=10;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-30)fg.gain=-30;
  if(rx_hware_init_flag==0)sdr14_att_counter++;
  return;
  }
if(ui.rx_addev_no == SDRIQ_DEVICE_CODE)
  {
  fg.gain_increment=1;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-36)fg.gain=-36;
  if(rx_hware_init_flag==0)sdr14_att_counter++;
  return;
  }
if(ui.rx_addev_no == PERSEUS_DEVICE_CODE)
  {
  fg.gain_increment=10;
  fg.gain/=10;
  fg.gain*=10;
  if(fg.gain > 0)fg.gain=0;
  if(fg.gain<-30)fg.gain=-30;
  if(rx_hware_init_flag==0)perseus_att_counter++;
  return;
  }
if( ui.parport == 0 || !allow_parport)return;
if(hware_no_of_updates > 127)lirerr(213767);
switch (rx_fqinfo[fg_new_band].label)
  {   
  case RXHFA_TO_ANT_1800:
  case RXHFA_TO_ANT_3500:
  case RXHFA_TO_ANT_7000:
  case RXHFA_TO_ANT_10000:
  case RXHFA_TO_ANT_14000:
  fg.gain_increment=5;
  fg.gain/=5;
  fg.gain*=5;
  if(fg.gain < -15)fg.gain=-15;
  if(fg.gain > 10)fg.gain=10;
  i=fg.gain/5+3;
  if(device_data[RXHFA_GAIN] != gaindat[i])
    {
    device_data[RXHFA_GAIN]=gaindat[i];
    device_bits[RXHFA_GAIN]=4;
    hware_no_of_updates++;
    hware_updates[hware_no_of_updates]=RXHFA_GAIN;
    hware_flag=1;
    hware_time=current_time();
    }
  break;
  
  default:
  break;
  }
}

void set_hardware_rx_frequency(void)
{
float t1;
int i, stepno;
int old_updates;
if(diskread_flag >= 2)
  {
  sc[SC_WG_FQ_SCALE]++;
  return;
  }
if(rx_mode < MODE_TXTEST)users_set_band_no();
if(ui.rx_addev_no == SDR14_DEVICE_CODE ||
   ui.rx_addev_no == SDRIQ_DEVICE_CODE )
  {
  t1=fg.passband_center;
  fg.passband_direction=1;
  if(fg.passband_center > adjusted_sdr_clock)
    {
    i=fg.passband_center/adjusted_sdr_clock;
    t1=fg.passband_center-i*adjusted_sdr_clock;
    }
  if(t1 > 0.5*adjusted_sdr_clock)
    {
    t1=adjusted_sdr_clock-t1;
    fg.passband_direction=-1;
    }
  fft1_direction=fg.passband_direction;
  fg.passband_increment=.1;
  if(rx_hware_init_flag==0)sdr14_nco_counter++;
  return;
  }
if(ui.rx_addev_no == PERSEUS_DEVICE_CODE)
  {
  fg.passband_direction=1;
  fft1_direction=fg.passband_direction;
  fg.passband_increment=.1;
  if(rx_hware_init_flag==0)perseus_nco_counter++;
  return;
  }
if( ui.parport == 0 || !allow_parport)return;
if(hware_no_of_updates > 127)lirerr(213768);
fg_new_band=0;
while(fg_new_band < MAX_RX_BANDS-1 && 
     fg.passband_center > rx_fqinfo[fg_new_band].highest)fg_new_band++;
if(fg.passband_center < rx_fqinfo[fg_new_band].lowest && fg_new_band > 0) 
  {
  t1=0.5*(rx_fqinfo[fg_new_band].lowest+rx_fqinfo[fg_new_band-1].highest);
  if(fg.passband_center < t1)fg_new_band--;
  }
if(fg_old_band != fg_new_band)set_hardware_rx_gain();
fg_old_band=fg_new_band;
t1=(fg.passband_center-rx_fqinfo[fg_new_band].lowest)/
                                         rx_fqinfo[fg_new_band].increment;
stepno=t1+0.5;
if(stepno < 0)stepno=0;
set_fq:;
fg.passband_center=rx_fqinfo[fg_new_band].lowest+
                                   stepno*rx_fqinfo[fg_new_band].increment;  
if(fg.passband_center > rx_fqinfo[fg_new_band].highest+0.0000001)
  {
  stepno--;
  goto set_fq;
  }  
fg.passband_increment=rx_fqinfo[fg_new_band].increment;
fg.passband_direction=rx_fqinfo[fg_new_band].direction;
//
// Here the frequency scale can be inverted on some condition.
// Possibly a new user parameter....
//#if(OSNUM == OS_FLAG_LINUX)
//if(ui.use_alsa == TRUE)fg.passband_direction*=-1;
//#endif
fft1_direction=fg.passband_direction;
// Send a message to the screen thread to redraw the frequency scale
// on the main spectrum.
sc[SC_WG_FQ_SCALE]++;
// Set up data to switch relays to connect units properly.
// and to select the correct LO frequency for all units in use.  
old_updates=hware_no_of_updates;
rx_hware_fqshift=0;
switch (rx_fqinfo[fg_new_band].label)
  {   
  case RX_AUDIO_MODE:
// Send nothing to the hardware, just set frequency
// and direction to zero so frequency scale in waterfall and
// other graphs gives audio frequency.
  break;
     
  case RX2500_TO_ANT:
// Route the signal source to the RX2500 input. 
  rx_hware_fqshift+=rx2500_fqcorr;
  break;
  
  case RX10700_TO_ANT:
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RX10700 input. 
  goto set_10700;
  
  case RX70_TO_ANT:
// Route RX70 output to the RX10700 input. 
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RX10700 input. 
  goto set_70;

  case RXHFA_TO_ANT_1800:
// Route RXHFA output to the RX70 input. 
// Route RX70 output to the RX10700 input. 
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RXHFA input. 
  set_rxhfa_data(16);
  goto set_70;

  case RXHFA_TO_ANT_3500:
// Route RXHFA output to the RX70 input. 
// Route RX70 output to the RX10700 input. 
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RXHFA input. 
  set_rxhfa_data(8);
  goto set_70;

  case RXHFA_TO_ANT_7000:
// Route RXHFA output to the RX70 input. 
// Route RX70 output to the RX10700 input. 
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RXHFA input. 
  set_rxhfa_data(4);
  goto set_70;

  case RXHFA_TO_ANT_10000:
// Route RXHFA output to the RX70 input. 
// Route RX70 output to the RX10700 input. 
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RXHFA input. 
  set_rxhfa_data(2);
  goto set_70;

  case RXHFA_TO_ANT_14000:
// Route RXHFA output to the RX70 input. 
// Route RX70 output to the RX10700 input. 
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RXHFA input. 
  set_rxhfa_data(1);
  goto set_70;

  case RX144_TO_ANT:
// Route RX144 output to the RX70 input. 
// Route RX70 output to the RX10700 input. 
// Route RX10700 output to the RX2500 input. 
// Route the signal source to the RX10700 input. 
  stepno=set_rx144_data(stepno);
set_70:;
  stepno=set_rx70_data(stepno);
set_10700:;
  set_rx10700_data(stepno);
  rx_hware_fqshift+=rx2500_fqcorr;
  break;
  }
if(hware_no_of_updates != old_updates)
  {
  hware_flag=1;
  hware_time=current_time();
  }
else
  {
  rx_hware_init_flag=0;
  }
}

void clear_hware_data(void)
{
int i;
// Set -1 in device_data. 
// A data word is max 31 bit, the sign bit is a flag
// saying the data is not valid.
for(i=0; i<256; i++)
  {
  device_data[i]=-1;
  device_name[i]=undefmsg;
  }
device_name[RX10700]=device_name_rx10700;
device_name[RX70]=device_name_rx70;
device_name[RX144]=device_name_rx144;
device_name[RXHFA]=device_name_rxhfa;
device_name[RXHFA_GAIN]=device_name_rxhfa_gain;
device_name[TX10700]=device_name_tx10700;
device_name[TX70]=device_name_tx70;
device_name[TXHFA]=device_name_txhfa144;
rx_hware_init_flag=1;
hware_no_of_updates=-1;
hware_flag=0;
hware_time=current_time();
fg_old_band=-1;
if(ui.rx_addev_no == SDR14_DEVICE_CODE ||
   ui.rx_addev_no == SDRIQ_DEVICE_CODE ||
   ui.rx_addev_no == PERSEUS_DEVICE_CODE)
  {
  rx_hware_init_flag=0;
  }
}

void control_hware(void)
{
int err;
unsigned char outdat;
char s[80];
switch(hware_flag)
  {
  case 1:
  if(hware_no_of_updates <0)
    {
    lirerr(988342);
    return;
    }
  hware_device_no=hware_updates[hware_no_of_updates];
  hware_error_flag=0;
  hware_data=device_data[hware_device_no];
  hware_bits=device_bits[hware_device_no];
// A new unit is selected.
// Write the number of this unit to the data port
  outdat=hware_device_no;
  lir_outb(outdat,ui.parport);
  hware_flag=2;
// Set clock low and make data the first bit of current output.
  lir_mutex_lock();
  if( (hware_data & 1) == 0)
    {
    hware_outdat=HWARE_DATA+HWARE_CLOCK;
    }
  else
    {
    hware_outdat=HWARE_CLOCK;
    }  
  lir_outb(hware_outdat,parport_control);
  lir_mutex_unlock();
  break;

  case 2:
// Set clock high to clock in new data bit.
  lir_mutex_lock();
  hware_outdat^=HWARE_CLOCK;
  lir_outb(hware_outdat,parport_control);
  lir_mutex_unlock();
  hware_flag=3;
  hware_retry=0;
  break;

  case 3:
// Read status to see if we really got the data bit into the unit.
  outdat=lir_inb(parport_status)&parport_ack;
  outdat^=parport_ack_sign;
  err=0;
  if(outdat == 0)
    {
    if( (hware_outdat&HWARE_DATA) != 0)
      {
      err=1;
      }
    }
  else  
    {
    if( (hware_outdat&HWARE_DATA) == 0)
      {
      err=1;
      }
    }
  hware_retry++;
  if(err==1)
    {
    if(hware_retry<4)return;
    hware_error_flag=1;
    }
  hware_bits--;
  if(hware_bits == 0)
    {
// No more. Deselect unit.
    lir_outb(0,ui.parport);
    hware_flag=4;
    return;
    }
  else
    {
// Set clock low and make data the next bit of current output.
    hware_data=hware_data>>1;
    lir_mutex_lock();
    if( (hware_data & 1) == 0)
      {
      hware_outdat=HWARE_DATA+HWARE_CLOCK;
      }
    else
      {
      hware_outdat=HWARE_CLOCK;
      }  
    lir_outb(hware_outdat,parport_control);
    lir_mutex_unlock();
    hware_flag=2;
    }
  break;

  case 4:
// Select unit. This time we actually clock the new data 
// from the shift register into the latch.
  outdat=hware_device_no;
  lir_outb(outdat,ui.parport);
  hware_flag=5;
  return;

  case 5:
  lir_outb(0,ui.parport);
  hware_no_of_updates--;
  if(hware_no_of_updates >= 0)
    {
    hware_flag=1;
    }
  else
    {  
    hware_flag=0;
    rx_hware_init_flag=0;
    }
  lir_mutex_lock();
  hware_outdat=HWARE_CLOCK;
  lir_outb(hware_outdat,parport_control);
  lir_mutex_unlock();
  if(hware_error_flag != 0)
    {
    sprintf(s,"  ERROR: %s",device_name[hware_device_no]);
    wg_error(s,WGERR_HWARE);
    }

  break;      
  }
}
