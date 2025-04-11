/*
Software is developed by Nikolay Malakhov and Fedor Malakhov, St. Petersburg, Russia

Revision history:
10.04.2025 - Created initial version
*/

#include "common_lib.h"

int updateSocketSize(int sock, int attribute, unsigned long bufSize)
{
    int            optionLen;
    unsigned long  curSize;
    unsigned long  newSize;

    /* Get the current size of the buffer */
    optionLen = sizeof(curSize); //-V1048
    if (getsockopt(sock, SOL_SOCKET, attribute,
                   (char *)(&curSize), (socklen_t*)&optionLen) < 0)
    {
        printf("\nERROR: getsockopt");
        return (-1);
    }

    /* Check if the current size is already big enough */
    if (curSize < bufSize)
    {
        /* Set the new size */
        optionLen = sizeof(bufSize); //-V1048
        if (setsockopt(sock, SOL_SOCKET, attribute,
                       (char*)(&bufSize), optionLen) < 0)
        {
            printf("\nERROR: setsockopt");
            return (-1);
        }

        /* Retrive the new size and verify it took */
        optionLen = sizeof(newSize); //-V1048
        if (getsockopt(sock, SOL_SOCKET, attribute,
                       (char*)(&newSize), (socklen_t*)&optionLen) < 0)
        {
            printf("\nERROR: getsockopt");
            return (-1);
        }

        /* Check the sizes match */
        if (newSize < bufSize)
        {
            printf("\nFailed to set socket size to %ld, size now %ld\n", bufSize, newSize);
            return (-1);
        }
    }
    /* All good */
    return (0);
}

uint8_t* Uint16Pack(uint8_t* pBuf, unsigned int Value)
{
	*pBuf++ = (uint8_t)((Value & 0x0000ff00) >> 8);
	*pBuf++ = (uint8_t)(Value & 0x000000ff);
	return pBuf;
}

uint8_t* Uint16Unpack(uint8_t* pBuf, unsigned int* Value)
{
    *Value = (uint8_t)(*pBuf++) << 8;
	*Value |= (uint8_t)(*pBuf++);
	return pBuf;
}

void delay_ms(unsigned short Delay)
{
    struct timespec sleep_time;
    
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = (Delay * 1000000);
    nanosleep(&sleep_time, 0);
}

int FindCmdBuf(char* pBuf, char *pCheckText)
{
	int i = 0;
	int ReturnFind = CMD_NOT_FOUND;
	int k = strlen(pCheckText);
	if ((int)strlen(pBuf) < k) {
		return ReturnFind;
	}

	int j = strlen(pBuf)+ 1 - k;
	while (i < j) {
		int rs = strncmp(&pBuf[i], pCheckText, k);
		if (rs == 0) {
			ReturnFind = i + k;
			break;
		}

		i++;
	}

	return ReturnFind;
}

char const* BeginValueMove(char* pStr)
{
	char c = *pStr++;
	if (c != ':') {
		return NULL;
	}

	for (;;) {
		c = *pStr;
		if (!c) { NULL; }
		if ((c != ' ') && (c != '\t')) {
			return pStr;
		}
		pStr++;
	}

	return NULL;
}