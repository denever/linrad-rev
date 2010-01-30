#include <ctype.h>
#include <globdef.h>
#include <uidef.h>
#include <hwaredef.h>
#include <fft1def.h>
#include <screendef.h>
#include <seldef.h>
#include <sdrdef.h>
#include <thrdef.h>
#include <txdef.h>

int hware_flag;
int fg_new_band;
int fg_old_band;

char hware_error_flag;
char rx_hware_init_flag;
double hware_time;
int parport_status;
int parport_control;
int parport_ack;
int parport_ack_sign;
int sdr14_att_counter;
int sdr14_nco_counter;
int perseus_att_counter;
int perseus_nco_counter;

#include <semaphore.h>
#include <ldef.h>
extern int serport;
// #include "users_hwaredriver.c"

void mouse_on_users_graph(void){}
void init_users_control_window(void){}
void users_init_mode(void){}
void users_eme(void){}
void userdefined_u(void){}
void userdefined_q(void){}
void users_set_band_no(void){}
void update_users_rx_frequency(void){};

// **********************************************************
//                  open and close
// **********************************************************

void users_close_devices(void)
{
// WSE units do not use the serial port.
// and there is nothing to close.
}

void users_open_devices(void)
{
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
}

#include "wse_sdrxx.c"
