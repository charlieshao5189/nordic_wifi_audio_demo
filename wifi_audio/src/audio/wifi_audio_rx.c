/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrfx_clock.h>

#include "streamctrl.h"
#include "audio_datapath.h"
#include "macros_common.h"
#include "audio_system.h"
#include "audio_sync_timer.h"
#include "wifi_audio_rx.h"
#include "socket_util.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_audio_rx, CONFIG_WIFI_AUDIO_RX_LOG_LEVEL);

struct ble_iso_data {
	// uint8_t data[251];
	uint8_t data[1920];
	size_t data_size;
	bool bad_frame;
	uint32_t sdu_ref;
	uint32_t recv_frame_ts;
} __packed;

struct rx_stats {
	uint32_t recv_cnt;
	uint32_t bad_frame_cnt;
	uint32_t data_size_mismatch_cnt;
};

static bool initialized;
static struct k_thread audio_datapath_thread_data;
static k_tid_t audio_datapath_thread_id;
K_THREAD_STACK_DEFINE(audio_datapath_thread_stack, CONFIG_AUDIO_DATAPATH_STACK_SIZE);

struct audio_pcm_data_t {
	size_t size;
	uint8_t data[1920];
};

#define CONFIG_BUF_WIFI_RX_PACKET_NUM 5

DATA_FIFO_DEFINE(wifi_audio_rx, CONFIG_BUF_WIFI_RX_PACKET_NUM, sizeof(struct audio_pcm_data_t));

#define CONFIG_CODEC_OPUS

#ifdef CONFIG_CODEC_OPUS

#endif
static int16_t rx_data_continute_count = 0;
void audio_data_frame_process(uint8_t *p_data, size_t data_size)
{
	int ret;
	uint32_t blocks_alloced_num, blocks_locked_num;
	struct audio_pcm_data_t *data_received = NULL;
	// static struct rx_stats rx_stats[AUDIO_CH_NUM];
	static uint32_t num_overruns;
	static uint32_t num_thrown;

	if (!initialized) {
		ERR_CHK_MSG(-EPERM, "Data received but wifi_audio_rx is not initialized");
	}

	// /* Capture timestamp of when audio frame is received */
	// uint32_t recv_frame_ts = audio_sync_timer_capture();

	// rx_stats[channel_index].recv_cnt++;

	// if (data_size != desired_data_size) {
	// 	/* A valid frame should always be equal to desired_data_size, set bad_frame
	// 	 * if that is not the case
	// 	 */
	// 	bad_frame = true;
	// 	rx_stats[channel_index].data_size_mismatch_cnt++;
	// }

	// if (bad_frame) {
	// 	rx_stats[channel_index].bad_frame_cnt++;
	// }

	// if ((rx_stats[channel_index].recv_cnt % 100) == 0 && rx_stats[channel_index].recv_cnt) {
	// 	/* NOTE: The string below is used by the Nordic CI system */
	// 	LOG_DBG("ISO RX SDUs: Ch: %d Total: %d Bad: %d Size mismatch %d", channel_index,
	// 		rx_stats[channel_index].recv_cnt, rx_stats[channel_index].bad_frame_cnt,
	// 		rx_stats[channel_index].data_size_mismatch_cnt);
	// }

	if (stream_state_get() != STATE_STREAMING) {
		/* Throw away data */
		num_thrown++;
		if ((num_thrown % 100) == 1) {
			LOG_WRN("Not in streaming state (%d), thrown %d packet(s)",
				stream_state_get(), num_thrown);
		}
		return;
	}

	// if (channel_index != AUDIO_CH_L && IS_ENABLED(CONFIG_AUDIO_GATEWAY)) {
	// 	/* Only left channel RX data in use on gateway */
	// 	return;
	// }

	ret = data_fifo_num_used_get(&wifi_audio_rx, &blocks_alloced_num, &blocks_locked_num);
	ERR_CHK(ret);

	if (blocks_alloced_num >= CONFIG_BUF_WIFI_RX_PACKET_NUM) {
		/* FIFO buffer is full, swap out oldest frame for a new one */

		void *stale_data;
		size_t stale_size;
		num_overruns++;

		if ((num_overruns % 100) == 1) {
			LOG_WRN("BLE ISO RX overrun: Num: %d", num_overruns);
		}

		ret = data_fifo_pointer_last_filled_get(&wifi_audio_rx, &stale_data, &stale_size,
							K_NO_WAIT);
		ERR_CHK(ret);

		data_fifo_block_free(&wifi_audio_rx, stale_data);
		rx_data_continute_count = 0;
	}

	ret = data_fifo_pointer_first_vacant_get(&wifi_audio_rx, (void *)&data_received, K_NO_WAIT);
	ERR_CHK_MSG(ret, "Unable to get FIFO pointer");

	if (data_size > ARRAY_SIZE(data_received->data)) {
		ERR_CHK_MSG(-ENOMEM, "Data size too large for buffer");
		return;
	}

	// memcpy(iso_received->data, p_data+2, data_size-2);
	memcpy(data_received->data, p_data, data_size);
	// iso_received->bad_frame = bad_frame;
	data_received->size = data_size;
	// iso_received->sdu_ref = sdu_ref;
	// iso_received->recv_frame_ts = recv_frame_ts;

	ret = data_fifo_block_lock(&wifi_audio_rx, (void *)&data_received,
				   sizeof(struct audio_pcm_data_t));
	ERR_CHK_MSG(ret, "Failed to lock block");
}

#define TOTAL_PACKET_SIZE (1024 + 896) // Total size of the two packets to be assembled

#define MAX_AUDIO_FRAME_SIZE 1920
#define HEADER_SIZE          3 // Start sequence (2 bytes) + identifier (1 byte)
#define FOOTER_SIZE          2 // End sequence (2 bytes)
#define FULL_FRAME_SIZE      (HEADER_SIZE + MAX_AUDIO_FRAME_SIZE + FOOTER_SIZE)

void wifi_audio_rx_data_handler(uint8_t *p_data, size_t data_size)
{

	// LOG_DBG("Received data size: %d", data_size);
	// LOG_HEXDUMP_INF(p_data, data_size, "Received data:");
	// Static buffer to store accumulated data
	static uint8_t frame_buffer[FULL_FRAME_SIZE]; // Buffer sized for a full frame with headers
						      // and footers
	static size_t current_frame_size = 0;

	// Copy incoming data chunk to frame buffer if it fits
	if (current_frame_size + data_size > FULL_FRAME_SIZE) {
		LOG_ERR("Frame buffer overflow, discarding accumulated data.");
		current_frame_size = 0; // Reset if overflowed
		return;
	}

	memcpy(frame_buffer + current_frame_size, p_data, data_size);
	current_frame_size += data_size;

	// Check if we have at least the minimum size for a complete frame
	if (current_frame_size >= HEADER_SIZE + FOOTER_SIZE) {
		// Verify start sequence
		if (frame_buffer[0] == START_SEQUENCE_1 && frame_buffer[1] == START_SEQUENCE_2) {
			// Check for end sequence at the expected position
			if (current_frame_size >= HEADER_SIZE + FOOTER_SIZE &&
			    frame_buffer[current_frame_size - 2] == END_SEQUENCE_1 &&
			    frame_buffer[current_frame_size - 1] == END_SEQUENCE_2) {

				// Verify the identifier
				if (frame_buffer[2] == SEND_DATA_SIGN) {
					// Calculate audio data length
					size_t audio_data_length =
						current_frame_size - HEADER_SIZE - FOOTER_SIZE;
					if (audio_data_length <= MAX_AUDIO_FRAME_SIZE) {
						// Process the audio data
						audio_data_frame_process(frame_buffer + HEADER_SIZE,
									 audio_data_length);
						LOG_DBG("Audio frame data length: %d",
							audio_data_length);
					} else {
						LOG_ERR("Audio data length exceeds maximum frame "
							"size.");
					}
				} else {
					LOG_ERR("Unexpected data identifier.");
				}

				// Reset buffer for the next frame
				current_frame_size = 0;
			}
		} else {
			LOG_WRN("Invalid start sequence, discarding packet.");
			current_frame_size = 0; // Reset on invalid start sequence
		}
	}
}

/**
 * @brief	Receive data from BLE through a k_fifo and send to USB or audio datapath.
 */
static void audio_datapath_thread(void *dummy1, void *dummy2, void *dummy3)
{
	int ret;
	struct audio_pcm_data_t *iso_received = NULL;
	size_t size_received;

	while (1) {
		ret = data_fifo_pointer_last_filled_get(&wifi_audio_rx, (void *)&iso_received,
							&size_received, K_FOREVER);
		ERR_CHK(ret);

		if (IS_ENABLED(CONFIG_AUDIO_SOURCE_USB) && IS_ENABLED(CONFIG_AUDIO_GATEWAY)) {
			// ret = audio_system_decode(iso_received->data, iso_received->data_size,
			//                          iso_received->bad_frame);
			ERR_CHK(ret);
		} else {
			audio_datapath_stream_out(iso_received->data, iso_received->size);
		}
		data_fifo_block_free(&wifi_audio_rx, (void *)iso_received);

		STACK_USAGE_PRINT("audio_datapath_thread", &audio_datapath_thread_data);
	}
}

static int audio_datapath_thread_create(void)
{
	int ret;

	audio_datapath_thread_id = k_thread_create(
		&audio_datapath_thread_data, audio_datapath_thread_stack,
		CONFIG_AUDIO_DATAPATH_STACK_SIZE, (k_thread_entry_t)audio_datapath_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_AUDIO_DATAPATH_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(audio_datapath_thread_id, "AUDIO_DATAPATH");
	if (ret) {
		LOG_ERR("Failed to create audio_datapath thread");
		return ret;
	}

	return 0;
}

int wifi_audio_rx_init(void)
{
	int ret;

	if (initialized) {
		return -EALREADY;
	}

	ret = data_fifo_init(&wifi_audio_rx);
	if (ret) {
		LOG_ERR("Failed to set up ble_rx FIFO");
		return ret;
	}

	ret = audio_datapath_thread_create();
	if (ret) {
		return ret;
	}

	initialized = true;

	return 0;
}

void send_audio_command(uint8_t audio_command)
{
	// Define the command packet with placeholders for start, command, and end
	uint8_t command_packet[] = {
		START_SEQUENCE_1, // 0xFF
		START_SEQUENCE_2, // 0xAA
		SEND_CMD_SIGN,    // 0x00 command; 0x01 data
		audio_command,    // Command: Variable (e.g., AUDIO_START_CMD or AUDIO_STOP_CMD)
		END_SEQUENCE_1,   // 0xFF
		END_SEQUENCE_2    // 0xBB
	};

	size_t packet_size = sizeof(command_packet); // Calculate packet size
	socket_util_tx_data((uint8_t *)command_packet, packet_size);
}

void send_audio_frame(uint8_t *audio_data, size_t data_length)
{
	// Define the data packet size, including start and end sequences
	size_t total_packet_size = 5 + data_length; // 4 bytes for headers + data_length

	// Create a buffer for the complete data packet
	uint8_t *data_packet = (uint8_t *)k_malloc(total_packet_size);
	if (data_packet == NULL) {
		LOG_ERR("Memory allocation failed for data_packet.");
		return;
	}

	// Fill the data packet with the specified format
	data_packet[0] = START_SEQUENCE_1; // 0xFF
	data_packet[1] = START_SEQUENCE_2; // 0xAA
	data_packet[2] = SEND_DATA_SIGN;   // Data identifier (can be changed as needed)

	// Copy the audio data into the packet starting at the offset for the actual data
	bytecpy(data_packet + 3, audio_data, data_length); // 3rd byte is data identifier

	// Fill the end header
	data_packet[total_packet_size - 2] = END_SEQUENCE_1; // 0xFF
	data_packet[total_packet_size - 1] = END_SEQUENCE_2; // 0xBB

	// Send the prepared data packet
	socket_util_tx_data(data_packet, total_packet_size);

	// Free allocated memory
	k_free(data_packet);
}