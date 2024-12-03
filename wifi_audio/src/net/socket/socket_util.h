#ifndef _SOCKET_UTIL_H_
#define _SOCKET_UTIL_H_

#include <zephyr/kernel.h>

/*Wi-Fi default MTU*/
#define BUFFER_MAX_SIZE 1500

typedef struct {
	uint8_t buf[BUFFER_MAX_SIZE];
	size_t len;
} socket_receive_t;

extern struct k_msgq socket_recv_queue;

enum wifi_modes {
	WIFI_STATION_MODE = 0,
	WIFI_SOFTAP_MODE,
};

typedef void (*net_util_socket_rx_callback_t)(uint8_t *data, size_t len);

void socket_util_set_rx_callback(net_util_socket_rx_callback_t socket_rx_callback);
int socket_util_tx_data(uint8_t *data, size_t length);
void socket_util_thread(void);

#endif 