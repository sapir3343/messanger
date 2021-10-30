#pragma once
#include <sstream>
#include <vector>
#include "otherClient.h"
#include "serverConnection.h"
#include "cryptopp_wrapper/AESWrapper.h"
#include "cryptopp_wrapper/RSAWrapper.h"
#include "structs/singleOtherClient.h"

class Client {
	public:
		Client();
		~Client();
		bool checkRegistration();

		// manage user requests 
		void registeToServer();
		void getUsersList();
		void getUserPublicKeyWithUserInput();
		void createAndSendSymetricKeyFromUser();
		void sendSymetricKeyRequest();
		void sendATextMessageToUser();
		void getClientWaitingMessages();
	private:
		// server conection
		ServerConnection *server;

		// client attrubutes
		std::string m_name;
		std::string m_uuid_hex;
		std::string m_private_key_base64;
		std::string m_public_key;
		std::vector<SingleOtherClient> m_clients_list;

		// encryption objects 
		RSAPublicWrapper* rsapub_encrypt;
		RSAPrivateWrapper rsapriv_decrypt;
		bool saveNewUserInFile();
		bool loadInformation();
		bool compareIds(const unsigned char* id1, const unsigned char* id2);
		bool createAndSendSymetricKey(SingleOtherClient& clientToSend);
		bool getUsernameFromUser(std::string& username, std::string& message);
		bool saveNewUser(const std::string& username, const unsigned char client_id[UUID_LENGTH]);
		int getClientByUsername(std::string& username);
		int getClientById(const unsigned char* client_id);
		void send(std::vector<uint8_t>& request);
		std::string getUserPublicKey(const SingleOtherClient& singleClient);
		std::vector<uint8_t> recvResponseFromServer(uint16_t excpected_return_code);
		std::vector<uint8_t> recvMessagesFromServer();
		SingleOtherClient* receivedAndFindClientInList(std::string message);
		void createAndSendPacketToServer(uint16_t code, std::vector<unsigned char>& payload);
		void printMessage(SingleOtherClient& sender_client, std::string message);
		void sendMessage(SingleOtherClient& clientToSend, unsigned char type, const std::vector<unsigned char>& content);
};
		