

#include <stdio.h>
#include "wdef.h"
#include "globdef.h"
#include "uidef.h"
#include "screendef.h"
#include "vernr.h"
#include "options.h"
#include "keyboard_def.h"
#include "thrdef.h"

int mbutton_state;
int newcomer_escflag;


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow)
{
int i, j, ws, hs;
static char szAppName[] = PROGRAM_NAME;
MSG msg;
WNDCLASSEX wndclass;
LARGE_INTEGER lrgi;
LPBITMAPINFO lpbi;
LPBYTE m_pBits;
char *phony;
HINSTANCE h_inpout;
DWORD procmask, sysmask;
if(DUMPFILE)
  {
  dmp = fopen("dmp", "wc");
  DEB"\n******************************\n");
  }
else
  {
  dmp=NULL;
  }
first_mempix=0x7fffffff;
last_mempix=0;
phony=(void*)hPrevInstance;
phony=(void*)szCmdLine;
os_flag=OS_FLAG_WINDOWS;
// Set up things we need to evaluate CPU load.
CurrentProcess=GetCurrentProcess();
j=GetProcessAffinityMask(CurrentProcess, &procmask, &sysmask);
if(j==0)
  {
  no_of_processors=1;
  }
else
  {
  no_of_processors=0;
  for(i=0; i<32; i++)
    {
    no_of_processors+=procmask&1;
    procmask/=2;
    }
  }
ui_setup();
newcomer_escflag=FALSE;
switch(ui.process_priority)
  {
  case 1:
  SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
  break;
  
  case 2:
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
  break;
  
  case 3:
  SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
  break;
  }
serport=INVALID_HANDLE_VALUE;
init_os_independent_globals();
keyboard_buffer_ptr=0;
keyboard_buffer_used=0;
keyboard_buffer=malloc(KEYBOARD_BUFFER_SIZE*sizeof(int));
if(GetWindowRect(GetDesktopWindow(), &desktop_screen) == 0)
  {
  printf("\nGetWindowRect(...) failed");
  exit(0);
  }
wndclass.cbSize        = sizeof (wndclass);
wndclass.style         = CS_HREDRAW | CS_VREDRAW;
wndclass.lpfnWndProc   = WndProc;
wndclass.cbClsExtra    = 0;
wndclass.cbWndExtra    = 0;
wndclass.hInstance     = hInstance;
wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
wndclass.hCursor       = LoadCursor (NULL, IDC_CROSS);
wndclass.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);
wndclass.lpszMenuName  = NULL;
wndclass.lpszClassName = szAppName;
wndclass.hIconSm       = LoadIcon (NULL, IDI_APPLICATION);
RegisterClassEx (&wndclass);
ws=desktop_screen.right-desktop_screen.left;
screen_width=(ui.screen_width_factor*ws/100)&-4;
hs=desktop_screen.bottom-desktop_screen.top;
screen_height=ui.screen_height_factor*hs/100;
screen_totpix=screen_width*(screen_height+1);
// Make the size a bit too large. The proper width and height
// to set here is true size plus frame widths.
linrad_hwnd = CreateWindow (szAppName, PROGRAM_NAME,
                     WS_OVERLAPPEDWINDOW,
                     0,0,
                     screen_width+0.05*ws,
                     screen_height+0.05*hs,
                     NULL, NULL, hInstance, NULL);
if(linrad_hwnd == NULL)
  {
  printf("\nCreateWindow(...) failed");
  exit(0);
  }
ShowWindow (linrad_hwnd, iCmdShow);
UpdateWindow (linrad_hwnd);
screen_hdc=GetDC(linrad_hwnd);
wave_in_open_flag1=0;
wave_in_open_flag2=0;
// ****************************************************************
// Try to install inpout32.dll to try to get access to the
// parallel port for hardware control.
h_inpout = LoadLibrary("inpout32.dll");
if(h_inpout == NULL)
  {
  parport_installed=FALSE;
  }
else
  {
  parport_installed=TRUE;
// get the addresses of the functions
  inp32 = (inpfuncPtr) GetProcAddress(h_inpout, "Inp32");
  if(inp32 == NULL)
    {
    lirerr(1197);
    goto wmain_error;
    }
  oup32 = (oupfuncPtr) GetProcAddress(h_inpout, "Out32");
  if (oup32 == NULL)
    {
    lirerr(1197);
    goto wmain_error;
    }
  }
// *****************************************************************
// Set the variables Linrad uses to access the screen.
init_font(ui.font_scale);
if(lir_errcod != 0)goto wmain_error;
// ******************************************************************
// Rearrange the palette. It was designed for svgalib under Linux
for(i=0;i<3*256;i++)
  {
  svga_palette[i]<<=2;
  }
for(i=0;i<3*256;i+=3)
  {
  j=svga_palette[i];
  svga_palette[i]=svga_palette[i+2];
  svga_palette[i+2]=j;
  }
// ***************************************************************
// Create a BITMAP in our own memory space. The SetPixel function
// of WinAPI os hopelessly slow.
lpbi = malloc(sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
lpbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
lpbi->bmiHeader.biWidth = screen_width;
lpbi->bmiHeader.biHeight = screen_height+1;
lpbi->bmiHeader.biPlanes = 1;
lpbi->bmiHeader.biBitCount = 8;
lpbi->bmiHeader.biCompression = BI_RGB;
lpbi->bmiHeader.biSizeImage = 0;
lpbi->bmiHeader.biXPelsPerMeter = 0;
lpbi->bmiHeader.biYPelsPerMeter = 0;
lpbi->bmiHeader.biClrUsed = 0;
lpbi->bmiHeader.biClrImportant = 0;
for (i=0; i<255; i++)
  {
  lpbi->bmiColors[i].rgbRed=svga_palette[3*i];
  lpbi->bmiColors[i].rgbGreen=svga_palette[3*i+1];
  lpbi->bmiColors[i].rgbBlue=svga_palette[3*i+2];
  lpbi->bmiColors[i].rgbReserved=0;
  }
memory_hbm= CreateDIBSection(screen_hdc, lpbi, DIB_RGB_COLORS,
                                (LPVOID *)&m_pBits, NULL, 0 );
free(lpbi);
// ****************************************************************
// Create a device context for the memory bitmap.
memory_hdc=CreateCompatibleDC(screen_hdc);
SelectObject(memory_hdc, memory_hbm);
// ***************************************************************
// Initiate our timer
if(QueryPerformanceFrequency(&lrgi)==0)lirerr(1215);
internal_clock_frequency=
     (double)(0x10000)*(double)(0x10000)*lrgi.HighPart+lrgi.LowPart;
if(QueryPerformanceCounter(&lrgi)==0)lirerr(1216);
internal_clock_offset=lrgi.HighPart;
mouse_cursize=0;
mbutton_state=0;
sem_kill_all=CreateSemaphore( NULL, 0, 1024, NULL);
rxin1_bufready=NULL;
thread_handle_kill_all = CreateThread(NULL, 0, winthread_kill_all,
                                      (PVOID)phony,0, &thread_id_kill_all);
if(thread_handle_kill_all == NULL)
  {
  lirerr(1201);
  goto wmain_error;
  }
expose_time=current_time();  
i=check_mmx();
mmx_present=i&1;
if(mmx_present != 0)simd_present=i/2; else simd_present=0;
GetObject(memory_hbm,sizeof(memory_bm),&memory_bm);
if(memory_bm.bmType != 0)lirerr(10003);
mempix=memory_bm.bmBits;
lir_status=LIR_OK;
// Create a thread for the main menu.
lir_sem_init(SEM_KEYBOARD);
users_open_devices();
if(kill_all_flag) goto wmain_error;
thread_handle_main_menu = CreateThread(NULL, 0, winthread_main_menu,
                                      (PVOID)phony,0, &thread_id_main_menu);
if(thread_handle_main_menu == NULL)lirerr(1201);
lir_sem_init(SEM_MOUSE);
thread_handle_mouse = CreateThread(NULL, 0, winthread_mouse,
                                     (PVOID)phony,0, &thread_id_mouse);
if(thread_handle_mouse == NULL)lirerr(1201);
while (GetMessage (&msg, NULL, 0, 0))
  {
  TranslateMessage (&msg);
  DispatchMessage (&msg);
  }
if(!kill_all_flag)kill_all();
lir_remove_mouse_thread();
WaitForSingleObject(thread_handle_mouse,INFINITE);
wmain_error:;
users_close_devices();
lir_close_serport();
ReleaseDC (linrad_hwnd, screen_hdc);
if(dmp!=NULL)fclose(dmp);
if(parport_installed)
  {
  FreeLibrary(h_inpout);
  }
return lir_errcod;
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
double dt1;
int cc;
long old;
HDC hdc;
PAINTSTRUCT  ps;
int keydown_type;
int k, m;
int mx, my;
switch (iMsg)
  {
  case WM_CREATE:
// We could initiate things here rather than in the main program (???).
  return 0;

  case WM_SIZE:
  first_mempix=0;
  last_mempix=screen_totpix-1;
  break;

  case WM_PAINT:
  expose_time=recent_time;
  hdc=BeginPaint (linrad_hwnd, &ps);
  if( thread_status_flag[THREAD_SCREEN] != THRFLAG_SEM_WAIT &&
      thread_status_flag[THREAD_SCREEN] != THRFLAG_ACTIVE &&
      thread_status_flag[THREAD_SCREEN] != THRFLAG_IDLE)
      {
      BitBlt(hdc, 0, 0, screen_width, screen_height, memory_hdc, 0, 0, SRCCOPY);
      }
  EndPaint (linrad_hwnd, &ps);
  return 0;

  case WM_CHAR:
  if(newcomer_escflag)
    {
    cc=toupper(wParam);
    if(cc=='Y')goto escexit;
    if(cc!='N')break;
    newcomer_escpress(1);
    newcomer_escflag=FALSE;
    break;
    }
  if( wParam==27 && ui.newcomer_mode != 0 && !kill_all_flag)
    {
    newcomer_escpress(0);
    newcomer_escflag=TRUE;
    break;
    }
  if( wParam==27 || kill_all_flag)
    {
escexit:;    
    ReleaseSemaphore(sem_kill_all,1,&old);
    ReleaseSemaphore(rxin1_bufready,1,&old);
    while(!kill_all_flag) lir_sleep(3000);
    ReleaseSemaphore(rxin1_bufready,1,&old);
    store_in_kbdbuf(0);
    dt1=current_time();
    while(threads_running && current_time()-dt1 < 1) lir_sleep(3000);
    if( current_time()-dt1  < 0.99)
      {
      WaitForSingleObject(&thread_id_kill_all,INFINITE);
      }
    PostQuitMessage(0);
    return 0;
    }
  store_in_kbdbuf(wParam);
  break;

  case WM_KEYUP:
  keydown_type=(lParam>>31)&1;
  if(keydown_type == 1 && wParam==FUNCTION_SHIFT)shift_key_status=0;
  break;

  case WM_KEYDOWN:
  keydown_type=lParam>>24;
  cc=0;
  if((keydown_type&1)!=0)
    {
    switch(wParam)
      {
      case NUMBER_HOME:
      cc=HOME_KEY;
      break;

      case NUMBER_UP:
      cc=ARROW_UP_KEY;
      break;

      case NUMBER_PGUP:
      cc=PAGE_UP_KEY;
      break;

      case NUMBER_LEFT:
      cc=ARROW_LEFT_KEY;
      break;

      case NUMBER_RIGHT:
      cc=ARROW_RIGHT_KEY;
      break;

      case NUMBER_END:
      cc=END_KEY;
      break;

      case NUMBER_DOWN:
      cc=ARROW_DOWN_KEY;
      break;

      case NUMBER_PGDN:
      cc=PAGE_DOWN_KEY;
      break;

      case NUMBER_INSERT:
      cc=INSERT_KEY;
      break;

      case NUMBER_DELETE:
      cc=DELETE_KEY;
      break;

      }
    }
  else
    {
    if((keydown_type&32)==0)
      {
      if(wParam == FUNCTION_SHIFT)
        {
        shift_key_status=1;
        return 0;
        }
      if(wParam >=96 && wParam <=105)return 0;
      switch(wParam)
        {
        case NUMERIC_0:
        cc='0';
        break;

        case NUMERIC_1:
        cc='1';
        break;

        case NUMERIC_2:
        cc='2';
        break;

        case NUMERIC_3:
        cc='3';
        break;

        case NUMERIC_4:
        cc='4';
        break;

        case NUMERIC_5:
        cc='5';
        break;

        case NUMERIC_6:
        cc='6';
        break;

        case NUMERIC_7:
        cc='7';
        break;

        case NUMERIC_8:
        cc='8';
        break;

        case NUMERIC_9:
        cc='9';
        break;
        }
      if(shift_key_status == 0)
        {
        switch(wParam)
          {
          case FUNCTION_F1:
          cc=F1_KEY;
          break;

          case FUNCTION_F2:
          cc=F2_KEY;
          break;

          case FUNCTION_F3:
          cc=F3_KEY;
          break;

          case FUNCTION_F4:
          cc=F4_KEY;
          break;

          case FUNCTION_F5:
          cc=F5_KEY;
          break;

          case FUNCTION_F6:
          cc=F6_KEY;
          break;

          case FUNCTION_F7:
          cc=F7_KEY;
          break;

          case FUNCTION_F8:
          cc=F8_KEY;
          break;

          case FUNCTION_F9:
          cc=F9_KEY;
          break;

          case FUNCTION_F11:
          cc=F11_KEY;
          break;

          case FUNCTION_F12:
          cc=F12_KEY;
          break;
          }
        }
      else
        {
        switch(wParam)
          {
          case FUNCTION_F1:
          cc=SHIFT_F1_KEY;
          break;

          case FUNCTION_F2:
          cc=SHIFT_F2_KEY;
          break;

          case FUNCTION_F3:
          cc=SHIFT_F3_KEY;
          break;

          case FUNCTION_F4:
          cc=SHIFT_F4_KEY;
          break;

          case FUNCTION_F5:
          cc=SHIFT_F5_KEY;
          break;

          case FUNCTION_F6:
          cc=SHIFT_F6_KEY;
          break;

          case FUNCTION_F7:
          cc=SHIFT_F7_KEY;
          break;

          case FUNCTION_F8:
          cc=SHIFT_F8_KEY;
          break;
          }
        }
      }
    }
  if(cc != 0)store_in_kbdbuf(cc);
  break;

  case WM_MOUSEWHEEL:
  k=GET_WHEEL_DELTA_WPARAM(wParam);
  if(mbutton_state==0)
    {
    if(k<0)
      {
      step_rx_frequency(1);
      }
    else
      {
      step_rx_frequency(-1);
      }
    }
  else
    {
    m=bg.wheel_stepn;
    if (k<0)
      {
      m++;
      if(m>30)m=30;
      }
    else
      {
      m=bg.wheel_stepn;
      m--;
      if(m<-30)m=-30;
      if(genparm[AFC_ENABLE]==0 && m<0)m=0;
      }
    bg.wheel_stepn=m;
    sc[SC_SHOW_WHEEL]++;
    make_modepar_file(GRAPHTYPE_BG);
    }
  break;

  case WM_MBUTTONDOWN:
  mbutton_state=1;
  break;

  case WM_MBUTTONUP:
  mbutton_state=0;
  break;

  case WM_LBUTTONDOWN:
  new_lbutton_state=1;
  goto mousepost; 

  case WM_LBUTTONUP:
  new_lbutton_state=0;
  goto mousepost;

  case WM_RBUTTONDOWN:
  new_rbutton_state=1;
  goto mousepost;

  case WM_RBUTTONUP:
  new_rbutton_state=0;
  goto mousepost;

  case WM_MOUSEMOVE:
  mx=new_mouse_x;
  my=new_mouse_y;
  new_mouse_x= LOWORD (lParam);
  new_mouse_y= HIWORD (lParam);
  if(new_mouse_x >= screen_width)new_mouse_x=screen_width-1;
  if(new_mouse_y >= screen_height)new_mouse_y=screen_height-1;
  if(new_mouse_x < 0)new_mouse_x=0;
  if(new_mouse_y < 0)new_mouse_y=0;
  if(  mx == new_mouse_x &&   my==new_mouse_y)break;
  if( (mx == new_mouse_x && new_mouse_x == screen_width-1) || 
      (my == new_mouse_y && new_mouse_y == screen_height-1) ||
      (mx == new_mouse_x && new_mouse_x == 0) || 
      (my == new_mouse_y && new_mouse_y == 0))break;
mousepost:;
  lir_sem_post(SEM_MOUSE);
  break;

  case WM_DESTROY:
  PostQuitMessage(0);
  return 0;
  }
return DefWindowProc (hwnd, iMsg, wParam, lParam);
}

void ui_setup(void)
{
FILE *file;
int i, j, k;
char s[10];
char chr;
int *uiparm;
char *parinfo;
uiparm=(int*)(&ui);
parinfo=NULL;
file = fopen(userint_filename, "rb");
if (file == NULL)
  {
  printf("\n\nSetup file %s missing.",userint_filename);
full_setup:;
  for(i=0; i<MAX_UIPARM; i++) uiparm[i]=0;
  printf("\nUse W to create a new %s file after setup.\n\n",userint_filename);
  printf(
    "Press S for setup routines in normal mode or press N for NEWCOMER mode");
  printf("\nAny other key to exit. (You might want to manually edit %s)",
                                                       userint_filename);
  printf("\nThen press enter\n\n=>");
  fgets(s,8,stdin);
  chr=toupper(s[0]);
  if(chr != 'S' && chr != 'N') exit(0);
  if(chr == 'N')
    {
    ui.newcomer_mode=1;
    }
  else
    {
    ui.newcomer_mode=0;
    }
  win_global_uiparms(0);
  if(kill_all_flag) exit(0);
  uiparm_change_flag=TRUE;
  ui.rx_input_mode=-1;
  ui.tx_dadev_no=-1;
  ui.rx_dadev_no=-1;
  }
else
  {
  if(parinfo==NULL)
    {
    parinfo=malloc(4096);
    if(parinfo==NULL)
      {
      lirerr(1078);
      return;
      }
    }
  i=fread(parinfo,1,4095,file);
  fclose(file);
  file=NULL;
  if(i >= 4095)
    {
    goto go_full_setup;
    }
  k=0;
  for(i=0; i<MAX_UIPARM; i++)
    {
    while(parinfo[k]==' ' ||
          parinfo[k]== '\n' )k++;
    j=0;
    while(parinfo[k]== uiparm_text[i][j])
      {
      k++;
      j++;
      } 
    if(uiparm_text[i][j] != 0)goto go_full_setup;
    while(parinfo[k]!='[')k++;
    sscanf(&parinfo[k],"[%d]",&uiparm[i]);
    while(parinfo[k]!='\n')k++;
    }
  if( ui.font_scale < 1 || 
      ui.font_scale >5 ||
      ui.process_priority < 0 || 
      ui.process_priority > 3 ||
      ui.check != UI_VERNR ||
      ui.newcomer_mode < 0 ||
      ui.newcomer_mode > 1)
    {
go_full_setup:;    
    printf("\n\nSetup file %s has errors",userint_filename);
    goto full_setup;
    }
  uiparm_change_flag=FALSE;
  }
free(parinfo);
}

