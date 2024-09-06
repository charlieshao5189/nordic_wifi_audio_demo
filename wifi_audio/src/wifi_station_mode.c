/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_station_mode, CONFIG_LOG_DEFAULT_LEVEL);

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
/* Macro called upon a fatal error, reboots the device. */
#include <zephyr/sys/reboot.h>

/* Include the necessary header files */
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>
#include <net/wifi_mgmt_ext.h>
/* For net_sprint_ll_addr_buf */
#include "net_private.h"


/* Include the header file for the Wi-FI credentials library */
#include <net/wifi_credentials.h>

#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log_ctrl.h>

/* Define a macro for the relevant network events */
#define L2_EVENT_MASK (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)
#define L3_EVENT_MASK NET_EVENT_IPV4_DHCP_BOUND

/**********External Resources START**************/
extern struct k_sem wifi_net_ready;
/**********External Resources END**************/

/* Declare the callback structure for Wi-Fi events */
static struct net_mgmt_event_callback wifi_mgmt_cb;
static struct net_mgmt_event_callback net_mgmt_cb;

/* Define the boolean wifi_connected and the semaphore wifi_net_ready */
static bool wifi_connected;

static int cmd_wifi_status(void)
{
	struct net_if *iface;
	struct wifi_iface_status status = { 0 };

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-FI interface");
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
				sizeof(struct wifi_iface_status))) {
		LOG_ERR("Status request failed");
		return -ENOEXEC;
	}

	LOG_INF("Status: successful");
	LOG_INF("==================");
	LOG_INF("State: %s", wifi_state_txt(status.state));

	if (status.state >= WIFI_STATE_ASSOCIATED) {
		uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

		LOG_INF("Interface Mode: %s", wifi_mode_txt(status.iface_mode));
		LOG_INF("Link Mode: %s", wifi_link_mode_txt(status.link_mode));
		LOG_INF("SSID: %.32s", status.ssid);
		LOG_INF("BSSID: %s",
			net_sprint_ll_addr_buf(status.bssid,
					       WIFI_MAC_ADDR_LEN, mac_string_buf,
					       sizeof(mac_string_buf)));
		LOG_INF("Band: %s", wifi_band_txt(status.band));
		LOG_INF("Channel: %d", status.channel);
		LOG_INF("Security: %s", wifi_security_txt(status.security));
		LOG_INF("MFP: %s", wifi_mfp_txt(status.mfp));
		LOG_INF("Beacon Interval: %d", status.beacon_interval);
		LOG_INF("DTIM: %d", status.dtim_period);
		LOG_INF("TWT: %s",
			status.twt_capable ? "Supported" : "Not supported");
	}

	return 0;
}


static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
									uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event)
	{
	case NET_EVENT_WIFI_CONNECT_RESULT:
		LOG_INF("WiFi connected");
		wifi_connected = true;
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		if (wifi_connected == false)
		{
			LOG_INF("Waiting for WiFi to be wifi_connected");
		}
		else
		{
			dk_set_led_off(DK_LED1);
			LOG_INF("WiFi disconnected");
			wifi_connected = false;
		}
		k_sem_reset(&wifi_net_ready);
		break;
	default:
		break;
	}
	cmd_wifi_status();
}

static void on_net_event_dhcp_bound(struct net_mgmt_event_callback *cb)
{
	/* Get DHCP info from struct net_if_dhcpv4 and print */
	const struct net_if_dhcpv4 *dhcpv4 = cb->info;
	const struct in_addr *addr = &dhcpv4->requested_ip;
	char dhcp_info[128];
	net_addr_ntop(AF_INET, addr, dhcp_info, sizeof(dhcp_info));
	LOG_INF("\r\n\r\nWiFi is ready on nRF5340 Audio DK + nRF7002EK. Try to connect the socket(udp by default) to address %s:60010.\r\n", dhcp_info);
}

/* Define the callback function for network events */
static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
								   uint32_t mgmt_event, struct net_if *iface)
{
	if ((mgmt_event & L3_EVENT_MASK) != mgmt_event)
	{
		return;
	}

	if (mgmt_event == NET_EVENT_IPV4_DHCP_BOUND)
	{
		LOG_INF("Network DHCP bound");
		on_net_event_dhcp_bound(cb);
		dk_set_led_on(DK_LED1);
		k_sem_give(&wifi_net_ready);
		return;
	}
}

int wifi_station_mode_ready(void)
{

        /* Wait to allow initialization of Wi-Fi driver */
	k_sleep(K_SECONDS(1));

        /* Initialize and add the callback function for network events */
	net_mgmt_init_event_callback(&wifi_mgmt_cb, wifi_mgmt_event_handler, L2_EVENT_MASK);
	net_mgmt_add_event_callback(&wifi_mgmt_cb);

	net_mgmt_init_event_callback(&net_mgmt_cb, net_mgmt_event_handler, L3_EVENT_MASK);
	net_mgmt_add_event_callback(&net_mgmt_cb);

#if defined(CONFIG_WIFI_CREDENTIALS_STATIC)
	/* Declare the variable for the network configuration parameters */

	/* Get the network interface */
	struct net_if *iface = net_if_get_first_wifi();
	if (iface == NULL)
	{
		LOG_ERR("Returned network interface is NULL");
		return -1;
	}

	/* Call net_mgmt() to request the Wi-Fi connection */
	LOG_INF("Connecting to Wi-Fi with static crendentials.");
	int err = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0);
        if (err)
	{
		LOG_ERR("Connecting to Wi-Fi failed, err: %d", err);
		return ENOEXEC;
	}
#else
	LOG_INF("\r\n\r\nRunnning on WiFi Station mode.\r\nPlease connect to router with 'wifi_cred' commands, use 'wifi_cred help' to get help.\r\n");
#endif
        k_sem_take(&wifi_net_ready, K_FOREVER);
	return 0;
}
