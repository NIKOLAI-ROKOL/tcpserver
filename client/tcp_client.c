/*
Software is developed by Nikolay Malakhov and Fedor Malakhov, St. Petersburg, Russia

Revision history:
10.04.2025 - Created initial version
*/

#include "interface.h"
#include "tcp_client.h"

static CLIENT_INFO TestTaskList[MAX_SIMULT_TEST_THR + 1];

static void SessionClose(CLIENT_INFO* pClientCtx);

static uint16_t 		TcpServerPort = SERVER_DEF_ACCESS_PORT;
static unsigned int		TestIterations = 1;
static unsigned int		TestInstances = 1;
static uint16_t			InterReqDelay = 0;
static unsigned long long int TestDuation = 0;

static char client_info_delimer[] = "-------------------------------------------";
static char def_server_addr[] = SERVER_DEF_ACCESS_ADDR;

static char test_data[] = 
	"0123456789qwertyuiopasdfghjklzxcvbnm0123456789qwertyuiopasdfghjklzxcvbnm"
	"0123456789qwertyuiopasdfghjklzxcvbnm0123456789qwertyuiopasdfghjklzxcvbnm"
	"0123456789qwertyuiopasdfghjklzxcvbnm0123456789qwertyuiopasdfghjklzxcvbnm"
	"0123456789qwertyuiopasdfghjklzxcvbnm0123456789qwertyuiopasdfghjklzxcvbnm";
uint32_t test_data_len = sizeof(test_data)/sizeof(char);

static void client_usage_show(char* parg, int args)
{
	fprintf(stderr, "Client usage: %s (%u) <duration> <delay> <instances> [<address>] [<port>]\n", parg, args);
	fprintf(stderr, "	duration(M) -       Test duration in seconds (5...600)\n");
	fprintf(stderr, "	delay(M) -          Delay between requests in ms (0...900)\n");
	fprintf(stderr, "	instances(M) -      Number of test clients running (1...%u)\n", MAX_SIMULT_TEST_THR);
	fprintf(stderr, "	address(O) -        Test TCP server IP address\n");
	fprintf(stderr, "	address(O) -        Test TCP server IP port\n");
	fprintf(stderr, "\nExample:\n	tcpclient 20 300 4 192.168.1.4 3800\n");
}

int main(int argc, char *argv[])
{
    unsigned int i, j;
    unsigned int CompleteCount;
    struct hostent *he = NULL;
	char* pServerAddr = def_server_addr;
	
    if(argc < 4) {
		client_usage_show(argv[0], argc);
        exit(1);
    }

    signal(SIGPIPE, SIG_IGN);
	
	TestDuation = (unsigned long long int)atoi(argv[1]);
	InterReqDelay = (uint16_t)atoi(argv[2]);
	TestInstances = (unsigned int)atoi(argv[3]);

	if ((TestDuation < 5) || (TestInstances > 600)) {
		printf("Test duration (%llu) is out of range\n", TestDuation);
		exit(1);
	}
	
	TestDuation *= 1000000;
	if (InterReqDelay > 900) {
		printf("Inter-request delay (%u) is out of range\n", InterReqDelay);
		exit(1);
	}

	if ((TestInstances < 1) || (TestInstances > 500)) {
		printf("Test instances (%u) is out of range\n", TestInstances);
		exit(1);
	}

	if(argc > 4) {
		pServerAddr = argv[4];
	}
	
	if(argc > 5) {
		TcpServerPort = (uint16_t)atoi(argv[5]);
	}

    if((he = gethostbyname(pServerAddr)) == NULL) {
        printf("gethostbyname()\n");        
        exit(1);
    }

	printf("Client's configuration:\n%s\n", client_info_delimer);
	printf("Server IP address:      %s\n", pServerAddr);
	printf("Server IP port:         %u\n", TcpServerPort);
	printf("Test duration:          %llu sec\n", TestDuation / 1000000);
	printf("Inter request delay:    %u ms\n", InterReqDelay);
	printf("Clients running:        %u\n", TestInstances);
	printf("%s\n", client_info_delimer);
	
    for (i=0;i < TestInstances;i++) {
		CLIENT_INFO* pClient = &TestTaskList[i];
        pClient->TaskId = i + 1;
        pClient->HostName = pServerAddr;
        pClient->isTestComplete = false;
		pClient->HostAddr = (struct in_addr*)he->h_addr;
        if (!TestClientThreadCreate(pClient)) {
            exit(1);
        }
    }

    sleep(1);
    for(;;) {
        sleep(1);
        CompleteCount = 0;
        for (i=0;i < TestInstances;i++) {
            if (TestTaskList[i].isTestComplete) {
				CompleteCount++;
			}
        }

        if (CompleteCount == TestInstances) {
			for (i=0;i < TestInstances;i++) {
				CLIENT_INFO* pClientCtx = &TestTaskList[i];
				printf("Client task test is done (Task: %d, connects: %u, requests: %u, completed: %u, rejected: %u, error: %u)\n", 
					pClientCtx->TaskId, pClientCtx->Connects, pClientCtx->SendRequests, pClientCtx->CompletedRequests,
					pClientCtx->RejectedRequests, pClientCtx->ErrorRequests);
			}
			break;
		}
    }
	
    printf("Full test done!!!!\n\n");
    return 0;
}

static bool handle_server_message(CLIENT_INFO* pClientCtx)
{
	uint8_t* p_resp_msg = pClientCtx->client_msg_buf;
	uint8_t msg_tag = *p_resp_msg++;
	
	if (msg_tag == CLIENT_API_MSG_RESP) {
		pClientCtx->CompletedRequests++;
	} else if (msg_tag == CLIENT_API_MSG_ERR) {
		pClientCtx->ErrorRequests++;
	} else if (msg_tag == CLIENT_API_MSG_REJ) {
		pClientCtx->RejectedRequests++;
	}

	delay_ms(InterReqDelay);
	pClientCtx->RespMsgDone = true;
	return true;
}

static bool HandleServerPacketFormer(CLIENT_INFO* pClientCtx, uint8_t* pBody, uint32_t len)
{
	bool status = true;
	do {
		if (!pClientCtx->is_msg_header_read) {
			if (len < 2) {
				status = false;
				break;
			}

			pBody = Uint16Unpack(pBody, &pClientCtx->body_size);			
			if (pClientCtx->body_size > MAX_SERVER_MSG_SIZE) {
				status = false;
				break;				
			}

			pClientCtx->is_msg_header_read = true;
			len -= 2;
		}

		uint32_t ds = pClientCtx->body_size - pClientCtx->body_offset;
		if (len < ds) {
			memcpy(&pClientCtx->client_msg_buf[pClientCtx->body_offset], pBody,  len);
			pClientCtx->body_offset += len;
			break;
		}

		memcpy(&pClientCtx->client_msg_buf[pClientCtx->body_offset], pBody,  ds);
		if (!handle_server_message(pClientCtx)) {
			status = false;
			break;
		}

		len -= ds;
		pClientCtx->body_offset = 0;
		pClientCtx->is_msg_header_read = false;
	} while (len > 0);
	return status;
}

void* THRSimClient(void *arg)
{
	bool			is_work_done = false;
	bool			is_connect_close = false;
    int				rc;
    int				NoDelay = 1;
    CLIENT_INFO*	pClientCtx = NULL;
    struct sockaddr_in ServerAddr;
    unsigned long long int StartTime, CurrTime;
    struct timespec	spec;
    fd_set          master_rset, work_rset;
	struct timeval  select_time;
    int             maxfdp = 0;
    int		        Select_result;
	char			RxBuf[MAX_SERV_RX_PACKET + 1];
    char			SentBuf[MAX_SERVER_MSG_SIZE + 4];
	
    pClientCtx = (CLIENT_INFO*)arg;
	pClientCtx->CompletedRequests = 0;
	pClientCtx->ConnectRejectCount = 0;
	pClientCtx->Socket = -1;
	
	printf("Started client %d\n", pClientCtx->TaskId);

 	pClientCtx->pResp = (uint8_t*)malloc((MAX_SERVER_MSG_SIZE + 4)*sizeof(uint8_t));
	if (!pClientCtx->pResp) {
        printf("mem alloc Thr: %d\n", pClientCtx->TaskId);
        pthread_exit((void *)0);
	}

	clock_gettime(CLOCK_REALTIME, &spec);
	StartTime = (unsigned long long int)spec.tv_sec * 1000000;
	StartTime += (unsigned long long int)(spec.tv_nsec / 1000);
	for (;;) {
		is_connect_close = false;
		if((pClientCtx->Socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			printf("socket() Thr: %d\n", pClientCtx->TaskId);
			break;
		} 

		if (updateSocketSize(pClientCtx->Socket, SO_SNDBUF, MAX_SERV_RX_PACKET * 8) != 0) {
			printf("updateSocketSize TX Thr: %d\n", pClientCtx->TaskId);
			break;
		}
		
		if (updateSocketSize(pClientCtx->Socket, SO_RCVBUF, MAX_SERV_RX_PACKET * 8) != 0) {
			printf("updateSocketSize RX Thr: %d\n", pClientCtx->TaskId);
			break;
		}      

		if (setsockopt(pClientCtx->Socket, IPPROTO_TCP, TCP_NODELAY, (char*)&NoDelay, sizeof(NoDelay)) == -1) {
			printf("TCP_NODELAY Thr: %d\n", pClientCtx->TaskId);
			break;              
		}

		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(TcpServerPort);
		ServerAddr.sin_addr = *(pClientCtx->HostAddr);
		memset(&(ServerAddr.sin_zero), '\0', 8); 

		if(connect(pClientCtx->Socket, (struct sockaddr*)&ServerAddr, sizeof(struct sockaddr)) == -1) {
			printf("connect() Thr: %d\n", pClientCtx->TaskId);
			break;
		}

		pClientCtx->Connects++;
		is_connect_close = false;
		
		for (;;) {
			uint8_t* p_req_msg = SentBuf + 2;
			uint8_t* p_body_start = p_req_msg;

			*p_req_msg++ = CLIENT_API_MSG_REQ;
			memcpy(p_req_msg, (uint8_t*)test_data, test_data_len);
			p_req_msg += test_data_len;

			Uint16Pack(SentBuf, (uint32_t)(p_req_msg - p_body_start));	
			if ((rc = send(pClientCtx->Socket, SentBuf, (uint32_t)(p_req_msg - (uint8_t*)SentBuf), 0)) == -1) {
				printf("Request body send() is failed (error:  %d)\n", errno);
				is_connect_close = true;
				break;
			} else {
				pClientCtx->SendRequests++;
				pClientCtx->RespMsgDone = false;
			}

			FD_ZERO (&master_rset);
			maxfdp = 0;

			FD_SET (pClientCtx->Socket, &master_rset);
			if (pClientCtx->Socket > maxfdp) maxfdp = pClientCtx->Socket;

			for (;;) {    
				memcpy(&work_rset, &master_rset, sizeof(master_rset)); 
				select_time.tv_sec  = 1;
				select_time.tv_usec = 0;

				Select_result = select(maxfdp+1, &work_rset, NULL, NULL, &select_time);        
				if (Select_result < 0) {
					if (errno == EINTR) {
						continue;
					} else {
						printf("Select error %u (%s)\n", errno, strerror(errno));
						is_connect_close = true;
						break;
					}
				} else if (Select_result > 0) {
					if (FD_ISSET(pClientCtx->Socket, &work_rset)) {
						rc = recv(pClientCtx->Socket, RxBuf, MAX_SERV_RX_PACKET, 0);
						if (rc < 0) {
							if (errno != ECONNRESET) {
								printf("Fail to read from socket error %u (%s)\n", errno, strerror(errno));
							}

							is_connect_close = true;
							break;
						} else if (rc == 0) {
							printf("Session is closed %u (%s)\n", errno, strerror(errno));
							is_connect_close = true;
							break;
						}

						if (!HandleServerPacketFormer(pClientCtx, RxBuf, rc)) {
							is_connect_close = true;
							break;					
						}
						
						if (pClientCtx->RespMsgDone) {
							break;
						}
					}
				}
				
				clock_gettime(CLOCK_REALTIME, &spec);
				CurrTime = (unsigned long long int)spec.tv_sec * 1000000;
				CurrTime += (unsigned long long int)(spec.tv_nsec / 1000);
				if ((CurrTime - StartTime) > TestDuation) {
					is_work_done = true;
					break;
				}
			}
			
			if (is_connect_close || is_work_done) {
				SessionClose(pClientCtx);
				break;
			}
			
			clock_gettime(CLOCK_REALTIME, &spec);
			CurrTime = (unsigned long long int)spec.tv_sec * 1000000;
			CurrTime += (unsigned long long int)(spec.tv_nsec / 1000);
			if ((CurrTime - StartTime) > TestDuation) {
				is_work_done = true;
				break;
			}
		}
		
		if (is_work_done) {
			break;
		}

		delay_ms(100); // Delay in ms before next connect try
	}

	SessionClose(pClientCtx);
    pClientCtx->isTestComplete = true;
	if (pClientCtx->pResp) { free(pClientCtx->pResp); }
    pthread_exit((void *)0);
}

static void SessionClose(CLIENT_INFO* pClientCtx)
{
	if (pClientCtx->Socket > 0) {
		close(pClientCtx->Socket); 
		pClientCtx->Socket = -1;
	}
}

bool TestClientThreadCreate(CLIENT_INFO *ClientTaskPtr)
{
    pthread_attr_t	attr, *attrPtr = &attr;
    struct sched_param	sched;
	bool Result = true;

    pthread_attr_init(attrPtr);
    pthread_attr_setdetachstate(attrPtr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(attrPtr, PTHREAD_SCOPE_SYSTEM);
    if (pthread_attr_getschedparam(attrPtr, &sched) == 0) {
	    sched.sched_priority = 0;
	    pthread_attr_setschedparam(attrPtr, &sched);
    }
	
    if (pthread_create(&ClientTaskPtr->TestThr, &attr, &THRSimClient, ClientTaskPtr) == -1) {
	    printf("TestClient thread create error! (Thr: %d)\n",ClientTaskPtr->TaskId);
	    Result = false;
    }
	return Result;
}
