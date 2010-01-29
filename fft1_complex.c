
#include "globdef.h"
#include "uidef.h"
#include "fft1def.h"


void fft1_complex_one(void)
{
int j, m, nn;
int p0, pa, pb;
int ia,ib,ic,id;
float t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
float x1,x2;
int ja, jb, jc, jd;
float a1,a2;
float b1,b2;
float c1,c2;
float d1,d2;
float *x;
x=&fft1_float[fft1_pa];
nn=fft1_size/2;
// If permute=1, select one of the DIF routines
if( fft_cntrl[FFT1_CURMODE].permute == 1)
  {
  if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    m=timf1_bytemask/sizeof(short int);
    p0=timf1p_px/sizeof(short int);
    p0=(p0-fft1_interleave_points*2+timf1_bytes)&m;
    pa=p0;
    pb=(pa+fft1_size)&m;
    ib=fft1_size-2;
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      for( ia=0; ia<nn; ia++)
        {
        ib+=2;
        t1=timf1_short_int[pa  ]*fft1_window[2*ia];
        t2=timf1_short_int[pa+1]*fft1_window[2*ia];      
        t3=timf1_short_int[pb  ]*fft1_window[2*ia+1];
        t4=timf1_short_int[pb+1]*fft1_window[2*ia+1];   
        x1=t1-t3;
        fftw_tmp[2*ia  ]=  t1+t3;
        x2=t4-t2;
        fftw_tmp[2*ia+1]=-(t2+t4);
        pa=(pa+2)&m;
        pb=(pb+2)&m;
        fftw_tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        fftw_tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    else
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_short_int[pa  ];
        ib+=2;
        t2=timf1_short_int[pa+1];      
        t3=timf1_short_int[pb  ];
        t4=timf1_short_int[pb+1];   
        x1=t1-t3;
        x2=t4-t2;
        fftw_tmp[2*ia  ]=  t1+t3;
        fftw_tmp[2*ia+1]=-(t2+t4);
        fftw_tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        fftw_tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        pa=(pa+2)&m;
        pb=(pb+2)&m;
        }
      }
    }  
  else
    {
    m=timf1_bytemask/sizeof(int);
    p0=timf1p_px/sizeof(int);
    p0=(p0-fft1_interleave_points*2+timf1_bytes)&m;
    pa=p0;
    pb=(pa+fft1_size)&m;
    ib=fft1_size-2;
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      for( ia=0; ia<nn; ia++)
        {
        ib+=2;
        t1=timf1_int[pa  ]*fft1_window[2*ia];
        t2=timf1_int[pa+1]*fft1_window[2*ia];      
        t3=timf1_int[pb  ]*fft1_window[2*ia+1];
        t4=timf1_int[pb+1]*fft1_window[2*ia+1];   
        x1=t1-t3;
        fftw_tmp[2*ia  ]=  t1+t3;
        x2=t4-t2;
        fftw_tmp[2*ia+1]=-(t2+t4);
        pa=(pa+2)&m;
        pb=(pb+2)&m;
        fftw_tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        fftw_tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    else
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_int[pa  ];
        ib+=2;
        t2=timf1_int[pa+1];      
        t3=timf1_int[pb  ];
        t4=timf1_int[pb+1];   
        x1=t1-t3;
        x2=t4-t2;
        fftw_tmp[2*ia  ]=  t1+t3;
        fftw_tmp[2*ia+1]=-(t2+t4);
        fftw_tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        fftw_tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        pa=(pa+2)&m;
        pb=(pb+2)&m;
        }
      }
    }
  if(FFT1_CURMODE == 7)
    {
    bulk_of_dif(fft1_size, fft1_n, fftw_tmp, fft1tab, yieldflag_wdsp_fft1);
    }
  else
    {
    asmbulk_of_dif(fft1_size, fft1_n, fftw_tmp, fft1tab, yieldflag_wdsp_fft1);
    }
  nn=fft1_size;
  for(ia=0; ia < nn; ia+=2)
    {
    ib=2*fft1_permute[ia];                               
    ic=2*fft1_permute[ia+1];                             
    x[ib  ]=fftw_tmp[2*ia  ]+fftw_tmp[2*ia+2];
    x[ic  ]=fftw_tmp[2*ia  ]-fftw_tmp[2*ia+2];
    x[ib+1]=fftw_tmp[2*ia+1]+fftw_tmp[2*ia+3];
    x[ic+1]=fftw_tmp[2*ia+1]-fftw_tmp[2*ia+3];
    }
  }
else
  {
  nn=fft1_size;
  if( (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    m=timf1_bytemask/sizeof(short int);
    p0=timf1p_px/sizeof(short int);
    p0=(p0-fft1_interleave_points*2+timf1_bytes)&m;
    pa=p0;
    pb=(pa+fft1_size)&m;
    ib=fft1_size-2;
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      for(j=0; j<nn; j+=4)
        {
        ja=fft1_permute[j  ];
        jb=fft1_permute[j+1];
        jc=fft1_permute[j+2];
        jd=fft1_permute[j+3];
        ia=(p0+2*ja)&m;
        ib=(p0+2*jb)&m;
        ic=(p0+2*jc)&m;
        id=(p0+2*jd)&m;
        a1=fft1_window[ja]*timf1_short_int[ia  ];
        a2=fft1_window[ja]*timf1_short_int[ia+1];

        b1=fft1_window[jb]*timf1_short_int[ib  ];
        b2=fft1_window[jb]*timf1_short_int[ib+1];
    
        c1=fft1_window[jc]*timf1_short_int[ic  ];
        c2=fft1_window[jc]*timf1_short_int[ic+1];
    
        d1=fft1_window[jd]*timf1_short_int[id  ];
        d2=fft1_window[jd]*timf1_short_int[id+1];
        t1=a1+b1;
        t2=a2+b2;
      
        t3=c1+d1;
        t4=c2+d2;
      
        t5=a1-b1;
        t7=a2-b2;
      
        t10=c1-d1;
        t6= c2-d2;
    
        x[2*j  ]=t1+t3;
        x[2*j+1]=t2+t4;
      
        x[2*j+4]=t1-t3;
        x[2*j+5]=t2-t4;
      
        t11=t5-t6;
        t8=t7-t10;
        
        t12=t5+t6;
        t9=t7+t10;
      
        x[2*j+2]=t12;
        x[2*j+3]=t8;
      
        x[2*j+6]=t11;
        x[2*j+7]=t9;
        }
      }
    else
      {
      for(j=0; j<nn; j+=4)
        {
        ia=(p0+2*fft1_permute[j  ])&m;
        ib=(p0+2*fft1_permute[j+1])&m;
        ic=(p0+2*fft1_permute[j+2])&m;
        id=(p0+2*fft1_permute[j+3])&m;

        t1=(float)(timf1_short_int[ia  ])+(float)(timf1_short_int[ib  ]);
        t2=(float)(timf1_short_int[ia+1])+(float)(timf1_short_int[ib+1]);

        t3=(float)(timf1_short_int[ic  ])+(float)(timf1_short_int[id  ]);
        t4=(float)(timf1_short_int[ic+1])+(float)(timf1_short_int[id+1]);
  
        t5=(float)(timf1_short_int[ia  ])-(float)(timf1_short_int[ib  ]);
        t7=(float)(timf1_short_int[ia+1])-(float)(timf1_short_int[ib+1]);
  
        t10=(float)(timf1_short_int[ic  ])-(float)(timf1_short_int[id  ]);
        t6= (float)(timf1_short_int[ic+1])-(float)(timf1_short_int[id+1]);
  
        x[2*j  ]=t1+t3;
        x[2*j+1]=t2+t4;
  
        x[2*j+4]=t1-t3;
        x[2*j+5]=t2-t4;
  
        t11=t5-t6;
        t8=t7-t10;
    
        t12=t5+t6;
        t9=t7+t10;
  
        x[2*j+2]=t12;
        x[2*j+3]=t8;
  
        x[2*j+6]=t11;
        x[2*j+7]=t9;
        }
      }
    }  
  else
    {
    m=timf1_bytemask/sizeof(int);
    p0=timf1p_px/sizeof(int);
    p0=(p0-fft1_interleave_points*2+timf1_bytes)&m;
    pa=p0;
    pb=(pa+fft1_size)&m;
    ib=fft1_size-2;
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      for(j=0; j<nn; j+=4)
        {
        ja=fft1_permute[j  ];
        jb=fft1_permute[j+1];
        jc=fft1_permute[j+2];
        jd=fft1_permute[j+3];

        ia=(p0+2*ja)&m;
        ib=(p0+2*jb)&m;
        ic=(p0+2*jc)&m;
        id=(p0+2*jd)&m;
        a1=fft1_window[ja]*timf1_int[ia  ];
        a2=fft1_window[ja]*timf1_int[ia+1];

        b1=fft1_window[jb]*timf1_int[ib  ];
        b2=fft1_window[jb]*timf1_int[ib+1];

        c1=fft1_window[jc]*timf1_int[ic  ];
        c2=fft1_window[jc]*timf1_int[ic+1];

        d1=fft1_window[jd]*timf1_int[id  ];
        d2=fft1_window[jd]*timf1_int[id+1];

        t1=a1+b1;
        t2=a2+b2;
      
        t3=c1+d1;
        t4=c2+d2;
      
        t5=a1-b1;
        t7=a2-b2;
      
        t10=c1-d1;
        t6= c2-d2;
    
        x[2*j  ]=t1+t3;
        x[2*j+1]=t2+t4;
      
        x[2*j+4]=t1-t3;
        x[2*j+5]=t2-t4;
      
        t11=t5-t6;
        t8=t7-t10;
        
        t12=t5+t6;
        t9=t7+t10;
      
        x[2*j+2]=t12;
        x[2*j+3]=t8;
      
        x[2*j+6]=t11;
        x[2*j+7]=t9;
        }
      }
    else
      {
      for(j=0; j<nn; j+=4)
        {
        ia=(p0+2*fft1_permute[j  ])&m;
        ib=(p0+2*fft1_permute[j+1])&m;
        ic=(p0+2*fft1_permute[j+2])&m;
        id=(p0+2*fft1_permute[j+3])&m;

        t1=(float)(timf1_int[ia  ])+(float)(timf1_int[ib  ]);
        t2=(float)(timf1_int[ia+1])+(float)(timf1_int[ib+1]);

        t3=(float)(timf1_int[ic  ])+(float)(timf1_int[id  ]);
        t4=(float)(timf1_int[ic+1])+(float)(timf1_int[id+1]);
  
        t5=(float)(timf1_int[ia  ])-(float)(timf1_int[ib  ]);
        t7=(float)(timf1_int[ia+1])-(float)(timf1_int[ib+1]);
  
        t10=(float)(timf1_int[ic  ])-(float)(timf1_int[id  ]);
        t6= (float)(timf1_int[ic+1])-(float)(timf1_int[id+1]);
  
        x[2*j  ]=t1+t3;
        x[2*j+1]=t2+t4;
  
        x[2*j+4]=t1-t3;
        x[2*j+5]=t2-t4;
  
        t11=t5-t6;
        t8=t7-t10;
    
        t12=t5+t6;
        t9=t7+t10;
  
        x[2*j+2]=t12;
        x[2*j+3]=t8;
  
        x[2*j+6]=t11;
        x[2*j+7]=t9;
        }
      }
    }
  bulk_of_dit(fft1_size, fft1_n, x, fft1tab, yieldflag_wdsp_fft1);
  }
}

void fft1_complex_two(void)
{
int chan, i, j, m, n, nn, k;
int p0, pa, pb, pc;
int ia,ib,ic,id;
int ja,jb,jc,jd;
float x1,x2;
float t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12;
float r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12;
float a1,a2,a3,a4;
float b1,b2,b3,b4;
float c1,c2,c3,c4;
float d1,d2,d3,d4;
float *x;
x=&fft1_float[fft1_pa];
n=fft1_size;
nn=fft1_size/2;
switch (FFT1_CURMODE)
  {
  case 7:
  case 8:
  for(chan=0; chan<2; chan++)
    {
    if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
      {
      m=timf1_bytemask/sizeof(short int);
      p0=timf1p_px/sizeof(short int);
      p0=(p0-fft1_interleave_points*4+timf1_bytes)&m;
      pa=(p0+2*chan)&m;
      pb=(pa+2*fft1_size)&m;
      ib=fft1_size-2;
      if(genparm[FIRST_FFT_SINPOW] != 0)
        {
        for( ia=0; ia<nn; ia++)
          {
          ib+=2;
          t1=timf1_short_int[pa  ]*fft1_window[2*ia];
          t2=timf1_short_int[pa+1]*fft1_window[2*ia];      
          t3=timf1_short_int[pb  ]*fft1_window[2*ia+1];
          t4=timf1_short_int[pb+1]*fft1_window[2*ia+1];   
          x1=t1-t3;
          fftw_tmp[2*ia  ]=  t1+t3;
          x2=t4-t2;
          fftw_tmp[2*ia+1]=-(t2+t4);
          pa=(pa+4)&m;
          pb=(pb+4)&m;
          fftw_tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
          fftw_tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
          }
        }
      else
        {
        for( ia=0; ia<nn; ia++)
          {
          t1=timf1_short_int[pa  ];
          ib+=2;
          t2=timf1_short_int[pa+1];      
          t3=timf1_short_int[pb  ];
          t4=timf1_short_int[pb+1];   
          x1=t1-t3;
          x2=t4-t2;
          fftw_tmp[2*ia  ]=  t1+t3;
          fftw_tmp[2*ia+1]=-(t2+t4);
          fftw_tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
          fftw_tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
          pa=(pa+4)&m;
          pb=(pb+4)&m;
          }
        }
      }  
    else
      {
      m=timf1_bytemask/sizeof(int);
      p0=timf1p_px/sizeof(int);
      p0=(p0-fft1_interleave_points*4+m+1)&m;
      pa=(p0+2*chan)&m;
      pb=(pa+2*fft1_size)&m;
      ib=fft1_size-2;
      if(genparm[FIRST_FFT_SINPOW] != 0) 
        {
        for( ia=0; ia<nn; ia++)
          {
          t1=timf1_int[pa  ]*fft1_window[2*ia];
          ib+=2;
          t2=timf1_int[pa+1]*fft1_window[2*ia];      
          t3=timf1_int[pb  ]*fft1_window[2*ia+1];
          t4=timf1_int[pb+1]*fft1_window[2*ia+1];   
          x1=t1-t3;
          fftw_tmp[2*ia  ]=  t1+t3;
          x2=t4-t2;
          fftw_tmp[2*ia+1]=-(t2+t4);
          pa=(pa+4)&m;
          pb=(pb+4)&m;
          fftw_tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
          fftw_tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
          }
        }
      else
        {
        for( ia=0; ia<nn; ia++)
          {
          t1=timf1_int[pa  ];
          ib+=2;
          t2=timf1_int[pa+1];      
          t3=timf1_int[pb  ];
          t4=timf1_int[pb+1];   
          fftw_tmp[2*ia  ]=  t1+t3;
          x1=t1-t3;
          fftw_tmp[2*ia+1]=-(t2+t4);
          x2=t4-t2;
          pa=(pa+4)&m;
          pb=(pb+4)&m;
          fftw_tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
          fftw_tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
          }
        }
      }
    if(FFT1_CURMODE == 7)
      {
      bulk_of_dif(fft1_size, fft1_n, fftw_tmp, fft1tab, yieldflag_wdsp_fft1);
      }
    else
      {
      asmbulk_of_dif(fft1_size, 
                             fft1_n, fftw_tmp, fft1tab, yieldflag_wdsp_fft1);
      }
    pc=fft1_pa+2*chan;
    for(ia=0; ia < n; ia+=2)
      {
      ib=pc+4*fft1_permute[ia];                               
      ic=pc+4*fft1_permute[ia+1];                             
      fft1_float[ib  ]=fftw_tmp[2*ia  ]+fftw_tmp[2*ia+2];
      fft1_float[ic  ]=fftw_tmp[2*ia  ]-fftw_tmp[2*ia+2];
      fft1_float[ib+1]=fftw_tmp[2*ia+1]+fftw_tmp[2*ia+3];
      fft1_float[ic+1]=fftw_tmp[2*ia+1]-fftw_tmp[2*ia+3];
      }
    }
  break;

  case 9:
  if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    m=timf1_bytemask/sizeof(short int);
    p0=timf1p_px/sizeof(short int);
    p0=(p0-fft1_interleave_points*4+timf1_bytes)&m;
    pa=p0;
    pb=(pa+2*fft1_size)&m;
    ib=2*fft1_size-4;
    if(genparm[FIRST_FFT_SINPOW] != 0) 
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_short_int[pa  ]*fft1_window[2*ia];
        t2=timf1_short_int[pa+1]*fft1_window[2*ia];      
        t3=timf1_short_int[pb  ]*fft1_window[2*ia+1];
        t4=timf1_short_int[pb+1]*fft1_window[2*ia+1];   
        ib+=4;
        x1=t1-t3;
        fftw_tmp[4*ia  ]=  t1+t3;
        x2=t4-t2;
        fftw_tmp[4*ia+1]=-(t2+t4);
        fftw_tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        fftw_tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        t1=timf1_short_int[pa+2]*fft1_window[2*ia];
        t2=timf1_short_int[pa+3]*fft1_window[2*ia];      
        t3=timf1_short_int[pb+2]*fft1_window[2*ia+1];
        t4=timf1_short_int[pb+3]*fft1_window[2*ia+1];   
        x1=t1-t3;
        fftw_tmp[4*ia+2]=  t1+t3;
        x2=t4-t2;
        fftw_tmp[4*ia+3]=-(t2+t4);
        pa=(pa+4)&m;
        pb=(pb+4)&m;
        fftw_tmp[ib+2]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        fftw_tmp[ib+3]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    else
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_short_int[pa  ];
        t2=timf1_short_int[pa+1];      
        t3=timf1_short_int[pb  ];
        t4=timf1_short_int[pb+1];   
        ib+=4;
        x1=t1-t3;
        fftw_tmp[4*ia  ]=  t1+t3;
        x2=t4-t2;
        fftw_tmp[4*ia+1]=-(t2+t4);
        fftw_tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        fftw_tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        t1=timf1_short_int[pa+2];
        t2=timf1_short_int[pa+3];      
        t3=timf1_short_int[pb+2];
        t4=timf1_short_int[pb+3];   
        x1=t1-t3;
        fftw_tmp[4*ia+2]=  t1+t3;
        x2=t4-t2;
        fftw_tmp[4*ia+3]=-(t2+t4);
        pa=(pa+4)&m;
        pb=(pb+4)&m;
        fftw_tmp[ib+2]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        fftw_tmp[ib+3]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    }  
  else
    {
    m=timf1_bytemask/sizeof(int);
    p0=timf1p_px/sizeof(int);
    p0=(p0-fft1_interleave_points*4+m+1)&m;
    pa=p0;
    pb=(pa+2*fft1_size)&m;
    ib=2*fft1_size-4;
    if(genparm[FIRST_FFT_SINPOW] != 0) 
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_int[pa  ]*fft1_window[2*ia];
        t2=timf1_int[pa+1]*fft1_window[2*ia];      
        t3=timf1_int[pb  ]*fft1_window[2*ia+1];
        t4=timf1_int[pb+1]*fft1_window[2*ia+1];   
        ib+=4;
        x1=t1-t3;
        fftw_tmp[4*ia  ]=  t1+t3;
        x2=t4-t2;
        fftw_tmp[4*ia+1]=-(t2+t4);
        fftw_tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        fftw_tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        t1=timf1_int[pa+2]*fft1_window[2*ia];
        t2=timf1_int[pa+3]*fft1_window[2*ia];      
        t3=timf1_int[pb+2]*fft1_window[2*ia+1];
        t4=timf1_int[pb+3]*fft1_window[2*ia+1];   
        x1=t1-t3;
        fftw_tmp[4*ia+2]=  t1+t3;
        x2=t4-t2;
        fftw_tmp[4*ia+3]=-(t2+t4);
        pa=(pa+4)&m;
        pb=(pb+4)&m;
        fftw_tmp[ib+2]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        fftw_tmp[ib+3]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    else
      {
      for( ia=0; ia<nn; ia++)
        {
        t1=timf1_int[pa  ];
        t2=timf1_int[pa+1];      
        t3=timf1_int[pb  ];
        t4=timf1_int[pb+1];   
        ib+=4;
        x1=t1-t3;
        fftw_tmp[4*ia  ]=  t1+t3;
        x2=t4-t2;
        fftw_tmp[4*ia+1]=-(t2+t4);
        fftw_tmp[ib  ]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        fftw_tmp[ib+1]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        t1=timf1_int[pa+2];
        t2=timf1_int[pa+3];      
        t3=timf1_int[pb+2];
        t4=timf1_int[pb+3];   
        x1=t1-t3;
        fftw_tmp[4*ia+2]=  t1+t3;
        x2=t4-t2;
        fftw_tmp[4*ia+3]=-(t2+t4);
        pa=(pa+4)&m;
        pb=(pb+4)&m;
        fftw_tmp[ib+2]=fft1tab[ia].cos*x1-fft1tab[ia].sin*x2;
        fftw_tmp[ib+3]=fft1tab[ia].sin*x1+fft1tab[ia].cos*x2;
        }
      }
    }
  asmbulk_of_dual_dif(fft1_size, fft1_n, 
                                    fftw_tmp, fft1tab, yieldflag_wdsp_fft1);
  pc=fft1_pa;
  for(ia=0; ia < n; ia+=2)
    {
    ib=pc+4*fft1_permute[ia];                               
    ic=pc+4*fft1_permute[ia+1];                             
    fft1_float[ib  ]=fftw_tmp[4*ia  ]+fftw_tmp[4*ia+4];
    fft1_float[ic  ]=fftw_tmp[4*ia  ]-fftw_tmp[4*ia+4];
    fft1_float[ib+1]=fftw_tmp[4*ia+1]+fftw_tmp[4*ia+5];
    fft1_float[ic+1]=fftw_tmp[4*ia+1]-fftw_tmp[4*ia+5];
    fft1_float[ib+2]=fftw_tmp[4*ia+2]+fftw_tmp[4*ia+6];
    fft1_float[ic+2]=fftw_tmp[4*ia+2]-fftw_tmp[4*ia+6];
    fft1_float[ib+3]=fftw_tmp[4*ia+3]+fftw_tmp[4*ia+7];
    fft1_float[ic+3]=fftw_tmp[4*ia+3]-fftw_tmp[4*ia+7];
    }
  break;

  case 6:
  for(chan=0; chan<2; chan++)
    {
    if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
      {
      m=timf1_bytemask/sizeof(short int);
      p0=timf1p_px/sizeof(short int);
      p0=(p0-fft1_interleave_points*4+timf1_bytes+2*chan)&m;
      if(genparm[FIRST_FFT_SINPOW] == 0)
        {
        for(j=0; j<n; j+=4)
          {
          ia=(p0+4*fft1_permute[j  ])&m;
          ib=(p0+4*fft1_permute[j+1])&m;
          ic=(p0+4*fft1_permute[j+2])&m;
          id=(p0+4*fft1_permute[j+3])&m;
      
          t1=(float)(timf1_short_int[ia  ])+(float)(timf1_short_int[ib  ]);
          t2=(float)(timf1_short_int[ia+1])+(float)(timf1_short_int[ib+1]);
      
          t3=(float)(timf1_short_int[ic  ])+(float)(timf1_short_int[id  ]);
          t4=(float)(timf1_short_int[ic+1])+(float)(timf1_short_int[id+1]);
      
          t5=(float)(timf1_short_int[ia  ])-(float)(timf1_short_int[ib  ]);
          t7=(float)(timf1_short_int[ia+1])-(float)(timf1_short_int[ib+1]);
      
          t10=(float)(timf1_short_int[ic  ])-(float)(timf1_short_int[id  ]);
          t6= (float)(timf1_short_int[ic+1])-(float)(timf1_short_int[id+1]);
      
          fftw_tmp[2*j  ]=t1+t3;
          fftw_tmp[2*j+1]=t2+t4;
      
          fftw_tmp[2*j+4]=t1-t3;
          fftw_tmp[2*j+5]=t2-t4;
      
          t11=t5-t6;
          t8=t7-t10;
        
          t12=t5+t6;
          t9=t7+t10;
      
          fftw_tmp[2*j+2]=t12;
          fftw_tmp[2*j+3]=t8;
     
          fftw_tmp[2*j+6]=t11;
          fftw_tmp[2*j+7]=t9;
          }
        }
      else 
        {
        for(j=0; j<n; j+=4)
          {
          ja=fft1_permute[j  ];
          jb=fft1_permute[j+1];
          jc=fft1_permute[j+2];
          jd=fft1_permute[j+3];
  
          ia=(p0+4*ja)&m;
          ib=(p0+4*jb)&m;
          ic=(p0+4*jc)&m;
          id=(p0+4*jd)&m;
  
          a1=fft1_window[ja]*timf1_short_int[ia  ];
          a2=fft1_window[ja]*timf1_short_int[ia+1];
   
          b1=fft1_window[jb]*timf1_short_int[ib  ];
          b2=fft1_window[jb]*timf1_short_int[ib+1];
    
          c1=fft1_window[jc]*timf1_short_int[ic  ];
          c2=fft1_window[jc]*timf1_short_int[ic+1];
    
          d1=fft1_window[jd]*timf1_short_int[id  ];
          d2=fft1_window[jd]*timf1_short_int[id+1];
  
          t1=a1+b1;
          t2=a2+b2;
    
          t3=c1+d1;
          t4=c2+d2;
    
          t5=a1-b1;
          t7=a2-b2;
      
          t10=c1-d1;
          t6= c2-d2;
   
          fftw_tmp[2*j  ]=t1+t3;
          fftw_tmp[2*j+1]=t2+t4;
     
          fftw_tmp[2*j+4]=t1-t3;
          fftw_tmp[2*j+5]=t2-t4;
    
          t11=t5-t6;
          t8=t7-t10;
       
          t12=t5+t6;
          t9=t7+t10;
     
          fftw_tmp[2*j+2]=t12;
          fftw_tmp[2*j+3]=t8;
      
          fftw_tmp[2*j+6]=t11;
          fftw_tmp[2*j+7]=t9;
          }
        }
      }
    else
      {  
      m=timf1_bytemask/sizeof(int);
      p0=timf1p_px/sizeof(int);
      p0=(p0-fft1_interleave_points*4+m+1+2*chan)&m;
      if(genparm[FIRST_FFT_SINPOW] == 0)
        {
        for(j=0; j<n; j+=4)
          {
          ia=(p0+4*fft1_permute[j  ])&m;
          ib=(p0+4*fft1_permute[j+1])&m;
          ic=(p0+4*fft1_permute[j+2])&m;
          id=(p0+4*fft1_permute[j+3])&m;
      
          t1=(float)(timf1_int[ia  ])+(float)(timf1_int[ib  ]);
          t2=(float)(timf1_int[ia+1])+(float)(timf1_int[ib+1]);
      
          t3=(float)(timf1_int[ic  ])+(float)(timf1_int[id  ]);
          t4=(float)(timf1_int[ic+1])+(float)(timf1_int[id+1]);
      
          t5=(float)(timf1_int[ia  ])-(float)(timf1_int[ib  ]);
          t7=(float)(timf1_int[ia+1])-(float)(timf1_int[ib+1]);
      
          t10=(float)(timf1_int[ic  ])-(float)(timf1_int[id  ]);
          t6= (float)(timf1_int[ic+1])-(float)(timf1_int[id+1]);
      
          fftw_tmp[2*j  ]=t1+t3;
          fftw_tmp[2*j+1]=t2+t4;
      
          fftw_tmp[2*j+4]=t1-t3;
          fftw_tmp[2*j+5]=t2-t4;
      
          t11=t5-t6;
          t8=t7-t10;
        
          t12=t5+t6;
          t9=t7+t10;
      
          fftw_tmp[2*j+2]=t12;
          fftw_tmp[2*j+3]=t8;
     
          fftw_tmp[2*j+6]=t11;
          fftw_tmp[2*j+7]=t9;
          }
        }
      else 
        {
        for(j=0; j<n; j+=4)
          {
          ja=fft1_permute[j  ];
          jb=fft1_permute[j+1];
          jc=fft1_permute[j+2];
          jd=fft1_permute[j+3];
  
          ia=(p0+4*ja)&m;
          ib=(p0+4*jb)&m;
          ic=(p0+4*jc)&m;
          id=(p0+4*jd)&m;
  
          a1=fft1_window[ja]*timf1_int[ia  ];
          a2=fft1_window[ja]*timf1_int[ia+1];
   
          b1=fft1_window[jb]*timf1_int[ib  ];
          b2=fft1_window[jb]*timf1_int[ib+1];
    
          c1=fft1_window[jc]*timf1_int[ic  ];
          c2=fft1_window[jc]*timf1_int[ic+1];
    
          d1=fft1_window[jd]*timf1_int[id  ];
          d2=fft1_window[jd]*timf1_int[id+1];

          t1=a1+b1;
          t2=a2+b2;
    
          t3=c1+d1;
          t4=c2+d2;
    
          t5=a1-b1;
          t7=a2-b2;
      
          t10=c1-d1;
          t6= c2-d2;
   
          fftw_tmp[2*j  ]=t1+t3;
          fftw_tmp[2*j+1]=t2+t4;
     
          fftw_tmp[2*j+4]=t1-t3;
          fftw_tmp[2*j+5]=t2-t4;
    
          t11=t5-t6;
          t8=t7-t10;
       
          t12=t5+t6;
          t9=t7+t10;
     
          fftw_tmp[2*j+2]=t12;
          fftw_tmp[2*j+3]=t8;
      
          fftw_tmp[2*j+6]=t11;
          fftw_tmp[2*j+7]=t9;
          }
        }
      }
    bulk_of_dit(fft1_size, fft1_n, fftw_tmp, fft1tab, yieldflag_wdsp_fft1);
    k=2*chan;
    m=2*fft1_size;
    for(i=0; i<m; i+=2)
      {
      x[k  ]=fftw_tmp[i];
      x[k+1]=fftw_tmp[i+1];  
      k+=4;
      }
    }     
  break;

  case 10:
  if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    m=timf1_bytemask/sizeof(short int);
    p0=timf1p_px/sizeof(short int);
    p0=(p0-fft1_interleave_points*4+timf1_bytes)&m;
    pa=p0;
    pb=(pa+2*fft1_size)&m;
    ib=2*fft1_size-4;
    if(genparm[FIRST_FFT_SINPOW] == 0)
      {
      for(j=0; j<n; j+=4)
        {
        ia=(p0+4*fft1_permute[j  ])&m;
        ib=(p0+4*fft1_permute[j+1])&m;
        ic=(p0+4*fft1_permute[j+2])&m;
        id=(p0+4*fft1_permute[j+3])&m;
  
        t1=(float)(timf1_short_int[ia  ])+(float)(timf1_short_int[ib  ]);
        t2=(float)(timf1_short_int[ia+1])+(float)(timf1_short_int[ib+1]);
        r1=(float)(timf1_short_int[ia+2])+(float)(timf1_short_int[ib+2]);
        r2=(float)(timf1_short_int[ia+3])+(float)(timf1_short_int[ib+3]);
  
        t3=(float)(timf1_short_int[ic  ])+(float)(timf1_short_int[id  ]);
        t4=(float)(timf1_short_int[ic+1])+(float)(timf1_short_int[id+1]);
        r3=(float)(timf1_short_int[ic+2])+(float)(timf1_short_int[id+2]);
        r4=(float)(timf1_short_int[ic+3])+(float)(timf1_short_int[id+3]);
  
        t5=(float)(timf1_short_int[ia  ])-(float)(timf1_short_int[ib  ]);
        t7=(float)(timf1_short_int[ia+1])-(float)(timf1_short_int[ib+1]);
        r5=(float)(timf1_short_int[ia+2])-(float)(timf1_short_int[ib+2]);
        r7=(float)(timf1_short_int[ia+3])-(float)(timf1_short_int[ib+3]);
  
        t10=(float)(timf1_short_int[ic  ])-(float)(timf1_short_int[id  ]);
        t6= (float)(timf1_short_int[ic+1])-(float)(timf1_short_int[id+1]);
        r10=(float)(timf1_short_int[ic+2])-(float)(timf1_short_int[id+2]);  
        r6= (float)(timf1_short_int[ic+3])-(float)(timf1_short_int[id+3]);
  
        x[4*j  ]=t1+t3;
        x[4*j+1]=t2+t4;
        x[4*j+2]=r1+r3;
        x[4*j+3]=r2+r4;
  
        x[4*j+8]=t1-t3;
        x[4*j+9]=t2-t4;
        x[4*j+10]=r1-r3;
        x[4*j+11]=r2-r4;
  
        t11=t5-t6;
        t8=t7-t10;
        r11=r5-r6;
        r8=r7-r10;
    
        t12=t5+t6;
        t9=t7+t10;
        r12=r5+r6;
        r9=r7+r10;
  
        x[4*j+4]=t12;
        x[4*j+5]=t8;
        x[4*j+6]=r12;
        x[4*j+7]=r8;
  
        x[4*j+12]=t11;
        x[4*j+13]=t9;
        x[4*j+14]=r11;
        x[4*j+15]=r9;
        }
      }
    else
      {
      for(j=0; j<n; j+=4)
        {
        ja=fft1_permute[j  ];
        jb=fft1_permute[j+1];
        jc=fft1_permute[j+2];
        jd=fft1_permute[j+3];

        ia=(p0+4*ja)&m;
        ib=(p0+4*jb)&m;
        ic=(p0+4*jc)&m;
        id=(p0+4*jd)&m;

        a1=fft1_window[ja]*timf1_short_int[ia  ];
        a2=fft1_window[ja]*timf1_short_int[ia+1];
        a3=fft1_window[ja]*timf1_short_int[ia+2];
        a4=fft1_window[ja]*timf1_short_int[ia+3];

        b1=fft1_window[jb]*timf1_short_int[ib  ];
        b2=fft1_window[jb]*timf1_short_int[ib+1];
        b3=fft1_window[jb]*timf1_short_int[ib+2];
        b4=fft1_window[jb]*timf1_short_int[ib+3];

        c1=fft1_window[jc]*timf1_short_int[ic  ];
        c2=fft1_window[jc]*timf1_short_int[ic+1];
        c3=fft1_window[jc]*timf1_short_int[ic+2];
        c4=fft1_window[jc]*timf1_short_int[ic+3];

        d1=fft1_window[jd]*timf1_short_int[id  ];
        d2=fft1_window[jd]*timf1_short_int[id+1];
        d3=fft1_window[jd]*timf1_short_int[id+2];
        d4=fft1_window[jd]*timf1_short_int[id+3];

        t1=a1+b1;
        t2=a2+b2;
        r1=a3+b3;
        r2=a4+b4;
  
        t3=c1+d1;
        t4=c2+d2;
        r3=c3+d3;
        r4=c4+d4;
  
        t5=a1-b1;
        t7=a2-b2;
        r5=a3-b3;
        r7=a4-b4;
  
        t10=c1-d1;
        t6= c2-d2;
        r10=c3-d3;
        r6= c4-d4;

        x[4*j  ]=t1+t3;
        x[4*j+1]=t2+t4;
        x[4*j+2]=r1+r3;
        x[4*j+3]=r2+r4;
  
        x[4*j+8]=t1-t3;
        x[4*j+9]=t2-t4;
        x[4*j+10]=r1-r3;
        x[4*j+11]=r2-r4;
  
        t11=t5-t6;
        t8=t7-t10;
        r11=r5-r6;
        r8=r7-r10;
    
        t12=t5+t6;
        t9=t7+t10;
        r12=r5+r6;
        r9=r7+r10;
  
        x[4*j+4]=t12;
        x[4*j+5]=t8;
        x[4*j+6]=r12;
        x[4*j+7]=r8;
  
        x[4*j+12]=t11;
        x[4*j+13]=t9;
        x[4*j+14]=r11;
        x[4*j+15]=r9;
        }
      }
    }
  else
    {
    m=timf1_bytemask/sizeof(int);
    p0=timf1p_px/sizeof(int);
    p0=(p0-fft1_interleave_points*4+m+1)&m;
    if(genparm[FIRST_FFT_SINPOW] == 0)
      {
      for(j=0; j<n; j+=4)
        {
        ia=(p0+4*fft1_permute[j  ])&m;
        ib=(p0+4*fft1_permute[j+1])&m;
        ic=(p0+4*fft1_permute[j+2])&m;
        id=(p0+4*fft1_permute[j+3])&m;
  
        t1=(float)(timf1_int[ia  ])+(float)(timf1_int[ib  ]);
        t2=(float)(timf1_int[ia+1])+(float)(timf1_int[ib+1]);
        r1=(float)(timf1_int[ia+2])+(float)(timf1_int[ib+2]);
        r2=(float)(timf1_int[ia+3])+(float)(timf1_int[ib+3]);
  
        t3=(float)(timf1_int[ic  ])+(float)(timf1_int[id  ]);
        t4=(float)(timf1_int[ic+1])+(float)(timf1_int[id+1]);
        r3=(float)(timf1_int[ic+2])+(float)(timf1_int[id+2]);
        r4=(float)(timf1_int[ic+3])+(float)(timf1_int[id+3]);
  
        t5=(float)(timf1_int[ia  ])-(float)(timf1_int[ib  ]);
        t7=(float)(timf1_int[ia+1])-(float)(timf1_int[ib+1]);
        r5=(float)(timf1_int[ia+2])-(float)(timf1_int[ib+2]);
        r7=(float)(timf1_int[ia+3])-(float)(timf1_int[ib+3]);
  
        t10=(float)(timf1_int[ic  ])-(float)(timf1_int[id  ]);
        t6= (float)(timf1_int[ic+1])-(float)(timf1_int[id+1]);
        r10=(float)(timf1_int[ic+2])-(float)(timf1_int[id+2]);  
        r6= (float)(timf1_int[ic+3])-(float)(timf1_int[id+3]);
  
        x[4*j  ]=t1+t3;
        x[4*j+1]=t2+t4;
        x[4*j+2]=r1+r3;
        x[4*j+3]=r2+r4;
  
        x[4*j+8]=t1-t3;
        x[4*j+9]=t2-t4;
        x[4*j+10]=r1-r3;
        x[4*j+11]=r2-r4;
  
        t11=t5-t6;
        t8=t7-t10;
        r11=r5-r6;
        r8=r7-r10;
    
        t12=t5+t6;
        t9=t7+t10;
        r12=r5+r6;
        r9=r7+r10;
  
        x[4*j+4]=t12;
        x[4*j+5]=t8;
        x[4*j+6]=r12;
        x[4*j+7]=r8;
  
        x[4*j+12]=t11;
        x[4*j+13]=t9;
        x[4*j+14]=r11;
        x[4*j+15]=r9;
        }
      }
    else
      {
      for(j=0; j<n; j+=4)
        {
        ja=fft1_permute[j  ];
        jb=fft1_permute[j+1];
        jc=fft1_permute[j+2];
        jd=fft1_permute[j+3];

        ia=(p0+4*ja)&m;
        ib=(p0+4*jb)&m;
        ic=(p0+4*jc)&m;
        id=(p0+4*jd)&m;

        a1=fft1_window[ja]*timf1_int[ia  ];
        a2=fft1_window[ja]*timf1_int[ia+1];
        a3=fft1_window[ja]*timf1_int[ia+2];
        a4=fft1_window[ja]*timf1_int[ia+3];

        b1=fft1_window[jb]*timf1_int[ib  ];
        b2=fft1_window[jb]*timf1_int[ib+1];
        b3=fft1_window[jb]*timf1_int[ib+2];
        b4=fft1_window[jb]*timf1_int[ib+3];

        c1=fft1_window[jc]*timf1_int[ic  ];
        c2=fft1_window[jc]*timf1_int[ic+1];
        c3=fft1_window[jc]*timf1_int[ic+2];
        c4=fft1_window[jc]*timf1_int[ic+3];

        d1=fft1_window[jd]*timf1_int[id  ];
        d2=fft1_window[jd]*timf1_int[id+1];
        d3=fft1_window[jd]*timf1_int[id+2];
        d4=fft1_window[jd]*timf1_int[id+3];

        t1=a1+b1;
        t2=a2+b2;
        r1=a3+b3;
        r2=a4+b4;
  
        t3=c1+d1;
        t4=c2+d2;
        r3=c3+d3;
        r4=c4+d4;
  
        t5=a1-b1;
        t7=a2-b2;
        r5=a3-b3;
        r7=a4-b4;
  
        t10=c1-d1;
        t6= c2-d2;
        r10=c3-d3;
        r6= c4-d4;

        x[4*j  ]=t1+t3;
        x[4*j+1]=t2+t4;
        x[4*j+2]=r1+r3;
        x[4*j+3]=r2+r4;
  
        x[4*j+8]=t1-t3;
        x[4*j+9]=t2-t4;
        x[4*j+10]=r1-r3;
        x[4*j+11]=r2-r4;
  
        t11=t5-t6;
        t8=t7-t10;
        r11=r5-r6;
        r8=r7-r10;
    
        t12=t5+t6;
        t9=t7+t10;
        r12=r5+r6;
        r9=r7+r10;
  
        x[4*j+4]=t12;
        x[4*j+5]=t8;
        x[4*j+6]=r12;
        x[4*j+7]=r8;
  
        x[4*j+12]=t11;
        x[4*j+13]=t9;
        x[4*j+14]=r11;
        x[4*j+15]=r9;
        }
      }
    }
  bulk_of_dual_dit(fft1_size, fft1_n, x, fft1tab, yieldflag_wdsp_fft1);
  break;

  case 11:
  if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    if(genparm[FIRST_FFT_SINPOW] == 0)
      {
      simd1_16_nowin();
      }
    else
      {
      simd1_16_win();
      }
    }
  else
    {
    if(genparm[FIRST_FFT_SINPOW] == 0)
      {
      simd1_32_nowin();
      }
    else
      {
      simd1_32_win();
      }
    }
  simdbulk_of_dual_dit(fft1_size, fft1_n, x, fft1tab);
  break;
  }
}

