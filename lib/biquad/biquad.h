//
//  Biquad.hpp
//  PinkTrombone - All
//
//  Created by Samuel Tarakajian on 8/30/19.
//

#ifndef _BIQUAD_H
#define _BIQUAD_H

#include <stdio.h>

typedef struct biquad
{
    double frequency, q, gain;
    double a0, a1, a2, b1, b2;
    double xm1, xm2, ym1, ym2;
    double sampleRate, twopiOverSampleRate;
} biquad_t;

biquad_t *BQ_init(double sampleRate);
void BQ_destroy(biquad_t *self);
void BQ_setFrequency(biquad_t *self, double f);
void BQ_setQ(biquad_t *self, double f);
void BQ_setGain(biquad_t *self, double g);
double BQ_runStep(biquad_t *self, double xn);

void BQ_updateCoefficients(biquad_t *self);

#endif /* _BIQUAD_H */