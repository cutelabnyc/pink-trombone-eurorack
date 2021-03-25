//
//  WhiteNoise.hpp
//  PinkTrombone - All
//
//  Created by Samuel Tarakajian on 8/30/19.
//

#ifndef _WHITENOISE_H
#define _WHITENOISE_H

#include <stdio.h>

typedef struct whitenoise
{
    long index;
    double *buffer;
    long size;
} whitenoise_t;

whitenoise_t *WH_init(long sampleLength);
void WH_destroy(whitenoise_t *self);
double WH_runStep(whitenoise_t *self);

#endif /* _WHITENOISE_H */
