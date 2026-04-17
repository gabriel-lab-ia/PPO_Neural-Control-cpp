#pragma once

#include <cstddef>
#include <cstdint>

extern "C" {

using sqlite3_int64 = long long;

struct sqlite3;
struct sqlite3_stmt;

using sqlite3_destructor_type = void (*)(void*);

int sqlite3_open(const char* filename, sqlite3** ppDb);
int sqlite3_close(sqlite3* db);
int sqlite3_exec(
    sqlite3* db,
    const char* sql,
    int (*callback)(void*, int, char**, char**),
    void* context,
    char** err_msg
);
int sqlite3_prepare_v2(
    sqlite3* db,
    const char* sql,
    int n_byte,
    sqlite3_stmt** pp_stmt,
    const char** pz_tail
);
int sqlite3_step(sqlite3_stmt* stmt);
int sqlite3_finalize(sqlite3_stmt* stmt);
int sqlite3_bind_text(
    sqlite3_stmt* stmt,
    int index,
    const char* value,
    int n,
    sqlite3_destructor_type destructor
);
int sqlite3_bind_int64(sqlite3_stmt* stmt, int index, sqlite3_int64 value);
int sqlite3_bind_double(sqlite3_stmt* stmt, int index, double value);
sqlite3_int64 sqlite3_column_int64(sqlite3_stmt* stmt, int i_col);
int sqlite3_changes(sqlite3* db);
const char* sqlite3_errmsg(sqlite3* db);
void sqlite3_free(void*);

}  // extern "C"

constexpr int SQLITE_OK = 0;
constexpr int SQLITE_ROW = 100;
constexpr int SQLITE_DONE = 101;

inline sqlite3_destructor_type sqlite_transient() {
    return reinterpret_cast<sqlite3_destructor_type>(static_cast<std::intptr_t>(-1));
}
