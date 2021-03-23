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

    float tongueX = 0.0;
    float tongueY = 0.0;
    float constrictionX = 0.0;
    float constrictionY = 0.0;
    float fricativeIntensity = 0.0;
    bool muteAudio = false;
    bool constrictionActive = false;

    t_tractProps *getTractProps();

    t_tractProps tractProps;
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
