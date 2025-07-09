#ifndef REDIS_DATABASE_H
#define REDIS_DATABASE_H

#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <chrono>

class RedisDatabase {
    public:
        // get the singleton instance of RedisDatabase
        static RedisDatabase& getInstance();

        // commands  
        bool flushAll(); // Flush all data

        // key value operations
        bool set(const std::string& key, const std::string& value);
        bool get(const std::string& key, std::string& value);
        std::vector<std::string> keys(); // Get all keys
        std::string type(const std::string& key); // Get type of key (string, list, hash)
        bool del(const std::string& key); // Delete a key

        //expire
        bool expire(const std::string& key, const std::string& seconds); // Set expiration time for a key

        //rename
        bool rename(const std::string& oldKey, const std::string& newKey); // Rename a key

        // Persistance: Dump / load database from file
        bool dump(const std::string& filename);
        bool load(const std::string& filename);


    private:
        RedisDatabase() = default; // Private constructor to prevent instantiation
        ~RedisDatabase() = default; // Private destructor to prevent deletion   
        RedisDatabase(const RedisDatabase&) = delete; // Prevent copy construction
        RedisDatabase& operator=(const RedisDatabase&) = delete; // Prevent assignment

        std::mutex db_mutex; // Mutex for thread safety
        std::unordered_map<std::string, std::string> kv_store; // datastructure for key-value store
        std::unordered_map<std::string, std::vector<std::string>> list_store; // datastructure for lists
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash_store; // datastructure for hashes

        std::unordered_map<std::string, std::chrono::steady_clock::time_point> expiry_map; // Store expiration times for keys
};

#endif
