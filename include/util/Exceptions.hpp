#pragma once

#include <stdexcept>
#include <string>



class BaseException : public std::runtime_error {
public:
    explicit BaseException(const std::string& message) : std::runtime_error(message) {}
};

class NetworkException : public BaseException {
public:
    explicit NetworkException(const std::string& message) : BaseException(message) {}
};

class SimulationException : public BaseException {
public:
    explicit SimulationException(const std::string& message) : BaseException(message) {}
};

class ParseException : public BaseException {
public:
    explicit ParseException(const std::string& message) : BaseException(message) {}
};


