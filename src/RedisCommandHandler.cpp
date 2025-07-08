#include "../include/RedisCommandHandler.h"

#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream> // for debugging

// RESP parser:
// *2/r/n$4/r/n/PING/r/n$4/r/nTest/r/n
// *2 -> array has 2 elements
// $4 -> next string has 4 chars
// PING
// TEST

std::vector<std::string> parseRespCommand(const std::string &input) {
    std::vector<std::string> tokens;
    if (input[0] != '*') {
        return tokens; // Invalid RESP format
    }
    // if doesnt start with *, fallback to splitting by whitespaces
    if (input[0] != '*') {
        std::istringstream iss(input);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }

    size_t pos = 0;
    // Expect * followed by the number of elements
    if (input[pos] != '*') {
        return tokens; // Invalid RESP format
    }
    pos ++; // Skip '*'

    // crlf = Carriage Return (/r), Line feed (/n)
    size_t crlf = input.find("\r\n", pos);
    if (crlf == std::string::npos) {
        return tokens; // if cant find
    }

    int numElements = std::stoi(input.substr(pos, crlf - pos));
    pos = crlf + 2;

    for (int i = 0; i < numElements; i++) {
        if (pos >= input.size() || input[pos] != '$') {
            break; // format error
        }
        pos++; // Skip '$'
       
        crlf = input.find("\r\n", pos);
        if (crlf == std::string::npos) {
            break;
        }
        int len = std::stoi(input.substr(pos, crlf - pos));
        pos = crlf + 2;
        if (pos + len > input.size()) {
            break;
        }
        std::string token = input.substr(pos, len);
        tokens.push_back(token);
        // skip token and crlf
        pos += len + 2;
    }
    return tokens;
}

RedisCommandHandler::RedisCommandHandler() {};

std::string RedisCommandHandler::processCommand(const std::string& commandLine) {
    auto tokens = parseRespCommand(commandLine);
    if (tokens.empty()) return "-Error: Empty command\r\n";
    for (auto& t : tokens) {
        std::cout << t << "\n";
    }

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    std::ostringstream response;
    
    // connect to database

    //check commands

    return response.str();
}