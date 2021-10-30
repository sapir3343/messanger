#include "client.h"
#include "consts.h"
#include <vector>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "cryptopp_wrapper/Base64Wrapper.h"
#include "cryptopp_wrapper/RSAWrapper.h"
#include "cryptopp_wrapper/AESWrapper.h"
#include "serverConnection.h"
#include <iostream>
#include <iomanip>
#include "exceptions.h"
#include "structs/serverHeader.h"
#include "structs/registerResponse.h"
#include "structs/publicKeyResponse.h"
#include "structs/messageHeader.h"
#include "structs/messageResponseHeader.h"

Client::Client()
{
	if (this->loadInformation()) {
		this->rsapriv_decrypt.setPrivateKey(Base64Wrapper::decode(this->m_private_key_base64));
	}

	this->m_private_key_base64 = Base64Wrapper::encode(this->rsapriv_decrypt.getPrivateKey());
	
	this->m_public_key = this->rsapriv_decrypt.getPublicKey();
	this->rsapub_encrypt = new RSAPublicWrapper(this->m_public_key);
	this->server = new ServerConnection();
	
}

Client::~Client()
{
	delete server;
}

bool Client::checkRegistration()
{
	if (this->m_uuid_hex.empty()) {
		std::cerr << REGISTRATION_ERROR << std::endl;
		return false;
	}
	return true;
}

bool Client::saveNewUser(const std::string& username, const unsigned char client_id[16])
{
	this->m_uuid_hex = boost::algorithm::hex(std::string((char*)client_id));
	this->m_name = username;
	saveNewUserInFile();

	return true;;
}

bool Client::saveNewUserInFile()
{
	boost::filesystem::ofstream output;
	output.open(USER_INFO_FILE);
	
	boost::replace_all(this->m_private_key_base64, "\n", "");
	output << this->m_name << std::endl;
	output << this->m_uuid_hex << std::endl;
	output << this->m_private_key_base64 << std::endl;
	output.close();

	return true;;
}

void Client::registeToServer()
{
	/// register client to chat, save personal information if success 
	/// throw const boost::system::system_error if cant get response from server
	if (boost::filesystem::exists(USER_INFO_FILE)) {
		std::cerr << "user already registered" << std::endl;
		return;
	}

	std::string username;
	std::string message = "enter your name: ";
	if (!this->getUsernameFromUser(username, message)) {
		return;
	}

	std::vector<unsigned char> payload(USERNAME_LENGTH + PUBLIC_KEY_LENGTH, 0);
	std::copy(username.begin(), username.end(), payload.begin());
	std::copy(this->m_public_key.begin(), this->m_public_key.end(), 
					     payload.begin() + USERNAME_LENGTH);
	
	this->createAndSendPacketToServer(REGISTER_CODE, payload);
	std::vector<unsigned char> unparse_response;
	unparse_response = this->recvResponseFromServer(RESPONSE_REGISTER_SUCCESS);
	RegisterResponse* res = (RegisterResponse*)unparse_response.data();
	saveNewUser(username, res->client_id);
	std::cout << "client register successfully" << std::endl;
}

std::vector<uint8_t> Client::recvResponseFromServer(uint16_t excpected_return_code)
{
		/// read protocol expectes header 
		/// then read the rest of the packet accoding to payload_size
		/// throw ServerResponseWithError if server response with 9000
		/// throw UnexpectedServerResponse if server not response 
		/// with param: excpected_return_code
		/// return response payload
		std::vector<unsigned char> unparse_response_header;
		std::vector<unsigned char> unparse_response;
		
		unparse_response_header.resize(VERSION_LENGTH + CODE_LENGTH + PAYLOAD_SIZE_LENGTH, 0);
		try {
			this->server->recv(unparse_response_header.size(), unparse_response_header);
			ServerHeader* hdr = (ServerHeader*)unparse_response_header.data();
			if (hdr->code == RESPONSE_GENERAL_ERROR) {
				throw ServerResponseWithError();
			}
			if (hdr->code != excpected_return_code) {
				throw UnexpectedServerResponse();
			}
			
			unparse_response.resize(hdr->payload_size);
			this->server->recv(hdr->payload_size, unparse_response);	
		}
		catch (const boost::system::system_error& e) {
			std::cerr << "socket error" << std::endl;
			throw e;
		}

	return unparse_response;
}


void Client::createAndSendPacketToServer(uint16_t code, std::vector<unsigned char>& payload)
{
	/// create protocol header and whole packet and send it to server
	/// if connection has lost - reconnect to server 
	std::vector<unsigned char> header(HEADER_LENGTH, 0);
	std::vector<unsigned char> request;
	uint16_t command_code = static_cast<uint16_t>(code);
	uint32_t payload_size = static_cast<uint32_t>(payload.size());

	std::vector<uint8_t> uid_bytes;
	if (this->m_uuid_hex.empty())
	{
		uid_bytes.resize(16, 0);
	}
	else {
		boost::algorithm::unhex(this->m_uuid_hex, std::back_inserter(uid_bytes));
	}
	std::copy((std::uint8_t*)uid_bytes.data(), (std::uint8_t*)uid_bytes.data() + UUID_LENGTH,
		       header.begin());
	
	header[UUID_LENGTH] = CLIENT_VERSION;
	std::copy((std::uint8_t*)&code, (std::uint8_t*)&code + CODE_LENGTH, header.begin() + UUID_LENGTH + VERSION_LENGTH);
	std::copy((std::uint8_t*)&payload_size, (std::uint8_t*)&payload_size + sizeof(std::uint32_t), header.begin() + UUID_LENGTH + VERSION_LENGTH + CODE_LENGTH);
	request.insert(request.begin(), header.begin(), header.end());
	if (payload_size > 0) {
		request.insert(request.end(), payload.begin(), payload.end());
	}
	this->send(request);
}

void Client::send(std::vector<uint8_t>& request) {
	try {
		this->server->send(request);
	}
	catch (const boost::system::system_error& e) {
		std::cerr << "socket error" << e.what() << std::endl;
		// try to reconnected server 
		delete this->server;
		this->server = new ServerConnection();
		this->server->send(request);
	}
}

void Client::getUsersList()
{

	std::vector<unsigned char> message;
	std::vector<unsigned char> response;
	this->createAndSendPacketToServer(GET_ALL_CLIENTS_CODE, message);

	response = this->recvResponseFromServer(RESPONSE_GET_CLIENTS_SUCCESS);
	unsigned int clientsNumber = response.size() / SINGLE_CLIENT_RESPONSE_SIZE;

	std::cout << "Clients list:" << std::endl;
	for (unsigned int i = 0; i < clientsNumber; ++i) {
		std::vector<uint8_t> singleClientFromResponse(response.begin() + i * SINGLE_CLIENT_RESPONSE_SIZE,
			response.begin() + (i + 1) * SINGLE_CLIENT_RESPONSE_SIZE);
		SingleOtherClient singleClient;
		std::memcpy((uint8_t*)&singleClient.client_id, (uint8_t*)singleClientFromResponse.data(), UUID_LENGTH);
		std::memcpy((uint8_t*)&singleClient.name, (uint8_t*)singleClientFromResponse.data()
			+ UUID_LENGTH, singleClientFromResponse.size() - UUID_LENGTH);

		if (this->getClientById(singleClient.client_id) == -1) {
			singleClient.aes_encryptor = nullptr;
			//singleClient.public_key = "";
			this->m_clients_list.push_back(singleClient);
		}
		std::cout << "Client name: " << singleClient.name << std::endl;
		std::cout << "Client id: " << boost::algorithm::hex(std::string((char*)singleClient.client_id)) << std::endl;
		std::cout << "----------------------" << std::endl;
	}

	return;
}

bool Client::getUsernameFromUser(std::string &username, std::string &message) {
	// received username from client and check his length
	std::cout << message << std::endl;
	std::getline(std::cin, username);
	if (username.length() > USERNAME_LENGTH - 1) {
		std::cerr << USERNAME_IS_TOO_LONG << std::endl;
		return false;
	}
	return true;
}

int Client::getClientByUsername(std::string &username) {
	// search client in m_clients_list according useename
	for (unsigned int i = 0; i < this->m_clients_list.size(); ++i) {
		std::string clientName((char*)this->m_clients_list[i].name);
		if (clientName == username) {
			return i;
		}
	}
	return -1;
}

int Client::getClientById(const unsigned char* client_id) {
	// search client in m_clients_list according client_id
	for (unsigned int i = 0; i < this->m_clients_list.size(); ++i) {
		if (this->compareIds(client_id, this->m_clients_list[i].client_id)) {
			return i;
		}
	}
	return -1;
}

std::string Client::getUserPublicKey(const SingleOtherClient& singleClient)
{
	/// send request for get public key and manage the response
	std::vector<unsigned char> payload(UUID_LENGTH, 0);
	std::copy(singleClient.client_id, singleClient.client_id + UUID_LENGTH, payload.begin());
	
	this->createAndSendPacketToServer(GET_PUBLIC_KEY_CODE, payload);

	std::vector<unsigned char> unparse_response;
	unparse_response = this->recvResponseFromServer(RESPONSE_GET_PUBLIC_KEY_SUCCESS);

	PublicKeyResponse* response = (PublicKeyResponse*)unparse_response.data();

	if (!this->compareIds(response->client_id, singleClient.client_id)) {
		throw UnexpectedServerResponse();
	}

	std::string public_key((char*)response->public_key, PUBLIC_KEY_LENGTH);
	return public_key;
}

bool Client::compareIds(const unsigned char* id1, const unsigned char* id2) {
	// check if 2 ids are the same
	if (memcmp(id1, id2, UUID_LENGTH) != 0) {
		return false;
	}
	return true;
}

void Client::getUserPublicKeyWithUserInput()
{
	// Handle send request of get user public key
	SingleOtherClient* singleOtherClient = this->receivedAndFindClientInList(std::string("enter user name to get his public key: "));
	if (singleOtherClient == nullptr) {
		return;
	}
	singleOtherClient->public_key = this->getUserPublicKey(*singleOtherClient);
	std::cout << "Public key for " << singleOtherClient->name << " received successfully" << std::endl;
}

void Client::getClientWaitingMessages() {
	// Handle get all waiting messages request from user, print all messages.
	// for symetric key request - create and sent symetric key
	// for received symetric key - decrypt it and save it in m_clients_list
	// for text message - decrypt and print it.
	std::vector<unsigned char> message;
	std::vector<unsigned char> unparse_response;
	this->createAndSendPacketToServer(PULLING_MESSAGES_CODE, message);

	unparse_response = this->recvResponseFromServer(RESPONSE_PULLING_MESSAGES_SUCCESS);

	unsigned int i = 0;
	while (i < unparse_response.size()) {
		auto current_message_start = unparse_response.begin() + i;
		std::vector<unsigned char> unparse_header(unparse_response.begin() + i,
			unparse_response.begin() + i + sizeof MessageResponseHeader);
		MessageResponseHeader* hdr = (MessageResponseHeader*)unparse_header.data();

		std::vector<uint8_t> content(unparse_response.begin() + i + sizeof MessageResponseHeader,
			unparse_response.begin() + i + sizeof MessageResponseHeader + hdr->content_size);

		std::string message;
		std::string message_decryptes_content;

		int clientVectorIndex = getClientById(hdr->client_id);
		if (clientVectorIndex == -1) {
			std::cerr << USERNAME_CANNOT_FOUND << std::endl;
			return;
		}
		SingleOtherClient* senderClient = &this->m_clients_list[clientVectorIndex];
		
		switch (hdr->message_type) {
		case MessagesTypes::SymetricKeyRequest:
			if (senderClient->public_key.empty()) {
				senderClient->public_key = this->getUserPublicKey(*senderClient);
			}
			this->createAndSendSymetricKey(*senderClient);
			message = SYMETRIC_KEY_REQUEST;
			break;
		case MessagesTypes::SymetricKeyRecived:
		{
			message_decryptes_content.assign(current_message_start + sizeof MessageResponseHeader,
				current_message_start + sizeof MessageResponseHeader + hdr->content_size);
			std::string symetric_key_str = this->rsapriv_decrypt.decrypt(message_decryptes_content);
			senderClient->symetric_key.assign(symetric_key_str.begin(), symetric_key_str.end());
			senderClient->aes_encryptor = new AESWrapper((unsigned char*)senderClient->symetric_key.data(), AESWrapper::DEFAULT_KEYLENGTH);

			message = SYMETRIC_KEY_RECIVED;
			break;
		}
		case MessagesTypes::TextMessage:
			message_decryptes_content.assign(current_message_start + sizeof MessageResponseHeader,
				current_message_start + sizeof MessageResponseHeader + hdr->content_size);
			if (senderClient->symetric_key.empty()) {
				message = CANT_DECRYPT_MESSAGE;
			}
			else {
				try {
					message = senderClient->aes_encryptor->decrypt(message_decryptes_content.c_str(),
						message_decryptes_content.length());
				}
				catch (const std::exception& e) {
					message = CANT_DECRYPT_MESSAGE;
				}
			}
			break;

		}
		printMessage(*senderClient, message);
		i = i + sizeof MessageResponseHeader + hdr->content_size;
	}
	return;
}


bool Client::loadInformation() {
	if (!boost::filesystem::exists(USER_INFO_FILE)) {
		return false;
	}
	boost::filesystem::ifstream infoFile;
	try {
		infoFile.open("me.info");
		std::getline(infoFile, this->m_name);
		std::getline(infoFile, this->m_uuid_hex);
		std::getline(infoFile, this->m_private_key_base64);
		infoFile.close();
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	return false;
}

void Client::createAndSendSymetricKeyFromUser() { 
	/// Received client name from client, create and send to him symetric key.
	SingleOtherClient* singleOtherClient = this->receivedAndFindClientInList(std::string("enter user name: "));
	if (singleOtherClient == nullptr) {
		return;
	}
	this->createAndSendSymetricKey(*singleOtherClient);
	std::cout << "symetric key sent successfully" << std::endl;
}

SingleOtherClient* Client::receivedAndFindClientInList(std::string message) {
	std::string username;

	if (!this->getUsernameFromUser(username, message)) {
		return nullptr;
	}

	int clientVectorIndex = this->getClientByUsername(username);
	if (clientVectorIndex == -1) {
		std::cerr << USERNAME_CANNOT_FOUND << std::endl;
		return nullptr;
	}

	return &(this->m_clients_list[clientVectorIndex]);

}
void Client::sendSymetricKeyRequest() { 
	/// send request for symetric key to user according client input
	SingleOtherClient* singleOtherClient = this->receivedAndFindClientInList(std::string("enter user name: "));
	if (singleOtherClient == nullptr) {
		return;
	}
	std::vector<unsigned char> empty_content = {};
	this->sendMessage(*singleOtherClient, MessagesTypes::SymetricKeyRequest, empty_content);
	std::cout << "symetric key request message sent successfully" << std::endl;
}

bool Client::createAndSendSymetricKey(SingleOtherClient& clientToSend) {
	/// create symetric key for clientToSend
	/// save it in client and sent symetric key received message
	if (clientToSend.public_key.empty()) {
		std::cout << "you should have the user public key first" << std::endl;
		return false;
	}
	if (clientToSend.symetric_key.empty()) {
		unsigned char symetric_key[AESWrapper::DEFAULT_KEYLENGTH];
		clientToSend.aes_encryptor = new AESWrapper(AESWrapper::GenerateKey(symetric_key,
			AESWrapper::DEFAULT_KEYLENGTH), AESWrapper::DEFAULT_KEYLENGTH);
		std::string symetric_key_str(symetric_key, symetric_key + AESWrapper::DEFAULT_KEYLENGTH);
		clientToSend.symetric_key = symetric_key_str;
	}
	RSAPublicWrapper rsapub(clientToSend.public_key);
	std::string cipher = rsapub.encrypt((const char*)clientToSend.symetric_key.data(), 
										 clientToSend.symetric_key.size());
	std::vector<uint8_t> content(cipher.begin(), cipher.end());
	this->sendMessage(clientToSend, MessagesTypes::SymetricKeyRecived, content);

	return false;
}


void Client::sendATextMessageToUser() {
	// send a message text to user according to client inputs
	std::string message_to_send;
	std::string message_to_print = "enter user name of user you want to send a message to: ";
	SingleOtherClient * singleOtherClient = this->receivedAndFindClientInList(message_to_print);
	if (singleOtherClient == nullptr) {
		return;
	}

	if (singleOtherClient->aes_encryptor == nullptr) {
		std::cout << "you should ask for symetric key first" << std::endl;
		return;
	}

	std::cout << "insert your message(one line only): " << std::endl;
	std::getline(std::cin, message_to_send);

	std::string encrypt_message = singleOtherClient->aes_encryptor->encrypt(message_to_send.c_str(),
		message_to_send.length());

	std::vector<unsigned char> vector_encrypt_message(encrypt_message.begin(), encrypt_message.end());
	this->sendMessage(*singleOtherClient, MessagesTypes::TextMessage, vector_encrypt_message);
	std::cout << "message sent successfully" << std::endl;

}

void Client::sendMessage(SingleOtherClient& clientToSend, unsigned char type, const std::vector<unsigned char>& content) {
	/// Create message according to protocol and send to clientToSend with the content at the end.

	std::vector<unsigned char> message_header(sizeof MessageHeader, 0);
	std::vector<unsigned char> request;
	uint8_t message_type = static_cast<uint8_t>(type);
	uint32_t content_size = static_cast<uint32_t>(content.size());

	std::copy(clientToSend.client_id, clientToSend.client_id + UUID_LENGTH,
		message_header.begin());
	message_header[UUID_LENGTH] = message_type;
	std::copy((std::uint8_t*)&content_size, (std::uint8_t*)&content_size + sizeof(std::uint32_t), message_header.begin() + UUID_LENGTH + MESSAGE_CODE_LENGTH);
	request.insert(request.begin(), message_header.begin(), message_header.end());

	// append content to request
	if (content_size > 0) {
		request.insert(request.end(), content.begin(), content.end());
	}

	this->createAndSendPacketToServer(SENDING_MESSAGE_CODE, request);
	this->recvResponseFromServer(RESPONSE_SENDING_MESSAGE_SUCCESS);
}

void Client::printMessage(SingleOtherClient& sender_client, std::string message) {
	std::cout << "From: " << sender_client.name << std::endl;
	std::cout << "Content: " << std::endl;
	std::cout << message << std::endl;
	std::cout << "----<EOM>----" << std::endl;
	std::cout << std::endl;

}
