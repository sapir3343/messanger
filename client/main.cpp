// #include <boost/asio.hpp>
#include <map>
#include <boost/array.hpp>
#pragma comment(lib, "Ws2_32.lib")
#include <iostream>
#include <string>
#include <sstream>
#include "consts.h"
#include "client.h"
#include "exceptions.h"


// #include <windows.h>
// #include "client.h"

void PrintUsage()
{
	std::cout << "MessageU client at your service." << std::endl << std::endl;
	std::cout << Commands::Register << ") Register" << std::endl;
	std::cout << Commands::GetClientsList << ") Request for clients list" << std::endl;
	std::cout << Commands::GetPublicKey << ") Request for public key" << std::endl;
	std::cout << Commands::GetWaitingMessages << ") Request for waiting messages" << std::endl;
	std::cout << Commands::SendAMessage << ") Send a text message" << std::endl;
	std::cout << Commands::SendSymetricKeyRequest << ") Send a request for symmetric key" << std::endl;
	std::cout << Commands::SendSymetricKey << ") Send your symmetric key" << std::endl;
	std::cout << Commands::Exit << ") Exit client" << std::endl;
	std::cout << "?" << std::endl;
}

void HandleUserCommand(int userChoice, Client& client)
{
	try {
		if (userChoice == Commands::Register) {
			client.registeToServer();
			return;
		}
		else {
			if (!client.checkRegistration()) {
				return;
			}
		}
		switch (userChoice) {
		case Commands::Exit:
			exit(0);
			break;
		case Commands::GetClientsList:
			client.getUsersList();
			break;
		case Commands::GetPublicKey:
			client.getUserPublicKeyWithUserInput();
			break;
		case Commands::SendAMessage:
			client.sendATextMessageToUser();
			break;
		case Commands::GetWaitingMessages:
			client.getClientWaitingMessages();
			break;
		case Commands::SendSymetricKeyRequest:
			client.sendSymetricKeyRequest();
			break;
		case Commands::SendSymetricKey:
			client.createAndSendSymetricKeyFromUser();
			break;
		default:
			std::cout << ERROR_MESSAGE_COMMAND << std::endl;
			std::cout << std::endl;
			break;
		}
	}
	catch (ServerResponseWithError& e) {
		std::cout << ERROR_MESSAGE << std::endl;
	}
	catch (UnexpectedServerResponse& e) {
		std::cout << ERROR_MESSAGE << std::endl;
	}

	catch (const boost::system::system_error& e) {
		std::cout << ERROR_MESSAGE << std::endl;
	}
}

void ClearScreen() 
{
	// clear scream from data
	// Sleep(5000);
	/*std::cout << std::string(100, '\n');*/
	system("cls");
}

int main() 
{
	
	std::string userInputString;
	unsigned int userInput;
	try {
		Client client;

		while (true) {
			// print usage meesage
			PrintUsage();
			// Get user input
			std::getline(std::cin, userInputString);

			if (userInputString == "?") {
				continue;
			}
			// Convert user input to int
			else {
				try {
					userInput = std::stoi(userInputString);
					ClearScreen();
					HandleUserCommand(userInput, client);
				}
				catch (const std::exception& e) {
					ClearScreen();
					std::cout << ERROR_MESSAGE_COMMAND << std::endl;
					std::cout << std::endl;
				}
			}
		}
	}
	catch (const boost::system::system_error& e) {
		std::cout << ERROR_IN_SERVER << std::endl;
		exit(0);
	}
	catch (const ServerInfoFileError e) {
		std::cerr << "server info file is not exists" << std::endl;
		exit(0);
	}
	return 0;
}