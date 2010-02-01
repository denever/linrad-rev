#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pthread.h>
#include <semaphore.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>
#include <globdef.h>
#include <thrdef.h>
#include <xdef.h>
#include <ldef.h>

#ifdef HAVE_SHM
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
XShmSegmentInfo *shminfo;
#endif
int X11_accesstype;

pthread_t thread_identifier_process_event;
unsigned char *mempix_char;
unsigned short int *mempix_shi;
int first_mempix;
int last_mempix;
int process_event_flag;
int expose_event_done;
int shift_key_status;
int color_depth;
unsigned char *xpalette;


GC xgc;
XImage *ximage;
Display *xdis;
Window xwin;
Colormap lir_colormap;

