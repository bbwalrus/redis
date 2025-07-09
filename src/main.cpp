#include "../include/RedisServer.h"
#include "../include/RedisDatabase.h"
#include <thread>
#include <chrono>
#include <iostream>

int main(int argc, char* argv[]) {
    // use default port 6379 if not specified
    int port = 6379;
    // if a port is specified as a command line argument, use it
    if (argc >= 2) port = std::stoi(argv[1]);

    // call our server
    RedisServer server(port);

    // Background persistance, dump database every 300 seconds (5 * 60 seconds save database)
    // Threads to handle multiple clients
    std::thread persistanceThread([]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(300));
            // dump database
            if (!RedisDatabase::getInstance().dump("dump.my_rdb")) {
                std::cerr << "Error dumping database\n";
            } else {
                std::cout << "Database dumped successfully to dump.my_rdb\n";
            }
        }
    });
    persistanceThread.detach();
    
    server.run();
    return 0;
} 