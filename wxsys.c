
#include "globdef.h"
#include "uidef.h"
#include "thrdef.h"
#include "keyboard_def.h"

void wxmouse(void)
{
mouse_thread_flag=THRFLAG_ACTIVE;
while(mouse_thread_flag==THRFLAG_ACTIVE)
  {
// **********************************************
// Wait until a mouse event occurs. 
  lir_sem_wait(SEM_MOUSE);
  if( thread_status_flag[THREAD_SCREEN] == THRFLAG_SEM_WAIT ||
      thread_status_flag[THREAD_SCREEN] == THRFLAG_ACTIVE ||
      thread_status_flag[THREAD_SCREEN] == THRFLAG_IDLE) 
    {
    lir_move_mouse_cursor();
    check_mouse_actions();
    }
  }
mouse_thread_flag=THRFLAG_RETURNED;  
}

void lir_remove_mouse_thread(void)
{
if(mouse_thread_flag == THRFLAG_NOT_ACTIVE)return;
mouse_thread_flag=THRFLAG_KILL;
while(mouse_thread_flag != THRFLAG_RETURNED)
  {
  lir_sem_post(SEM_MOUSE);
  lir_sleep(1000);
  }
}

void store_in_kbdbuf(int c)
{
if(c==13)c=10;
keyboard_buffer[keyboard_buffer_ptr]=c;
keyboard_buffer_ptr=(keyboard_buffer_ptr+1)&(KEYBOARD_BUFFER_SIZE-1);
lir_sem_post(SEM_KEYBOARD);
}

// *********************************************************
// Mouse routines for Windows and X11. Draw nothing on screen.
// *********************************************************

void unconditional_hide_mouse(void)
{
// Do nothing, Windows or X11 take care of the mouse cursor
}

void show_mouse(void)
{
// Do nothing, Windows or X11 take care of the mouse cursor
}


void hide_mouse(int x1, int x2, int iy1, int y2)
{
int i;
i=x1|x2|iy1|y2;
// Do nothing, Windows or X11 take care of the mouse cursor
}

void lir_move_mouse_cursor(void)
{
set_button_coordinates();
// We do not have a cursor of our own. Do nothing.
}

