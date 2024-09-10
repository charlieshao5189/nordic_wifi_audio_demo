#ifndef __SOCKET_UTIL_H
#define __SOCKET_UTIL_H

#include <zephyr/kernel.h>

#define CAM_COMMAND_MAX_SIZE 6

typedef void (*socket_rx_callback_t)(uint8_t *data, uint16_t len);
typedef int (*network_ready_callback_t)();


void set_socket_rx_callback(socket_rx_callback_t socket_rx_callback);
void set_network_ready_callback(network_ready_callback_t network_ready_callback);

void socket_tx(const void *buf, size_t len);
uint8_t process_socket_rx(char *udp_rx_buf, char *command_buf);

#endif