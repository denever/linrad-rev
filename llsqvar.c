#include "globdef.h"
#include "llsqdef.h"
int llsq_neq;
int llsq_npar;
float *llsq_derivatives;
float *llsq_errors;
float llsq_steps[2*LLSQ_MAXPAR];
