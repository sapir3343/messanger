#pragma once
# include <tuple>
#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
//#include "logger.h"

using boost::asio::ip::tcp;

class ServerConnection {
public:
	ServerConnection();
	~ServerConnection();
	void send(std::vector<unsigned char> message);
	unsigned int recv(unsigned int size, std::vector<uint8_t>& message);
private:
	std::string m_ip;
	std::string m_port;
	boost::asio::io_context io_context;
	tcp::socket* connection;
	void ParseServerInfo();
	bool isValidIpAddress(const std::string& ipAddress);
	bool isValidPort(const std::string & port);
	
};