global _fft2_mmx_b1hi
global _fft2_mmx_b2hi
global _fft2_mmx_b1med
global _fft2_mmx_b2med
global _fft2_mmx_b1low
global _fft2_mmx_b2low

extern _fft2_short_int
extern _fft2_size
extern _fft2_mmxcosin
extern _fft2_na
extern _fft2_m1
extern _fft2_m2
extern _fft2_inc

fft2_short_int equ _fft2_short_int
fft2_size equ _fft2_size
fft2_mmxcosin equ _fft2_mmxcosin
fft2_na equ _fft2_na
fft2_m1 equ _fft2_m1
fft2_m2 equ _fft2_m2
fft2_inc equ _fft2_inc

%include "fft2mmxb.s"

_fft2_mmx_b1hi equ fft2_mmx_b1hi 
_fft2_mmx_b2hi equ fft2_mmx_b2hi
_fft2_mmx_b1med equ fft2_mmx_b1med
_fft2_mmx_b2med equ fft2_mmx_b2med
_fft2_mmx_b1low equ fft2_mmx_b1low
_fft2_mmx_b2low equ fft2_mmx_b2low
