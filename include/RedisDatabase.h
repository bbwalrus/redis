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
        bool expire(const std::string& key, const std::string& seconds); // Set expiration time for a key
        bool rename(const std::string& oldKey, const std::string& newKey); // Rename a key

        // list operations
        ssize_t llen(const std::string& key); // Get length of list
        void lpush(const std::string& key, const std::string& value); // Push value to the left of the list
        void rpush(const std::string& key, const std::string& value); //
        bool lpop(const std::string& key, std::string& value); // Pop value from the left of the list
        bool rpop(const std::string& key, std::string& value); // Pop value from the right of the list
        int lrem(const std::string& key, int count, const std::string& value); // Remove value from the list
        bool lindex(const std::string& key, int index, std::string& value); // Get value at index in the list
        bool lset(const std::string& key, int index, const std::string& value); // Set value at index in the list

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
