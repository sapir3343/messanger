#pragma once
#include <sstream>
#include "cryptopp_wrapper/AESWrapper.h"

struct OtherClient {
	std::string m_name;
	std::string m_uuid;
	std::string m_public_key;
	unsigned char m_symetric_key[AESWrapper::DEFAULT_KEYLENGTH];;
};