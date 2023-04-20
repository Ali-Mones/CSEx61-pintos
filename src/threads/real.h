#ifndef THREADS_REAL_H
#define THREADS_REAL_H

struct real
{
  int value;    /* Integer representation of the fixed point value */
};

struct real to_fixed_point(int n);
int to_integer_chopping(struct real x);
int to_integer_to_nearest(struct real x);
struct real add_real_to_real(struct real x, struct real y);
struct real subtract_real_from_real(struct real x, struct real y);
struct real add_real_to_integer(struct real x, int n);
struct real subtract_integer_from_real(struct real x, int n);
struct real multiply_real_by_real(struct real x, struct real y);
struct real multiply_real_by_integer(struct real x, int n);
struct real divide_real_by_real(struct real x, struct real y);
struct real divide_real_by_integer(struct real x, int n);

#endif