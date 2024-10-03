/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(socket_util, CONFIG_SOCKET_UTIL_MODULE_LOG_LEVEL);

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/net/socket.h>
/* Macro called upon a fatal error, reboots the device. */
#include <zephyr/sys/reboot.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log_ctrl.h>
#include "socket_util.h"

#define FATAL_ERROR()                              \
	LOG_ERR("Fatal error! Rebooting the device."); \
	LOG_PANIC();                                   \
	IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)))

/* size of stack area used by each thread */
#define STACKSIZE 8192
/* scheduling priority used by each thread */
#define PRIORITY 3


//#define pc_port  60000
#define socket_port 60010 // use for either udp or tcp server


/**********External Resources START**************/
extern int wifi_station_mode_ready(void);
extern int wifi_softap_mode_ready(void);
/**********External Resources END**************/

int udp_socket;
int tcp_server_listen_fd;
int tcp_server_socket;
struct sockaddr_in self_addr;
bool socket_connected=false;

char target_addr_str[32];
struct sockaddr_in target_addr;
socklen_t target_addr_len=sizeof(target_addr);

/* Define the semaphore net_connect_ready */
struct k_sem net_connect_ready;
enum wifi_modes wifi_mode=WIFI_STATION_MODE;



static socket_receive_t socket_receive;

K_MSGQ_DEFINE(socket_recv_queue, sizeof(socket_receive), 1, 4);

static net_util_socket_rx_callback_t socket_rx_cb = 0; 

void socket_util_set_rx_callback(net_util_socket_rx_callback_t socket_rx_callback)
{
	socket_rx_cb = socket_rx_callback;

	// If any messages are waiting in the queue, forward them immediately
	socket_receive_t socket_receive;
	while (k_msgq_get(&socket_recv_queue, &socket_receive, K_NO_WAIT) == 0) {
		socket_rx_cb(socket_receive.buf, socket_receive.len);
	}
}

uint8_t process_socket_rx_buffer(char *socket_rx_buf, char *command_buf)
{
	uint8_t command_length = 0;

	// // Check if socket_rx_buf contains start code (0x55)
	// if (socket_rx_buf[0] == 0x55)
	// {
	// 	// Find the end code (0xAA) and extract the command
	// 	for (uint8_t i = 1; i < CAM_COMMAND_MAX_SIZE; i++)
	// 	{
	// 		if (socket_rx_buf[i] == 0xAA)
	// 		{
	// 			// Copy the command to command_buf
	// 			for (uint8_t j = 1; j < i; j++)
	// 			{
	// 				command_buf[j - 1] = socket_rx_buf[j];
	// 			}
	// 			command_length = i - 1; // Length of the command
	// 			break;
	// 		}
	// 	}
	// }
	return command_length;
}

static void socket_util_trigger_rx_callback_if_set()
{
        LOG_DBG("Socket received %d bytes", socket_receive.len);
        // LOG_HEXDUMP_DBG(socket_receive.buf, socket_receive.len, "Buffer contents(HEX):");
	if (socket_rx_cb != 0) {
		socket_rx_cb(socket_receive.buf, socket_receive.len);
	} else {
		k_msgq_put(&socket_recv_queue, &socket_receive, K_NO_WAIT);
	}
}

int socket_util_tx_data(uint8_t *data, size_t length)
{

    size_t chunk_size = 1024;
    ssize_t bytes_sent;

    while (length > 0) {
        size_t to_send = (length >= chunk_size) ? chunk_size : length;

        #if defined(USE_TCP_SOCKET)
            bytes_sent = send(tcp_server_socket, data, to_send, 0);
        #else
            bytes_sent = sendto(udp_socket, data, to_send, 0, (struct sockaddr *)&target_addr, sizeof(target_addr));
        #endif

        if (bytes_sent == -1) {
            perror("Sending failed");
            
            #if defined(USE_TCP_SOCKET)
                close(tcp_server_socket);
            #else
                close(udp_socket);
            #endif

            FATAL_ERROR();
            return;
        }

        data += bytes_sent;
        length -= bytes_sent;
    }
    return bytes_sent;
}

/* Thread to setup WiFi, Sockets step by step */
void socket_util_thread(void)
{
	int ret;

	k_sem_init(&net_connect_ready, 0, 1);

	if (dk_leds_init() != 0)
	{
		LOG_ERR("Failed to initialize the LED library");
	}

        #if defined(CONFIG_NRF700X_AP_MODE)
                ret = wifi_softap_mode_ready();
        #else
                ret = wifi_station_mode_ready();
        #endif
        
	if (ret < 0)
	{
		LOG_ERR("wifi network connection is not ready, error: %d", -errno);
		FATAL_ERROR();
		return;
	}

        //TODO:Change All camaddr to dev_addr
	self_addr.sin_family = AF_INET;
	self_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	self_addr.sin_port = htons(socket_port);


        target_addr.sin_family = AF_INET,
        target_addr.sin_port = htons(60010),  // Convert port to network byte order
        // Set the IP address (convert from presentation format to network format)
        inet_pton(AF_INET, "192.168.50.199", &(target_addr.sin_addr));

	#if defined(USE_TCP_SOCKET)
		tcp_server_listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		
		if (tcp_server_listen_fd < 0)
		{
			LOG_ERR("Failed to create socket: %d", -errno);
			FATAL_ERROR();
			return;
		}
		
		ret = bind(tcp_server_listen_fd, (struct sockaddr *)&self_addr, sizeof(self_addr));
		if (ret < 0)
		{
			LOG_ERR("bind, error: %d", -errno);
			FATAL_ERROR();
			return;
		}

		ret = listen(tcp_server_listen_fd, 5);
		if (ret < 0)
		{
			LOG_ERR("listen, error: %d", -errno);
			FATAL_ERROR();
			return;
		}

		while(1){
			tcp_server_socket = accept(tcp_server_listen_fd, (struct sockaddr *)&target_addr, &target_addr_len);
			if (tcp_server_socket < 0)
			{
				LOG_ERR("accept, error: %d", -errno);
				FATAL_ERROR();
				return;
			}
			LOG_INF("Accepted connection from client\n");
			inet_ntop(target_addr.sin_family, &target_addr.sin_addr, target_addr_str, sizeof(target_addr_str));
			LOG_INF("Connect socket client to IP Address %s:%d\n", target_addr_str, ntohs(target_addr.sin_port));
			// Handle the client connection
			while((socket_receive.len = recv(tcp_server_socket, socket_receive.buf, BUFFER_MAX_SIZE,0))>0){
				k_msgq_put(&socket_recv_queue, &socket_receive, K_FOREVER);
                //socket_util_trigger_rx_callback_if_set();
			}
			if (socket_receive.len == -1) {
				LOG_ERR("Receiving failed");
			} else if (socket_receive.len == 0) {
				LOG_INF("Client disconnected.\n");
			}
			close(tcp_server_socket);
		}
		// Close the server socket (should be unreachable  as the server runs indefinitely)
		close(tcp_server_listen_fd);
	#else
		udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (udp_socket < 0)
		{
			LOG_ERR("Failed to create socket: %d", -errno);
			FATAL_ERROR();
			return;
		}
		ret = bind(udp_socket, (struct sockaddr *)&self_addr, sizeof(self_addr));
		if (ret < 0)
		{
			LOG_ERR("bind, error: %d", -errno);
			FATAL_ERROR();
			return;
		}
		while((socket_receive.len = recvfrom(udp_socket, socket_receive.buf, BUFFER_MAX_SIZE, 0, (struct sockaddr *)&target_addr, &target_addr_len))>0){
			if(socket_connected == false){
				inet_ntop(target_addr.sin_family, &target_addr.sin_addr, target_addr_str, sizeof(target_addr_str));
				LOG_INF("Connect socket client to IP Address %s:%d\n", target_addr_str, ntohs(target_addr.sin_port));
				socket_connected=true;
			}
			socket_util_trigger_rx_callback_if_set();
		}
		if (socket_receive.len == -1) {
			LOG_ERR("Receiving failed");
		} else if (socket_receive.len == 0) {
			LOG_INF("Client disconnected.\n");
		}

		close(udp_socket);
		socket_connected=false;

	#endif
}


// K_THREAD_STACK_DEFINE(socket_util_thread_stack, CONFIG_SOCKET_STACK_SIZE);
// static struct k_thread socket_util_thread_data;
// static k_tid_t socket_util_thread_id;

// int socket_util_init(void){
//         int ret;
//         /* Start thread to handle events from socket connection */
//         socket_util_thread_id = k_thread_create(&socket_util_thread_data, socket_util_thread_stack, CONFIG_SOCKET_STACK_SIZE,
//                                 (k_thread_entry_t)socket_util_thread,  NULL, NULL, NULL,
// 			K_PRIO_PREEMPT(CONFIG_SOCKET_UTIL_THREAD_PRIO), 0, K_NO_WAIT);

//         ret = k_thread_name_set(socket_util_thread_id, "SOCKET");
// 	return ret;
// }
