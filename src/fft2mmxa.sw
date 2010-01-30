global _fft2mmx_a2_nowin
global _fft2mmx_a1_nowin
global _fft2mmx_a2_win
global _fft2mmx_a1_win

extern _fft2_short_int
extern _fft2_size
extern _fft2_bigpermute
extern _fft2_na
extern _timf2_shi
extern _timf2_mask
extern _timf2_px
extern _fft2_mmxwin



fft2_short_int equ _fft2_short_int
fft2_size equ _fft2_size
fft2_bigpermute equ _fft2_bigpermute
fft2_na equ _fft2_na
timf2_shi equ _timf2_shi
timf2_mask equ _timf2_mask
timf2_px equ _timf2_px
fft2_mmxwin equ _fft2_mmxwin

%include "fft2mmxa.s"

_fft2mmx_a2_nowin equ fft2mmx_a2_nowin
_fft2mmx_a1_nowin equ fft2mmx_a1_nowin
_fft2mmx_a2_win equ fft2mmx_a2_win
_fft2mmx_a1_win equ fft2mmx_a1_win
