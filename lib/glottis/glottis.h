//
//  Glottis.hpp
//  PinkTrombone - VST3
//
//  Created by Samuel Tarakajian on 8/28/19.
//

#ifndef _GLOTTIS_H
#define _GLOTTIS_H

#include <stdio.h>
#include <stdbool.h>

typedef struct glottis
{
    double sampleRate;
    double timeInWaveform;
    double frequency, oldFrequency, newFrequency, smoothFrequency, UIFrequency;
    double oldTenseness, newTenseness, UITenseness;
    double waveformLength;
    double Rd;
    double alpha;
    double E0;
    double epsilon;
    double shift;
    double Delta;
    double Te;
    double omega;
    double totalTime;
    double intensity, loudness;
    double vibratoAmount;
    double vibratoFrequency;
    bool autoWobble;
    bool isTouched;
    bool alwaysVoice;
} glottis_t;

void GL_init(glottis_t *self, double sampleRate);
void GL_destroy(glottis_t *self);
double GL_runStep(glottis_t *self, double lambda, double noiseSource);
void GL_finishBlock(glottis_t *self);
double GL_getNoiseModulator(glottis_t *self);

void GL_setupWaveform(glottis_t *self, double lambda);
double GL_normalizedLFWaveform(glottis_t *self, double t);

#endif /* _GLOTTIS_H */
