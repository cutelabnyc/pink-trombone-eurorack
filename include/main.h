/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "biquad.h"
#include "whitenoise.h"
#include "tract.h"
#include <stdbool.h>

#define I2S_BUFFER_SIZE 512
#define SAMPLE_RATE hi2s3.Init.AudioFreq

    void Error_Handler(void);

    // NOTE: This is where the vocal constriction parameters are set. Assign potentiometers
    // to these in the main loop above to make some cool sounds. The range is 0.0 to 1.0 for
    // each variable.

    float tongueX = 1.0;
    float tongueY = 0.3;
    float constrictionX = 0.5;
    float constrictionY = 0.3;
    float fricativeIntensity = 0.6;
    bool muteAudio = false;
    bool constrictionActive = true;

    t_tractProps *getTractProps();

    t_tractProps *tractProps;
    glottis_t *glottis;
    tract_t *tract;
    whitenoise_t *whiteNoise;
    biquad_t *fricativeFilter;
    biquad_t *aspirateFilter;

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
