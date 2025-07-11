# Redis-like In-Memory Database

A lightweight, Redis-like in-memory database implementation supporting basic key-value operations, lists, and hashes with persistence capabilities.

## Features

- **Key-Value Store**: Basic SET/GET operations with expiration support
- **List Operations**: LPUSH, RPUSH, LPOP, RPOP, LLEN, LREM, LINDEX, LSET
- **Hash Operations**: HSET, HGET, HDEL, HEXISTS, HGETALL, HKEYS, HVALS, HLEN, HMSET
- **Persistence**: Periodic database dumps to disk and load on startup
- **Thread-Safe**: All operations are protected by mutex locks
- **RESP Protocol**: Redis Serialization Protocol (RESP) compatible interface
- **Multi-Client Support**: Handles multiple client connections concurrently

## Supported Commands

### General Commands
- `PING` - Returns "PONG"
- `ECHO <message>` - Returns the given message
- `FLUSHALL` - Removes all keys from all databases

### Key-Value Commands
- `SET <key> <value>` - Set key to hold the string value
- `GET <key>` - Get the value of key
- `KEYS` - Get all keys
- `TYPE <key>` - Get the type of value stored at key
- `DEL <key>` - Delete a key
- `EXPIRE <key> <seconds>` - Set a key's time to live in seconds
- `RENAME <oldkey> <newkey>` - Rename a key

### List Commands
- `LLEN <key>` - Get the length of a list
- `LPUSH <key> <value>` - Prepend one or multiple values to a list
- `RPUSH <key> <value>` - Append one or multiple values to a list
- `LPOP <key>` - Remove and get the first element in a list
- `RPOP <key>` - Remove and get the last element in a list
- `LREM <key> <count> <value>` - Remove elements from a list
- `LINDEX <key> <index>` - Get an element from a list by its index
- `LSET <key> <index> <value>` - Set the value of an element in a list by its index

### Hash Commands
- `HSET <key> <field> <value>` - Set the string value of a hash field
- `HGET <key> <field>` - Get the value of a hash field
- `HEXISTS <key> <field>` - Determine if a hash field exists
- `HDEL <key> <field>` - Delete one or more hash fields
- `HGETALL <key>` - Get all fields and values in a hash
- `HKEYS <key>` - Get all fields in a hash
- `HVALS <key>` - Get all values in a hash
- `HLEN <key>` - Get the number of fields in a hash
- `HMSET <key> <field> <value> [field value ...]` - Set multiple hash fields

## Building and Running

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/redis-like-database.git
   cd redis-like-database
   ```

2. Build the project:
   ```bash
   make
   ```

3. Run the server (default port 6379):
   ```bash
   ./redis_server
   ```
   
   Or specify a custom port:
   ```bash
   ./redis_server 6380
   ```

4. Connect using a Redis client:
   ```bash
   redis-cli -p 6379
   ```

## Persistence

The database automatically saves to `dump.my_rdb` every 5 minutes. On server shutdown, a final dump is performed. To load the database on startup, place the dump file in the same directory as the server executable.

## Implementation Details

- **Threading Model**: Uses one thread per client connection
- **Data Structures**:
  - Key-Value: `std::unordered_map`
  - Lists: `std::vector` wrapped in `std::unordered_map`
  - Hashes: `std::unordered_map` wrapped in `std::unordered_map`
- **Persistence Format**: Simple text-based format with markers for different data types

## Limitations

- No replication support
- No cluster mode
- Basic persistence without AOF (Append Only File) support
- No authentication/security features

## Future Improvements

- Implement AOF persistence
- Add authentication
- Support for more data types (sets, sorted sets)
- Replication support
- Cluster mode

## License

MIT License
