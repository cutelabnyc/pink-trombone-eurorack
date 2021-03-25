//
//  Tract
//  PinkTrombone - VST3
//
//  Created by Samuel Tarakajian on 9/5/19.
//

#include "tract.h"
#include <string.h>
#include <math.h>
#include "../../include/globals.h"

#define MAX_TRANSIENTS (20)

t_tractProps *initializeTractProps(int n)
{
    t_tractProps *props = malloc(sizeof(t_tractProps));
    props->n = n;
    props->bladeStart = 10;
    props->tipStart = 32;
    props->lipStart = 39;
    props->bladeStart = (int)floor(props->bladeStart * (double)n / 44.0);
    props->tipStart = (int)floor(props->tipStart * (double)n / 44.0);
    props->lipStart = (int)floor(props->lipStart * (double)n / 44.0);
    props->tongueIndex = props->bladeStart;
    props->tongueDiameter = 3.5;
    props->tractDiameter = (double *)calloc(n, sizeof(double));
    props->noseLength = (int)floor(28.0 * (double)props->n / 44.0);
    props->noseDiameter = (double *)calloc(props->noseLength, sizeof(double));
    props->noseStart = props->n - props->noseLength + 1;
    props->noseOffset = 0.8;

    return props;
}

tract_t *T_init(double sampleRate, double blockTime, t_tractProps *props)
{
    tract_t *self = malloc(sizeof(tract_t));
    self->glottalReflection = 0.75f;
    self->lipReflection = -0.85f;
    self->lastObstruction = -1;
    self->fade = 1.0f;
    self->movementSpeed = 15;
    self->velumTarget = 0.01f;
    self->constrictionIndex = 3.0f;
    self->constrictionDiameter = 1.0f;

    self->sampleRate = sampleRate;
    self->blockTime = blockTime;
    self->transients = (t_transient *)calloc(MAX_TRANSIENTS, sizeof(t_transient));
    self->transientCount = 0;
    self->tractProps = props;
    self->fricativeIntensity = 0;

    self->noseOutput = 0;
    self->lipOutput = 0;

    self->diameter = (double *)calloc(self->tractProps->n, sizeof(double));
    self->restDiameter = (double *)calloc(self->tractProps->n, sizeof(double));
    self->targetDiameter = (double *)calloc(self->tractProps->n, sizeof(double));
    self->newDiameter = (double *)calloc(self->tractProps->n, sizeof(double));
    for (int i = 0; i < self->tractProps->n; i++)
    {
        double diameter = 0;
        if (i < 7 * (double)self->tractProps->n / 44.0 - 0.5)
            diameter = 0.6;
        else if (i < 12 * (double)self->tractProps->n / 44.0)
            diameter = 1.1;
        else
            diameter = 1.5;
        self->diameter[i] = self->restDiameter[i] = self->targetDiameter[i] = self->newDiameter[i] = diameter;
    }
    self->R = (double *)calloc(self->tractProps->n, sizeof(double));
    self->L = (double *)calloc(self->tractProps->n, sizeof(double));
    self->reflection = (double *)calloc(self->tractProps->n + 1, sizeof(double));
    self->newReflection = (double *)calloc(self->tractProps->n + 1, sizeof(double));
    self->junctionOutputR = (double *)calloc(self->tractProps->n + 1, sizeof(double));
    self->junctionOutputL = (double *)calloc(self->tractProps->n + 1, sizeof(double));
    self->A = (double *)calloc(self->tractProps->n, sizeof(double));
    self->maxAmplitude = (double *)calloc(self->tractProps->n, sizeof(double));

    self->noseR = (double *)calloc(self->tractProps->noseLength, sizeof(double));
    self->noseL = (double *)calloc(self->tractProps->noseLength, sizeof(double));
    self->noseJunctionOutputR = (double *)calloc(self->tractProps->noseLength + 1, sizeof(double));
    self->noseJunctionOutputL = (double *)calloc(self->tractProps->noseLength + 1, sizeof(double));
    self->noseReflection = (double *)calloc(self->tractProps->noseLength + 1, sizeof(double));
    self->noseDiameter = (double *)calloc(self->tractProps->noseLength, sizeof(double));
    self->noseA = (double *)calloc(self->tractProps->noseLength, sizeof(double));
    self->noseMaxAmplitude = (double *)calloc(self->tractProps->noseLength, sizeof(double));
    for (int i = 0; i < self->tractProps->noseLength; i++)
    {
        double diameter;
        double d = 2.0 * ((double)i / (double)self->tractProps->noseLength);
        if (d < 1.0)
            diameter = 0.4 + 1.6 * d;
        else
            diameter = 0.5 + 1.5 * (2.0 - d);
        diameter = fmin(diameter, 1.9);
        self->noseDiameter[i] = diameter;
    }
    self->newReflectionLeft = self->newReflectionRight = self->newReflectionNose = 0.0;
    T_calculateReflections(self);
    T_calculateNoseReflections(self);
    self->noseDiameter[0] = self->velumTarget;
    memcpy(self->tractProps->tractDiameter, self->diameter, sizeof(double) * self->tractProps->n);
    memcpy(self->tractProps->noseDiameter, self->noseDiameter, sizeof(double) * self->tractProps->noseLength);

    return self;
}

long T_getTractIndexCount(tract_t *self)
{
    return self->tractProps->n;
}

long T_tongueIndexLowerBound(tract_t *self)
{
    return self->tractProps->bladeStart + 2;
}

long T_tongueIndexUpperBound(tract_t *self)
{
    return self->tractProps->tipStart - 3;
}

void T_addTransient(tract_t *self, int position)
{
    if (self->transientCount < MAX_TRANSIENTS)
    {
        t_transient *trans = NULL;
        for (int i = 0; i < MAX_TRANSIENTS; i++)
        {
            trans = self->transients + i;
            if (trans->living == false)
                break;
        }
        if (trans)
        {
            trans->position = position;
            trans->timeAlive = 0;
            trans->lifeTime = 0.2;
            trans->strength = 0.3;
            trans->exponent = 200;
            trans->living = true;
        }
        self->transientCount++;
    }
}

void T_addTurbulenceNoise(tract_t *self, double turbulenceNoise, glottis_t *glottis)
{
    if (self->constrictionIndex < 2.0 || self->constrictionIndex > (double)self->tractProps->n)
    {
        return;
    }
    if (self->constrictionDiameter <= 0.0)
        return;
    double intensity = self->fricativeIntensity;
    T_addTurbulenceNoiseAtIndex(self, 0.66 * turbulenceNoise * intensity, self->constrictionIndex, self->constrictionDiameter, glottis);
}

void T_addTurbulenceNoiseAtIndex(tract_t *self, double turbulenceNoise, double index, double diameter, glottis_t *glottis)
{
    long i = (long)floor(index);
    double delta = index - (double)i;
    turbulenceNoise *= GL_getNoiseModulator(glottis);
    double thinness0 = clamp(8.0 * (0.7 - diameter), 0.0, 1.0);
    double openness = clamp(30.0 * (diameter - 0.3), 0.0, 1.0);
    double noise0 = turbulenceNoise * (1.0 - delta) * thinness0 * openness;
    double noise1 = turbulenceNoise * delta * thinness0 * openness;
    self->R[i + 1] += noise0 / 2.0;
    self->L[i + 1] += noise0 / 2.0;
    self->R[i + 2] += noise1 / 2.0;
    self->L[i + 2] += noise1 / 2.0;
}

void T_calculateReflections(tract_t *self)
{
    for (int i = 0; i < self->tractProps->n; i++)
    {
        self->A[i] = self->diameter[i] * self->diameter[i]; //ignoring PI etc.
    }
    for (int i = 1; i < self->tractProps->n; i++)
    {
        self->reflection[i] = self->newReflection[i];
        if (self->A[i] == 0)
            self->newReflection[i] = 0.999; //to prevent some bad behaviour if 0
        else
            self->newReflection[i] = (self->A[i - 1] - self->A[i]) / (self->A[i - 1] + self->A[i]);
    }

    //now at junction with nose
    self->reflectionLeft = self->newReflectionLeft;
    self->reflectionRight = self->newReflectionRight;
    self->reflectionNose = self->newReflectionNose;
    double sum = self->A[self->tractProps->noseStart] + self->A[self->tractProps->noseStart + 1] + self->noseA[0];
    self->newReflectionLeft = (2.0 * self->A[self->tractProps->noseStart] - sum) / sum;
    self->newReflectionRight = (2 * self->A[self->tractProps->noseStart + 1] - sum) / sum;
    self->newReflectionNose = (2 * self->noseA[0] - sum) / sum;
}

void T_calculateNoseReflections(tract_t *self)
{
    for (int i = 0; i < self->tractProps->noseLength; i++)
    {
        self->noseA[i] = self->noseDiameter[i] * self->noseDiameter[i];
    }
    for (int i = 1; i < self->tractProps->noseLength; i++)
    {
        self->noseReflection[i] = (self->noseA[i - 1] - self->noseA[i]) / (self->noseA[i - 1] + self->noseA[i]);
    }
}

void T_finishBlock(tract_t *self)
{
    T_reshapeTract(self, self->blockTime);
    T_calculateReflections(self);
    memcpy(self->tractProps->tractDiameter, self->diameter, sizeof(double) * self->tractProps->n);
    memcpy(self->tractProps->noseDiameter, self->noseDiameter, sizeof(double) * self->tractProps->noseLength);
}

void T_setRestDiameter(tract_t *self, double tongueIndex, double tongueDiameter)
{
    self->tractProps->tongueIndex = tongueIndex;
    self->tractProps->tongueDiameter = tongueDiameter;
    for (long i = self->tractProps->bladeStart; i < self->tractProps->lipStart; i++)
    {
        double t = 1.1 * PI * (double)(tongueIndex - i) / (double)(self->tractProps->tipStart - self->tractProps->bladeStart);
        double fixedTongueDiameter = 2 + (tongueDiameter - 2) / 1.5;
        double curve = (1.5 - fixedTongueDiameter + 1.7) * cos(t);
        if (i == self->tractProps->bladeStart - 2 || i == self->tractProps->lipStart - 1)
            curve *= 0.8;
        if (i == self->tractProps->bladeStart || i == self->tractProps->lipStart - 2)
            curve *= 0.94;
        self->restDiameter[i] = 1.5 - curve;
    }
    for (long i = 0; i < self->tractProps->n; i++)
    {
        self->targetDiameter[i] = self->restDiameter[i];
    }
}

void T_setConstriction(tract_t *self, double cindex, double cdiam, double fricativeIntensity)
{
    self->constrictionIndex = cindex;
    self->constrictionDiameter = cdiam;
    self->fricativeIntensity = fricativeIntensity;

    // This is basically the Tract touch handling code
    self->velumTarget = 0.01;
    if (self->constrictionIndex > self->tractProps->noseStart && self->constrictionDiameter < -self->tractProps->noseOffset)
    {
        self->velumTarget = 0.4;
    }
    if (self->constrictionDiameter < -0.85 - self->tractProps->noseOffset)
    {
        return;
    }

    double diameter = self->constrictionDiameter - 0.3;
    if (diameter < 0)
        diameter = 0;
    long width = 2;
    if (self->constrictionIndex < 25)
        width = 10;
    else if (self->constrictionIndex >= self->tractProps->tipStart)
        width = 5;
    else
        width = 10.0 - 5 * (self->constrictionIndex - 25) / ((double)self->tractProps->tipStart - 25.0);
    if (self->constrictionIndex >= 2 && self->constrictionIndex < self->tractProps->n && diameter < 3)
    {
        long intIndex = round(self->constrictionIndex);
        for (long i = -ceil(width) - 1; i < width + 1; i++)
        {
            if (intIndex + i < 0 || intIndex + i >= self->tractProps->n)
                continue;
            double relpos = (intIndex + i) - self->constrictionIndex;
            relpos = abs(relpos) - 0.5;
            double shrink;
            if (relpos <= 0)
                shrink = 0;
            else if (relpos > width)
                shrink = 1;
            else
                shrink = 0.5 * (1 - cos(PI * relpos / (double)width));
            if (diameter < self->targetDiameter[intIndex + i])
            {
                self->targetDiameter[intIndex + i] = diameter + (self->targetDiameter[intIndex + i] - diameter) * shrink;
            }
        }
    }
}

void T_processTransients(tract_t *self)
{
    for (int i = 0; i < self->transientCount; i++)
    {
        t_transient *trans = self->transients + i;
        double amplitude = trans->strength * pow(2.0, -trans->exponent * trans->timeAlive);
        self->R[trans->position] += amplitude / 2.0;
        self->L[trans->position] += amplitude / 2.0;
        trans->timeAlive += 1.0 / (self->sampleRate * 2.0);
    }
    for (int i = self->transientCount - 1; i >= 0; i--)
    {
        t_transient *trans = self->transients + i;
        if (trans->timeAlive > trans->lifeTime)
        {
            trans->living = false;
        }
    }
}

void T_reshapeTract(tract_t *self, double deltaTime)
{
    double amount = deltaTime * self->movementSpeed;

    int newLastObstruction = -1;
    for (int i = 0; i < self->tractProps->n; i++)
    {
        double diameter = self->diameter[i];
        double targetDiameter = self->targetDiameter[i];
        if (diameter <= 0)
            newLastObstruction = i;
        double slowReturn;
        if (i < self->tractProps->noseStart)
            slowReturn = 0.6;
        else if (i >= self->tractProps->tipStart)
            slowReturn = 1.0;
        else
            slowReturn = 0.6 + 0.4 * (i - self->tractProps->noseStart) / (self->tractProps->tipStart - self->tractProps->noseStart);
        self->diameter[i] = moveTowardsUpDown(diameter, targetDiameter, slowReturn * amount, 2 * amount);
    }
    if (self->lastObstruction > -1 && newLastObstruction == -1 && self->noseA[0] < 0.05)
    {
        T_addTransient(self, self->lastObstruction);
    }
    self->lastObstruction = newLastObstruction;

    amount = deltaTime * self->movementSpeed;
    self->noseDiameter[0] = moveTowardsUpDown(self->noseDiameter[0], self->velumTarget, amount * 0.25, amount * 0.1);
    self->tractProps->noseDiameter[0] = self->noseDiameter[0];
    self->noseA[0] = self->noseDiameter[0] * self->noseDiameter[0];
}

void T_runStep(tract_t *self, double glottalOutput, double turbulenceNoise, double lambda, glottis_t *glottis)
{
    double updateAmplitudes = ((double)rand() / (double)RAND_MAX) < 0.1;

    //mouth
    T_processTransients(self);
    T_addTurbulenceNoise(self, turbulenceNoise, glottis);

    //self->glottalReflection = -0.8 + 1.6 * Glottis.newTenseness;
    self->junctionOutputR[0] = self->L[0] * self->glottalReflection + glottalOutput;
    self->junctionOutputL[self->tractProps->n] = self->R[self->tractProps->n - 1] * self->lipReflection;

    for (int i = 1; i < self->tractProps->n; i++)
    {
        double r = self->reflection[i] * (1 - lambda) + self->newReflection[i] * lambda;
        double w = r * (self->R[i - 1] + self->L[i]);
        self->junctionOutputR[i] = self->R[i - 1] - w;
        self->junctionOutputL[i] = self->L[i] + w;
    }

    //now at junction with nose
    int i = self->tractProps->noseStart;
    double r = self->newReflectionLeft * (1 - lambda) + self->reflectionLeft * lambda;
    self->junctionOutputL[i] = r * self->R[i - 1] + (1 + r) * (self->noseL[0] + self->L[i]);
    r = self->newReflectionRight * (1 - lambda) + self->reflectionRight * lambda;
    self->junctionOutputR[i] = r * self->L[i] + (1 + r) * (self->R[i - 1] + self->noseL[0]);
    r = self->newReflectionNose * (1 - lambda) + self->reflectionNose * lambda;
    self->noseJunctionOutputR[0] = r * self->noseL[0] + (1 + r) * (self->L[i] + self->R[i - 1]);

    for (int i = 0; i < self->tractProps->n; i++)
    {
        self->R[i] = self->junctionOutputR[i] * 0.999;
        self->L[i] = self->junctionOutputL[i + 1] * 0.999;

        //self->R[i] = Math.clamp(self->junctionOutputR[i] * self->fade, -1, 1);
        //self->L[i] = Math.clamp(self->junctionOutputL[i+1] * self->fade, -1, 1);

        if (updateAmplitudes)
        {
            double amplitude = fabs(self->R[i] + self->L[i]);
            if (amplitude > self->maxAmplitude[i])
                self->maxAmplitude[i] = amplitude;
            else
                self->maxAmplitude[i] *= 0.999;
        }
    }

    self->lipOutput = self->R[self->tractProps->n - 1];

    //nose
    self->noseJunctionOutputL[self->tractProps->noseLength] = self->noseR[self->tractProps->noseLength - 1] * self->lipReflection;

    for (int i = 1; i < self->tractProps->noseLength; i++)
    {
        int w = self->noseReflection[i] * (self->noseR[i - 1] + self->noseL[i]);
        self->noseJunctionOutputR[i] = self->noseR[i - 1] - w;
        self->noseJunctionOutputL[i] = self->noseL[i] + w;
    }

    for (int i = 0; i < self->tractProps->noseLength; i++)
    {
        self->noseR[i] = self->noseJunctionOutputR[i] * self->fade;
        self->noseL[i] = self->noseJunctionOutputL[i + 1] * self->fade;

        //self->noseR[i] = Math.clamp(self->noseJunctionOutputR[i] * self->fade, -1, 1);
        //self->noseL[i] = Math.clamp(self->noseJunctionOutputL[i+1] * self->fade, -1, 1);

        if (updateAmplitudes)
        {
            double amplitude = fabs(self->noseR[i] + self->noseL[i]);
            if (amplitude > self->noseMaxAmplitude[i])
                self->noseMaxAmplitude[i] = amplitude;
            else
                self->noseMaxAmplitude[i] *= 0.999;
        }
    }

    self->noseOutput = self->noseR[self->tractProps->noseLength - 1];
}
