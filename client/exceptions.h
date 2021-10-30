#pragma once

#include <exception>
#include <string> 
#include <sstream> 
//#include "logger.cpp"
#include <boost/log/trivial.hpp>

class ServerInfoFileError : public std::exception
{
    // exception for trying to get not exists attribute
    const char* what() const throw ()
    {
        return "Server file format is not ok";
    }
};

class ServerResponseWithError : public std::exception
{
    // exception for getting 9000 from server
    const char* what() const throw ()
    {
        
        return "server error";
    }
};

class UnexpectedServerResponse : public std::exception
{
    // exception for getting unexpected code from server
    const char* what() const throw ()
    {
        
        return "server response with enexpected code";
    }
};
