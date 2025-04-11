#pragma once

/*
Software is developed by Nikolay Malakhov and Fedor Malakhov, St. Petersburg, Russia

Revision history:
10.04.2025 - Created initial version
*/

/* System Header Files */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/timerfd.h>
#include <sys/timex.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <pthread.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <fcntl.h>

#include <stdio.h>
#include <locale.h>
#include <stdarg.h>
#include <stdlib.h>

#ifndef __cplusplus
#define bool    unsigned char
#define false   0
#define true    1
#endif

#define CMD_NOT_FOUND	-1

int updateSocketSize(int sock, int attribute, unsigned long bufSize);
void IpV4AddrToStr(char* IpStr, unsigned int IpStrLen, unsigned int IpAddr);

#ifdef __cplusplus
extern "C" {
#endif

uint8_t* Uint16Pack(uint8_t* pBuf, unsigned int Value);
uint8_t* Uint16Unpack(uint8_t* pBuf, unsigned int* Value);
void delay_ms(unsigned short Delay);

int FindCmdBuf(char* pBuf, char *pCheckText);
char const* BeginValueMove(char* pStr);

#ifdef __cplusplus
}
#endif
