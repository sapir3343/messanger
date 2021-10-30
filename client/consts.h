#pragma once

const unsigned int CLIENT_VERSION = 1;
// errors
const std::string ERROR_MESSAGE_COMMAND = "Unknown command, try again";
const std::string ERROR_MESSAGE = "server response with an error";
const std::string ERROR_IN_SERVER = "server is unreachable - try again";
const std::string USERNAME_IS_TOO_LONG = "username length should be less then 254 char";
const std::string USERNAME_CANNOT_FOUND = "username cannot be found - you migth need to ask for user list";
const std::string REGISTRATION_ERROR = "you are not register!";

enum Commands {
	Exit = 0,
	Register = 10,
	GetClientsList = 20,
	GetPublicKey = 30,
	GetWaitingMessages = 40,
	SendAMessage = 50,
	SendSymetricKeyRequest = 51,
	SendSymetricKey = 52
};

enum MessagesTypes {
	SymetricKeyRequest = 1,
	SymetricKeyRecived = 2,
	TextMessage = 3
};

// server info file parsing
const std::string SERVER_INFO_FILE = "server.info";
const std::string SERVER_INFO_DETAILS_PROBLEM = "ip or port are invalid or not exists";
const std::string SERVER_INFO_PARSE_ERROR = "Error while parsing configuration file - please fix it";

const std::string USER_INFO_FILE = "./me.info";

const std::string SERVER_INFO_PORT_PARSE_ERROR = "connection error - please check if ip and port are ok";
const unsigned int MAX_PORT = 65535;
const unsigned int MIN_PORT = 1000;

// string for messages output
const std::string SYMETRIC_KEY_RECIVED = "symetric key received";
const std::string SYMETRIC_KEY_REQUEST = "request for symetric key";
const std::string CANT_DECRYPT_MESSAGE = "can't decrypt message";


// request header size consts
const unsigned int VERSION_LENGTH = 1;
const unsigned int CODE_LENGTH = 2;
const unsigned int PAYLOAD_SIZE_LENGTH = 4;
const unsigned int UUID_LENGTH = 16;
const unsigned int HEADER_LENGTH = VERSION_LENGTH + CODE_LENGTH +
										PAYLOAD_SIZE_LENGTH + UUID_LENGTH;
const unsigned int MESSAGE_CODE_LENGTH = 1;

// consts for length 
const unsigned int USERNAME_LENGTH = 255;
const unsigned int SINGLE_CLIENT_RESPONSE_SIZE = UUID_LENGTH + USERNAME_LENGTH;
const unsigned int PUBLIC_KEY_LENGTH = 160;

// consts for request codes
const uint16_t REGISTER_CODE = 1000;
const uint16_t GET_ALL_CLIENTS_CODE = 1001;
const uint16_t GET_PUBLIC_KEY_CODE = 1002;
const uint16_t SENDING_MESSAGE_CODE = 1003;
const uint16_t PULLING_MESSAGES_CODE = 1004;

// messages types
const int ASK__FOR_SYMMETRIC_KEY_TYPE = 1;
const int SEND_SYMMETRIC_KEY_TYPE = 2;
const int SEND_TEXT_MESSAGE_TYPE = 3;

// consts for response codes
const int RESPONSE_GENERAL_ERROR = 9000;
const int RESPONSE_REGISTER_SUCCESS = 2000;
const int RESPONSE_GET_CLIENTS_SUCCESS = 2001;
const int RESPONSE_GET_PUBLIC_KEY_SUCCESS = 2002;
const int RESPONSE_SENDING_MESSAGE_SUCCESS = 2003;
const int RESPONSE_PULLING_MESSAGES_SUCCESS = 2004;


//log 
const long int MAX_LOG_SIZE = 1024 * 1024 * 10;