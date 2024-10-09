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
        size_t  size;
	uint8_t data[1920];
};

# define CONFIG_BUF_WIFI_RX_PACKET_NUM 5

DATA_FIFO_DEFINE(wifi_audio_rx, CONFIG_BUF_WIFI_RX_PACKET_NUM, sizeof(struct audio_pcm_data_t));

# define CONFIG_CODEC_OPUS

# ifdef CONFIG_CODEC_OPUS


# endif
static int16_t rx_data_continute_count=0;
void audio_data_frame_process(uint8_t *p_data, size_t data_size) {
        int ret;
	uint32_t blocks_alloced_num, blocks_locked_num;
	struct audio_pcm_data_t *data_received = NULL;
	static struct rx_stats rx_stats[AUDIO_CH_NUM];
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
                rx_data_continute_count=0;
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


#define TOTAL_PACKET_SIZE (1024 + 896)  // Total size of the two packets to be assembled
#define MAX_PACKET_SIZE 1920            // Maximum packet size after assembly (1024 + 896)

void wifi_audio_rx_data_handler(uint8_t *p_data, size_t data_size) {
    static uint8_t assembled_data[MAX_PACKET_SIZE];  // Buffer to hold the assembled data
    static size_t assembled_size = 0;                // Keeps track of how much data has been assembled

    // Ensure that the incoming data won't overflow the buffer
    if (assembled_size + data_size > MAX_PACKET_SIZE) {
        LOG_WRN("Data overflow: Incoming data exceeds buffer capacity.\r\n assembeled_size: %zu, data_size: %zu\n", assembled_size, data_size);
				assembled_size = 0; // Reset the buffer, discards the data
        return;
    }

    // Copy the incoming data into the assembly buffer
    memcpy(assembled_data + assembled_size, p_data, data_size);
    assembled_size += data_size;

    // Check if the buffer contains the full packet (1024 + 896 = 1920 bytes)
    if (assembled_size == TOTAL_PACKET_SIZE) {
        // Full packet has been received, process the assembled data
        LOG_INF("Assembled complete packet of size %zu\n", assembled_size);
        // You can now process the assembled data here...
        audio_data_frame_process(assembled_data, assembled_size) ;
        // Reset the buffer for future packets
        assembled_size = 0;
    } else {
        // Still waiting for more data
        LOG_INF("Waiting for more data... Assembled so far: %zu bytes\n", assembled_size);
    }
}	


/**
 * @brief	Receive data from BLE through a k_fifo and send to USB or audio datapath.
 */
static void audio_datapath_thread(void *dummy1, void *dummy2, void *dummy3)
{
	int ret;
	struct audio_pcm_data_t *iso_received = NULL;
	size_t iso_received_size;

	while (1) {
		ret = data_fifo_pointer_last_filled_get(&wifi_audio_rx, (void *)&iso_received,
							iso_received->size, K_FOREVER);
		ERR_CHK(ret);

		if (IS_ENABLED(CONFIG_AUDIO_SOURCE_USB) && IS_ENABLED(CONFIG_AUDIO_GATEWAY)) {
			// ret = audio_system_decode(iso_received->data, iso_received->data_size,
			// 			  iso_received->bad_frame);
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
