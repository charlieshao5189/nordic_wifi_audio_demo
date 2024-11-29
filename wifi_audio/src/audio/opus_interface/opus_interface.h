/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __OPUS_INTERFACE_H
#define __OPUS_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "opus.h"
#include "opus_defines.h"
#include "opus_private.h"

/* Exported types ------------------------------------------------------------*/
/**
 * @brief Opus codec configuration parameters.
 */
typedef enum {
	OPUS_SUCCESS = 0x00, /*!< Success.*/
	OPUS_ERROR = 0x01    /*!< Error.*/
} Opus_Status;

/* Private defines -----------------------------------------------------------*/
#define SILK_MODE   0x00
#define HYBRID_MODE 0x01
#define CELT_MODE   0x02

/**
 * @brief Opus codec configuration parameters.
 */
typedef struct {
	float ms_frame; /*!< ms of audio in a single frame */

	uint32_t sample_freq; /*!< Audio sampling frequency. */

	uint8_t channels; /*!< Number of audio input channels */

	uint16_t application; /*!< Specifies the application type (OPUS_APPLICATION_VOIP,
				 OPUS_APPLICATION_AUDIO). */

	uint32_t bitrate; /*!< Specifies the choosen encoding bitrate. */

	uint8_t complexity; /*!< Specifies the choosen encoding complexity. */

	uint8_t *pInternalMemory; /*!< Pointer to the internal memory */

} ENC_Opus_ConfigTypeDef;

/**
 * @brief Opus codec configuration parameters.
 */
typedef struct {
	float ms_frame; /*!< ms of audio in a single frame */

	uint32_t sample_freq; /*!< Audio sampling frequency. */

	uint8_t channels; /*!< Number of audio input channels */

	uint8_t *pInternalMemory; /*!< Pointer to the internal memory */

} DEC_Opus_ConfigTypeDef;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
uint32_t ENC_Opus_getMemorySize(ENC_Opus_ConfigTypeDef *EncConfigOpus);
uint32_t DEC_Opus_getMemorySize(DEC_Opus_ConfigTypeDef *DecConfigOpus);
Opus_Status ENC_Opus_Init(ENC_Opus_ConfigTypeDef *ENC_configOpus, int *opus_err);
void ENC_Opus_Deinit(void);
uint8_t ENC_Opus_IsConfigured(void);
Opus_Status DEC_Opus_Init(DEC_Opus_ConfigTypeDef *DEC_configOpus, int *opus_err);
void DEC_Opus_Deinit(void);
uint8_t DEC_Opus_IsConfigured(void);
Opus_Status ENC_Opus_Set_Bitrate(int bitrate, int *opus_err);
Opus_Status ENC_Opus_Set_CBR(void);
Opus_Status ENC_Opus_Set_VBR(void);
Opus_Status ENC_Opus_Set_Complexity(int complexity, int *opus_err);
Opus_Status ENC_Opus_Force_SILKmode(void);
Opus_Status ENC_Opus_Force_CELTmode(void);
int ENC_Opus_Encode(uint8_t *buf_in, uint8_t *buf_out);
int DEC_Opus_Decode(uint8_t *buf_in, uint32_t len, uint8_t *buf_out);

#ifdef __cplusplus
}
#endif

#endif /* __OPUS_INTERFACE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
