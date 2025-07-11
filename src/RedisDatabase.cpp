#include "../include/RedisDatabase.h"

#include <mutex>
#include <fstream>
#include <sstream>
#include <algorithm>

RedisDatabase& RedisDatabase::getInstance() {
    static RedisDatabase instance;
    return instance;
}

// Flush all data
bool RedisDatabase::flushAll() {
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store.clear();
    list_store.clear();
    hash_store.clear();
    expiry_map.clear(); // Clear expiration times
    return true;
}

// key value operations
bool RedisDatabase::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store[key] = value;
    // Clear any existing expiration time for this key
    expiry_map.erase(key);
    return true;
}

bool RedisDatabase::get(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = kv_store.find(key);
    if (it != kv_store.end()) {
        value = it->second;
        return true;
    }
    return false; // Key not found
}

// Get all keys
std::vector<std::string> RedisDatabase::keys() {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<std::string> result;
    for (const auto& pair : kv_store) {
        result.push_back(pair.first);
    }
    for (const auto& pair : list_store) {
        result.push_back(pair.first);
    }
    for (const auto& pair : hash_store) {
        result.push_back(pair.first);
    }
    return result;
}
// Get type of key (string, list, hash)
std::string RedisDatabase::type(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (kv_store.find(key) != kv_store.end()) {
        return "string";
    } else if (list_store.find(key) != list_store.end()) {
        return "list";
    } else if (hash_store.find(key) != hash_store.end()) {
        return "hash";
    }
    else {
        return "none"; // Key does not exist
    }
}
 // Delete a key
bool RedisDatabase::del(const std::string& key) { 
    std::lock_guard<std::mutex> lock(db_mutex);
    bool erased = false;
    // Remove from key-value store
    erased |= (kv_store.erase(key) > 0);
    // Remove from list store
    erased |= (list_store.erase(key) > 0);
    // Remove from hash store
    erased |= (hash_store.erase(key) > 0);
    return false;
}

// Set expiration time for a key
bool RedisDatabase::expire(const std::string& key, const std::string& seconds) { 
    std::lock_guard<std::mutex> lock(db_mutex);
    bool exist = (kv_store.find(key) != kv_store.end() ||
                     list_store.find(key) != list_store.end() ||
                     hash_store.find(key) != hash_store.end());
    if (!exist) {
        return false; // Key does not exist
    }

    expiry_map[key] = std::chrono::steady_clock::now() + std::chrono::seconds(std::stoi(seconds));
    return true;
}

// Rename a key
bool RedisDatabase::rename(const std::string& oldKey, const std::string& newKey) { 
    std::lock_guard<std::mutex> lock(db_mutex);
    // Check if oldKey exists
    bool found = false;

    auto itKv = kv_store.find(oldKey);
    if (itKv != kv_store.end()) {
        kv_store[newKey] = itKv->second; // Copy value to new key
        kv_store.erase(itKv); // Remove old key
        found = true;
    }

    auto itList = list_store.find(oldKey);
    if (itList != list_store.end()) {
        list_store[newKey] = itList->second; // Copy list to new key
        list_store.erase(itList); // Remove old key
        found = true;
    }

    auto itHash = hash_store.find(oldKey);
    if (itHash != hash_store.end()) {
        hash_store[newKey] = itHash->second; // Copy hash to new key
        hash_store.erase(itHash); // Remove old key
        found = true;
    }

    auto itExpiry = expiry_map.find(oldKey);
    if (itExpiry != expiry_map.end()) {
        expiry_map[newKey] = itExpiry->second; // Copy expiration time to new key
        expiry_map.erase(itExpiry); // Remove old key
        found = true;
    }

    return found;
}

// Get length of list
ssize_t RedisDatabase::llen(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it != list_store.end()) {
        return it->second.size(); // Return the size of the list
    }
    return 0; // Key not found or not a list
}

// Push value to the left of the list
void RedisDatabase::lpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    list_store[key].insert(list_store[key].begin(), value); // Insert at the front of the list
}

// Push value to the right of the list
void RedisDatabase::rpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    list_store[key].push_back(value);
}

// Pop value from the left of the list
bool RedisDatabase::lpop(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it != list_store.end() && !it->second.empty()) {
        value = it->second.front();
        it->second.erase(it->second.begin()); // Remove the first element
        return true;
    }
    return false; // Key not found or list is empty
}

// Pop value from the right of the list
bool RedisDatabase::rpop(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it != list_store.end() && !it->second.empty()) {
        value = it->second.back();
        it->second.pop_back();
        return true;
    }
    return false; // Key not found or list is empty
}

// Remove value from the list
int RedisDatabase::lrem(const std::string& key, int count, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    int removed = 0;
    auto it = list_store.find(key);
    if (it == list_store.end()) {
        return 0; // Key not found
    }

    auto& lst = it->second;

    if (count == 0) {
        // Remove all occurrences
        auto new_end = std::remove(lst.begin(), lst.end(), value);
        removed = std::distance(new_end, lst.end());
        lst.erase(new_end, lst.end()); // Erase the removed elements
    }
    else if (count > 0) {
        // Remove first 'count' occurrences
        for (auto iter = lst.begin(); iter != lst.end() && removed < count; ) {
            if (*iter == value) {
                iter = lst.erase(iter); // Erase returns the next iterator
                ++removed;
            } else {
                ++iter;
            }
        }
    } else {
        // Remove last 'count' occurrences
        for (auto riter = lst.rbegin(); riter != lst.rend() && removed < (-count); ) {
            if (*riter == value) {
                auto fwdIterator = riter.base();
                --fwdIterator;
                fwdIterator = lst.erase(fwdIterator); // Erase returns the next iterator
                ++removed;
                riter = std::reverse_iterator<std::vector<std::string>::iterator>(fwdIterator);
            }
            else {
                ++riter;
            }
        }
    }
}

// Get value at index in the list
bool RedisDatabase::lindex(const std::string& key, int index, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it == list_store.end()) {
        return false; // Key not found
    }
    const auto& lst = it->second;
    if (index < 0) {
        index = lst.size() + index;
    }
    if (index < 0 || static_cast<size_t>(index) >= lst.size()) {
        return false; // Index out of range
    }

    value = lst[index]; // Get the value at the specified index
    return true; // Successfully retrieved value
}

// Set value at index in the list
bool RedisDatabase::lset(const std::string& key, int index, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it == list_store.end()) {
        return false;
    }

    auto& lst = it->second;
    if (index < 0) {
        index = lst.size() + index;
    }
    if (index < 0 || static_cast<size_t>(index) >= lst.size()) {
        return false; // Index out of range
    }

    lst[index] = value; // Get the value at the specified index
    return true;
        
}

/*
Simple text based persistence format where each line encodes a record

Memory -> File -> dump()
File -> Memory - load()

K = key value
L = list
H = hash
*/

bool RedisDatabase::dump(const std::string& filename) {
    // use mutex to be thread safe, lock the database mutex
    std::lock_guard<std::mutex> lock(db_mutex);
    // Open the file for writing
    std::ofstream ofs(filename, std::ios::binary);
    // Check if the file is open
    if (!ofs) return  false;

    for (const auto& kv: kv_store) {
        ofs << "K " << kv.first << " " << kv.second << "\n";
    }
    for (const auto& kv: list_store) {
        ofs << "L " << kv.first;
        for (const auto& item : kv.second) {
            ofs << " " << item;
        }
        ofs << "\n";
    }
    for (const auto& kv : hash_store) {
        ofs << "H " << kv.first;
        for (const auto& field_val : kv.second) {
            ofs << " " << field_val.first << ":" << field_val.second;
        }
        ofs << "\n";
    }
    return true;
}

bool RedisDatabase::load(const std::string& filename) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) return false;

    kv_store.clear();
    list_store.clear();
    hash_store.clear(); 

    std::string line;
    while (std::getline(ifs,line)) {
        std::istringstream iss(line);
        char type;
        iss >> type;
        if (type == 'K') {
            std::string key, value;
            iss >> key >> value;
            kv_store[key] = value;
        } else if (type == 'L') {
            std::string key;
            iss >> key;
            std::string iteml;
            std::vector<std::string> list;
            while (iss >> iteml) {
                list.push_back(iteml);
            }
            list_store[key] = list;
        } else if (type == 'H') {
            std::string key;
            iss >> key;
            std::unordered_map<std::string, std::string> hash;
            std::string pair;
            while (iss >> pair) {
                auto pos = pair.find(':');
                if (pos != std::string::npos) {
                    std::string field = pair.substr(0, pos);
                    std::string value = pair.substr(pos + 1);
                    hash[field] = value;
                }
            }
            hash_store[key] = hash;
        }
    }
    return true;
}