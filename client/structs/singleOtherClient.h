#pragma once
#pragma pack(push, 1)

#include <vector>
#include <sstream>
#include "../consts.h"

struct SingleOtherClient {
	unsigned char client_id[UUID_LENGTH];
	unsigned char name[USERNAME_LENGTH];
	std::string public_key;
	std::string symetric_key;
	AESWrapper* aes_encryptor;
};

#pragma pack(pop)
