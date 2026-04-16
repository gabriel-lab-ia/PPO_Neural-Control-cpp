#include "persistence/sqlite_store.h"

#include <sqlite3.h>

#include <stdexcept>

namespace orbital::backend::persistence {
namespace {

void check_sqlite(const int code, sqlite3* db, const char* context) {
    if (code != SQLITE_OK && code != SQLITE_ROW && code != SQLITE_DONE) {
        throw std::runtime_error(std::string(context) + ": " + sqlite3_errmsg(db));
    }
}

class Statement final {
public:
    Statement(sqlite3* db, const char* sql) : db_(db) {
        const int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt_, nullptr);
        check_sqlite(rc, db_, "sqlite3_prepare_v2");
    }

    ~Statement() {
        if (stmt_ != nullptr) {
            sqlite3_finalize(stmt_);
        }
    }

    sqlite3_stmt* get() {
        return stmt_;
    }

private:
    sqlite3* db_ = nullptr;
    sqlite3_stmt* stmt_ = nullptr;
};

std::string text_column(sqlite3_stmt* stmt, const int index) {
    const auto* raw = sqlite3_column_text(stmt, index);
    return raw != nullptr ? reinterpret_cast<const char*>(raw) : "";
}

}  // namespace

SQLiteStore::SQLiteStore(std::filesystem::path db_path)
    : db_path_(std::move(db_path)) {
    const int rc = sqlite3_open(db_path_.string().c_str(), &db_);
    if (rc != SQLITE_OK || db_ == nullptr) {
        throw std::runtime_error("unable to open sqlite db: " + db_path_.string());
    }

    check_sqlite(sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr), db_, "pragma_wal");
    check_sqlite(sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr), db_, "pragma_sync");
    check_sqlite(sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr), db_, "pragma_fk");
    check_sqlite(sqlite3_exec(db_, "PRAGMA busy_timeout=5000;", nullptr, nullptr, nullptr), db_, "pragma_busy_timeout");

    check_sqlite(
        sqlite3_exec(
            db_,
            "CREATE TABLE IF NOT EXISTS runs ("
            "  run_id TEXT PRIMARY KEY,"
            "  mode TEXT NOT NULL,"
            "  environment TEXT NOT NULL,"
            "  seed INTEGER NOT NULL,"
            "  started_at TEXT NOT NULL,"
            "  ended_at TEXT,"
            "  status TEXT NOT NULL,"
            "  artifact_dir TEXT NOT NULL,"
            "  config_json TEXT NOT NULL,"
            "  summary_json TEXT"
            ");",
            nullptr,
            nullptr,
            nullptr
        ),
        db_,
        "create_runs"
    );
    check_sqlite(
        sqlite3_exec(
            db_,
            "CREATE TABLE IF NOT EXISTS events ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  run_id TEXT NOT NULL,"
            "  level TEXT NOT NULL,"
            "  event_type TEXT NOT NULL,"
            "  message TEXT NOT NULL,"
            "  payload_json TEXT,"
            "  created_at TEXT NOT NULL"
            ");",
            nullptr,
            nullptr,
            nullptr
        ),
        db_,
        "create_events"
    );
    check_sqlite(
        sqlite3_exec(
            db_,
            "CREATE TABLE IF NOT EXISTS benchmarks ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  benchmark_name TEXT NOT NULL,"
            "  run_id TEXT,"
            "  summary_json TEXT NOT NULL,"
            "  created_at TEXT NOT NULL"
            ");",
            nullptr,
            nullptr,
            nullptr
        ),
        db_,
        "create_benchmarks"
    );
    check_sqlite(
        sqlite3_exec(
            db_,
            "CREATE TABLE IF NOT EXISTS schema_migrations ("
            "  version INTEGER PRIMARY KEY,"
            "  applied_at TEXT NOT NULL"
            ");",
            nullptr,
            nullptr,
            nullptr
        ),
        db_,
        "create_schema_migrations"
    );

    check_sqlite(
        sqlite3_exec(
            db_,
            "INSERT OR IGNORE INTO schema_migrations(version, applied_at) VALUES (1, datetime('now'));",
            nullptr,
            nullptr,
            nullptr
        ),
        db_,
        "seed_schema_migrations"
    );

    check_sqlite(
        sqlite3_exec(
            db_,
            "CREATE INDEX IF NOT EXISTS idx_runs_started_at ON runs(started_at DESC);",
            nullptr,
            nullptr,
            nullptr
        ),
        db_,
        "idx_runs_started_at"
    );
    check_sqlite(
        sqlite3_exec(
            db_,
            "CREATE INDEX IF NOT EXISTS idx_events_run_created_at ON events(run_id, created_at);",
            nullptr,
            nullptr,
            nullptr
        ),
        db_,
        "idx_events_run_created_at"
    );
    check_sqlite(
        sqlite3_exec(
            db_,
            "CREATE INDEX IF NOT EXISTS idx_benchmarks_created_at ON benchmarks(created_at DESC);",
            nullptr,
            nullptr,
            nullptr
        ),
        db_,
        "idx_benchmarks_created_at"
    );
}

SQLiteStore::~SQLiteStore() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

std::vector<domain::RunRecord> SQLiteStore::list_runs(const std::int64_t limit, const std::int64_t offset) const {
    Statement stmt(
        db_,
        "SELECT run_id, mode, environment, seed, started_at, COALESCE(ended_at,''), status, artifact_dir, "
        "config_json, COALESCE(summary_json,'') "
        "FROM runs ORDER BY started_at DESC LIMIT ? OFFSET ?;"
    );

    sqlite3_bind_int64(stmt.get(), 1, limit);
    sqlite3_bind_int64(stmt.get(), 2, offset);

    std::vector<domain::RunRecord> runs;
    runs.reserve(static_cast<std::size_t>(limit));
    while (true) {
        const int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_DONE) {
            break;
        }
        check_sqlite(rc, db_, "runs_step");

        domain::RunRecord run;
        run.run_id = text_column(stmt.get(), 0);
        run.mode = text_column(stmt.get(), 1);
        run.environment = text_column(stmt.get(), 2);
        run.seed = sqlite3_column_int64(stmt.get(), 3);
        run.started_at = text_column(stmt.get(), 4);
        run.ended_at = text_column(stmt.get(), 5);
        run.status = text_column(stmt.get(), 6);
        run.artifact_dir = text_column(stmt.get(), 7);
        run.config_json = text_column(stmt.get(), 8);
        run.summary_json = text_column(stmt.get(), 9);
        runs.emplace_back(std::move(run));
    }

    return runs;
}

std::optional<domain::RunRecord> SQLiteStore::get_run(const std::string& run_id) const {
    Statement stmt(
        db_,
        "SELECT run_id, mode, environment, seed, started_at, COALESCE(ended_at,''), status, artifact_dir, "
        "config_json, COALESCE(summary_json,'') FROM runs WHERE run_id = ?;"
    );
    sqlite3_bind_text(stmt.get(), 1, run_id.c_str(), -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_DONE) {
        return std::nullopt;
    }
    check_sqlite(rc, db_, "run_step");

    domain::RunRecord run;
    run.run_id = text_column(stmt.get(), 0);
    run.mode = text_column(stmt.get(), 1);
    run.environment = text_column(stmt.get(), 2);
    run.seed = sqlite3_column_int64(stmt.get(), 3);
    run.started_at = text_column(stmt.get(), 4);
    run.ended_at = text_column(stmt.get(), 5);
    run.status = text_column(stmt.get(), 6);
    run.artifact_dir = text_column(stmt.get(), 7);
    run.config_json = text_column(stmt.get(), 8);
    run.summary_json = text_column(stmt.get(), 9);
    return run;
}

std::vector<domain::EventRecord> SQLiteStore::list_events(
    const std::string& run_id,
    const std::int64_t limit,
    const std::int64_t offset
) const {
    Statement stmt(
        db_,
        "SELECT id, run_id, level, event_type, message, COALESCE(payload_json,''), created_at "
        "FROM events WHERE run_id = ? ORDER BY id ASC LIMIT ? OFFSET ?;"
    );
    sqlite3_bind_text(stmt.get(), 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt.get(), 2, limit);
    sqlite3_bind_int64(stmt.get(), 3, offset);

    std::vector<domain::EventRecord> events;
    events.reserve(static_cast<std::size_t>(limit));
    while (true) {
        const int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_DONE) {
            break;
        }
        check_sqlite(rc, db_, "event_step");

        domain::EventRecord event;
        event.id = sqlite3_column_int64(stmt.get(), 0);
        event.run_id = text_column(stmt.get(), 1);
        event.level = text_column(stmt.get(), 2);
        event.event_type = text_column(stmt.get(), 3);
        event.message = text_column(stmt.get(), 4);
        event.payload_json = text_column(stmt.get(), 5);
        event.created_at = text_column(stmt.get(), 6);
        events.emplace_back(std::move(event));
    }

    return events;
}

std::vector<domain::BenchmarkRecord> SQLiteStore::list_benchmarks(
    const std::int64_t limit,
    const std::int64_t offset
) const {
    Statement stmt(
        db_,
        "SELECT id, benchmark_name, COALESCE(run_id,''), summary_json, created_at "
        "FROM benchmarks ORDER BY id DESC LIMIT ? OFFSET ?;"
    );
    sqlite3_bind_int64(stmt.get(), 1, limit);
    sqlite3_bind_int64(stmt.get(), 2, offset);

    std::vector<domain::BenchmarkRecord> benchmarks;
    benchmarks.reserve(static_cast<std::size_t>(limit));
    while (true) {
        const int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_DONE) {
            break;
        }
        check_sqlite(rc, db_, "benchmark_step");

        domain::BenchmarkRecord record;
        record.id = sqlite3_column_int64(stmt.get(), 0);
        record.benchmark_name = text_column(stmt.get(), 1);
        record.run_id = text_column(stmt.get(), 2);
        record.summary_json = text_column(stmt.get(), 3);
        record.created_at = text_column(stmt.get(), 4);
        benchmarks.emplace_back(std::move(record));
    }

    return benchmarks;
}

std::optional<domain::BenchmarkRecord> SQLiteStore::get_benchmark(const std::string& benchmark_id_or_name) const {
    Statement stmt(
        db_,
        "SELECT id, benchmark_name, COALESCE(run_id,''), summary_json, created_at "
        "FROM benchmarks WHERE CAST(id AS TEXT) = ? OR benchmark_name = ? LIMIT 1;"
    );
    sqlite3_bind_text(stmt.get(), 1, benchmark_id_or_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, benchmark_id_or_name.c_str(), -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_DONE) {
        return std::nullopt;
    }
    check_sqlite(rc, db_, "benchmark_get_step");

    domain::BenchmarkRecord record;
    record.id = sqlite3_column_int64(stmt.get(), 0);
    record.benchmark_name = text_column(stmt.get(), 1);
    record.run_id = text_column(stmt.get(), 2);
    record.summary_json = text_column(stmt.get(), 3);
    record.created_at = text_column(stmt.get(), 4);
    return record;
}

}  // namespace orbital::backend::persistence
