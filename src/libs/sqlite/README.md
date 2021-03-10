# SQLite

Minimum version is the same as the sqlite version in the source tree.

We compile SQLite with the flowing settings:
* SQLITE_THREADSAFE=2
* SQLITE_ENABLE_FTS5
* SQLITE_ENABLE_UNLOCK_NOTIFY
* SQLITE_ENABLE_COLUMN_METADATA
* SQLITE_ENABLE_JSON1

Be prepared that we demand more functionality from SQLite in the future.
