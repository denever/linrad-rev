global _compress_rawdat
global _expand_rawdat

extern _rawsave_tmp
extern _fft1_char 
extern _fft1_bytes
extern _rx_read_bytes
extern _timf1_char
extern _timf1p_pa

rawsave_tmp equ _rawsave_tmp
fft1_char equ _fft1_char 
fft1_bytes equ _fft1_bytes
rx_read_bytes equ _rx_read_bytes
timf1_char equ _timf1_char
timf1p_pa equ _timf1p_pa

%include "getiq.s"

_compress_rawdat equ compress_rawdat
_expand_rawdat equ expand_rawdat