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

// Basic command helper functions to process commands
static std::string handlePing(const std::vector<std::string>& tokens, RedisDatabase& db) {
    return "+PONG\r\n";
}

static std::string handleEcho(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) {
        return "-Error: ECHO command requires a message\r\n";
    } 
    return "+" + tokens[1] + "\r\n";
}

static std::string handleFlushAll(const std::vector<std::string>& tokens, RedisDatabase& db) {
    db.flushAll();
    return "+OK\r\n";
}

// Key vallue operations

static std::string handleSet(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) {
        return "-Error: SET requires key and value\r\n";
    }
    db.set(tokens[1], tokens[2]);
    return "+OK\r\n";
}

static std::string handleGet(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) {
        return "-Error: GET requires a key\r\n";
    } 
    std::string value;
    if (db.get(tokens[1], value)) {
        return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
    }
    return "$-1\r\n"; // key not found
}

static std::string handleKeys(const std::vector<std::string>& tokens, RedisDatabase& db) {
    std::vector<std::string> keys = db.keys();
    std::string result = "*" + std::to_string(keys.size()) + "\r\n";
    for (const auto& key : keys) {
        result += "$" + std::to_string(key.size()) + "\r\n" + key + "\r\n";
    }
    return result;
}

static std::string handleType(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) {
        return "-Error: TYPE requires a key\r\n";
    } 
    return "+" + db.type(tokens[1]) + "\r\n";
}

static std::string handleDel(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) {
        return "-Error: DEL requires a key\r\n";
    } 
    bool res = db.del(tokens[1]);
    return ":" + std::to_string(res ? 1 : 0) + "\r\n";
}

static std::string handleExpire(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) {
        return "-Error: EXPIRE requires a key and seconds\r\n";
    } 
    if (db.expire(tokens[1], tokens[2])) {
        return "+OK\r\n";
    }
    return "-Error: EXPIRE failed\r\n";
}

static std::string handleRename(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) {
        return "-Error: RENAME requires a old key name and a new key name\r\n";
    } 
    if (db.rename(tokens[1], tokens[2])) {
        return "+OK\r\n";
    }
    return "-Error: RENAME failed\r\n";
}

// List Operations
static std::string handleLlen (const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) {
        return "-Error: LLEN requires a key\r\n";
    }
    ssize_t len = db.llen(tokens[1]);
    return ":" + std::to_string(len) + "\r\n";
}

static std::string handleLpush (const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) {
        return "-Error: LPUSH requires a key and at least one value\r\n";
    }
    for (size_t i = 2; i < tokens.size(); ++i) {
        db.lpush(tokens[1], tokens[i]);
    }
    ssize_t len = db.llen(tokens[1]);
    return ":" + std::to_string(len) + "\r\n";
}

static std::string handleRpush (const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) {
        return "-Error: RPUSH requires a key and at least one value\r\n";
    }
    for (size_t i = 2; i < tokens.size(); ++i) {
        db.rpush(tokens[1], tokens[i]);
    }
    ssize_t len = db.llen(tokens[1]);
    return ":" + std::to_string(len) + "\r\n";
}

static std::string handleLpop (const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) {
        return "-Error: LPOP requires a key\r\n";
    }
    std::string value;
    if (db.lpop(tokens[1], value)) {
        return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
    }
    return "$-1\r\n"; // list is empty or key not found
}

static std::string handleRpop (const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) {
        return "-Error: RPOP requires a key\r\n";
    }
    std::string value;
    if (db.rpop(tokens[1], value)) {
        return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
    }
    return "$-1\r\n"; // list is empty or key not found
}

static std::string handleLrem (const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 4) {
        return "-Error: LREM requires a key, count and value\r\n";
    }
    try {
        int count = std::stoi(tokens[2]);
        int removed = db.lrem(tokens[1], count, tokens[3]);
        return ":" + std::to_string(removed) + "\r\n";
    } catch (const std::exception& e) {  
        return "-Error: Invalid count for LREM\r\n";
    }
}

static std::string handleLindex (const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) {
        return "-Error: LINDEX requires a key and an index\r\n";
    }
    try {
        int index = std::stoi(tokens[2]);
        std::string value;
        if (db.lindex(tokens[1], index, value)) {
            return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
        }
        return "$-1\r\n"; // index out of range or key not found
    } catch (const std::exception& e) {
        return "-Error: Invalid index for LINDEX\r\n";
    }
}

static std::string handleLset (const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 4) {
        return "-Error: LSET requires a key, index and value\r\n";
    }
    try {
        int index = std::stoi(tokens[2]);
        if (db.lset(tokens[1], index, tokens[3])) {
            return "+OK\r\n";
        }
        return "-Error: LSET failed\r\n"; // index out of range or key not found
    } catch (const std::exception& e) {
        return "-Error: Invalid index for LSET\r\n";
    }
}

// hash operations
static std::string handleHset(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 4) {
        return "-Error: HSET requires a key, field and value\r\n";
    }
    db.hset(tokens[1], tokens[2], tokens[3]);
    return ":1\r\n"; // HSET always returns 1 for success
}

static std::string handleHget(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) {
        return "-Error: HGET requires a key and field\r\n";
    }
    std::string value;
    if (db.hget(tokens[1], tokens[2], value)) {
        return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
    }
    return "$-1\r\n"; // field not found
}

static std::string handleHexists(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) {
        return "-Error: HEXISTS requires a key and field\r\n";
    }
    bool exists = db.hexists(tokens[1], tokens[2]);
    return ":" + std::to_string(exists ? 1 : 0) + "\r\n";
}

static std::string handleHdel(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) {
        return "-Error: HDEL requires a key and field\r\n";
    }
    bool deleted = db.hdel(tokens[1], tokens[2]);
    return ":" + std::to_string(deleted ? 1 : 0) + "\r\n"; // returns 1 if field was deleted, 0 if it didn't exist
}

static std::string handleHgetall (const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 
        return "-Error: HGETALL requires key\r\n";
    auto hash = db.hgetall(tokens[1]);
    std::ostringstream oss;
    oss << "*" << hash.size() * 2 << "\r\n";
    for (const auto& pair: hash) {
        oss << "$" << pair.first.size() << "\r\n" << pair.first << "\r\n";
        oss << "$" << pair.second.size() << "\r\n" << pair.second << "\r\n";
    }
    return oss.str();
}

static std::string handleHkeys(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 
        return "-Error: HKEYS requires key\r\n";
    auto keys = db.hkeys(tokens[1]);
    std::ostringstream oss;
    oss << "*" << keys.size() << "\r\n";
    for (const auto& key: keys) {
        oss << "$" << key.size() << "\r\n" << key << "\r\n";
    }
    return oss.str();
}

static std::string handleHvals(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 
        return "-Error: HVALS requires key\r\n";
    auto values = db.hvals(tokens[1]);
    std::ostringstream oss;
    oss << "*" << values.size() << "\r\n";
    for (const auto& val: values) {
        oss << "$" << val.size() << "\r\n" << val << "\r\n";
    }
    return oss.str();
}

static std::string handleHlen(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) {
        return "-Error: HLEN requires a key\r\n";
    }
    size_t len = db.hlen(tokens[1]);
    return ":" + std::to_string(len) + "\r\n"; // returns the number of fields in the hash
}

static std::string handleHmset(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) {
        return "-Error: HMSET requires a key and at least one field-value pair\r\n";
    }
    std::string key = tokens[1];
    for (size_t i = 2; i < tokens.size(); i += 2) {
        if (i + 1 >= tokens.size()) {
            return "-Error: HMSET requires field and value pairs\r\n";
        }
        db.hset(key, tokens[i], tokens[i + 1]);
    }
    return "+OK\r\n"; // HMSET always returns OK
}

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
    
    // common commands
    if (cmd == "PING") {
        return handlePing(tokens, db);
    } 
    else if (cmd == "ECHO") {
        return handleEcho(tokens, db);
    }
    else if (cmd == "FLUSHALL") {
        return handleFlushAll(tokens, db);
    }
    // key value operations
    else if (cmd == "SET") {
        return handleSet(tokens, db);
    }
    else if (cmd == "GET") {
        return handleGet(tokens, db);
    }
    else if (cmd == "KEYS") {
        return handleKeys(tokens, db);
    }
    else if (cmd == "TYPE") {
        return handleType(tokens, db);
    }
    else if (cmd == "DEL" || cmd == "UNLINK") {
        return handleDel(tokens, db);
    }
    else if (cmd == "EXPIRE") {
        return handleExpire(tokens, db);
    }
    else if (cmd == "RENAME") {
        return handleRename(tokens, db);
    }
    // List operations
    else if (cmd == "LLEN") {
        return handleLlen(tokens, db);
    }
    else if (cmd == "LPUSH") {
        return handleLpush(tokens, db);
    }
    else if (cmd == "RPUSH") {
        return handleRpush(tokens, db);
    }
    else if (cmd == "LPOP") {
        return handleLpop(tokens, db);
    }
    else if (cmd == "RPOP") {
        return handleRpop(tokens, db);
    }
    else if (cmd == "LREM") {
        return handleLrem(tokens, db);
    }
    else if (cmd == "LINDEX") {
        return handleLindex(tokens, db);
    }
    else if (cmd == "LSET") {
        return handleLset(tokens, db);
    }
    else if (cmd == "HSET") {
        return handleHset(tokens, db);
    }
    else if (cmd == "HGET") {
        return handleHget(tokens, db);
    }
    else if (cmd == "HEXISTS") {
        return handleHexists(tokens, db);
    }
    else if (cmd == "HDEL") {
        return handleHdel(tokens, db);
    }
    else if (cmd == "HGETALL") {
        return handleHgetall(tokens, db);
    }
    else if (cmd == "HKEYS") {
        return handleHkeys(tokens, db);
    }
    else if (cmd == "HVALS") {
        return handleHvals(tokens, db);
    }
    else if (cmd == "HLEN") {
        return handleHlen(tokens, db);
    }
    else if (cmd == "HMSET") {
        return handleHmset(tokens, db);
    }
    else {
        return "-Error: Unknown command '" + cmd + "'\r\n";
    }
}