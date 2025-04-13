/*
Software is developed by Nikolay Malakhov and Fedor Malakhov, St. Petersburg, Russia

Revision history:
12.04.2025 - Created initial version
*/

#include "tcp_former.h"

void tcp_former_init(struct tcp_former* pFormer, void* data, OnMessageRxT on_msg_rx)
{
	pFormer->body_offset = 0;
	pFormer->is_msg_header_read = false;
	pFormer->data = data;
	pFormer->on_msg_rx = on_msg_rx;
}

bool tcp_former_process_data(struct tcp_former* pFormer, uint8_t* pBody, uint32_t len)
{
	bool status = true;
	do {
		if (!pFormer->is_msg_header_read) {
			if (len < 2) {
				status = false;
				break;
			}

			pBody = Uint16Unpack(pBody, &pFormer->body_size);
			pFormer->is_msg_header_read = true;
			len -= 2;
		}

		uint32_t ds = pFormer->body_size - pFormer->body_offset;
		if (len < ds) {
			memcpy(&pFormer->client_msg_buf[pFormer->body_offset], pBody,  len);
			pFormer->body_offset += len;
			break;
		}

		memcpy(&pFormer->client_msg_buf[pFormer->body_offset], pBody,  ds);
		if (pFormer->on_msg_rx && !(pFormer->on_msg_rx)(pFormer->data, pFormer->client_msg_buf, pFormer->body_size)) {
			status = false;
			break;
		}

		len -= ds;
		pFormer->body_offset = 0;
		pFormer->is_msg_header_read = false;
	} while (len > 0);
	return status;
}
