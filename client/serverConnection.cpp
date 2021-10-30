# include <sstream>
# include <fstream>
# include <boost/filesystem.hpp>
# include <boost/asio.hpp>
# include "consts.h"
#include <vector>
# include "serverConnection.h"
# include <boost/log/trivial.hpp>
# include "exceptions.h"



ServerConnection::ServerConnection() {
	this->connection = new tcp::socket(io_context);
	tcp::resolver resolver(io_context);
	this->ParseServerInfo();
	try {
		boost::asio::connect(*connection, resolver.resolve(this->m_ip, this->m_port));
	}
	catch (const boost::system::system_error& ex) {
		std::cerr << "Connection cannot be made" << std::endl;
		exit(0);
	}
}

ServerConnection::~ServerConnection() {
	this->connection->close();
	delete this->connection;
}

bool ServerConnection::isValidIpAddress(const std::string& ipAddress) {
	/// check if ip address is valid
	/// 
	/// <param name="ipAddress"> string of ip address</param>
	/// <returns> true if valid otherwise false </returns>
	struct sockaddr_in socketAddr;
	if (inet_pton(AF_INET, ipAddress.c_str(), &(socketAddr.sin_addr)) == 1) {
		return true;
	}
	return false;
};

bool ServerConnection::isValidPort(const std::string& port) {
	/// check if port is valid
	/// 
	/// <param name="port"> string of port</param>
	/// <returns> true if valid otherwise false </returns>
	unsigned int portNumber;
	try {
		portNumber = std::stoi(port);
		if (portNumber >= MIN_PORT && portNumber <= MAX_PORT)
			return true;
	}
	catch (const std::exception& e) {
		return false;
	}

	return false;
}
void ServerConnection::send(std::vector<unsigned char> message) {
	boost::asio::write(*this->connection,
		boost::asio::buffer(message.data(), message.size()));
};

unsigned int ServerConnection::recv(unsigned int size, std::vector<uint8_t>& message) {
	std::string data;
	size_t reply_length = boost::asio::read(*this->connection,
		boost::asio::buffer(message.data(), size));
	return reply_length;
};

void ServerConnection::ParseServerInfo() {
	/// parse SERVER_INFO_FILE
	/// throw ServerInfoFileError if not exists or not in format
	/// store server detail m_port, m_ip
	std::string ip;
	std::string port;
	if (!boost::filesystem::exists(SERVER_INFO_FILE)) {
		throw ServerInfoFileError();
	};

	boost::filesystem::ifstream input;
	input.open(SERVER_INFO_FILE);

	if (!std::getline(input, ip, ':')) throw ServerInfoFileError();
	if (!std::getline(input, port)) throw ServerInfoFileError();
	input.close();

	if (!isValidIpAddress(ip) || !isValidPort(port)) {
		std::cerr << "ip or host are invalid" << std::endl;
		std::cout << ERROR_MESSAGE;
	}
	else {
		this->m_ip = ip;
		this->m_port = port;
	}
}


