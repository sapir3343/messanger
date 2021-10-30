#pragma once
#pragma pack(push, 1)

#include "..\consts.h"

struct PublicKeyResponse {
	unsigned char client_id[UUID_LENGTH];
	unsigned char public_key[PUBLIC_KEY_LENGTH];
};

#pragma pack(pop)