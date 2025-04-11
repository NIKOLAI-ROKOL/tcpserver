#pragma once

/*
Software is developed by Nikolay Malakhov and Fedor Malakhov, St. Petersburg, Russia

Revision history:
10.04.2025 - Created initial version
*/

#include "common_lib.h"

#define MAX_SERV_RX_PACKET			1500
#define MAX_RX_EVENTS				1000
#define MAX_SERVER_CONFIG_LINE_LEN	512
#define MAX_CLIENT_MSG_SIZE			8192
#define MAX_DEF_SIMILT_CLIENTS		100
#define MAX_DEF_CLIENT_REQ_PER_INT	10

typedef enum {
	CT_SERVER = 1,
	CT_TIMER,
	CT_CLIENT
} CHANNEL_TYPE;

struct server_info {
	uint32_t accept_connections;
	uint32_t reset_connections;
	uint32_t received_requests;
	uint32_t completed_requests;
	uint32_t rejected_requests;
	uint32_t error_requests;
};

struct channel_info {
	// Channels list related
    struct channel_info*	pNext;
    struct channel_info*	pPrev;

	// Channel info related
	CHANNEL_TYPE			Type;
	int						Socket;
	struct sockaddr_in		ClientAddr;
	
	struct server_info*		pserver;
	
	// Client related
	bool					is_msg_header_read;
	uint32_t				body_size;
	uint32_t				body_offset;
	uint32_t				rx_msg_count;
	uint8_t					client_msg_buf[MAX_CLIENT_MSG_SIZE + 1];
};

struct channel_list {
	uint32_t				Count;
    struct channel_info*	pFistChannel;
    struct channel_info*	pCurrent;
};
