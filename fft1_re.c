
#include "globdef.h"
#include "uidef.h"
#include "fft1def.h"


void fft1_reherm_dit_one(void)
{
int m, k, n, nn;
int p0, pa, pb, pc;
int ia,ib;
n=fft1_size;
nn=2*fft1_size;
if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
  {
  m=timf1_bytemask/sizeof(short int);
  k=m+1;
  p0=timf1p_px/sizeof(short int);
  p0=(p0-fft1_interleave_points*2+timf1_bytes)&m;
  pa=p0;
  if(genparm[FIRST_FFT_SINPOW] != 0)
    {
    ib=2*fft1_size-1;
    pb=(pa+ib)&m;
    for( ia=0; ia<n; ia++)
      {
      fftw_tmp[fft1_permute[ia  ]]=timf1_short_int[pa]*fft1_window[ia];
      fftw_tmp[fft1_permute[ib  ]]=timf1_short_int[pb]*fft1_window[ia];
      ib--; 
      pa=(pa+1)&m;
      pb=(pb-1+k)&m;
      }
    }
  else
    {
    for( ia=0; ia<nn; ia++)
      {
      fftw_tmp[fft1_permute[ia]]=timf1_short_int[pa];
      pa=(pa+1)&m;
      }
    }
  }
else
  {
  m=timf1_bytemask/sizeof(int);
  k=m+1;
  p0=timf1p_px/sizeof(int);
  p0=(p0-fft1_interleave_points*2+timf1_bytes)&m;
  pa=p0;
  if(genparm[FIRST_FFT_SINPOW] != 0)
    {
    ib=2*fft1_size-1;
    pb=(pa+ib)&m;
    for( ia=0; ia<n; ia++)
      {
      fftw_tmp[fft1_permute[ia  ]]=timf1_int[pa]*fft1_window[ia];
      fftw_tmp[fft1_permute[ib  ]]=timf1_int[pb]*fft1_window[ia];
      ib--; 
      pa=(pa+1)&m;
      pb=(pb-1+k)&m;
      }
    }
  else
    {
    for( ia=0; ia<nn; ia++)
      {
      fftw_tmp[fft1_permute[ia]]=timf1_int[pa];
      pa=(pa+1)&m;
      }
    }
  }
fft_real_to_hermitian( fftw_tmp, 2*fft1_size, fft1_n, fft1tab);
// Output is {Re(z^[0]),...,Re(z^[n/2),Im(z^[n/2-1]),...,Im(z^[1]).
if(fft1_direction > 0)
  {
  pc=fft1_pa;
  fft1_float[pc+1]=fftw_tmp[0];
  fft1_float[pc  ]=fftw_tmp[fft1_size];
  k=fft1_first_point;
  m=fft1_last_point;
  if(k==0)k=1;
  pc+=2*k;
  for(ia=k; ia <= m; ia++)
    {
    fft1_float[pc+1]=fftw_tmp[ia  ];
    fft1_float[pc  ]=fftw_tmp[2*fft1_size-ia];
    pc+=2;
    }
  }
else
  {
  pc=fft1_pa+2*(fft1_size-1);
  fft1_float[pc  ]=fftw_tmp[0];
  fft1_float[pc+1]=fftw_tmp[fft1_size];
  k=fft1_size-1-fft1_last_point;
  m=1+k+fft1_last_point-fft1_first_point;
  if(k==0)k=1;
  pc-=2*(k-1);
  for(ia=k; ia <= m; ia++)
    {
    fft1_float[pc  ]=fftw_tmp[ia  ];
    fft1_float[pc+1]=fftw_tmp[2*fft1_size-ia];
    pc-=2;
    }
  }
}


void fft1_reherm_dit_two(void)
{
int chan, m, k;
int p0, pa, pb, pc;
int ia,ib;
for(chan=0; chan<2; chan++)
  {
  if(  (ui.rx_input_mode&DWORD_INPUT) == 0)
    {
    m=timf1_bytemask/sizeof(short int);
    k=m+1;
    p0=timf1p_px/sizeof(short int);
    p0=(p0-fft1_interleave_points*4+timf1_bytes)&m;
    pa=p0+chan;
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      ib=2*fft1_size-1;
      pb=(pa+2*ib)&m;
      for( ia=0; ia<fft1_size; ia++)
        {
        fftw_tmp[fft1_permute[ia  ]]=timf1_short_int[pa]*fft1_window[ia];
        fftw_tmp[fft1_permute[ib  ]]=timf1_short_int[pb]*fft1_window[ia];
        ib--; 
        pa=(pa+2)&m;
        pb=(pb-2+k)&m;
        }
      }
    else
      {
      for( ia=0; ia<2*fft1_size; ia++)
        {
        fftw_tmp[fft1_permute[ia]]=timf1_short_int[pa];
        pa=(pa+2)&m;
        }
      }
    }
  else
    {
    m=timf1_bytemask/sizeof(int);
    k=m+1;
    p0=timf1p_px/sizeof(int);
    p0=(p0-fft1_interleave_points*4+timf1_bytes)&m;
    pa=p0+chan;  
    if(genparm[FIRST_FFT_SINPOW] != 0)
      {
      ib=2*fft1_size-1;
      pb=(pa+2*ib)&m;
      for( ia=0; ia<fft1_size; ia++)
        {
        fftw_tmp[fft1_permute[ia  ]]=timf1_int[pa]*fft1_window[ia];
        fftw_tmp[fft1_permute[ib  ]]=timf1_int[pb]*fft1_window[ia];
        ib--; 
        pa=(pa+2)&m;
        pb=(pb-2+k)&m;
        }
      }
    else
      {
      for( ia=0; ia<2*fft1_size; ia++)
        {
        fftw_tmp[fft1_permute[ia]]=timf1_int[pa];
        pa=(pa+2)&m;
        }
      }
    }
  fft_real_to_hermitian( fftw_tmp, 2*fft1_size, fft1_n, fft1tab);
  if(fft1_direction > 0)
    {
    pc=fft1_pa+2*chan;
    fft1_float[pc+1]=fftw_tmp[0];
    fft1_float[pc  ]=fftw_tmp[fft1_size];
    k=fft1_first_point;
    m=fft1_last_point;
    if(k==0)k=1;
    pc+=4*k;
    for(ia=k; ia <= m; ia++)
      {
      fft1_float[pc+1]=fftw_tmp[ia  ];
      fft1_float[pc  ]=fftw_tmp[2*fft1_size-ia];
      pc+=4;
      }
    }
  else
    {
    pc=fft1_pa+2*chan+4*(fft1_size-1);
    fft1_float[pc  ]=fftw_tmp[0];
    fft1_float[pc+1]=fftw_tmp[fft1_size];
    k=fft1_size-1-fft1_last_point;
    m=1+k+fft1_last_point-fft1_first_point;
    if(k==0)k=1;
    pc-=4*(k-1);
    for(ia=k; ia <= m; ia++)
      {
      fft1_float[pc  ]=fftw_tmp[ia  ];
      fft1_float[pc+1]=fftw_tmp[2*fft1_size-ia];
      pc-=4;
      }
    }
  }
}
