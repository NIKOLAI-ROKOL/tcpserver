/*
Software is developed by Nikolay Malakhov and Fedor Malakhov, St. Petersburg, Russia

Revision history:
10.04.2025 - Created initial version
*/

#include "interface.h"
#include "tcp_server.h"
#include "tcp_former.h"

static bool CreateServerFreeChannelsList(struct server_info* pserver);
static bool ServerConfigRead(struct server_info* pserver, char const* file);
static bool ClientChannelCreate(struct server_info* pserver, int Socket, struct sockaddr_in* pClientAddr);
static void ClientChannelClose(struct channel_info* pChan);
static bool HandleClientPacketFormer(struct channel_info* pChan, uint8_t* pBody, uint32_t len);
static void HelpInfoPrint();
static void AddChannelList(struct channel_list *pChannelsList, struct channel_info *pChannel);
static void RemChannelList(struct channel_list *pChannelsList, struct channel_info *pChannel);
static struct channel_info* GetFistChannelList(struct channel_list *pChannelsList);
static struct channel_info* GetNextChannelList(struct channel_list *pChannelsList);
static bool handle_clients_message(void* data, uint8_t* p_req, uint32_t data_len);

bool gExitRequest = false;

struct channel_info serverChan;
struct channel_info timerChan;
struct server_info server_info; 
	
static char server_ip_addr_tag[] = "server-ip-address";
static char server_ip_port_tag[] = "server-ip-port";
static char max_simult_clients_tag[] = "max-sim-clients";
static char max_client_req_int_tag[] = "max-client-req-int";

static char server_info_delimer[] = "----------------------------------------------";

static void sighup_hdlr(int signal)
{
	gExitRequest = true;
}

int main(int argc, char* argv[])
{
	int					i;
	bool				config_ussue = false;
	unsigned long   	LocalIP = 0;
	struct in_addr		addr;
    struct hostent  	*he;
	struct channel_info	*pChan = NULL;
	struct channel_info	*pSelChan = NULL;
	int					flags;
	int					timeout = 200;
	int					ReuseIpAddr = 1;
	struct sockaddr_in	serveraddr;
	struct itimerspec	its;
	struct epoll_event	ev;
	unsigned char		MsgBuffer[MAX_SERV_RX_PACKET + 1];

	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	signal(SIGIO, SIG_IGN);
	signal(SIGINT, &sighup_hdlr);
		
	for(;;)
	{
		printf("\nTCP server test application\n\n");
		memset(&server_info, 0, sizeof(struct server_info)); 
		memset(&timerChan, 0, sizeof(struct channel_info));
		memset(&serverChan, 0, sizeof(struct channel_info));
		timerChan.pserver = &server_info;
		serverChan.pserver = &server_info;

		server_info.epollfd = -1;
		server_info.TcpServerIpPort = SERVER_DEF_ACCESS_PORT;
		server_info.MaxClientReqInt = MAX_DEF_CLIENT_REQ_PER_INT;
		server_info.MaxSimultClients = MAX_DEF_SIMILT_CLIENTS;
		strcpy(server_info.TcpServerAddr, SERVER_DEF_ACCESS_ADDR);

		if (argc != 2) {
			printf("Not all mandatory fields are set\n");
			config_ussue = true;
			break;
		}

		if (ServerConfigRead(&server_info, argv[1])) {
			printf("Please correct server's configuration file\n");
			config_ussue = true;
			break;			
		}

		printf("Server's configuration:\n%s\n", server_info_delimer);
		printf("Server IP addr:                   %s\n", server_info.TcpServerAddr);
		printf("Server IP port:                   %u\n", server_info.TcpServerIpPort);
		printf("Max simultaneously clients:       %u\n", server_info.MaxSimultClients);
		printf("Max client's requests per second: %u\n", server_info.MaxClientReqInt);
		printf("%s\n\n", server_info_delimer);

		if (!CreateServerFreeChannelsList(&server_info)) {
			break;
		}
		
		server_info.pClientRespBuf = (uint8_t*)malloc((MAX_CLIENT_MSG_SIZE + 3) * sizeof(uint8_t));
		if (!server_info.pClientRespBuf) {
			printf("Fail to memory allocation for client's response\n");
			break;
		}

		server_info.Events = (struct epoll_event*)malloc(MAX_RX_EVENTS*sizeof(struct epoll_event));
		if (!server_info.Events) {
			printf("Fail to memory allocation\n");
			break;	
		}
		
		memset((unsigned char*)server_info.Events, 0, MAX_RX_EVENTS*sizeof(struct epoll_event));
		server_info.epollfd = epoll_create1(0);
		if (server_info.epollfd == -1) {
			printf("Fail to epoll create.\n");
			break;
		}

		timerChan.Type = CT_TIMER;
		timerChan.Socket = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
		if (timerChan.Socket < 0) {
			printf("Fail to timer FD create rc = %d - error %u (%s).\n", timerChan.Socket, errno, strerror(errno));
			break;		
		}
		
		memset(&ev, 0, sizeof(struct epoll_event));
		ev.events = EPOLLIN | EPOLLERR;
		ev.data.ptr = &timerChan;

		if (epoll_ctl(server_info.epollfd, EPOLL_CTL_ADD, timerChan.Socket, &ev) == -1) {
			printf("Fail to add %d timer socket to epoll (error: %s)\n", timerChan.Socket, strerror(errno));
			close(timerChan.Socket);
			timerChan.Socket = -1;
			break;
		}
			
		if((he = gethostbyname(server_info.TcpServerAddr)) == NULL) {
			printf("Fail to gethostbyname() for %s address\n", server_info.TcpServerAddr);
			break;
		}

		if (he->h_addr_list[0] != 0) {
			memcpy(&addr, he->h_addr_list[0], sizeof(struct in_addr));
			LocalIP = inet_addr(inet_ntoa(addr));
		} else {
			printf("Fail to fiund local address for %s\n", server_info.TcpServerAddr);
			break;	
		}

		serverChan.Type = CT_SERVER;
		serverChan.Socket = socket(AF_INET, SOCK_STREAM, 0);
		if (serverChan.Socket < 0) {
			printf("Failed to open Server's socket (error %u (%s))\n", errno, strerror(errno));
			break;
		}

		if (setsockopt(serverChan.Socket, SOL_SOCKET, SO_REUSEADDR, &ReuseIpAddr, sizeof(int)) == -1) {
			printf("Failed to set SO_REUSEADDR for server socket with error (%d)\n", errno);
			break;
		}
	
		flags = fcntl(serverChan.Socket, F_GETFL);
		if (flags < 0) {
			printf("Failed to get socket flags with error (%d)\n", errno);
			break;
		}

		flags |= O_NONBLOCK;
		flags = fcntl(serverChan.Socket, F_SETFL, flags);
		if (flags < 0) {
			printf("Failed to set socket flags with error (%d)\n", errno);
			break;
		}

		bzero((char*) &serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = LocalIP;
		serveraddr.sin_port = htons(server_info.TcpServerIpPort);
		
		if (bind(serverChan.Socket, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) {
			printf("Failed to bind SERVER's socket (port: %u, error %u (%s))\n", server_info.TcpServerIpPort, errno, strerror(errno));
			break;
		}

        if (listen(serverChan.Socket, SOMAXCONN) < 0) {
            printf("Listen is failed for SERVER's socket (error %u (%s))\n", errno, strerror(errno));
            break;
        }

		memset(&ev, 0, sizeof(struct epoll_event));
		ev.events = EPOLLIN | EPOLLERR;
		ev.data.ptr = &serverChan;

		if (epoll_ctl(server_info.epollfd, EPOLL_CTL_ADD, serverChan.Socket, &ev) == -1) {
			printf("Ctx: %p, Fail to add %d socket to epoll (error: %s)\n",
				&serverChan, serverChan.Socket, strerror(errno));
			break;
		}
		
		// Start 1 second periodical timer
		const struct timespec load_check_int = {1, 0};
		its.it_value = load_check_int;
		its.it_interval = load_check_int;
		
		if (timerfd_settime(timerChan.Socket, 0, &its, NULL) != 0) {
			printf("Fail to set time error %u (%s).\n", errno, strerror(errno));
			break;
		}

		printf("Start client's requests receiving\n");
		printf("************ For stop processing please press Ctrl+C **************\n\n");

		while(!gExitRequest) {			
			int rc = epoll_wait(server_info.epollfd, server_info.Events, MAX_RX_EVENTS, timeout);
			if (rc < 0) {
				if (!gExitRequest) {
					printf("epoll_wait for %u msces completed, rc = %d - error %u (%s)\n", timeout, rc, errno, strerror(errno));
				}

				break;
			}

			if (rc == 0) { continue; }

			for (i = 0; i < rc; i++) {
				struct epoll_event* sel_event = &server_info.Events[i];
				pChan = (struct channel_info*)(sel_event->data.ptr);
				if (!pChan) {
					printf("Empty data ptr\n");
					continue;
				}

				if (pChan->Type == CT_SERVER) {
					int LineLocal = sizeof(struct sockaddr);
					struct sockaddr_in ClientAddr;
					int ClientSocket = accept(pChan->Socket, (struct sockaddr*)&ClientAddr, &LineLocal);
					if (ClientSocket > 0) {
						if ((uint32_t)server_info.UsedClientChannelsList.Count < server_info.MaxSimultClients) {
							if (ClientChannelCreate(&server_info, ClientSocket, &ClientAddr)) {
								pChan->pserver->accept_connections++;
								printf("Client's connection from %s:%u is established\n",
									inet_ntoa(*(struct in_addr*)&ClientAddr.sin_addr.s_addr), ntohs(ClientAddr.sin_port));
							}
						} else {
							pChan->pserver->reset_connections++;
							close(ClientSocket);
						}
					}
				} else if (pChan->Type == CT_TIMER) {
					uint64_t res;
					int ret = read(pChan->Socket, &res, sizeof(res));
					if (ret < 0) {
						printf("Fail to timer socket read (error: %d)\n", errno);
						break;
					}

					pSelChan = GetFistChannelList(&server_info.UsedClientChannelsList);
					while(pSelChan) {
						pSelChan->rx_msg_count = 0;
						pSelChan = GetNextChannelList(&server_info.UsedClientChannelsList);
					}
				} else if (pChan->Type == CT_CLIENT) {
					int rcv_bytes = recv(pChan->Socket, MsgBuffer, MAX_SERV_RX_PACKET, 0);
					if (rcv_bytes < 0) {
						printf("Fail to read from socket error %u (%s)\n", errno, strerror(errno));
						ClientChannelClose(pChan);
						continue;
					} else if (rcv_bytes == 0) {
						ClientChannelClose(pChan);
						continue;
					}

					if (!tcp_former_process_data(&pChan->former, MsgBuffer, rcv_bytes)) {
						ClientChannelClose(pChan);
						continue;					
					}
				} else {
					printf("Unexpected channel type (%u)\n", pChan->Type);
				}
			}
		}

		printf("\nServer stop is initiated\n");
		pSelChan = GetFistChannelList(&server_info.UsedClientChannelsList);
		while(pSelChan) {
			ClientChannelClose(pSelChan);
			pSelChan = GetFistChannelList(&server_info.UsedClientChannelsList);
		}

		printf("\nSummary: (accept conn: %u, reset conn: %u, rx requests: %u, completed: %u, rejected: %u, error: %u)\n",
			server_info.accept_connections, server_info.reset_connections, server_info.received_requests,
			server_info.completed_requests, server_info.rejected_requests, server_info.error_requests);
		break;
	}
	
	if (server_info.epollfd > 0) {
		if (timerChan.Socket > 0) {
			const struct timespec load_check_off = { 0, 0 };
			its.it_value = load_check_off;
			its.it_interval = load_check_off;
			
			if (timerfd_settime(timerChan.Socket, 0, &its, NULL) != 0) {
				printf("Fail to clear timer.\n");
			}

			memset(&ev, 0, sizeof(struct epoll_event));
			ev.events = EPOLLIN | EPOLLERR;
			ev.data.ptr = &timerChan;

			if (epoll_ctl(server_info.epollfd, EPOLL_CTL_DEL, timerChan.Socket, &ev) == -1) {
				printf("Fail to remove %d timer socket from epoll (error: %s)\n", timerChan.Socket, strerror(errno));
			}

			close(timerChan.Socket);
		}

		if (serverChan.Socket > 0) {
			memset(&ev, 0, sizeof(struct epoll_event));
			ev.events = EPOLLIN | EPOLLERR;
			ev.data.ptr = &serverChan;

			if (epoll_ctl(server_info.epollfd, EPOLL_CTL_DEL, serverChan.Socket, &ev) == -1) {
				printf("Fail to remove %d server socket from epoll (error: %s)\n", serverChan.Socket, strerror(errno));
			}

			close(serverChan.Socket);
		}

		close(server_info.epollfd);
	}

	pSelChan = GetFistChannelList(&server_info.FreeClientChannelsList);
	while(pSelChan) {
		RemChannelList(&server_info.FreeClientChannelsList, pSelChan);
		free(pSelChan);
		pSelChan = GetFistChannelList(&server_info.FreeClientChannelsList);
	}
		
	if (server_info.pClientRespBuf) { free(server_info.pClientRespBuf); }
	if (server_info.Events) { free(server_info.Events); }

	if (config_ussue) {
		HelpInfoPrint();
		exit(EXIT_FAILURE);
	} else {
		printf("Server stop is completed\n");
		exit(0);
	}
}

static bool CreateServerFreeChannelsList(struct server_info* pserver)
{
	int i;
	bool status = true;

	for (i=0;i < pserver->MaxSimultClients;i++) {
		struct channel_info* pChan = (struct channel_info*)malloc(sizeof(struct channel_info));
		if (!pChan) {
			printf("Failed to memory allocation for channel\n");
			status = false;
			break;
		}

		memset((uint8_t*)pChan, 0, sizeof(struct channel_info));
		AddChannelList(&pserver->FreeClientChannelsList, pChan);
	}
	
	return status;
}

static void ClientChannelClose(struct channel_info* pChan)
{
	struct epoll_event ev;
	
	if (pChan->Socket <= 0) { return; }

	printf("Client's connection from %s:%u is closed\n",
		inet_ntoa(*(struct in_addr*)&pChan->ClientAddr.sin_addr.s_addr), ntohs(pChan->ClientAddr.sin_port));

	memset(&ev, 0, sizeof(struct epoll_event));
	ev.events = EPOLLIN | EPOLLERR;
	ev.data.ptr = pChan;

	if (epoll_ctl(pChan->pserver->epollfd, EPOLL_CTL_DEL, pChan->Socket, &ev) == -1) {
		printf("Ctx: %p, Fail to delete client's %d socket from epoll (error: %s)\n",
			pChan, pChan->Socket, strerror(errno));
	}
	
	close(pChan->Socket);
	RemChannelList(&pChan->pserver->UsedClientChannelsList, pChan);
	AddChannelList(&pChan->pserver->FreeClientChannelsList, pChan);
}

static bool ClientChannelCreate(struct server_info* pserver, int Socket, struct sockaddr_in* pClientAddr) 
{
	bool status = false;
	int flags;
    int ReuseIpAddr = 1;
	struct sockaddr_in serveraddr;
	struct epoll_event ev;
	struct channel_info* pChan = NULL;
	
	for(;;)
	{
		pChan = GetFistChannelList(&pserver->FreeClientChannelsList);
		if (pChan) {
			RemChannelList(&pserver->FreeClientChannelsList, pChan);
		} else {
			pChan = (struct channel_info*)malloc(sizeof(struct channel_info));
			if (!pChan) {
				printf("Failed to memory allocation for channel\n");
				break;
			}
		}

		memset((uint8_t*)pChan, 0, sizeof(struct channel_info));
		pChan->pserver = pserver;
		pChan->Type = CT_CLIENT;
		pChan->Socket = Socket;
		pChan->ClientAddr = *pClientAddr;
		
		tcp_former_init(&pChan->former, pChan, handle_clients_message);
		if (updateSocketSize(pChan->Socket, SO_SNDBUF, MAX_SERV_RX_PACKET*8) != 0) {
			printf("Failed to update lient's socket TX buf size (errno %d)\n", errno);
			break;
		}
		
		if (updateSocketSize(pChan->Socket, SO_RCVBUF, MAX_SERV_RX_PACKET*8) != 0) {
			printf("Failed to update client's socket RX buf size (errno %d)\n", errno);
			break;
		}       

		if (setsockopt(pChan->Socket, SOL_SOCKET, SO_REUSEADDR, &ReuseIpAddr, sizeof(int)) == -1) {
			printf("Failed to set SO_REUSEADDR for client's socket with error (%d)\n", errno);
			break;
		}
		
		flags = fcntl(pChan->Socket, F_GETFL);
		if (flags < 0) {
			printf("Failed to get socket flags with error (%d)\n", errno);
			break;
		}

		flags |= O_NONBLOCK;
		flags = fcntl(pChan->Socket, F_SETFL, flags);
		if (flags < 0) {
			printf("Failed to set socket flags with error (%d)\n", errno);
			break;
		}
		
		memset(&ev, 0, sizeof(struct epoll_event));
		ev.events = EPOLLIN | EPOLLERR;
		ev.data.ptr = pChan;

		if (epoll_ctl(pserver->epollfd, EPOLL_CTL_ADD, pChan->Socket, &ev) == -1)
		{
			printf("Ctx: %p, Fail to add client's %d socket to epoll (error: %s)\n",
				pChan, pChan->Socket, strerror(errno));
			break;
		}
		
		AddChannelList(&pserver->UsedClientChannelsList, pChan);
		status = true;
		break;
	}
	
	if (!status) {
		if (pChan->Socket > 0) {
			close(pChan->Socket);			
		}

		AddChannelList(&pserver->FreeClientChannelsList, pChan);
	}
	
	return status;
}

static void HelpInfoPrint()
{
	printf("\nUsage: tcp_server [CFG]\n\n");
	printf("Mandatory parameters:\n");
	printf("    CFG     - TCP server configuration file\n");
	printf("Example: ./udp_server server.cfg\n\n");
	printf("Server's configuratino file structure\nAll parameters are optional.\n");
	printf("%s: <Address>    - Server's IP address or FQDN (default: %s)\n", server_ip_addr_tag, SERVER_DEF_ACCESS_ADDR);
	printf("%s: <1...65535>     - Server's IP port (default: %u)\n", server_ip_port_tag, SERVER_DEF_ACCESS_PORT);
	printf("%s: <10...%u>     - Maximum number of simultaneously processed clients (Default: %u)\n", max_simult_clients_tag, MAX_SIMILT_CLIENTS, MAX_DEF_SIMILT_CLIENTS);
	printf("%s: <1...10000> - Maximum number of processed client's requests per second (Default: %u)\n", max_client_req_int_tag, MAX_DEF_CLIENT_REQ_PER_INT);
}

bool ServerConfigRead(struct server_info* pserver, char const* file)
{
	char c;
	int i, pos;
	bool config_ussue = false;
	char cmdStr[MAX_SERVER_CONFIG_LINE_LEN + 1];

	FILE* pFh = fopen(file, "r");
	if (!pFh) {
		printf("Failed to open txp_server configuration file (%s) for read\n", file);
		return config_ussue;
	}

	do { 
		// read all lines in config file
		pos = 0;
		do { 
			c = fgetc(pFh);
			if((c != EOF) && (c != '\r') && (c != '\n') && (pos < MAX_SERVER_CONFIG_LINE_LEN)) {
				cmdStr[pos++] = (char)c;
			}
		} while((c != EOF) && (c != '\n') && (c != '\r'));
		if ((pos > 0) && (*cmdStr != '#')) {
			cmdStr[pos] = 0;
			for(;;) {
				i = FindCmdBuf(cmdStr, server_ip_addr_tag);
				if (i != CMD_NOT_FOUND) {
					char const* pval = BeginValueMove(&cmdStr[i]);
					if (!pval) {
						config_ussue = true;
						break;
					}

					strcpy(pserver->TcpServerAddr, pval);
					break;
				}

				i = FindCmdBuf(cmdStr, server_ip_port_tag);
				if (i != CMD_NOT_FOUND) {
					char const* pval = BeginValueMove(&cmdStr[i]);
					if (!pval) {
						config_ussue = true;
						break;
					}

					pserver->TcpServerIpPort = (unsigned int)atoi(pval);
					if ((pserver->TcpServerIpPort < 1) || (pserver->TcpServerIpPort > 65535)) {
						printf("IP port is OOR (%u)\n", pserver->TcpServerIpPort);
						config_ussue = true;
						break;
					}

					break;
				}

				i = FindCmdBuf(cmdStr, max_simult_clients_tag);
				if (i != CMD_NOT_FOUND) {
					char const* pval = BeginValueMove(&cmdStr[i]);
					if (!pval) {
						config_ussue = true;
						break;
					}

					pserver->MaxSimultClients = (unsigned int)atoi(pval);
					if ((pserver->MaxSimultClients < 10) || (pserver->MaxSimultClients > MAX_SIMILT_CLIENTS)) {
						printf("Maximum number of simult. clients is OOR (%u)\n", pserver->MaxSimultClients);
						config_ussue = true;
						break;
					}

					break;
				}

				i = FindCmdBuf(cmdStr, max_client_req_int_tag);
				if (i != CMD_NOT_FOUND) {
					char const* pval = BeginValueMove(&cmdStr[i]);
					if (!pval) {
						config_ussue = true;
						break;
					}

					pserver->MaxClientReqInt = (unsigned int)atoi(pval);
					if ((pserver->MaxClientReqInt < 1) || (pserver->MaxClientReqInt > 10000)) {
						printf("Maximum number of client's requests per interval is OOR (%u)\n", pserver->MaxClientReqInt);
						config_ussue = true;
						break;
					}

					break;
				}

				printf("Unexpected command {%s}\n", cmdStr);
				config_ussue = true;
				break;
			}

			if (config_ussue) {
				break;
			}
		}
	} while(c != EOF);
	fclose(pFh);
	
	return config_ussue;
}

static bool handle_clients_message(void* data, uint8_t* p_req, uint32_t data_len)
{
	int rc;
	bool status = true;

	struct channel_info* pChan = (struct channel_info*)data;
	uint8_t* p_resp_buf = pChan->pserver->pClientRespBuf;
	uint8_t* p_resp_msg = p_resp_buf + 2;
	uint8_t* p_body_start = p_resp_msg;
	uint8_t msg_tag = *p_req++;
	data_len--;	
	pChan->pserver->received_requests++;
	if (pChan->rx_msg_count < pChan->pserver->MaxClientReqInt) {
		pChan->rx_msg_count++;	
		if (msg_tag == CLIENT_API_MSG_REQ) {
			pChan->pserver->completed_requests++;
			*p_resp_msg++ = CLIENT_API_MSG_RESP;
			memcpy(p_resp_msg, p_req, data_len);
			p_resp_msg += data_len;
		} else {
			pChan->pserver->error_requests++;
			*p_resp_msg++ = CLIENT_API_MSG_ERR;
		}
	} else {
		pChan->pserver->rejected_requests++;
		*p_resp_msg++ = CLIENT_API_MSG_REJ;
	}

	Uint16Pack(p_resp_buf, (uint32_t)(p_resp_msg - p_body_start));	
    if ((rc = send(pChan->Socket, p_resp_buf, (uint32_t)(p_resp_msg - p_resp_buf), 0)) == -1) {
        printf("Response body send() is failed (error:  %d)\n", errno);
		status = false;
	}

	return status;
}

static void AddChannelList(struct channel_list *pChannelsList, struct channel_info *pChannel)
{
    struct channel_info     *pPrev;
    struct channel_info     *pNext;

	if (!pChannelsList->Count || !pChannelsList->pFistChannel) {
		pChannelsList->pFistChannel = pChannel;
	    pChannel->pNext = pChannel;
	    pChannel->pPrev = pChannel;
        pChannelsList->Count = 0;
	} else {
		pNext = pChannelsList->pFistChannel;
        pPrev = pNext->pPrev;
        pChannel->pNext = pNext;
        pChannel->pPrev = pPrev;
        if (pPrev) pPrev->pNext = pChannel;
		pNext->pPrev = pChannel;
    }
	pChannelsList->Count++;
}

static void RemChannelList(struct channel_list *pChannelsList, struct channel_info *pChannel)
{
	if (!pChannelsList->Count || !pChannel) { return; }
	struct channel_info* pPrev = pChannel->pPrev;
	struct channel_info* pNext = pChannel->pNext;
    if (pPrev) { pPrev->pNext = pNext; }
    if (pNext) { pNext->pPrev = pPrev; }
    if (pChannel == pChannelsList->pFistChannel) { pChannelsList->pFistChannel = pNext; }
	pChannelsList->Count--;
}

static struct channel_info* GetFistChannelList(struct channel_list *pChannelsList)
{
	if (!pChannelsList || !pChannelsList->Count) {return NULL; }
    pChannelsList->pCurrent = pChannelsList->pFistChannel;
    return pChannelsList->pCurrent;
}

static struct channel_info* GetNextChannelList(struct channel_list *pChannelsList)
{
	if (!pChannelsList || !pChannelsList->Count) { return NULL; }
	if (!pChannelsList->pCurrent || !pChannelsList->pFistChannel) { return NULL; }
    if (pChannelsList->pFistChannel == pChannelsList->pCurrent->pNext) { return NULL; }
    pChannelsList->pCurrent = pChannelsList->pCurrent->pNext;
    return pChannelsList->pCurrent;
}
