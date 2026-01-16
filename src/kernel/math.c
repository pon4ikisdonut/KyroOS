#include "math.h"

#define PI 3.1415926535

// Helper to normalize angle to be between -PI and PI
static double normalize_angle(double x) {
  x = x - (int)(x / (2 * PI)) * (2 * PI);
  if (x > PI) {
    x -= 2 * PI;
  }
  if (x < -PI) {
    x += 2 * PI;
  }
  return x;
}

/* Taylor series implementation removed as double is not allowed in kernel
 * -mno-sse */
double sin(double x) {
  (void)x;
  return 0.0;
}
