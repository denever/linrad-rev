global _fft2_mmx_c2
global _fft2_mmx_c1
extern _fft2_short_int
extern _fft2_size
extern _fft2_mmxcosin
extern _fft2_na
extern _fft2_n
extern _fft2_m2
extern _fft2_inc

fft2_short_int equ _fft2_short_int
fft2_size equ _fft2_size
fft2_mmxcosin equ _fft2_mmxcosin
fft2_na equ _fft2_na
fft2_n equ _fft2_n
fft2_m2 equ _fft2_m2
fft2_inc equ _fft2_inc

%include "fft2mmxc.s"

_fft2_mmx_c2 equ fft2_mmx_c2
_fft2_mmx_c1 equ fft2_mmx_c1
