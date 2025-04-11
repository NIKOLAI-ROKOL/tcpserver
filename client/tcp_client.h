#pragma once

/*
Software is developed by Nikolay Malakhov and Fedor Malakhov, St. Petersburg, Russia

Revision history:
10.04.2025 - Created initial version
*/

#include "common_lib.h"

#define MAX_SIMULT_TEST_THR		500
#define MAX_SERVER_MSG_SIZE		8192
#define MAX_SERV_RX_PACKET		1500

typedef struct {
    bool				isTestComplete;
	bool				RespMsgDone;
	int					Socket;
    unsigned int		TaskId;
	unsigned int		Connects;
	unsigned int		SendRequests;
	unsigned int		CompletedRequests;
	unsigned int		RejectedRequests;
	unsigned int		ErrorRequests;
	unsigned int		ConnectRejectCount;
    pthread_t			TestThr;
    char				*HostName;
	struct in_addr		*HostAddr;
	uint8_t				*pResp;
	// RX messages former related
	bool				is_msg_header_read;
	uint32_t			body_size;
	uint32_t			body_offset;
	uint32_t			rx_msg_count;
	uint8_t				client_msg_buf[MAX_SERVER_MSG_SIZE + 1];
} CLIENT_INFO;

void* THRSimClient(void *arg);
bool TestClientThreadCreate(CLIENT_INFO *ClientTaskPtr);
