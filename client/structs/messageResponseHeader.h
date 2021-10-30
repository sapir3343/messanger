#pragma once
#pragma pack(push, 1)

struct MessageResponseHeader {
	unsigned char client_id[16];
	unsigned int message_id;
	unsigned char message_type;
	unsigned int content_size;
};


#pragma pack(pop)