#pragma once
#pragma pack(push, 1)

struct MessageHeader {
	unsigned char client_id[16];
	unsigned char message_type;
	unsigned int content_size;
};


#pragma pack(pop)