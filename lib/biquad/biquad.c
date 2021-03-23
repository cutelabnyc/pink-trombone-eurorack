//
//  Biquad.cpp
//  PinkTrombone - All
//
//  Created by Samuel Tarakajian on 8/30/19.
//

#include "biquad.h"
#include <math.h>
#include "../../include/globals.h"

void BQ_init(biquad_t *self, double sampleRate)
{
    self->frequency = 200;
    self->q = 0.5f;
    self->gain = 1.0f;
    self->xm1 = 0.0f;
    self->xm2 = 0.0f;
    self->ym1 = 0.0f;
    self->ym2 = 0.0f;

    self->sampleRate = sampleRate;
    self->twopiOverSampleRate = PI * 2.0 / self->sampleRate;
    BQ_updateCoefficients(self);
}

void BQ_updateCoefficients(biquad_t *self)
{
    double omega = self->frequency * self->twopiOverSampleRate;
    double sn = sin(omega);
    double cs = cos(omega);
    double one_over_Q = 1. / self->q;
    double alpha = sn * 0.5 * one_over_Q;

    // Bandpass only, for now
    double b0 = 1. / (1. + alpha);
    self->a0 = alpha * b0;
    self->a1 = 0.;
    self->a2 = -alpha * b0;
    self->b1 = -2. * cs * b0;
    self->b2 = (1. - alpha) * b0;
}

void BQ_setFrequency(biquad_t *self, double f)
{
    self->frequency = f;
    BQ_updateCoefficients(self);
}

void BQ_setGain(biquad_t *self, double g)
{
    self->gain = g;
    BQ_updateCoefficients(self);
}

void BQ_setQ(biquad_t *self, double q)
{
    self->q = q;
    BQ_updateCoefficients(self);
}

double BQ_runStep(biquad_t *self, double xn)
{
    double yn = (self->a0 * xn) + (self->a1 * self->xm1) + (self->a2 * self->xm2) - (self->b1 * self->ym1) - (self->b2 * self->ym2);
    self->ym2 = self->ym1;
    self->ym1 = yn;
    self->xm2 = self->xm1;
    self->xm1 = xn;
    return yn;
}
