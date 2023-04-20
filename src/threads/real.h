#ifndef THREADS_REAL_H
#define THREADS_REAL_H

typedef struct
{
  int value;    /* Integer representation of the fixed point value */
} real;

real to_fixed_point(int n);
int to_integer_chopping(real x);
int to_integer_to_nearest(real x);
real add_real_to_real(real x, real y);
real subtract_real_from_real(real x, real y);
real add_real_to_integer(real x, int n);
real subtract_integer_from_real(real x, int n);
real multiply_real_by_real(real x, real y);
real multiply_real_by_integer(real x, int n);
real divide_real_by_real(real x, real y);
real divide_real_by_integer(real x, int n);

#endif