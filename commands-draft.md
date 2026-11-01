# Database Command Protocol

This document defines the proposed client-server command set for the project.

> **Note:** This is a protocol reference for future implementation.
> Not every command is required to be implemented immediately.
> The final implementation depends on the architecture and decisions
> made by the respective project modules.

---

# 1. Data Operations

| Command | Syntax | Description |
|---|---|---|
| `SET` | `SET <key> <value>` | Create a new key or update an existing key |
| `GET` | `GET <key>` | Retrieve the current value of a key |
| `DELETE` | `DELETE <key>` | Delete a key |
| `EXISTS` | `EXISTS <key>` | Check whether a key exists |
| `KEYS` | `KEYS` | List all currently stored keys |
| `SEARCH` | `SEARCH <query>` | Search for matching records or keys |
| `COUNT` | `COUNT` | Return the number of stored records or keys |

---

# 2. Batch Operations

| Command | Syntax | Description |
|---|---|---|
| `MSET` | `MSET <key> <value> ...` | Set multiple key-value pairs |
| `MGET` | `MGET <key> ...` | Retrieve multiple keys |
| `MDELETE` | `MDELETE <key> ...` | Delete multiple keys |

---

# 3. Transactions

| Command | Syntax | Description |
|---|---|---|
| `BEGIN` | `BEGIN` | Start a transaction |
| `BEGIN_TRANSACTION` | `BEGIN_TRANSACTION` | Start a transaction |
| `COMMIT` | `COMMIT` | Commit the current transaction |
| `ROLLBACK` | `ROLLBACK` | Undo the current uncommitted transaction |
| `SAVEPOINT` | `SAVEPOINT <name>` | Create a checkpoint inside a transaction |
| `ROLLBACK_TO` | `ROLLBACK_TO <name>` | Roll back to a specific savepoint |

> `BEGIN` and `BEGIN_TRANSACTION` represent the same conceptual operation.
> The team should choose one as the final protocol keyword rather than
> implementing both unless there is a specific reason to do so.

---

# 4. Version & History

| Command | Syntax | Description |
|---|---|---|
| `HISTORY` | `HISTORY <key>` | Retrieve the version history of a key |
| `GET_VERSION` | `GET_VERSION <key> <version>` | Retrieve a specific historical version |
| `VERSION` | `VERSION <key>` | Retrieve the current version of a key |
| `RESTORE` | `RESTORE <key> <version>` | Restore a key to a previous version |

---

# 5. Recovery & Time Travel

| Command | Syntax | Description |
|---|---|---|
| `RECOVER` | `RECOVER` | Rebuild the in-memory database from persistent storage/WAL |
| `RECOVER_AT` | `RECOVER_AT <timestamp>` | Recover the database to a specified point in time |
| `SNAPSHOT` | `SNAPSHOT <timestamp>` | Retrieve or create a snapshot at a specified point in time |
| `CREATE_SNAPSHOT` | `CREATE_SNAPSHOT` | Create a snapshot of the current database state |
| `RESTORE_SNAPSHOT` | `RESTORE_SNAPSHOT <id>` | Restore the database from a specific snapshot |
| `ROLLBACK_DATABASE` | `ROLLBACK_DATABASE <timestamp/version>` | Restore the entire database to an earlier state |

> `RECOVER_AT` and `SNAPSHOT <timestamp>` are included as proposed
> time-travel operations. Their exact semantics depend on the final
> recovery design.

---

# 6. Persistence & Storage

| Command | Syntax | Description |
|---|---|---|
| `SAVE` | `SAVE` | Force the current state to be persisted |
| `CHECKPOINT` | `CHECKPOINT` | Create a consistent recovery point |
| `COMPACT` | `COMPACT` | Compact persistent storage/WAL data |

---

# 7. WAL & Recovery Administration

| Command | Syntax | Description |
|---|---|---|
| `WAL_STATUS` | `WAL_STATUS` | Retrieve Write-Ahead Log status |
| `RECOVERY_STATUS` | `RECOVERY_STATUS` | Retrieve the current recovery status |

---

# 8. Database Administration

| Command | Syntax | Description |
|---|---|---|
| `STATUS` | `STATUS` | Retrieve server/database status |
| `INFO` | `INFO` | Retrieve general database information |
| `CLEAR` | `CLEAR` | Clear the current database/state |

> `CLEAR` should be treated as a highly privileged administrative operation.

---

# 9. Server & Connection

| Command | Syntax | Description |
|---|---|---|
| `PING` | `PING` | Check whether the server is alive |
| `QUIT` | `QUIT` | Close the client connection |

---

# 10. Security & Access Control

| Command | Syntax | Description |
|---|---|---|
| `LOGIN` | `LOGIN <credentials>` | Authenticate a client |
| `LOGOUT` | `LOGOUT` | End the current client session |
| `LOCKDOWN` | `LOCKDOWN` | Manually place the system into lockdown |
| `UNLOCK` | `UNLOCK` | Exit security lockdown |
| `CREATE_USER` | `CREATE_USER <id> <role>` | Create a new user |
| `DELETE_USER` | `DELETE_USER <id>` | Remove a user |
| `CHANGE_PERMISSION` | `CHANGE_PERMISSION <id> <permission>` | Modify user permissions |

> `AUTHENTICATE` and `AUTHORIZE` are considered internal Security Engine
> operations rather than normal client commands.

---

# 11. Optional Schema Operations

These commands are only relevant if the final database design introduces
schemas, tables, or other database objects.

| Command | Syntax | Description |
|---|---|---|
| `CREATE` | `CREATE <object>` | Create a database object |
| `ALTER` | `ALTER <object>` | Modify the structure of an object |
| `DROP` | `DROP <object>` | Remove a database object |
| `TRUNCATE` | `TRUNCATE <object>` | Remove all records while retaining the object |
| `RENAME` | `RENAME <object> <new_name>` | Rename a database object |

---

# 12. Security Classification

The Security Engine may classify commands according to their potential
impact. Classification is indicative and may change depending on the
client identity, request frequency, access patterns, and other factors.

## LOW

```text
GET
EXISTS
PING
QUIT
STATUS
HISTORY
GET_VERSION
VERSION
INFO
