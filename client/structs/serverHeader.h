#pragma once
#pragma pack(push, 1)

struct ServerHeader {
	unsigned char version;
	short code;
	int payload_size;
};

#pragma pack(pop)