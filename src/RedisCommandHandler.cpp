#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"

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

    // debugging
    // for (auto& t : tokens) {
    //     std::cout << t << "\n";
    // }

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    std::ostringstream response;
    
    // connect to database
    RedisDatabase& db = RedisDatabase::getInstance();
    
    //check commands (long list of commands)
    // common commands
    if (cmd == "PING") {
        response << "+PONG\r\n";
    } 
    else if (cmd == "ECHO") {
        if (tokens.size() < 2) {
            response << "-Error: ECHO command requires a message\r\n";
        } 
        else {
            response << "+" << tokens[1] << "\r\n"; // echo the message
        }
    }
    else if (cmd == "FLUSHALL") {
        db.flushAll();
        response << "+OK\r\n";
    }
    // key/value operations
    else if (cmd == "SET") {
        if (tokens.size() < 3) {
            response << "-Error: SET requires key and value\r\n";
        }
        else {
            db.set(tokens[1], tokens[2]);
            response << "+OK\r\n";
        }
    }
    else if (cmd == "GET") {
        if (tokens.size() < 2) {
            response << "-Error: GET requires a key\r\n";
        } 
        else {
            std::string value;
            if (db.get(tokens[1], value)) {
                response << "-Error: Get requires a key\r\n";
            } 
            else {
                std::string value;
                if (db.get(tokens[1], value)) {
                    response << "$" << value.size() << "\r\n" << value << "\r\n";
                }
                else {
                    response << "$-1\r\n"; // key not found
                }
            }
        }
    }
    else if (cmd == "KEYS") {
        std::vector<std::string> keys = db.keys();
        response << "*" << keys.size() << "\r\n";
        for (const auto& key : keys) {
            response << "$" << key.size() << "\r\n" << key << "\r\n";
        }
    }
    else if (cmd == "TYPE") {
        if (tokens.size() < 2) {
            response << "-Error: TYPE requires a key\r\n";
        } 
        else {
            response << "+" << db.type(tokens[1]) << "\r\n"; // return type of key
        }
    }
    else if (cmd == "DEL" || cmd == "UNLINK") {
        if (tokens.size() < 2) {
            response << "-Error: " << cmd << " requires a key\r\n";
        } 
        else {
            bool res = db.del(tokens[1]);
            response << ":" << (res ? 1 : 0) << "\r\n"; // return number of keys deleted
        }
    }
    else if (cmd == "EXPIRE") {
        if (tokens.size() < 3) {
            response << "-Error: EXPIRE requires a key and seconds\r\n";
        } 
        else {
            if (db.expire(tokens[1], tokens[2])) {
                response << "+Ok\r\n";
            }
            else {
                response << "-Error: EXPIRE failed\r\n";
            }
            
        }
    }
    else if (cmd == "RENAME") {
        if (tokens.size() < 3) {
            response << "-Error: RENAME requires a old key name and a new key name\r\n";
        } 
        else {
            if (db.rename(tokens[1], tokens[2])) {
                response << "+Ok\r\n";
            }
            else {
                response << "-Error: RENAME failed\r\n";
            }
        }
    }
    // list operations
    // hash operations

    else {
        response << "-Error: Unknown command '" << cmd << "'\r\n";
    }


    return response.str();
}