#ifndef __MY_INTEGRAL_3_1
#define __MY_INTEGRAL_3_1

#include "../include/cpu/integral_data_types.hpp"
namespace XCPU {
void integral_3_1(size_t npts,
                  double *points,
                  point rA,
                  point rB,
                  int nprim_pairs,
                  prim_pair *prim_pairs,
                  double *Xi,
                  double *Xj,
                  int ldX,
                  double *Gi,
                  double *Gj,
                  int ldG, 
                  double *weights, 
                  double *boys_table);
}

#endif
