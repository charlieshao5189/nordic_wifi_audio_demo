/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief WiFi Transceiver
 */

#define MODULE main

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MAIN_LOG_LEVEL);
#include "streamctrl.h"

static enum stream_state strm_state = STATE_PAUSED;

uint8_t stream_state_get(void)
{
	return strm_state;
}

void streamctrl_send(void const *const data, size_t size, uint8_t num_ch)
{
	ARG_UNUSED(data);
	ARG_UNUSED(size);
	ARG_UNUSED(num_ch);

	LOG_WRN("Sending is not possible for broadcast sink");
}

int main(void)
{
    LOG_INF("WiFi Audio Transceiver Start!");

	int ret;

	ret = nrf5340_audio_dk_init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize audio DK: %d", ret);
		return ret;
	}
	
	ret = audio_system_init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize audio system: %d", ret);
		return ret;
	}

	ret = audio_usb_init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize audio USB: %d", ret);
		return ret;
	}

	audio_system_start();

	/* Play a test tone */
	ret = audio_datapath_tone_play(500,5000,1);
	if (ret != 0) {
		LOG_ERR("Failed to play tone: %d", ret);
		return ret;
	}

	/* TODO:
	 * With CONFIG_AUDIO_USB_PASS_THROUGH enabled it should be possible to pass audio data from USB to the audio codec.
	 * Usb inpt is started by calling audio_usb_start(fifo_tx_in, fifo_rx_in);
	 * Could this be setup with the same fifo as the audio_datapath?
	 * if not, look into the audio_datapath_thread in le_audio_rx.c to see how the audio data is passed to the audio codec.
	 */

	return 0;
}
