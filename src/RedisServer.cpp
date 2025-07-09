#include "../include/RedisServer.h"
#include "../include/RedisCommandHandler.h"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <vector>
#include <thread>
#include <cstring>

// Global instance of RedisServer
static RedisServer* globalServer = nullptr;

void signalHandler(int signum) {
    if (globalServer) {
        std::cout << "Received signal " << signum << ", shutting down server...\n";
        globalServer->shutdown();
    }
    exit(signum);
}

void RedisServer::setupSignalHandler() {
    signal(SIGINT, signalHandler);  // Handle Ctrl+C
}

// Constructor for RedisServer
RedisServer::RedisServer(int port) : port(port), server_socket(-1), running(true) {
    // Set the global server instance
    globalServer = this; 
    setupSignalHandler(); // Setup signal handling for graceful shutdown
}

void RedisServer::shutdown() {
    // set running to false
    running = false;
    // check if socket is open and close it if it is
    if (server_socket != -1) {
        close(server_socket);
    }
    std::cout << "Server shutdown Complete!\n";
}

void RedisServer::run() {
    // Create a socket for the server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    // Check if socket creation was successful, if not there was error
    if (server_socket < 0) {
        std::cerr << "Error creating server socket\n";
        return;
    }
    
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // bind and listen, if it works then our redis is listening on the port
    if (bind(server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding server socket\n";
        close(server_socket);
        return;
    }

    if (listen(server_socket, 10) < 0) {
        std::cerr << "Error listening on server socket\n";
        return;
    }

    std::cout << "Redis Server Listening on port " << port << "\n";

    std::vector<std::thread> threads; // for handling multiple clients
    RedisCommandHandler cmdHandler;

    while (running) {
        int client_socket = accept(server_socket, nullptr, nullptr);
        // check if client socket connected successfully
        if (client_socket < 0) {
            if (running) {
                std::cerr << "Error Accepting Client Connection\n";
            }
            break;
        }
        threads.emplace_back([client_socket, &cmdHandler]() {
            char buffer[1024];
            while (true) {
                memset(buffer, 0, sizeof(buffer));
                int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                if (bytes <= 0) {
                    break; // Client disconnected or error
                }
                std::string request(buffer, bytes);
                std::string response = cmdHandler.processCommand(request);
                send(client_socket, response.c_str(), response.size(), 0);
            }
            close(client_socket);
        });
    }

    for (auto& t : threads) {
        // if joinable, join the thread
        if (t.joinable()) {
            t.join();
        }
    }

    // before shutdown check for persisting the database
    if (RedisDatabase::getInstance().dump("dump.my_rdb")) {
        std::cout << "Database dumped successfully to dump.my_rdb\n";
    } else {
        std::cerr << "Error dumping database\n";
    }

    // shutdown

}