#pragma once

/*
Software is developed by Nikolay Malakhov and Fedor Malakhov, St. Petersburg, Russia

Revision history:
10.04.2025 - Created initial version
*/

#include "common_lib.h"
#include "tcp_former.h"

#define MAX_SIMULT_TEST_THR		500
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
	struct tcp_former	former;
} CLIENT_INFO;

void* THRSimClient(void *arg);
bool TestClientThreadCreate(CLIENT_INFO *ClientTaskPtr);
