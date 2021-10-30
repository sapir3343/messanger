#pragma once
#include <iostream>

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include "consts.h"

void init_log() { 
	/// initialize logger

	boost::log::add_file_log(
		boost::log::keywords::open_mode = std::ios::app, //append to the last log file, if yet to rotate
		boost::log::keywords::auto_flush = true, // flush data to file 
		boost::log::keywords::file_name = "messageU_%N.log", // templete for logfile name
		boost::log::keywords::rotation_size = MAX_LOG_SIZE, // file size before rotate
		boost::log::keywords::format = "[%TimeStamp%] [%Severity%] %Message%" // log output formate
	);

	boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);

	boost::log::add_common_attributes();
}

void log(const std::string& str, bool print = false) {
	if (str.empty()) return;
	if (print) {
		std::cout << str << std::endl;
	}
	BOOST_LOG_TRIVIAL(info) << str;
};

void logMyError(const std::string& str, bool print = false) {
	if (str.empty()) return;
	if (print) {
		std::cout << str << std::endl;
	}
	BOOST_LOG_TRIVIAL(error) << str;
}
