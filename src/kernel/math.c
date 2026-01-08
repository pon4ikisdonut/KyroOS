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

// Simple sin implementation using Taylor series
// sin(x) = x - x^3/3! + x^5/5! - ...
double sin(double x) {
    x = normalize_angle(x);
    double term = x;
    double sum = term;
    for (int i = 1; i < 10; i++) {
        term = -term * x * x / ((2 * i) * (2 * i + 1));
        sum += term;
    }
    return sum;
}
