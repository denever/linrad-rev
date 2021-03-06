
global _simdbulk_of_dual_dit
global _simd1_32_win
global _simd1_32_nowin
global _simd1_16_win
global _simd1_16_nowin

extern _fft1_permute
fft1_permute equ _fft1_permute
extern _fft1_window
fft1_window equ _fft1_window
extern _timf1_int
timf1_int equ _timf1_int
extern _fft1_float
fft1_float equ _fft1_float
extern _fft1_pa
fft1_pa equ _fft1_pa
extern _timf1p_px
timf1p_px equ _timf1p_px
extern _fft1_size
fft1_size equ _fft1_size
extern _timf1_bytemask
timf1_bytemask equ _timf1_bytemask
extern _fft1_interleave_points
fft1_interleave_points equ _fft1_interleave_points
extern _yieldflag_wdsp_fft1
yieldflag_wdsp_fft1 equ _yieldflag_wdsp_fft1
extern _lir_sched_yield
lir_sched_yield equ _lir_sched_yield

%include "simdasm.s"

_simdbulk_of_dual_dit equ simdbulk_of_dual_dit
_simd1_32_win equ simd1_32_win
_simd1_32_nowin equ simd1_32_nowin
_simd1_16_win equ simd1_16_win
_simd1_16_nowin equ simd1_16_nowin
