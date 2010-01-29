// users_tr.c for external frequency control via serial port
// for IC-275/IC-706/FT-1000/FT-736/TS-850/TS-2000/FT-950/FT-897/FT-450/FT-2000
// first version: 4 may 2007 for linrad 02.31
// updated: 22 july 2008  for linrad-02.48 
// runs under windows and linux 
//
// 
// To generate a windows version in  linux:
//   install mingw32 from Leif's website,
//   save this users_tr.c as wusers_hwaredriver.c 
//   run ./configure and  make linrad.exe
// To generate a windows version in windows:
//   install mingw.zip and nasm.zip from Leif's website,
//   save this users_tr.c as wusers_hwaredriver.c 
//   run configure.exe  and  make.bat
// To add your own transceiver:
//   1. Add a line in the list of supported transceivers with the 
//      corresponding serial port parameters and frequency range.
//      (Look at line 58 ) 
//   2. change the max_transceiver_number variable at line 40 to reflect the new number of entries 
//   3. Add the corresponding serial I/O programming.
//      (Look at line 838 for the calibration logic 
//       and line 1021 for the 'set trx freq' logic )
// Calibration procedure:
//    Tune the TRX receiver and linrad to the same frequency (zero beat)
//    Select  the 'calibration' box with the mouse
//    Hit the Q-key to calculate and store the frequency offset
//
// Based on input from DL3IAE/IK7EZN/K1JT/ON4IY/ON7EH/W3SZ/YO4AUL
// with the support of Leif SM5BSZ
// All errors are mine
// Use at your own risk and enjoy
// Pierre/ON5GN

// ****************************************************************
// Change the define for RX_HARDWARE and modify the dummy routines
// in the prototype section to fit your needs.
// The routines in wse_sdrxx.c can serve as a starting point.
#define RX_HARDWARE 0
// ****************************************************************

#include <string.h>

int serport_open_status;
int serport_number = 1 ;           //default value
int transceiver_number = 0 ;       //default value
int max_transceiver_number = 12 ;  //total number of entries in 'transceiver list'-1.
int chnge_serport_switch =0;
int chnge_calibr_switch =0;
double offset_hz;
double offset_khz;
double old_hwfreq;

struct transceiver
{
 char name[11];
 int serport_baudrate;
 int serport_stopbits;
 int min_transceiver_freq;
 int max_transceiver_freq;
 };
// LIST OF SUPPORTED TRANSCEIVERS
// + corresponding values for  serport_baudrate = 110 to 57600 ( look at lxsys.c )
//                   serport_stopbits = 0->one stopbit or 1->two stopbits
//                   min and max transceiver_frequency(Hz) : check transceiver manual
const struct transceiver list[32] =
{{"FT-450     ",4800,1,    30000,  56000000},                
 {"FT-736     ",4800,1,144000000, 146000000},                
 {"FT-897     ",4800,1,  1500000, 470000000},                  
 {"FT-950     ",4800,1,    30000,  56000000},                      
 {"FT-1000    ",4800,1,  1600000,  30000000},                    
 {"FT-2000    ",4800,1,    30000,  60000000},                 
 {"IC-275     ",9600,0,144000000, 146000000},  
 {"ICOM-all   ",9600,0,  1800000, 470000000},               
 {"IC-706     ",9600,0,  1800000, 148000000},                   
 {"IC-706MKII ",9600,0,  1800000, 148000000},                   
 {"IC-706MKIIG",9600,0,  1800000, 148000000},                   
 {"TS-850     ",4800,1,  1600000,  30000000},                     
 {"TS-2000    ",9600,0,  1600000,1480000000}                    
};


// ******************************************************************************
//               ROUTINES FOR A 'MOVABLE' FIXED SIZE USERGRAPH
//             screenposition of usergraph and offset values are 
//                saved in the file "par_ssb_ug" ( for ssb mode )
// ******************************************************************************

#define NO_OF_BANDS 12               
float bandlim[]={2.5,  //0 
                 5.0,  //1   
                 9.0,  //2
                12.0,  //3
                16.0,  //4
                19.0,  //5
                23.0,  //6
                26.0,  //7
                40.0,  //8
               100.0,  //9
               300.0,  //10
               BIG};   //11


#define USERS_GRAPH_TYPE1 64
typedef struct {
int ytop;
int ybottom;
int xleft;
int xright;
int offs_hz[NO_OF_BANDS];
int offs_khz[NO_OF_BANDS];


int yborder;    // required for move graph
} UG_PARMS;

#define UG_TOP 0
#define UG_BOTTOM 1
#define UG_LEFT 2
#define UG_RIGHT 3
#define UG_INCREASE_OFFS_HZ 4
#define UG_DECREASE_OFFS_HZ 5
#define UG_INCREASE_OFFS_KHZ 6
#define UG_DECREASE_OFFS_KHZ 7
#define UG_INCREASE_SERPORT_NMBR 8
#define UG_DECREASE_SERPORT_NMBR 9
#define UG_INCREASE_TRANSCEIVER_NMBR 10
#define UG_DECREASE_TRANSCEIVER_NMBR 11
#define MAX_UGBUTT 12

UG_PARMS ug;
BUTTONS ugbutt[MAX_UGBUTT];

                 

int ug_old_y1;
int ug_old_y2;
int ug_old_x1;
int ug_old_x2;
int users_graph_scro;

int ug_oldx;     // required for move graph
int ug_oldy;     // required for move graph
int ug_yborder;  // required for move graph
int ug_hsiz;     // required for move graph
int ug_vsiz;     // required for move graph 

int ug_band_no;

#define MAX_MSGSIZE 30     //Size of messages. min size=30
char ug_msg0[MAX_MSGSIZE]; //messages in usergraph 
char ug_msg1[MAX_MSGSIZE];
char ug_msg2[MAX_MSGSIZE];
int ug_msg_color;   //switch for message-color in usergraph
char ugfile[20];  //name of usergraph parameter file
double min_transceiver_freq=0;
double max_transceiver_freq=0;



void set_press_q_key_new_freq_msg(void)
{
ug_msg_color=14;
strcpy(ug_msg0,"PRESS Q-KEY TO SET TRX FREQ."); 
ug_msg1[0]=0;
ug_msg2[0]=0;
}

void set_press_q_key_chnge_port_msg(void)
{
char s[80];
if (strncmp(serport_name,"COM",3)==0)
{
sprintf(&serport_name[3],"%d",serport_number);
}
if (strncmp(serport_name,"/dev/ttyS",9)==0)
{
if (serport_number <= 4){
sprintf(&serport_name[9],"%d",serport_number-1);
}
else{
sprintf(&serport_name[0],"%s","/dev/ttyUSB");
sprintf(&serport_name[11],"%d",serport_number-5);
}
}
if (strncmp(serport_name,"/dev/ttyUSB",11)==0)
{
if (serport_number > 4){
sprintf(&serport_name[11],"%d",serport_number-5);
}
else{
sprintf(&serport_name[0],"%s","/dev/ttyS");
sprintf(&serport_name[9],"%d",serport_number-1);
}
}
sprintf(s," SERIAL PORT:%s ",serport_name);
lir_pixwrite(ug.xleft+.5*text_width,ug.ytop+1.5*text_height,s);
ug_msg_color=15;
strcpy(ug_msg0,"PRESS Q-KEY TO OPEN SER. PORT");
ug_msg1[0]=0;
ug_msg2[0]=0;
chnge_serport_switch=1;
}

void check_ug_borders(void)  // required for move graph
{
current_graph_minh=ug_vsiz;
current_graph_minw=ug_hsiz;
check_graph_placement((void*)(&ug));
set_graph_minwidth((void*)(&ug));
}

void show_offset(void)
{
char s[80];
settextcolor(15);
sprintf(s,"Offset-fine(Hz): %d        ",ug.offs_hz[ug_band_no]);
s[25]=0;
lir_pixwrite(ug.xleft+1.5*text_width,ug.ytop+1+3.5*text_height,s);

sprintf(s,"Offset-raw(Khz): %d        ",ug.offs_khz[ug_band_no]);
s[25]=0;
lir_pixwrite(ug.xleft+1.5*text_width,ug.ytop+2.5*text_height,s);
settextcolor(7);
offset_hz= ug.offs_hz[ug_band_no];
offset_khz=ug.offs_khz[ug_band_no];
}

void blankfill_ugmsg(char *msg)
{
int i;
i=strlen(msg);
if(i>=MAX_MSGSIZE)lirerr(3413400);
while(i<MAX_MSGSIZE-1)
  {
  msg[i]=' ';
  i++;
  }
msg[MAX_MSGSIZE-1]=0;  
}

void show_user_parms(void)
{
int i;
char s[80];
extern char serport_name[10];
FILE *Fp;
// Show the user parameters on screen
// and issue hardware commands if required.
// Use hware_flag to direct calls to control_hware() when cpu time 
// is available in case whatever you want to do takes too much time 
// to be done immediately.
// Note that mouse control is done in the narrowband processing thread
// and that you can not use lir_sleep to wait for hardware response here.
hide_mouse(ug.xleft, ug.xright,ug.ytop,ug.ybottom);
sprintf(s,"CALIBRATE");
settextcolor(11);
lir_pixwrite(ug.xleft+.5*text_width,ug.ytop+0.35*text_height,s);
sprintf(s,"   CAT FREQUENCY CONTROL FOR TRX: %s ",list[transceiver_number].name);
settextcolor(14);
lir_pixwrite(ug.xleft+10.5*text_width,ug.ytop+0.35*text_height,s);
settextcolor(15);
sprintf(s," SERIAL PORT:%s ",serport_name);
lir_pixwrite(ug.xleft+.5*text_width,ug.ytop+1.5*text_height,s);

settextcolor(7);
show_offset();

//DISPLAY MESSAGES IN  MESSAGE-AREA OF USERGRAPH

// Fill message with blanks up to character MAX_MSGSIZE-2
blankfill_ugmsg(ug_msg0);
blankfill_ugmsg(ug_msg1);
blankfill_ugmsg(ug_msg2);
settextcolor(ug_msg_color);
lir_pixwrite(ug.xleft+31*text_width,ug.ytop+1.5*text_height,ug_msg0);
lir_pixwrite(ug.xleft+31*text_width,ug.ytop+2.5*text_height,ug_msg1);
lir_pixwrite(ug.xleft+31*text_width,ug.ytop+3.5*text_height,ug_msg2);
settextcolor(7);

//save screenposition and offset-values in usergraph, to usergraph parameter file
Fp=fopen(ugfile,"w");
fprintf(Fp,"%d\n%d\n%i\n%i\n",ug.xleft,ug.ytop,serport_number,transceiver_number);
for(i=0; i<NO_OF_BANDS; i++)fprintf(Fp,"%d\n",ug.offs_hz[i]);
for(i=0; i<NO_OF_BANDS; i++)fprintf(Fp,"%d\n",ug.offs_khz[i]);
fclose(Fp);
}

void users_set_band_no(void)
{
ug_band_no=0;
while(bandlim[ug_band_no] < fg.passband_center)ug_band_no++;
if(all_threads_started == TRUE)
  {
  pause_thread(THREAD_SCREEN);
  set_press_q_key_new_freq_msg();
  show_user_parms();
  resume_thread(THREAD_SCREEN);
  }
}


void make_users_control_graph(void)
{
pause_thread(THREAD_SCREEN);
// Set a negative number here.
// In case several windows are desired, give them different
// negative numbers and use scro[m].no to decide what to do
// in your mouse_on_users_graph routine.
check_ug_borders();  // required for move graph
scro[users_graph_scro].no=USERS_GRAPH_TYPE1;
// These are the coordinates of the border lines.
scro[users_graph_scro].x1=ug.xleft;
scro[users_graph_scro].x2=ug.xright;
scro[users_graph_scro].y1=ug.ytop;
scro[users_graph_scro].y2=ug.ybottom;
// Each border line is treated as a button.
// That is for the mouse to get hold of them so the window can be moved.
ugbutt[UG_LEFT].x1=ug.xleft;
ugbutt[UG_LEFT].x2=ug.xleft+2;
ugbutt[UG_LEFT].y1=ug.ytop;
ugbutt[UG_LEFT].y2=ug.ybottom;
ugbutt[UG_RIGHT].x1=ug.xright;
ugbutt[UG_RIGHT].x2=ug.xright-2;
ugbutt[UG_RIGHT].y1=ug.ytop;
ugbutt[UG_RIGHT].y2=ug.ybottom;
ugbutt[UG_TOP].x1=ug.xleft;
ugbutt[UG_TOP].x2=ug.xright;
ugbutt[UG_TOP].y1=ug.ytop;
ugbutt[UG_TOP].y2=ug.ytop+2;
ugbutt[UG_BOTTOM].x1=ug.xleft;
ugbutt[UG_BOTTOM].x2=ug.xright;
ugbutt[UG_BOTTOM].y1=ug.ybottom;
ugbutt[UG_BOTTOM].y2=ug.ybottom-2;
// Draw the border lines
graph_borders((void*)&ug,7);
ug_oldx=-10000;                   //from freq_control
settextcolor(7);
make_button(ug.xleft+27.5*text_width+2,ug.ybottom-1*text_height/2-1,
                                         ugbutt,UG_DECREASE_OFFS_HZ,25);
make_button(ug.xleft+29*text_width+2,ug.ybottom-1*text_height/2-1,
                                     ugbutt,UG_INCREASE_OFFS_HZ,24); 

make_button(ug.xleft+27.5*text_width+2,ug.ybottom-3*text_height/2-1,
                                         ugbutt,UG_DECREASE_OFFS_KHZ,25);
make_button(ug.xleft+29*text_width+2,ug.ybottom-3*text_height/2-1,
                                     ugbutt,UG_INCREASE_OFFS_KHZ,24); 

make_button(ug.xleft+27.5*text_width+2,ug.ybottom-5*text_height/2-1,
                                         ugbutt,UG_DECREASE_SERPORT_NMBR,25);
make_button(ug.xleft+29*text_width+2,ug.ybottom-5*text_height/2-1,
                                     ugbutt,UG_INCREASE_SERPORT_NMBR,24);

make_button(ug.xleft+57.5*text_width+2,ug.ytop+1.5*text_height/2-1,
                                         ugbutt,UG_DECREASE_TRANSCEIVER_NMBR,25);
make_button(ug.xleft+59*text_width+2,ug.ytop+1.5*text_height/2-1,
                                     ugbutt,UG_INCREASE_TRANSCEIVER_NMBR,24);
show_user_parms();
//draw separatorlines in usergraph
//vertical
lir_line(ug.xleft+30.5*text_width,ug.ytop+1.3*text_height,ug.xleft+30.5*text_width,ug.ybottom,7);
lir_line(ug.xleft+10*text_width,ug.ytop+0*text_height,ug.xleft+10*text_width,ug.ytop+1.3*text_height,7);
//horizontal
lir_line(ug.xleft,ug.ytop+1.3*text_height,ug.xright,ug.ytop+1.3*text_height,7);
//
resume_thread(THREAD_SCREEN);
}


void mouse_continue_users_graph(void)
{
int i, j;      // required for move graph
//char s[80];
switch (mouse_active_flag-1)
  {
// Move border lines immediately.
// for other functions, wait until button is released.
// Move fixed size window  based on coding in freq_control.c
case UG_TOP:
  if(ug.ytop!=mouse_y)goto ugm;
  break;

  case UG_BOTTOM:
  if(ug.ybottom!=mouse_y)goto ugm;
  break;

  case UG_LEFT:
  if(ug.xleft!=mouse_x)goto ugm;
  break;

  case UG_RIGHT:
  if(ug.xright==mouse_x)break;
ugm:;
  pause_screen_and_hide_mouse();
  graph_borders((void*)&ug,0);
  if(ug_oldx==-10000)
    {
    ug_oldx=mouse_x;
    ug_oldy=mouse_y;
    }
  else
    {
    i=mouse_x-ug_oldx;
    j=mouse_y-ug_oldy;  
    ug_oldx=mouse_x;
    ug_oldy=mouse_y;
    ug.ytop+=j;
    ug.ybottom+=j;
    ug.xleft+=i;
    ug.xright+=i;
    check_ug_borders();    // check borders
    ug.yborder=(ug.ytop+ug.ybottom)>>1;
    }
  graph_borders((void*)&ug,15); 
  resume_thread(THREAD_SCREEN);
  break;

  default:
  goto await_release;
  }
if(leftpressed == BUTTON_RELEASED)goto finish;
return;
await_release:;
if(leftpressed != BUTTON_RELEASED) return;
// Assuming the user wants to control hardware we
// allow commands only when data is not over the network.
if((ui.network_flag&NET_RX_INPUT) == 0) //for linrad02.23
  {
  switch (mouse_active_flag-1)
    {
    case UG_INCREASE_OFFS_HZ: 
    ug.offs_hz[ug_band_no]++;
    set_press_q_key_new_freq_msg();
    show_user_parms();
    break;

    case UG_DECREASE_OFFS_HZ:
    ug.offs_hz[ug_band_no]--;
    set_press_q_key_new_freq_msg();
    show_user_parms();
    break;

    case UG_INCREASE_OFFS_KHZ: 
    ug.offs_khz[ug_band_no]++;
    set_press_q_key_new_freq_msg();
    show_user_parms();
    break;

    case UG_DECREASE_OFFS_KHZ:
    ug.offs_khz[ug_band_no]--;
    set_press_q_key_new_freq_msg();
    show_user_parms();
    break;

    case UG_INCREASE_SERPORT_NMBR: 
    serport_number++;
    if (serport_number >= 8) serport_number =8;
    set_press_q_key_chnge_port_msg();
    show_user_parms();
    break;

    case UG_DECREASE_SERPORT_NMBR:
    serport_number--;  
    if (serport_number <= 1) serport_number =1;
    set_press_q_key_chnge_port_msg();
    show_user_parms();
    break;

    case UG_INCREASE_TRANSCEIVER_NMBR:
    transceiver_number++;
    if (transceiver_number >= max_transceiver_number) transceiver_number =max_transceiver_number;
    set_press_q_key_new_freq_msg();
    show_user_parms();
    break;

    case UG_DECREASE_TRANSCEIVER_NMBR:
    transceiver_number--;
    if (transceiver_number <= 0) transceiver_number =0;
    set_press_q_key_new_freq_msg();
    show_user_parms();
    break;

    default:
// This should never happen.    
    lirerr(211053);
    break;
    }
  }  
finish:;
hide_mouse(ug_old_x1,ug_old_x2,ug_old_y1,ug_old_y2);
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
graph_borders((void*)&ug,0);
lir_fillbox(ug_old_x1,ug_old_y1,ug_old_x2-ug_old_x1,ug_old_y2-ug_old_y1,0);
make_users_control_graph();
ug_oldx=-10000; 
}

void new_user_offs_hz(void)
{
ug.offs_hz[ug_band_no]=numinput_int_data;
pause_thread(THREAD_SCREEN);
set_press_q_key_new_freq_msg();
show_user_parms();
resume_thread(THREAD_SCREEN);
}

void new_user_offs_khz(void)
{
ug.offs_khz[ug_band_no]=numinput_float_data;
pause_thread(THREAD_SCREEN);
set_press_q_key_new_freq_msg();
show_user_parms();
resume_thread(THREAD_SCREEN);
}


void mouse_on_users_graph(void)
{
int event_no;
// First find out if we are on a button or border line.
for(event_no=0; event_no<MAX_UGBUTT; event_no++)
  {
  if( ugbutt[event_no].x1 <= mouse_x && 
      ugbutt[event_no].x2 >= mouse_x &&      
      ugbutt[event_no].y1 <= mouse_y && 
      ugbutt[event_no].y2 >= mouse_y) 
    {
    ug_old_y1=ug.ytop;
    ug_old_y2=ug.ybottom;
    ug_old_x1=ug.xleft;
    ug_old_x2=ug.xright;
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_users_graph;
    return;
    }
  }
// Not button or border.
// Prompt the user for offs_hz or offs_khz depending on whether the mouse is
// in the upper or the lower part of the window.
mouse_active_flag=1;
numinput_xpix=ug.xleft+18.5*text_width;
if(mouse_x > ug.xleft && mouse_x < ug.xleft+10*text_width)
{
if(mouse_y > ug.ytop && mouse_y < (ug.ytop+1.3*text_height))
  {
  ug_msg_color=11;
  strcpy(ug_msg0,"PRESS Q-KEY TO CALIBRATE"); 
  ug_msg1[0]=0;
  ug_msg2[0]=0;
  chnge_calibr_switch=1;
  show_user_parms();
  }
}
if(mouse_x > ug.xleft+18.5*text_width && mouse_x < ug.xleft+25*text_width)
  {
if(mouse_y > (ug.ytop+3.8*text_height))
    {
    numinput_ypix=ug.ytop+3.6*text_height;
    numinput_chars=4;
    erase_numinput_txt();
    numinput_flag=FIXED_INT_PARM;
    par_from_keyboard_routine=new_user_offs_hz; //offset-fine

    }
  else
    {
    numinput_ypix=ug.ytop+(2.4*text_height)+1;
    numinput_chars=7;
    erase_numinput_txt();
    numinput_flag=FIXED_FLOAT_PARM;
    par_from_keyboard_routine=new_user_offs_khz; //offset-raw
    }  
  }
else
  {
// If we did not select a numeric input by setting numinput_flag
// we have to set mouse_active flag.
// Set the routine to mouse_nothing, we just want to
// set flags when the mouse button is released.    
  current_mouse_activity=mouse_nothing;
  mouse_active_flag=1;
  }
}

void init_users_control_window(void)
{
int i, ir;
// Get and set  offset values by  reading usergraph  file
// 
FILE *Fp;
sprintf(ugfile,"%s_ug",rxpar_filenames[rx_mode]);
Fp=fopen(ugfile,"r");
if (Fp==NULL)                   // if file not found, use default-values 
  {
defaults:;
  ug.xleft=60*text_width;       //default-value for initial x position 
  ug.ytop=(screen_last_line-4)*text_height;  //default-value for initial y position
  for(i=0; i<NO_OF_BANDS; i++)
    {
    ug.offs_hz[i]=0;            //default-value for initial offset-fine
    ug.offs_khz[i]=0;           //default-value for initial offset-raw
    }
  }  
else
  {
  ir=fscanf(Fp,"%d%d%i%i",&ug.xleft,&ug.ytop,&serport_number,&transceiver_number);
  if(ir <= 0)
    {
ugfile_error:;
    fclose(Fp);
    goto defaults;
    }    
  for(i=0; i<NO_OF_BANDS; i++)
    {
    ir=fscanf(Fp,"%d",&ug.offs_hz[i]);
    if(ir <= 0)goto ugfile_error;
    }
  for(i=0; i<NO_OF_BANDS; i++)
    {
    ir=fscanf(Fp,"%d",&ug.offs_khz[i]);
    if(ir <= 0)goto ugfile_error;
    }
  fclose(Fp);
  }
//trace
//printf("\n serport_number %i\n",serport_number);
lir_close_serport();
serport_open_status=lir_open_serport(serport_number , // 1=COM1 or /dev/ttyS0 , 2=COM2....
     list[transceiver_number].serport_baudrate,       // baudrate 110 to 57600
     list[transceiver_number].serport_stopbits);      // 1=two stopbits, 0=one stopbit
set_press_q_key_new_freq_msg();
ug_hsiz=(30+MAX_MSGSIZE)*text_width;                  //  WIDTH OF USERGRAPH
ug_vsiz=(4.8*text_height);                            //  HEIGHT OF USERGRAPH
ug.xright=ug.xleft+ug_hsiz;
ug.ybottom= ug.ytop+ug_vsiz;
if(rx_mode < MODE_TXTEST)
  {
  users_graph_scro=no_of_scro;
  make_users_control_graph();
  no_of_scro++;
  if(no_of_scro >= MAX_SCRO)lirerr(89);
  }
}



void users_init_mode(void)
{
// A switch statement can be used to do different things depending on
// the processing mode.
//switch (rx_mode)
//  {
//  case MODE_WCW:
//  case MODE_HSMS:
//  case MODE_QRSS:
//  break;
//  case MODE_NCW:
//  case MODE_SSB:
// Open a window to allow mouse control of your own things
  init_users_control_window();  
//  break;
//  case MODE_FM:
//  case MODE_AM:
//  case MODE_TXTEST:
//  case MODE_RX_ADTEST:
//  case MODE_TUNE:
//  break;
//  }
}
// *************************************************************
//          END ROUTINES FOR 'MOVABLE' FIXED SIZE USERGRAPH
// *************************************************************

void users_eme(void)
{
}


void userdefined_u(void)
{
}


void edit_ugmsg(char *s, double fq)
{
double t1;
int i, j, k, l,m;
// We are called with a frequency in Hz and already something written
// in the msg string. Find out how many characters we have at our disposal.
j=strlen(s);
i=MAX_MSGSIZE-j-3;
// We will insert separators for each group of 3 digits so we loose i/3 
// positions.
i-=i/3;
// Find out the largest fq we can write in Hz. 
// Write in kHz if fq is above the limit.
t1=i;
t1=pow(10.0,t1)-1;
if(fabs(fq) < t1)
  {
  sprintf (&s[j],"%.0f Hz",fq);
  }
else
  {
  sprintf(&s[j],"%.0f kHz",fq/1000);
  }
// Locate the blank in front of Hz or kHz.  
i=strlen(s);
k=i;
while(s[i] != ' ')i--;
// Insert a separator for every 3 digits.
i--;
j=0;
while(s[i-1] != ' ' && s[i-1] != '-')
  {
  j++;
  if(j==3)
    {
    k++;
    m=k;
    while(m > i)
      {
      s[m]=s[m-1];
      m--;
      }
    s[i]=39;
    j=0;
    }
  i--;
  }
// Align frequency information  to the right
j=MAX_MSGSIZE-1;
i=strlen(s); 
for (l=i; l<=j;l++) s[l]='\x0';
k=j;
while(s[k] != 'z')k--;
if (k!=j-1){
m=j;
while(s[m] != '=')m--;
j--;
for (i=k; i > m; i--) {
   s[j] = s[i];
   s[i] = 32;
   j--;
   }
for (i=j; i > m; i--) {
   s[i] = 32;
   j--;
   }
 }
}


void users_close_devices(void)
{
lir_close_serport();
//trace
//printf("\n serport closed\n");
}

void users_open_devices(void)
{
// The parallel port is for the WSE units.
if(ui.parport == 0)
  {
  allow_parport=FALSE;
  }
else
  {
  if(lir_parport_permission()==TRUE)
    {
    allow_parport=TRUE;
    }
  else  
    {
    allow_parport=FALSE;
    }
  }  

// open selected serial port with the parameters defined in transceiver list

serport_open_status=lir_open_serport(serport_number , // 1=COM1 or /dev/ttyS0 , 2=COM2..,4=COM4 or /dev/ttyUSB0...
     list[transceiver_number].serport_baudrate,       // baudrate 110 to 57600
     list[transceiver_number].serport_stopbits);      // 1=two stopbits, 0=one stopbit
//trace to sceen
//printf("\n serport open:stat%i,nbr%i,baud%i,stopb%i\n",serport_open_status,serport_number,list
//[transceiver_number].serport_baudrate,            
//     list[transceiver_number].serport_stopbits ); 
}


// **************************************************************************
//   ROUTINES FOR: 
//                 ->CHANGING THE SERIAL PORT,
//                 ->CALIBRATION 
//                 ->SETTING TRX  FREQUENCY 
//     These routines are  called when the 'Q' key is pressed
// **************************************************************************
void userdefined_q(void)
{
double transceiver_freq;
double offset_hwfreq;
unsigned long freq;
double transverter_offset_raw;  //raw offset in Khz (for tranverter) 
double transverter_offset_fine; //fine offset in Hz (for transverter)
extern char serport_name[10];
unsigned char a;
int n=-1;
int i,j,k;
char s[80];

void analyze_serport_error (void)
{
  ug_msg_color=12;
  sprintf (ug_msg0,"SERIAL PORT  ERROR :");
  switch (serport_open_status )
 {
  case 1244: 
  sprintf (ug_msg1,"OPEN FAILED (%i)",serport_open_status);
  break;
  case 1277:
  sprintf (ug_msg1,"GET OPTIONS FAILED (%i)",serport_open_status);
  break;
  case 1278:
  sprintf (ug_msg1,"SET OPTIONS FAILED (%i)",serport_open_status);
  break;
  case 1279:
  sprintf (ug_msg1,"ILLEGAL PORT NUMBER (%i)",serport_open_status);
  break;
  case 1280:
  sprintf (ug_msg1,"ILLEGAL BAUDRATE (%i)",serport_open_status);
  break;
  default:
  sprintf (ug_msg1,"UNKNOWN ERR CODE (%i)",serport_open_status);
  } 
}

//select processing according switch values 

//***************************************************************
//                     PROCESS A CHANGE IN SERIAL PORT 
//***************************************************************

if(chnge_serport_switch==1)
{
chnge_serport_switch=0;
sprintf(s," SERIAL PORT:%s ",serport_name);
lir_pixwrite(ug.xleft+.5*text_width,ug.ytop+1.5*text_height,s);
users_close_devices();
users_open_devices(); 
//check status serial port
if ( serport_open_status!=0) 
{
analyze_serport_error ();
}
else
{
ug_msg_color=15;
sprintf (ug_msg0,"SERIAL PORT OPEN OK");
sprintf (ug_msg1,"PRESS Q-KEY TO SET TRX FREQ.");
}
goto userdefined_q_exit_1;
}

//********************************************************************************
//                     PROCESS A CALIBRATION REQUEST
//********************************************************************************


//calculate offset
void calculate_offset(void)
{
transceiver_freq=(double)k;
offset_hwfreq=transceiver_freq-(hwfreq*1000);
transverter_offset_raw=floor(offset_hwfreq/1000);
transverter_offset_fine=ceil(offset_hwfreq-transverter_offset_raw*1000);
ug.offs_khz[ug_band_no]=transverter_offset_raw;
ug.offs_hz[ug_band_no]=transverter_offset_fine;
ug_msg_color=11;
sprintf (ug_msg0,"TRX VFO FREQ:%i",k);
sprintf (ug_msg1,"NEW OFFSET VALUES LOADED");
sprintf (ug_msg2,"PRESS Q-KEY TO SET TRX FREQ.");
}

void fail_message_1(void)
{ug_msg_color=12;
sprintf (ug_msg0,"CALIBRATION FAILED:");
sprintf (ug_msg1,"WRITE TO SER PORT FAILED");
sprintf (ug_msg2,"PRESS Q-KEY TO SET TRX FREQ.");
}

void fail_message_2(void)
{ug_msg_color=12;
sprintf (ug_msg0,"CALIBRATION FAILED:");
sprintf (ug_msg1,"READ FROM SER PORT FAILED");
sprintf (ug_msg2,"PRESS Q-KEY TO SET TRX FREQ.");
}

void fail_message_3(void)
{ug_msg_color=12;
sprintf (ug_msg0,"CALIBRATION FAILED:");
sprintf (ug_msg1,"TRX VFO FREQ = 0");
sprintf (ug_msg2,"PRESS Q-KEY TO SET TRX FREQ.");
}

if(chnge_calibr_switch==1)
{
chnge_calibr_switch=0;

//EXIT IF NO LINRAD-FREQUENCY IS SELECTED
 
if(mix1_selfreq[0] < 0)  
  {
  ug_msg_color=12;
  sprintf (ug_msg0,"NO FREQ. SELECT. ON W/F GRAPH");
  goto userdefined_q_exit_1;
  }

// flush buffers 
users_close_devices();
users_open_devices();

// ************************CAL FT-897 *********************
if ((strcmp(list[transceiver_number].name,"FT-897     ")==0))
{
// command to request the vfo freq
unsigned char getfreq_cmnd[5]= { 0x00, 0x00, 0x00, 0x00, 0x03};
unsigned char ft897_zero[5]= { 0x0000000000};
unsigned char cat_cmnd[5];

int write_cat_cmnd(void)
{
int r;
for (i=0;i<5;i=i+1)
 {
  r=lir_write_serport((char*)&cat_cmnd[i],1);
  if (r<0) return -1;
// printf("\n %#4.2x ",cat_cmnd[i]);  //trace output to screen
  lir_sleep(60000); // wait 60msec between each byte
 }
return 0;
}
//write getfreq_cmd to the serial port
memcpy(cat_cmnd,getfreq_cmnd, 5);
n=write_cat_cmnd();
if (n > -1)
{
//read requested vfo freq
memcpy(s,ft897_zero, 5);
n=lir_read_serport(s,5);
if (n==5)
{
//s holds the vfo frequency encoded as 0x43 0x97 0x00 0x00 0xNN for a frequency of 439.700.000,00 MHz
//last byte, xNN, is the mode (usb,lsb,..) and will be  discarded
//convert now  pos 0 to pos 3 in s, from packed bcd to integer k
k=0;
j=10;
for (i=3; i >-1; i--) {
   a=(s[i]%16 & 0x0f);
   k= k+a*j;
   j=j*10;
   a= ((s[i]>>4)%16) & 0x0f;
   k= k+a*j;
   j=j*10;
   }
if (k==0)
   {
   fail_message_3();
   }
else{
    calculate_offset();
    }
}
else{
    fail_message_2();
    }
}
else{
    fail_message_1();
    }
goto userdefined_q_exit_1;  
}

// ************************CAL FT-1000 ROUTINE*********************
if ((strcmp(list[transceiver_number].name,"FT-1000    ")==0))
{
//command to get frequency of VFO A & B (  returns 2 x 16 bytes )
unsigned char getfreq_cmnd[5]= { 0x00, 0x00, 0x00, 0x03, 0x10};
unsigned char ft1000_zero[16]= { 0x00000000000000000000000000000000}; 
//write getfreq_cmnd to the serial port
for (i=0;i<5;i=i+1)
 {
  n=lir_write_serport((char*)&getfreq_cmnd[i],1);
  if (n<0)  goto cal_ft1000_exit;
  lir_sleep(5000); // wait 5msec between each write
 }
//read  vfo freq 
lir_sleep(60000); // wait 60msec
memcpy(s,ft1000_zero, 16);
n=lir_read_serport(s,32);
if (n==32)
{
//s holds the vfo frequencies of VFO A & B in 32 bytes
//convert now  pos 8 to pos 15 in s, from ascii to integer k
k=0;
j=1;
for (i=15; i > 7 ; i--) {
    k=k+(s[i] & 0x0f)*j;
    j=j*10;
    }
if (k==0)
   {
   fail_message_3();
   }
else{
    calculate_offset();
    }
}
else{
    fail_message_2();
    }
goto userdefined_q_exit_1;
cal_ft1000_exit:;
fail_message_1();
goto userdefined_q_exit_1;
}

// *********************** CAL TS-850/TS-2000 ROUTINE****************
if (((strcmp(list[transceiver_number].name,"TS-850     ")==0))
   |((strcmp(list[transceiver_number].name,"TS-2000    ")==0)))
{
// command to request the vfo freq
sprintf (s,"FA;");
n=lir_write_serport(s,3);
if (n > -1)
{
sprintf (s,"0000000000000;");
//read requested vfo freq
n=lir_read_serport(s,14);
if (n==14)
{
//s holds the vfo frequency encoded as FA00014250000; for the frequency 14.250.000 Hz   
//convert pos 2 to pos 12 of s  to integer k
k=0; 
for (i=2; i<=12;i++)
  {
  j=s[i]-'0';
  k=k*10+j;
  }
if (k==0)
   {
   fail_message_3();
   }
else{
    calculate_offset();
    }
}
else{
    fail_message_2();
    }
}
else{
    fail_message_1();
    }
goto userdefined_q_exit_1;
}

// *******************CAL FT-450/FT-950/FT-2000*********************
if   (((strcmp(list[transceiver_number].name,"FT-450     ")==0))
     |((strcmp(list[transceiver_number].name,"FT-950     ")==0))
     |((strcmp(list[transceiver_number].name,"FT-2000    ")==0)))

{
// command to request the vfo freq
sprintf (s,"FA;");
n=lir_write_serport(s,3);
//traces
//printf("\n %s",s); //trace output to screen
//printf("\n");      //'next line' to display last trace entry
if (n > -1)
{
sprintf (s,"0000000000;");
//read requested vfo freq
n=lir_read_serport(s,11);
//printf("\n s=%s n=%i",s,n); //trace output to screen
//printf("\n");      //'next line' to display last trace entry
if (n==11)
{
//s holds the vfo frequency encoded as FA14250000; for the frequency 14.250.000 Hz   
//convert pos2 to pos 9 of s  to integer k
k=0; 
for (i=2; i<=9;i++)
  {
  j=s[i]-'0';
  k=k*10+j;
  }
if (k==0)
   {
   fail_message_3();
   }
else{
    calculate_offset();
    }
}
else{
    fail_message_2();
    }
}
else{
    fail_message_1();
    }
goto userdefined_q_exit_1;  
}
//***************************** CATCH ALL *******************************

else{
    ug_msg_color=12;
    sprintf (ug_msg0,"NO CALIBRATION AVAILABLE");
    sprintf (ug_msg1,"FOR THIS TRX");
    sprintf (ug_msg2,"PRESS Q-KEY TO SET TRX FREQ.");
    }
goto userdefined_q_exit_1;
}

//***************************************************************************
//                     PROCESS A  "SET VFO " REQUEST
//***************************************************************************


// CHECK STATUS OF SER PORT

if ( serport_open_status!=0)
{
analyze_serport_error ();
goto userdefined_q_exit_1;
}

//INITIALIZE MESSAGE AREA IN USERGRAPH

ug_msg_color=12;
strcpy(ug_msg0,"FREQUENCY SETTING IN PROGRESS");
ug_msg1[0]=0;
ug_msg2[0]=0;
show_user_parms();

//EXIT IF NO LINRAD-FREQUENCY IS SELECTED
 
if(mix1_selfreq[0] < 0)  
  {
  ug_msg_color=12;
  sprintf (ug_msg0,"NO FREQ. SELECT. ON W/F GRAPH");
  goto userdefined_q_exit_1;
  }

// CHECK IF TRX-FREQUENCY WITHIN RANGE AND EXIT IF NOT   

transverter_offset_raw=ug.offs_khz[ug_band_no];    //raw offset in Khz for transverter 
transverter_offset_fine=ug.offs_hz[ug_band_no];    //fine offset in Hz for transverter 
offset_hwfreq= transverter_offset_raw +(transverter_offset_fine*.001) ; //Freq in KHz 
transceiver_freq= (hwfreq+offset_hwfreq)*1000;
transceiver_freq=floor(transceiver_freq);                               //Freq in Hz 
freq= transceiver_freq;

if ((transceiver_freq < list[transceiver_number].min_transceiver_freq )
     ||(transceiver_freq > list[transceiver_number].max_transceiver_freq))
{
  ug_msg_color=12;
  sprintf (ug_msg0,"INVALID FREQ.FOR %s",list[transceiver_number].name); 
  goto userdefined_q_exit_2;
}



// ISSUE  'SET FREQUENCY COMMAND' FOR SELECTED TRANSCEIVER_NAME
//    input: freq or transceiver_freq
//    proces:convert input to 'set frequency command' message string 
//    output:n=lir_write_serport(message string, message_length); to serial port'
//           messages in usergraph 


// ***********************IC-275/IC-706 ROUTINE******************** 
if  (((strcmp(list[transceiver_number].name,"IC-275     ")==0))
    |((strcmp(list[transceiver_number].name,"ICOM-all   ")==0))
    |((strcmp(list[transceiver_number].name,"IC-706     ")==0))
    |((strcmp(list[transceiver_number].name,"IC-706MKII ")==0))
    |((strcmp(list[transceiver_number].name,"IC-706MKIIG")==0)))

{

struct {            // 'Set Frequency command' for IC-275/IC-706
   char prefix[2];
   char radio_addr;
   char computer_addr;
   char cmd_nr;
   unsigned char freq[5];
   char suffix;
   } msg0 = {{0xFE, 0xFE},               // preamble code x 2
             0x10,                       // receive address for IC-275
             0xE0,                       // transmit address of controller
             0x05,                       // 'set freq' cmnd code
             {0x00,0x00,0x00,0x00,0x00}, // freq
             // 145.678.900 Hz  is encoded as x00 x89 x67 x45 x01
             0xFD};                    // end of message code

if  ((strcmp(list[transceiver_number].name,"ICOM-all   ")==0))
   msg0.radio_addr='\x00';   // receive address for any icom-radio broadcasting

if  ((strcmp(list[transceiver_number].name,"IC-706     ")==0))
   msg0.radio_addr='\x48';   // receive address for IC-706

if  ((strcmp(list[transceiver_number].name,"IC-706MKII ")==0))
   msg0.radio_addr='\x4e';   // receive address for IC-706MKII

if  ((strcmp(list[transceiver_number].name,"IC-706MKIIG")==0))
   msg0.radio_addr='\x58';   // receive address for IC-706MKIIG

//convert 10 digits of freq to packed bcd and store in msg0.freq,
//put first digit-pair in FIRST output byte
for (i=0; i < 5; i++) {
   a = freq%10;
   freq /= 10;
   a |= (freq%10)<<4;
   freq /= 10;
   msg0.freq[i]= a;
//printf("\n %#4.2x ",msg0.freq[i]); //trace output to screen
   }
//printf("\n");      //'next line' to display last trace entry
// write 'Set Frequency message' to serial port
n=lir_write_serport((char *) &msg0, sizeof(msg0));
goto display_messages_in_usersgraph;
}

// ***********************TS-850/TS-2000 ROUTINE****************
if (((strcmp(list[transceiver_number].name,"TS-850     ")==0))
   |((strcmp(list[transceiver_number].name,"TS-2000    ")==0)))
{
// FA is the "set VFO-A frequency" command code
// Freq in Hz; 11 digits, leading zeros, no decimal point, trailing semicolon.
sprintf (s,"FA%011.0f;",transceiver_freq);
n=lir_write_serport(s,14);
//printf("\n %s",s); //trace output to screen
//printf("\n");      //'next line' to display last trace entry
goto display_messages_in_usersgraph;
}

// ************************FT-736 ROUTINE**********************

if ((strcmp(list[transceiver_number].name,"FT-736     ")==0))
{
unsigned char open_cmnd[5] =  { 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char close_cmnd[5] = { 0x80, 0x80, 0x80, 0x80, 0x80};
// 145.678.900 HZ is encoded as 0x14 0x56 0x78 0x90 0x01, last byte 0x01 is set freq cmnd code 
unsigned char setfreq_cmnd[5]= { 0x00, 0x00, 0x00, 0x00, 0x01};
unsigned char cat_cmnd[5];

int write_cat_cmnd(void)
{
int r=0;
for (i=0;i<5;i=i+1)
 {
  r=lir_write_serport((char*)&cat_cmnd[i],1);
  if (r<0) return -1;
// printf("\n %#4.2x ",cat_cmnd[i]);  //trace output to screen
  lir_sleep(50000); // wait 50msec between each byte
 }
return 0;
}

//open CAT system
memcpy(cat_cmnd,open_cmnd, 5);
n=write_cat_cmnd();
if (n<0) goto ft736_exit;

//discard least significant digit of freq
//convert next 8 digits of freq to packed bcd and store in setfreq_cmd
//put first digit-pair in LAST  output byte 
freq /= 10;
for (i=0; i < 4; i++) {
   a = freq%10;
   freq /= 10;
   a |= (freq%10)<<4;
   freq /= 10;
   setfreq_cmnd[3-i]= a;
   }

//write setfreq_cmd to the serial port
memcpy(cat_cmnd,setfreq_cmnd, 5);
n=write_cat_cmnd();
if (n<0) goto ft736_exit;

//close CAT system
memcpy(cat_cmnd,close_cmnd, 5);
n=write_cat_cmnd();
ft736_exit:;
//printf("\n");  //'next line' to display last trace entry
goto display_messages_in_usersgraph;
}

// ***************************FT-1000 ROUTINE*********************
if ((strcmp(list[transceiver_number].name,"FT-1000    ")==0))
{
// 014.250.000 Hz is encoded as 0x00 0x50 0x42 0x01 0x0a (hex values )
// last byte 0x0a is set freq cmnd code
unsigned char setfreq_cmnd[5]= { 0x00, 0x00, 0x00, 0x00, 0x0a};

//discard least significant digit of freq
//convert next 8 digits of freq to packed bcd and store in setfreq_cmd,
//put first digit-pair in FIRST output byte 
freq /= 10;
for (i=0; i < 4; i++) {
   a = freq%10;
   freq /= 10;
   a |= (freq%10)<<4;
   freq /= 10;
   setfreq_cmnd[i]= a;
   }
 
//write setfreq_cmnd to the serial port
for (i=0;i<5;i=i+1)
 {
  n=lir_write_serport((char*)&setfreq_cmnd[i],1);
  if (n<0)  goto ft1000_exit;
//printf("\n %#4.2x ",setfreq_cmnd[i]);  //trace output to screen
  lir_sleep(5000); // wait 5msec between each write
 }
ft1000_exit:;
//printf("\n");  //'next line' to display last trace entry
goto display_messages_in_usersgraph;
}

// *********************FT-450/FT-950/FT-2000*********************
if   (((strcmp(list[transceiver_number].name,"FT-450     ")==0))
     |((strcmp(list[transceiver_number].name,"FT-950     ")==0))
     |((strcmp(list[transceiver_number].name,"FT-2000    ")==0)))

{
// 14.250.000 Hz is encoded as  FA14250000; (ASCII values)
// "FA" is the "SET VFO A "command code
// Freq in Hz; 8 digits, leading zeros, no decimal point,
//  ";" is a delimiter

sprintf (s,"FA%08.0f;",transceiver_freq);
n=lir_write_serport(s,11);
//printf("\n %s",s); //trace output to screen
//printf("\n");      //'next line' to display last trace entry
goto display_messages_in_usersgraph;
}

// ***************************FT-897 ROUTINE*********************
if ((strcmp(list[transceiver_number].name,"FT-897     ")==0))
 
{
// 439.70 MHz is encoded as  0x43 0x97 0x00 0x00 0x01 (hex values)
// last byte 0x01 is the "SET Frequency" opcode. 
unsigned char setfreq_cmnd[5]= { 0x00, 0x00, 0x00, 0x00, 0x01};
unsigned char cat_cmnd[5];

int write_cat_cmnd(void)
{
int r;
for (i=0;i<5;i=i+1)
 {
  r=lir_write_serport((char*)&cat_cmnd[i],1);
  if (r<0) return -1;
// printf("\n %#4.2x ",cat_cmnd[i]);  //trace output to screen
  lir_sleep(60000); // wait 60msec between each byte
 }
return 0;
}
//discard least significant digit of freq
//convert next 8 digits of freq to packed bcd and store in setfreq_cmd
//put first digit-pair in LAST  output byte 
freq /= 10;
for (i=0; i < 4; i++) {
   a = freq%10;
   freq /= 10;
   a |= (freq%10)<<4;
   freq /= 10;
   setfreq_cmnd[3-i]= a;
   }

//write setfreq_cmd to the serial port
memcpy(cat_cmnd,setfreq_cmnd, 5);
n=write_cat_cmnd();
//printf("\n");  //'next line' to display last trace entry
goto display_messages_in_usersgraph;
}
// ************************CATCH_ALL**********************
ug_msg_color=12;
sprintf (ug_msg0,"INTERNAL ERROR ");
goto userdefined_q_exit_1;
// *************END 'SET FREQUENCY COMMAND' ROUTINES ********

//DISPLAY MESSAGES IN USERGRAPH

display_messages_in_usersgraph:;
if (n<0)
  {
  ug_msg_color=12;
  sprintf (ug_msg0,"WRITE TO %s FAILED",serport_name);
  goto userdefined_q_exit_1;
  }

ug_msg_color=14;
sprintf (ug_msg0,"LINRAD FREQ = ");
edit_ugmsg(ug_msg0,floor(hwfreq*1000));
userdefined_q_exit_2:;
sprintf (ug_msg1,"TOTAL OFFSET= ");
edit_ugmsg(ug_msg1,offset_hwfreq*1000);
sprintf (ug_msg2,"TRX FREQ= ");
edit_ugmsg(ug_msg2, transceiver_freq);
//main  exit
userdefined_q_exit_1:;
show_user_parms();
}

// **************************************************************************
// ****        END ROUTINES FOR PROCESSING Q-KEY REQUESTS                 ***
// **************************************************************************

void update_users_rx_frequency(void)
{
// This routine is called from the screen thread.
if(fabs(hwfreq-old_hwfreq) > 0.001)
  {
  ug_msg_color=10;
  sprintf (ug_msg0,"LINRAD FREQ= ");
  edit_ugmsg(ug_msg0,floor(hwfreq*1000));
  hide_mouse(ug.xleft, ug.xright,ug.ytop,ug.ybottom);
  sprintf (ug_msg1,"PRESS Q-KEY TO SET TRX FREQ");
  sprintf (ug_msg2," ");
  show_user_parms();
  settextcolor(7);
  }
}  
// **********************************************************
//         WSE Rx and Tx and SDR-14/SDR-IQ Rx routines
// **********************************************************
#if (RX_HARDWARE == 0)
#include "wse_sdrxx.c"
#endif
// **********************************************************
// Dummy routines to support converters and other hardwares
// **********************************************************
#if (RX_HARDWARE == 1)
void control_hware(void){};
void set_hardware_tx_frequency(void){};
void set_hardware_rx_frequency(void){};
void set_hardware_rx_gain(void){};
void hware_interface_test(void){};
void hware_set_rxtx(int state)
  {
  int i;
  i=state;
  };
void hware_hand_key(void){};
void clear_hware_data(void){};
#endif


