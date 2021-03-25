//
//  Tract.hpp
//  PinkTrombone - VST3
//
//  Created by Samuel Tarakajian on 9/5/19.
//

#ifndef _TRACT_H
#define _TRACT_H

#include <stdio.h>
// #include "../JuceLibraryCode/JuceHeader.h"
#include "glottis.h"

typedef struct t_transient
{
    int position;
    double timeAlive;
    double lifeTime;
    double strength;
    double exponent;
    bool living;
} t_transient;

typedef struct t_tractProps
{
    int n;
    int lipStart;
    int bladeStart;
    int tipStart;
    int noseStart;
    int noseLength;
    double noseOffset;
    double tongueIndex;
    double tongueDiameter;
    double *noseDiameter;
    double *tractDiameter;
} t_tractProps;

typedef struct tract
{
    double sampleRate, blockTime;
    t_tractProps *tractProps;
    double glottalReflection;
    double lipReflection;
    int lastObstruction;
    double fade;
    double movementSpeed;
    double velumTarget;
    t_transient *transients;
    int transientCount;

    double *diameter;
    double *restDiameter;
    double *targetDiameter;
    double *newDiameter;

    double *R;
    double *L;
    double *reflection;
    double *newReflection;
    double *junctionOutputR;
    double *junctionOutputL;
    double *A;
    double *maxAmplitude;

    double *noseR;
    double *noseL;
    double *noseJunctionOutputR;
    double *noseJunctionOutputL;
    double *noseReflection;
    double *noseDiameter;
    double *noseA;
    double *noseMaxAmplitude;

    double reflectionLeft, reflectionRight, reflectionNose;
    double newReflectionLeft, newReflectionRight, newReflectionNose;

    double constrictionIndex;
    double constrictionDiameter;
    double fricativeIntensity;

    double noseOutput;
    double lipOutput;
} tract_t;

t_tractProps *initializeTractProps(int n);

tract_t *T_init(double sampleRate, double blockTime, t_tractProps *props);
void T_destroy(tract_t *self);
void T_runStep(tract_t *self, double glottalOutput, double turbulenceNoise, double lambda, glottis_t *glottis);
void T_finishBlock(tract_t *self);
void T_setRestDiameter(tract_t *self, double tongueIndex, double tongueDiameter);
void T_setConstriction(tract_t *self, double cindex, double cdiam, double fricativeIntensity);

long T_getTractIndexCount(tract_t *self);
long T_tongueIndexLowerBound(tract_t *self);
long T_tongueIndexUpperBound(tract_t *self);

void T_addTransient(tract_t *self, int position);
void T_addTurbulenceNoise(tract_t *self, double turbulenceNoise, glottis_t *glottis);
void T_addTurbulenceNoiseAtIndex(tract_t *self, double turbulenceNoise, double index, double diameter, glottis_t *glottis);
void T_calculateReflections(tract_t *self);
void T_calculateNoseReflections(tract_t *self);
void T_processTransients(tract_t *self);
void T_reshapeTract(tract_t *self, double deltaTime);

#endif /* _TRACT_H */
