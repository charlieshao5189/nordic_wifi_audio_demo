/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _WIFI_AUDIO_RX_H_
#define _WIFI_AUDIO_RX_H_

#define START_SEQUENCE_1 0xFF
#define START_SEQUENCE_2 0xAA
#define END_SEQUENCE_1   0xFF
#define END_SEQUENCE_2   0xBB
#define SEND_CMD_SIGN    0x00
#define SEND_DATA_SIGN   0x01
#define AUDIO_START_CMD  0x00
#define AUDIO_STOP_CMD   0x01

void send_audio_command(uint8_t audio_command);

void send_audio_frame(uint8_t *audio_data, size_t data_length);

/**
 * @brief Data handler when audio data has been received through WiFi.
 *
 * @param[in] p_data		Pointer to the received data.
 * @param[in] data_size		Size of the received data.
 * @param[in] bad_frame		Bad frame flag. (I.e. set for missed ISO data).
 * @param[in] sdu_ref		SDU reference timestamp.
 * @param[in] channel_index	Which channel is received.
 * @param[in] desired_data_size	The expected data size.
 *
 * @return 0 if successful, error otherwise.
 */
// void wifi_audio_rx_data_handler(uint8_t const *const p_data, size_t data_size, bool bad_frame,
// 			      uint32_t sdu_ref, enum audio_channel channel_index,
// 			      size_t desired_data_size);

void wifi_audio_rx_data_handler(uint8_t *p_data, size_t data_size);
/**
 * @brief Initialize the receive audio path.
 *
 * @return 0 if successful, error otherwise.
 */
int wifi_audio_rx_init(void);

/**
 * @brief	Encoded audio data and information.
 *
 * @note	Container for SW codec (typically LC3) compressed audio data.
 */
struct le_audio_encoded_audio {
	uint8_t const *const data;
	size_t size;
	uint8_t num_ch;
};

#endif /* _WIFI_AUDIO_RX_H_ */
