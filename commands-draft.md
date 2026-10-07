# Network Protocol Commands

## Data Operations
- `SET <key> <value>` — Create or update a key-value pair
- `GET <key>` — Retrieve the current value of a key
- `DELETE <key>` — Delete a key

## Query Operations
- `EXISTS <key>` — Check whether a key exists
- `KEYS` — List all stored keys

## Version & History Operations
- `HISTORY <key>` — Retrieve the version history of a key
- `GET_VERSION <key> <version>` — Retrieve a specific version of a key

## Server / Connection Operations
- `PING` — Check whether the server is alive
- `QUIT` — Close the client connection

## Persistence / Recovery Operations
- `SAVE` — Force the current state to be persisted
- `RECOVER` — Trigger database recovery from persistent storage
- `STATUS` — Get server/database status
