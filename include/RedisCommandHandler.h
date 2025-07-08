#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H

#include <string>

class RedisCommandHandler {
public:
    // Constructor
    RedisCommandHandler() = default;
    // Process a command from ma client and return RESP formatted response
    std::string processCommand(const std::string& commandLine);
};

#endif 