/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief WiFi Transceiver
 */

#define MODULE main

//#include <zephyr/logging/log.h>
//LOG_MODULE_REGISTER(MODULE, CONFIG_MAIN_LOG_LEVEL);
#include "streamctrl.h"
#include "audio/audio_codec_opus_api.h"

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

	//LOG_WRN("Sending is not possible for broadcast sink");
}

int main(void)
{
    //LOG_INF("WiFi Audio Transceiver Start!");
    audio_codec_opus_init();
}
