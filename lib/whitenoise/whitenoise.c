//
//  WhiteNoise.cpp
//  PinkTrombone - All
//
//  Created by Samuel Tarakajian on 8/30/19.
//

#include "whitenoise.h"
#include <stdlib.h>
#include <math.h>

void WH_init(whitenoise_t *self, long sampleLength)
{
    self->index = 0;
    self->size = sampleLength;
    self->buffer = (double *)malloc(sampleLength * sizeof(double));

    for (long i = 0; i < self->size; i++)
    {
        self->buffer[i] = ((double)rand() / (double)RAND_MAX) * 2.0 - 1.0;
    }
}

void WH_destroy(whitenoise_t *self)
{
    free(self->buffer);
    self->buffer = NULL;
}

double WH_runStep(whitenoise_t *self)
{
    double ov = self->buffer[self->index];
    self->index = self->index + 1;
    if (self->index >= self->size)
        self->index = 0;
    return ov;
}
