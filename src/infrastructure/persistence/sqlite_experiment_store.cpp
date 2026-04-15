#include "infrastructure/persistence/sqlite_experiment_store.h"

#include "infrastructure/persistence/sqlite3_shim.h"

#include <stdexcept>

namespace nmc::infrastructure::persistence {
namespace {

void check_sqlite_result(const int code, sqlite3* db, const char* context) {
    if (code != SQLITE_OK && code != SQLITE_DONE && code != SQLITE_ROW) {
        throw std::runtime_error(std::string(context) + ": " + sqlite3_errmsg(db));
    }
}

class Statement final {
public:
    Statement(sqlite3* db, const char* sql) : db_(db) {
        const int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt_, nullptr);
        check_sqlite_result(rc, db_, "sqlite3_prepare_v2");
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

}  // namespace

SQLiteExperimentStore::SQLiteExperimentStore(std::filesystem::path db_path)
    : db_path_(std::move(db_path)) {
    std::filesystem::create_directories(db_path_.parent_path());
    const int rc = sqlite3_open(db_path_.string().c_str(), &db_);
    if (rc != SQLITE_OK || db_ == nullptr) {
        throw std::runtime_error("unable to open sqlite db: " + db_path_.string());
    }
}

SQLiteExperimentStore::~SQLiteExperimentStore() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void SQLiteExperimentStore::execute(const std::string& sql) const {
    char* error = nullptr;
    const int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        const std::string message = error != nullptr ? error : "sqlite3_exec failed";
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}

void SQLiteExperimentStore::initialize() {
    execute("PRAGMA journal_mode=WAL;");
    execute("PRAGMA foreign_keys=ON;");

    execute(
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
        ");"
    );

    execute(
        "CREATE TABLE IF NOT EXISTS episodes ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  run_id TEXT NOT NULL,"
        "  phase TEXT NOT NULL,"
        "  episode_index INTEGER NOT NULL,"
        "  env_steps INTEGER NOT NULL,"
        "  episode_return REAL NOT NULL,"
        "  episode_length INTEGER NOT NULL,"
        "  success REAL NOT NULL,"
        "  created_at TEXT NOT NULL,"
        "  FOREIGN KEY(run_id) REFERENCES runs(run_id)"
        ");"
    );

    execute("CREATE INDEX IF NOT EXISTS idx_episodes_run ON episodes(run_id);");

    execute(
        "CREATE TABLE IF NOT EXISTS events ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  run_id TEXT NOT NULL,"
        "  level TEXT NOT NULL,"
        "  event_type TEXT NOT NULL,"
        "  message TEXT NOT NULL,"
        "  payload_json TEXT,"
        "  created_at TEXT NOT NULL,"
        "  FOREIGN KEY(run_id) REFERENCES runs(run_id)"
        ");"
    );

    execute("CREATE INDEX IF NOT EXISTS idx_events_run ON events(run_id);");

    execute(
        "CREATE TABLE IF NOT EXISTS benchmarks ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  benchmark_name TEXT NOT NULL,"
        "  run_id TEXT,"
        "  summary_json TEXT NOT NULL,"
        "  created_at TEXT NOT NULL"
        ");"
    );
}

void SQLiteExperimentStore::insert_run_start(const RunStart& run) {
    Statement statement(
        db_,
        "INSERT INTO runs("
        "run_id, mode, environment, seed, started_at, status, artifact_dir, config_json"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?);"
    );

    sqlite3_bind_text(statement.get(), 1, run.run_id.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 2, run.mode.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 3, run.environment.c_str(), -1, sqlite_transient());
    sqlite3_bind_int64(statement.get(), 4, run.seed);
    sqlite3_bind_text(statement.get(), 5, run.started_at.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 6, run.status.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 7, run.artifact_dir.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 8, run.config_json.c_str(), -1, sqlite_transient());

    const int rc = sqlite3_step(statement.get());
    check_sqlite_result(rc, db_, "insert_run_start");
}

void SQLiteExperimentStore::finalize_run(
    const std::string& run_id,
    const std::string& status,
    const std::string& ended_at,
    const std::string& summary_json
) {
    Statement statement(
        db_,
        "UPDATE runs SET ended_at = ?, status = ?, summary_json = ? WHERE run_id = ?;"
    );

    sqlite3_bind_text(statement.get(), 1, ended_at.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 2, status.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 3, summary_json.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 4, run_id.c_str(), -1, sqlite_transient());

    const int rc = sqlite3_step(statement.get());
    check_sqlite_result(rc, db_, "finalize_run");
}

void SQLiteExperimentStore::insert_episode(const EpisodeTelemetry& episode) {
    Statement statement(
        db_,
        "INSERT INTO episodes("
        "run_id, phase, episode_index, env_steps, episode_return, episode_length, success, created_at"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?);"
    );

    sqlite3_bind_text(statement.get(), 1, episode.run_id.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 2, episode.phase.c_str(), -1, sqlite_transient());
    sqlite3_bind_int64(statement.get(), 3, episode.episode_index);
    sqlite3_bind_int64(statement.get(), 4, episode.env_steps);
    sqlite3_bind_double(statement.get(), 5, static_cast<double>(episode.episode_return));
    sqlite3_bind_int64(statement.get(), 6, episode.episode_length);
    sqlite3_bind_double(statement.get(), 7, static_cast<double>(episode.success));
    sqlite3_bind_text(statement.get(), 8, episode.created_at.c_str(), -1, sqlite_transient());

    const int rc = sqlite3_step(statement.get());
    check_sqlite_result(rc, db_, "insert_episode");
}

void SQLiteExperimentStore::insert_event(const EventTelemetry& event) {
    Statement statement(
        db_,
        "INSERT INTO events("
        "run_id, level, event_type, message, payload_json, created_at"
        ") VALUES (?, ?, ?, ?, ?, ?);"
    );

    sqlite3_bind_text(statement.get(), 1, event.run_id.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 2, event.level.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 3, event.event_type.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 4, event.message.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 5, event.payload_json.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 6, event.created_at.c_str(), -1, sqlite_transient());

    const int rc = sqlite3_step(statement.get());
    check_sqlite_result(rc, db_, "insert_event");
}

void SQLiteExperimentStore::insert_benchmark(const BenchmarkTelemetry& benchmark) {
    Statement statement(
        db_,
        "INSERT INTO benchmarks(benchmark_name, run_id, summary_json, created_at) VALUES (?, ?, ?, ?);"
    );

    sqlite3_bind_text(statement.get(), 1, benchmark.benchmark_name.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 2, benchmark.run_id.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 3, benchmark.summary_json.c_str(), -1, sqlite_transient());
    sqlite3_bind_text(statement.get(), 4, benchmark.created_at.c_str(), -1, sqlite_transient());

    const int rc = sqlite3_step(statement.get());
    check_sqlite_result(rc, db_, "insert_benchmark");
}

}  // namespace nmc::infrastructure::persistence
