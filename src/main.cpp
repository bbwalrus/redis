#include <iostream>
#include "../include/RedisServer.h"

int main(int argc, char* argv[]) {
    // use default port 6379 if not specified
    int port = 6379;
    // if a port is specified as a command line argument, use it
    if (argc >= 2) port = std::stoi(argv[1]);

    // call our server
    RedisServer server(port);

    // Background persistance, dump database every 300 seconds (5 * 60 save database)
    std::thread persistanceThread([]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(300));
            // dump database
        }
    });
    persistanceThread.detach();
    
    server.run();
    return 0;
} 