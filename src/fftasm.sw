global _asmbulk_of_dual_dif
global _asmbulk_of_dif

extern _lir_sched_yield

%include "fftasm.s"

_asmbulk_of_dual_dif equ asmbulk_of_dual_dif
_asmbulk_of_dif equ asmbulk_of_dif
lir_sched_yield equ _lir_sched_yield
