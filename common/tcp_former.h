#pragma once

/*
Software is developed by Nikolay Malakhov and Fedor Malakhov, St. Petersburg, Russia

Revision history:
12.04.2025 - Created initial version
*/

#include "common_lib.h"

#define MAX_CLIENT_MSG_SIZE		8192

typedef bool (*OnMessageRxT)(void* data, uint8_t* pmsg, uint32_t len); 

struct tcp_former {
	// Client related
	bool			is_msg_header_read;
	uint32_t		body_size;
	uint32_t		body_offset;
	void*			data;
	OnMessageRxT	on_msg_rx;
	uint8_t			client_msg_buf[MAX_CLIENT_MSG_SIZE + 1];
};

void tcp_former_init(struct tcp_former* pFormer, void* data, OnMessageRxT on_msg_rx);
bool tcp_former_process_data(struct tcp_former* pFormer, uint8_t* pBody, uint32_t len);
