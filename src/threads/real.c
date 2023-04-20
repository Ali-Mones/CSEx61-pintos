#include "real.h"
#include <stdint.h>

/*  -- floating point arithmetic --

    - n is integer while x and y are fixed points
    - f = (1 << 14)

    - convert n to fixed point:	n * f
    - convert x to integer (rounding toward zero):	x / f
    - convert x to integer (rounding to nearest):	(x + f / 2) / f if x >= 0,
                                                    (x - f / 2) / f if x <= 0.
    - add x and y:	x + y
    - subtract y from x:	x - y
    - add x and n:	x + n * f
    - subtract n from x:	x - n * f
    - multiply x by y:	((int64_t) x) * y / f
    - multiply x by n:	x * n
    - divide x by y:	((int64_t) x) * f / y
    - divide x by n:	x / n
    ------------------------------- */

real
to_fixed_point(int n)
{
    real r;
    r.value = n * (1 << 14);
    return r;
}

int
to_integer_chopping(real x)
{
    return x.value / (1 << 14);
}

int
to_integer_to_nearest(real x)
{
    if (x.value > 0)
        return (x.value + (1 << 14) / 2) / (1 << 14);
    else
        return (x.value - (1 << 14) / 2) / (1 << 14);
}

real
add_real_to_real(real x, real y)
{
    real r;
    r.value = x.value + y.value;
    return r;
}

real
subtract_real_from_real(real x, real y)
{
    real r;
    r.value = x.value - y.value;
    return r;
}

real
add_real_to_integer(real x, int n)
{
    real r;
    r.value = x.value + n * (1 << 14);
    return r;
}

real
subtract_integer_from_real(real x, int n)
{
    real r;
    r.value = x.value - n * (1 << 14);
    return r;
}

real
multiply_real_by_real(real x, real y)
{
    real r;
    r.value = ((int64_t)x.value) * y.value / (1 << 14);
    return r;
}

real
multiply_real_by_integer(real x, int n)
{
    real r;
    r.value = x.value * n;
    return r;
}

real
divide_real_by_real(real x, real y)
{
    real r;
    r.value = ((int64_t) x.value) * (1 << 14) / y.value;
    return r;
}

real
divide_real_by_integer(real x, int n)
{
    real r;
    r.value = x.value / n;
    return r;
}
