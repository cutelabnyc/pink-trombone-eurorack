//
//  Glottis.cpp
//  PinkTrombone - VST3
//
//  Created by Samuel Tarakajian on 8/28/19.
//

#include "glottis.h"
#include <math.h>
#include <stdbool.h>
#include "../../include/globals.h"
#include "noise.h"

void GL_init(glottis_t *self, double sampleRate)
{
    self->timeInWaveform = 0;
    self->oldFrequency = 140;
    self->newFrequency = 140;
    self->smoothFrequency = 140;
    self->UIFrequency = 140;
    self->oldTenseness = 0.6f;
    self->newTenseness = 0.6f;
    self->totalTime = 0.0f;
    self->intensity = 0;
    self->loudness = 1;
    self->vibratoAmount = 0.005;
    self->vibratoFrequency = 6;
    self->autoWobble = true;
    self->isTouched = false;
    self->alwaysVoice = true;

    self->sampleRate = sampleRate;
    GL_setupWaveform(self, 0);
}

void GL_setupWaveform(glottis_t *self, double lambda)
{
    self->frequency = self->oldFrequency * (1 - lambda) + self->newFrequency * lambda;
    double tenseness = self->oldTenseness * (1 - lambda) + self->newTenseness * lambda;
    self->Rd = 3 * (1 - tenseness);
    self->waveformLength = 1.0 / self->frequency;

    double Rd = self->Rd;
    if (Rd < 0.5)
        Rd = 0.5;
    if (Rd > 2.7)
        Rd = 2.7;
    // double output;
    // normalized to time = 1, Ee = 1
    double Ra = -0.01 + 0.048 * Rd;
    double Rk = 0.224 + 0.118 * Rd;
    double Rg = (Rk / 4) * (0.5 + 1.2 * Rk) / (0.11 * Rd - Ra * (0.5 + 1.2 * Rk));

    double Ta = Ra;
    double Tp = 1 / (2.0 * Rg);
    double Te = Tp + Tp * Rk;

    double epsilon = 1 / Ta;
    double shift = exp(-epsilon * (1 - Te));
    double Delta = 1 - shift; //divide by this to scale RHS

    double RHSIntegral = (1 / epsilon) * (shift - 1) + (1 - Te) * shift;
    RHSIntegral = RHSIntegral / Delta;

    double totalLowerIntegral = -(Te - Tp) / 2.0 + RHSIntegral;
    double totalUpperIntegral = -totalLowerIntegral;

    double omega = PI / Tp;
    double s = sin(omega * Te);
    // need E0*e^(alpha*Te)*s = -1 (to meet the return at -1)
    // and E0*e^(alpha*Tp/2) * Tp*2/pi = totalUpperIntegral
    //             (our approximation of the integral up to Tp)
    // writing x for e^alpha,
    // have E0*x^Te*s = -1 and E0 * x^(Tp/2) * Tp*2/pi = totalUpperIntegral
    // dividing the second by the first,
    // letting y = x^(Tp/2 - Te),
    // y * Tp*2 / (pi*s) = -totalUpperIntegral;
    double y = -PI * s * totalUpperIntegral / (Tp * 2.0);
    double z = log(y);
    double alpha = z / (Tp / 2.0 - Te);
    double E0 = -1.0 / (s * exp(alpha * Te));
    self->alpha = alpha;
    self->E0 = E0;
    self->epsilon = epsilon;
    self->shift = shift;
    self->Delta = Delta;
    self->Te = Te;
    self->omega = omega;
}

double GL_getNoiseModulator(glottis_t *self)
{
    double voiced = 0.1 + 0.2 * fmax(0.0, sin(PI * 2 * self->timeInWaveform / self->waveformLength));
    //return 0.3;
    return self->UITenseness * self->intensity * voiced + (1 - self->UITenseness * self->intensity) * 0.3;
}

void GL_finishBlock(glottis_t *self)
{
    double vibrato = 0;
    vibrato += self->vibratoAmount * sin(2 * PI * self->totalTime * self->vibratoFrequency);
    vibrato += 0.02 * NO_simplex1(self->totalTime * 4.07);
    vibrato += 0.04 * NO_simplex1(self->totalTime * 2.15);
    if (self->autoWobble)
    {
        vibrato += 0.2 * NO_simplex1(self->totalTime * 0.98);
        vibrato += 0.4 * NO_simplex1(self->totalTime * 0.5);
    }
    if (self->UIFrequency > self->smoothFrequency)
        self->smoothFrequency = fmin(self->smoothFrequency * 1.1, self->UIFrequency);
    if (self->UIFrequency < self->smoothFrequency)
        self->smoothFrequency = fmax(self->smoothFrequency / 1.1, self->UIFrequency);
    self->oldFrequency = self->newFrequency;
    self->newFrequency = self->smoothFrequency * (1 + vibrato);
    self->oldTenseness = self->newTenseness;
    self->newTenseness = self->UITenseness +
                         0.1 * NO_simplex1(self->totalTime * 0.46) + 0.05 * NO_simplex1(self->totalTime * 0.36);
    if (!self->isTouched && self->alwaysVoice)
        self->newTenseness += (3 - self->UITenseness) * (1 - self->intensity);

    if (self->isTouched || self->alwaysVoice)
        self->intensity += 0.13;
    else
        self->intensity -= 0.05;
    self->intensity = clamp(self->intensity, 0.0, 1.0);
}

double GL_normalizedLFWaveform(glottis_t *self, double t)
{
    double output;
    if (t > self->Te)
        output = (-exp(-self->epsilon * (t - self->Te)) + self->shift) / self->Delta;
    else
        output = self->E0 * exp(self->alpha * t) * sin(self->omega * t);

    return output * self->intensity * self->loudness;
}

double GL_runStep(glottis_t *self, double lambda, double noiseSource)
{

    double timeStep = 1.0 / self->sampleRate;
    self->timeInWaveform += timeStep;
    self->totalTime += timeStep;
    if (self->timeInWaveform > self->waveformLength)
    {
        self->timeInWaveform -= self->waveformLength;
        GL_setupWaveform(self, lambda);
    }
    double out = GL_normalizedLFWaveform(self, self->timeInWaveform / self->waveformLength);
    double aspiration = self->intensity * (1 - sqrt(self->UITenseness)) * GL_getNoiseModulator(self) * noiseSource;
    aspiration *= 0.2 + 0.02 * NO_simplex1(self->totalTime * 1.99);
    out += aspiration;
    return out;
}
