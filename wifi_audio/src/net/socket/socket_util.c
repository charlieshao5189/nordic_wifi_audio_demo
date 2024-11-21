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
#include <zephyr/shell/shell.h>
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
#if defined(CONFIG_SOCKET_TYPE_TCP)
int tcp_server_listen_fd;
int tcp_server_socket;
int tcp_client_socket;
#elif defined(CONFIG_SOCKET_TYPE_UDP)
int udp_socket;
#endif
struct sockaddr_in self_addr;
bool socket_connected=false;


char target_addr_str[32];
struct sockaddr_in target_addr;
socklen_t target_addr_len=sizeof(target_addr);

#if defined(CONFIG_SOCKET_ROLE_CLIENT)
static struct k_sem target_addr_set_sem;
static bool target_addr_set = false; // Flag to indicate if the target address is set
#endif //#if defined(CONFIG_SOCKET_ROLE_CLIENT)

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
    ssize_t bytes_sent=0;

    while (length > 0) {
        size_t to_send = (length >= chunk_size) ? chunk_size : length;

        #if defined(CONFIG_SOCKET_TYPE_TCP)
                #if defined(CONFIG_SOCKET_ROLE_CLIENT)
                        bytes_sent = send(tcp_client_socket, data, to_send, 0);
                        LOG_INF("Sent %d bytes to server\n", bytes_sent);
                        // LOG_HEXDUMP_INF(data, bytes_sent, "Sent data(HEX):");
                #elif defined(CONFIG_SOCKET_ROLE_SERVER)
                        bytes_sent = send(tcp_server_socket, data, to_send, 0);
                        LOG_INF("Sent %d bytes to server\n", bytes_sent);
                        // LOG_HEXDUMP_INF(data, bytes_sent, "Sent data(HEX):");
                #endif //#if defined(CONFIG_SOCKET_ROLE_CLIENT)
        #elif defined(CONFIG_SOCKET_TYPE_UDP)
            bytes_sent = sendto(udp_socket, data, to_send, 0, (struct sockaddr *)&target_addr, sizeof(target_addr));
        #endif

        if (bytes_sent == -1) {
            perror("Sending failed");
            
        //     #if defined(CONFIG_SOCKET_TYPE_TCP)
        //         // close(tcp_server_socket);
        //     #else
        //         close(udp_socket);
        //     #endif

            FATAL_ERROR();
            return bytes_sent;
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

        #if defined(CONFIG_SOCKET_ROLE_CLIENT)
                k_sem_init(&target_addr_set_sem, 0, 1);
        #endif //#if defined(CONFIG_SOCKET_ROLE_CLIENT)

	if (dk_leds_init() != 0)
	{
		LOG_ERR("Failed to initialize the LED library");
	}

        #if defined(CONFIG_NRF70_AP_MODE)
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

	self_addr.sin_family = AF_INET;
	self_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	self_addr.sin_port = htons(socket_port);

        target_addr.sin_family = AF_INET;

        #if defined(CONFIG_SOCKET_ROLE_CLIENT)
                LOG_INF("\r\n\r\n Play as socket client, set target server address with shell command:socket set_target_addr <IP>:<Port>)\r\n");
                k_sem_take(&target_addr_set_sem, K_FOREVER);
        #elif defined(CONFIG_SOCKET_ROLE_SERVER)
                LOG_INF("\r\n\r\n Play as socket server, waiting for client connection...\r\n");
        #endif //#if defined(CONFIG_SOCKET_ROLE_CLIENT)

	#if defined(CONFIG_SOCKET_TYPE_TCP)
                #if defined(CONFIG_SOCKET_ROLE_CLIENT)
                        tcp_client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                        int opt = 1;
                        setsockopt(tcp_client_socket, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
                        ret = connect(tcp_client_socket, (struct sockaddr *)&target_addr, sizeof(target_addr));
                        if (ret < 0)
                        {
                                LOG_ERR("connect, error: %d", -errno);
                                FATAL_ERROR();
                                return;
                        }
                        LOG_INF("Connected to TCP server\n");
                        // Handle the client connection
                        while(1){
                                socket_receive.len = recv(tcp_client_socket, socket_receive.buf, BUFFER_MAX_SIZE,0);
                                if(socket_receive.len > 0)
                                {
                                        socket_util_trigger_rx_callback_if_set();
                                }
                                else if (socket_receive.len == -1) {
                                        LOG_ERR("Receiving failed");
                                        close(tcp_client_socket);
                                } 
                                else if (socket_receive.len == 0) {
                                        LOG_INF("TCP Server disconnected.\n");
                                        // close(tcp_client_socket);
                                }
                        }
                #elif defined(CONFIG_SOCKET_ROLE_SERVER)
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

                        while(1){
                                // Handle the client connection
                                socket_receive.len = recv(tcp_server_socket, socket_receive.buf, BUFFER_MAX_SIZE,0);
                                if(socket_receive.len > 0)
                                {
                                        socket_util_trigger_rx_callback_if_set();
                                }
                                else if (socket_receive.len == -1) {
                                        LOG_ERR("Receiving failed");
                                        close(tcp_server_socket);
                                } 
                                else if (socket_receive.len == 0) {
                                        LOG_INF("TCP Server disconnected.\n");
                                        close(tcp_server_socket);
                                }
                        }
                        close(tcp_server_socket);
                        close(tcp_server_listen_fd);     
                
                #endif //#if defined(CONFIG_SOCKET_ROLE_CLIENT)

	#elif defined(CONFIG_SOCKET_TYPE_UDP)
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
				LOG_INF("Connect socket to IP Address %s:%d\n", target_addr_str, ntohs(target_addr.sin_port));
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

#if defined(CONFIG_SOCKET_ROLE_CLIENT)

static int cmd_set_target_address(const struct shell *shell, size_t argc, const char **argv)
{
    // Ensure the command is provided with exactly one argument
    if (argc != 2) {
        shell_print(shell, "Usage: socket set_target_addr <IP:Port>");
        return -1;
    }

    char *target_addr_str = (char *)k_malloc(22); // Allocate memory for the string
    if (target_addr_str == NULL) {
        shell_print(shell, "Memory allocation failed");
        return -1;
    }

    // Get the target address string from the command argument
    strncpy(target_addr_str, argv[1], 22); // Use strncpy instead of strcpy for safety
    target_addr_str[21] = '\0'; // Ensure null-termination

    char ip_str[INET_ADDRSTRLEN];
    int port;

    // Split into IP address and port
    if (sscanf(target_addr_str, "%[^:]:%d", ip_str, &port) != 2) {
        shell_print(shell, "Invalid format. Expected <IP>:<Port>");
        k_free(target_addr_str); // Free the allocated memory
        return -1;
    }

    // Set up the sockaddr_in structure
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(port); // Convert port to network byte order

    if (inet_pton(AF_INET, ip_str, &(target_addr.sin_addr)) <= 0) {
        shell_print(shell, "Invalid IP address format: %s", ip_str);
        k_free(target_addr_str);
        return -1;
    }

    shell_print(shell, "Target address set to: %s:%d", ip_str, port);
    target_addr_set = true; // Set the flag to true
    k_sem_give(&target_addr_set_sem); // Signal that the target address is set
    k_free(target_addr_str); // Free memory afterwards

    return 0;
}

// Existing shell command definitions
SHELL_STATIC_SUBCMD_SET_CREATE(socket_cmd,
                               SHELL_CMD(set_target_addr, NULL, "Get and set target address in format <IP:Port>",
                                          cmd_set_target_address),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(socket, &socket_cmd, "Socket commands", NULL);

#endif //#if defined(CONFIG_SOCKET_ROLE_CLIENT)