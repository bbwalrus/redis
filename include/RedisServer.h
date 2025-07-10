#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include <string>
#include <atomic>

class RedisServer {
    public:
        RedisServer(int port);
        // our functions to run and shutdown the server
        void run();
        void shutdown();
        
    private:
        int port;
        int server_socket;
        // see if server is running 
        std::atomic<bool> running;
        
        // setup signal handling for graceful shutdown (even if we force quit with ctrl c will shutdown properly)
        void setupSignalHandler();
};

#endif