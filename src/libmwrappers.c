#include <math.h>

float __ieee754_log10f(float);

float log10f(float x) {
    return __ieee754_log10f(x);
}
