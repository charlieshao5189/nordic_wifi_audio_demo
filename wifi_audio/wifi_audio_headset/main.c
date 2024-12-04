/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief WiFi Transceiver
 */

#define MODULE main

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/sys/byteorder.h>

#include "zbus_common.h"
#include "nrf5340_audio_dk.h"
#include "led.h"
#include "button_assignments.h"
#include "macros_common.h"
#include "audio_system.h"
#include "audio_datapath.h"
#include "fw_info_app.h"
#include "streamctrl.h"
#include "socket_util.h"
#include "wifi_audio_rx.h"
#include "hw_codec.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/heap_listener.h>
#include <zephyr/zbus/zbus.h>

#if defined(CONFIG_SOCKET_ROLE_CLIENT)
extern volatile bool serveraddr_set_signall;
#endif /*#if defined(CONFIG_SOCKET_ROLE_CLIENT)*/

LOG_MODULE_REGISTER(MODULE, CONFIG_MAIN_LOG_LEVEL);

ZBUS_SUBSCRIBER_DEFINE(button_evt_sub, CONFIG_BUTTON_MSG_SUB_QUEUE_SIZE);

ZBUS_MSG_SUBSCRIBER_DEFINE(le_audio_evt_sub);

ZBUS_CHAN_DECLARE(button_chan);
ZBUS_CHAN_DECLARE(le_audio_chan);
ZBUS_CHAN_DECLARE(bt_mgmt_chan);
// ZBUS_CHAN_DECLARE(sdu_ref_chan);

ZBUS_OBS_DECLARE(sdu_ref_msg_listen);

static struct k_thread button_msg_sub_thread_data;
static struct k_thread le_audio_msg_sub_thread_data;

static k_tid_t button_msg_sub_thread_id;
static k_tid_t le_audio_msg_sub_thread_id;

struct bt_le_ext_adv *ext_adv;

K_THREAD_STACK_DEFINE(button_msg_sub_thread_stack, CONFIG_BUTTON_MSG_SUB_STACK_SIZE);
K_THREAD_STACK_DEFINE(le_audio_msg_sub_thread_stack, CONFIG_LE_AUDIO_MSG_SUB_STACK_SIZE);

static enum stream_state strm_state = STATE_PAUSED;

#ifdef CONFIG_SYS_HEAP_LISTENER
extern struct sys_heap _system_heap;
struct sys_memory_stats system_heap_stats;
uint32_t system_heap_free = 0;
uint32_t system_heap_used = 0;
uint32_t system_heap_max_used = 0;

void on_system_heap_alloc(uintptr_t heap_id, void *mem, size_t bytes)
{
	if (heap_id == HEAP_ID_FROM_POINTER(&_system_heap)) {
		sys_heap_runtime_stats_get((struct sys_heap *)&_system_heap.heap,
					   &system_heap_stats);
		system_heap_used = (uint32_t)system_heap_stats.allocated_bytes;
		system_heap_max_used = (uint32_t)system_heap_stats.max_allocated_bytes;
		system_heap_free = (uint32_t)system_heap_stats.free_bytes;
		LOG_INF("system_heap ALLOC %zu. Heap state: allocated %zu, free %zu, max allocated "
			"%zu, heap size %u.\n",
			bytes, system_heap_free, system_heap_used, system_heap_max_used,
			K_HEAP_MEM_POOL_SIZE);
	}
}

void on_system_heap_free(uintptr_t heap_id, void *mem, size_t bytes)
{
	if (heap_id == HEAP_ID_FROM_POINTER(&_system_heap)) {
		sys_heap_runtime_stats_get((struct sys_heap *)&_system_heap.heap,
					   &system_heap_stats);
		system_heap_used = (uint32_t)system_heap_stats.allocated_bytes;
		system_heap_max_used = (uint32_t)system_heap_stats.max_allocated_bytes;
		system_heap_free = (uint32_t)system_heap_stats.free_bytes;
		LOG_INF("system_heap ALLOC %zu. Heap state: allocated %zu, free %zu, max allocated "
			"%zu, heap size %u.\n",
			bytes, system_heap_free, system_heap_used, system_heap_max_used,
			K_HEAP_MEM_POOL_SIZE);
	}
}

HEAP_LISTENER_ALLOC_DEFINE(system_heap_listener_alloc, HEAP_ID_FROM_POINTER(&_system_heap),
			   on_system_heap_alloc);
HEAP_LISTENER_FREE_DEFINE(system_heap_listener_free, HEAP_ID_FROM_POINTER(&_system_heap),
			  on_system_heap_free);

#endif /* #ifdef HEAP_LISTENER */

/* Function for handling all stream state changes */
static void stream_state_set(enum stream_state stream_state_new)
{
	LOG_INF("Stream state changed from %d to %d", strm_state, stream_state_new);
	strm_state = stream_state_new;
}

uint8_t stream_state_get(void)
{
	return strm_state;
}

void streamctrl_send(void const *const data, size_t size)
{
	int ret = 0;
	static int prev_ret;

	if (strm_state == STATE_STREAMING) {
		// ret = broadcast_source_send(0, enc_audio);

		if (ret != 0 && ret != prev_ret) {
			if (ret == -ECANCELED) {
				LOG_WRN("Sending operation cancelled");
			} else {
				LOG_WRN("Problem with sending LE audio data, ret: %d", ret);
			}
		}

		prev_ret = ret;
	}
}

/**
 * @brief	Handle button activity.
 */
static void button_msg_sub_thread(void)
{
	int ret;
	const struct zbus_channel *chan;

	while (1) {
		ret = zbus_sub_wait(&button_evt_sub, &chan, K_FOREVER);
		ERR_CHK(ret);

		struct button_msg msg;

		ret = zbus_chan_read(chan, &msg, ZBUS_READ_TIMEOUT_MS);
		ERR_CHK(ret);

		LOG_DBG("Got btn evt from queue - id = %d, action = %d", msg.button_pin,
			msg.button_action);

		if (msg.button_action != BUTTON_PRESS) {
			LOG_WRN("Unhandled button action");
			return;
		}

		switch (msg.button_pin) {
		case BUTTON_PLAY_PAUSE:
			if (serveraddr_set_signall == true) {
				if (strm_state == STATE_STREAMING) {
					send_audio_command(AUDIO_STOP_CMD);
					stream_state_set(STATE_PAUSED);
					ret = led_on(LED_APP_1_BLUE);
					ERR_CHK(ret);

				} else if (strm_state == STATE_PAUSED) {
					stream_state_set(STATE_STREAMING);
					send_audio_command(AUDIO_START_CMD);
					ret = led_blink(LED_APP_1_BLUE);
					ERR_CHK(ret);

				} else {
					LOG_WRN("In invalid state: %d", strm_state);
				}
			} else {
				LOG_WRN("Please set socket server address first!");
			}
			break;

		case BUTTON_4:
			if (IS_ENABLED(CONFIG_AUDIO_TEST_TONE)) {
				if (strm_state != STATE_STREAMING) {
					LOG_WRN("Not in streaming state");
					break;
				}

				ret = audio_system_encode_test_tone_step();
				if (ret) {
					LOG_WRN("Failed to play test tone, ret: %d", ret);
				}

				break;
			}

			break;

		case BUTTON_VOLUME_UP:
			if (strm_state != STATE_STREAMING) {
				LOG_WRN("Not in streaming state");
				break;
			}
			/* TODO: Should be implemented the same way as nrf5340_audio to
			 * allow for bidirectional volume control, this is a temporary
			 * solution */
			ret = hw_codec_volume_increase();
			if (ret) {
				LOG_ERR("Failed to increase volume, ret: %d", ret);
			}
			break;

		case BUTTON_VOLUME_DOWN:
			if (strm_state != STATE_STREAMING) {
				LOG_WRN("Not in streaming state");
				break;
			}
			/* TODO: Should be implemented the same way as nrf5340_audio to
			 * allow for bidirectional volume control, this is a temporary
			 * solution */
			ret = hw_codec_volume_decrease();
			if (ret) {
				LOG_ERR("Failed to decrease volume, ret: %d", ret);
			}
			break;

		default:
			LOG_WRN("Unexpected/unhandled button id: %d", msg.button_pin);
		}

		STACK_USAGE_PRINT("button_msg_thread", &button_msg_sub_thread_data);
	}
}

/**
 * @brief	Handle Bluetooth LE audio events.
 */
static void le_audio_msg_sub_thread(void)
{
	int ret;
	const struct zbus_channel *chan;

	while (1) {
		struct le_audio_msg msg;

		ret = zbus_sub_wait_msg(&le_audio_evt_sub, &chan, &msg, K_FOREVER);
		ERR_CHK(ret);

		LOG_DBG("Received event = %d, current state = %d", msg.event, strm_state);

		switch (msg.event) {
		case LE_AUDIO_EVT_STREAMING:
			LOG_DBG("LE audio evt streaming");

			audio_system_encoder_start();

			if (strm_state == STATE_STREAMING) {
				LOG_DBG("Got streaming event in streaming state");
				break;
			}

			audio_system_start();
			stream_state_set(STATE_STREAMING);
			ret = led_blink(LED_APP_1_BLUE);
			ERR_CHK(ret);

			break;

		case LE_AUDIO_EVT_NOT_STREAMING:
			LOG_DBG("LE audio evt not_streaming");

			audio_system_encoder_stop();

			if (strm_state == STATE_PAUSED) {
				LOG_DBG("Got not_streaming event in paused state");
				break;
			}

			stream_state_set(STATE_PAUSED);
			audio_system_stop();
			ret = led_on(LED_APP_1_BLUE);
			ERR_CHK(ret);

			break;

		default:
			LOG_WRN("Unexpected/unhandled le_audio event: %d", msg.event);

			break;
		}

		STACK_USAGE_PRINT("le_audio_msg_thread", &le_audio_msg_sub_thread_data);
	}
}
/**
 * @brief	Create zbus subscriber threads.
 *
 * @return	0 for success, error otherwise.
 */
static int zbus_subscribers_create(void)
{
	int ret;

	button_msg_sub_thread_id = k_thread_create(
		&button_msg_sub_thread_data, button_msg_sub_thread_stack,
		CONFIG_BUTTON_MSG_SUB_STACK_SIZE, (k_thread_entry_t)button_msg_sub_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_BUTTON_MSG_SUB_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(button_msg_sub_thread_id, "BUTTON_MSG_SUB");
	if (ret) {
		LOG_ERR("Failed to create button_msg thread");
		return ret;
	}

	le_audio_msg_sub_thread_id = k_thread_create(
		&le_audio_msg_sub_thread_data, le_audio_msg_sub_thread_stack,
		CONFIG_LE_AUDIO_MSG_SUB_STACK_SIZE, (k_thread_entry_t)le_audio_msg_sub_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_LE_AUDIO_MSG_SUB_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(le_audio_msg_sub_thread_id, "LE_AUDIO_MSG_SUB");
	if (ret) {
		LOG_ERR("Failed to create le_audio_msg thread");
		return ret;
	}

	// ret = zbus_chan_add_obs(&sdu_ref_chan, &sdu_ref_msg_listen,
	// ZBUS_ADD_OBS_TIMEOUT_MS); if (ret) { 	LOG_ERR("Failed to add timestamp
	// listener"); 	return ret;
	// }

	return 0;
}

/**
 * @brief	Link zbus producers and observers.
 *
 * @return	0 for success, error otherwise.
 */
static int zbus_link_producers_observers(void)
{
	int ret;

	if (!IS_ENABLED(CONFIG_ZBUS)) {
		return -ENOTSUP;
	}

	ret = zbus_chan_add_obs(&button_chan, &button_evt_sub, ZBUS_ADD_OBS_TIMEOUT_MS);
	if (ret) {
		LOG_ERR("Failed to add button sub");
		return ret;
	}

	// ret = zbus_chan_add_obs(&le_audio_chan, &le_audio_evt_sub,
	// ZBUS_ADD_OBS_TIMEOUT_MS); if (ret) { 	LOG_ERR("Failed to add le_audio
	// sub"); 	return ret;
	// }

	// ret = zbus_chan_add_obs(&bt_mgmt_chan, &bt_mgmt_evt_listen,
	// ZBUS_ADD_OBS_TIMEOUT_MS); if (ret) { 	LOG_ERR("Failed to add bt_mgmt
	// listener"); 	return ret;
	// }

	return 0;
}

K_THREAD_STACK_DEFINE(socket_util_thread_stack, CONFIG_SOCKET_STACK_SIZE);
static struct k_thread socket_util_thread_data;
static k_tid_t socket_util_thread_id;

int socket_util_init(void)
{
	int ret;
	/* Start thread to handle events from socket connection */
	socket_util_thread_id = k_thread_create(
		&socket_util_thread_data, socket_util_thread_stack, CONFIG_SOCKET_STACK_SIZE,
		(k_thread_entry_t)socket_util_thread, NULL, NULL, NULL,
		K_PRIO_PREEMPT(CONFIG_SOCKET_UTIL_THREAD_PRIO), 0, K_NO_WAIT);

	ret = k_thread_name_set(socket_util_thread_id, "SOCKET");
	socket_util_set_rx_callback(wifi_audio_rx_data_handler);
	return ret;
}

int main(void)
{
	int ret;
	LOG_INF("WiFi Audio Transceiver Start!");

#ifdef CONFIG_SYS_HEAP_LISTENER

	heap_listener_register(&system_heap_listener_alloc);
	heap_listener_register(&system_heap_listener_free);

#endif /* #ifdef HEAP_LISTENER */

	ret = nrf5340_audio_dk_init();
	ERR_CHK(ret);

	ret = fw_info_app_print();
	ERR_CHK(ret);

	ret = socket_util_init();
	ERR_CHK(ret);

	LOG_INF("audio_system_init");
	ret = audio_system_init();
	ERR_CHK(ret);

	ret = audio_system_config_set(48000, 0, 48000);
	ERR_CHK_MSG(ret, "Failed to set sample rate for decoder");

	audio_system_start();

	ret = zbus_subscribers_create();
	ERR_CHK_MSG(ret, "Failed to create zbus subscriber threads");

	ret = zbus_link_producers_observers();
	ERR_CHK_MSG(ret, "Failed to link zbus producers and observers");

	ret = wifi_audio_rx_init();
	ERR_CHK_MSG(ret, "Failed to initialize rx path");

	return 0;
}
