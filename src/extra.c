#include <time.h>
#include <globdef.h>
#include <uidef.h>
#include <seldef.h>

extern float users_extra_update_interval;

void init_users_extra(void)
{
// This routine is called just before a receive mode is entered.
// Use it to set your own things from the keyboard.
//
// Set users_extra_update_interval to the highest rate at which
// you want your routines called.
// The users_extra routines will be called only when the CPU is idle
// and if the time interval you have specified is exceeded.
users_extra_update_interval=0.25;
}
 
void users_extra(void)
{
// Demo code for users_extra by ON5GN, Pierre
//
// color codes:
//
//0      - black.
//1      - blue.
//2      - green.
//3      - cyan.
//4      - red.
//5      - magenta.
//6      - brown.
//7      - grey.
//8      - dark grey (light black).
//9      - light blue.
//10     - light green.
//11     - light cyan.
//12     - light red.
//13     - light magenta.
//14     - yellow (light brown).
//15     - white (light grey).
//
struct tm *tm_ptr;
time_t the_time;
char s[80];
char s1[80];
double freq_Mhz;
double freq_khz;
double freq_hz;
double bw;
double bw_khz;
double bw_hz;

// Hide the mouse while we update the screen so it will
// not be destroyed by what we write.
hide_mouse(0,screen_width,0,text_height);

//display mode
sprintf (s,"MODE=%s",rxmodes[rx_mode]);
settextcolor(12); //red
lir_text (1,0,s);
 
//display bandwith
bw =0.001*baseband_bw_hz;
bw_khz= floor (bw);
bw_hz= floor ((bw-(bw_khz))*1000);
if (bw_khz)
  {
  sprintf (s,"B/W=%.3f Hz     ",bw);
  }
else
  {
  sprintf (s,"B/W=%3.0f Hz    ",bw_hz);
  }
settextcolor(14); //yellow
lir_text(1,1,s);

//display frequency
if(mix1_selfreq[0] < 0)
  {
  sprintf (s,"FREQ=        off         ");
  }
else
  {
  freq_Mhz= floor(hwfreq/1000) ;
  freq_khz= floor (hwfreq-(freq_Mhz*1000)) ;
  freq_hz=  floor (hwfreq*1000-(freq_Mhz*1000000)-(freq_khz*1000)) ;
  sprintf (s,"FREQ=%3.0f.%03.0f.%03.0f Hz  ",freq_Mhz,freq_khz,freq_hz);
  }
settextcolor(10); // light green
lir_text(20,1,s);

//display UTC date and time in ISO 8601 format
(void) time(&the_time);
tm_ptr=gmtime(&the_time);
sprintf(s,"UTC= %4d:%02d:%02d  %02d:%02d:%02d ",
               tm_ptr->tm_year+1900, tm_ptr->tm_mon+1, tm_ptr->tm_mday,
                    tm_ptr->tm_hour,tm_ptr->tm_min, tm_ptr->tm_sec);
settextcolor (9);  //9 light blue:
lir_text(45,1,s);

// display NETWORK mode

s1[0]='\0';
if((ui.network_flag & NET_RXOUT_RAW16) != 0)  strncat ( s1,"RAW16 ",6);
if((ui.network_flag & NET_RXOUT_RAW18) != 0)  strncat ( s1,"RAW18 ",6);
if((ui.network_flag & NET_RXOUT_RAW24) != 0)  strncat ( s1,"RAW24 ",6);
if((ui.network_flag & NET_RXOUT_FFT1)  != 0)  strncat ( s1,"FFT1 ",6);
if((ui.network_flag & NET_RXOUT_TIMF2) != 0)  strncat ( s1,"TIMF2 ",6);
if((ui.network_flag & NET_RXOUT_FFT2)  != 0)  strncat ( s1,"FFT2 ",6);
if (strlen(s1) == 0)  strncat ( s1,"off ",6);

switch (ui.network_flag & NET_RX_INPUT)
  {
  case NET_RXIN_RAW16:
  strncat (s1," RX->RAW16",9);
  break;

  case NET_RXIN_RAW18:
  strncat(s1," RX->RAW18",9);
  break;

  case NET_RXIN_RAW24:
  strncat (s1," RX->RAW24",9);
  break;

  case NET_RXIN_FFT1:
  strncat (s1," RX->FFT1",9);
  break;

  default:
  strncat (s1," RX->off",9);
  break;
  }
sprintf (s,"NETWORK= SEND->%s",s1);
settextcolor(3); //cyan
lir_text(45,0,s);
settextcolor(7);
}

