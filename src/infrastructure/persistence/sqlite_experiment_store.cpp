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

void check_bind_result(const int code, sqlite3* db, const char* context) {
    if (code != SQLITE_OK) {
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
        throw std::runtime_error(message + " | sql=" + sql);
    }
}

void SQLiteExperimentStore::initialize_pragmas() {
    execute("PRAGMA journal_mode=WAL;");
    execute("PRAGMA foreign_keys=ON;");
    execute("PRAGMA busy_timeout=5000;");
}

void SQLiteExperimentStore::ensure_migration_table() {
    execute(
        "CREATE TABLE IF NOT EXISTS schema_migrations ("
        "  version INTEGER PRIMARY KEY,"
        "  applied_at TEXT NOT NULL"
        ");"
    );
}

int64_t SQLiteExperimentStore::current_schema_version() const {
    Statement statement(db_, "SELECT COALESCE(MAX(version), 0) FROM schema_migrations;");
    const int rc = sqlite3_step(statement.get());
    check_sqlite_result(rc, db_, "select_schema_version");
    return sqlite3_column_int64(statement.get(), 0);
}

void SQLiteExperimentStore::record_migration(const int64_t version) {
    Statement statement(
        db_,
        "INSERT INTO schema_migrations(version, applied_at) VALUES(?, datetime('now'));"
    );
    check_bind_result(sqlite3_bind_int64(statement.get(), 1, version), db_, "bind migration version");
    const int rc = sqlite3_step(statement.get());
    check_sqlite_result(rc, db_, "record_schema_migration");
}

void SQLiteExperimentStore::apply_schema_v1() {
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
    execute("CREATE INDEX IF NOT EXISTS idx_episodes_run_phase_episode ON episodes(run_id, phase, episode_index);");

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
    execute("CREATE INDEX IF NOT EXISTS idx_events_run_created ON events(run_id, created_at);");

    execute(
        "CREATE TABLE IF NOT EXISTS benchmarks ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  benchmark_name TEXT NOT NULL,"
        "  run_id TEXT,"
        "  summary_json TEXT NOT NULL,"
        "  created_at TEXT NOT NULL"
        ");"
    );
    execute("CREATE INDEX IF NOT EXISTS idx_benchmarks_name_created ON benchmarks(benchmark_name, created_at);");
    execute("CREATE INDEX IF NOT EXISTS idx_runs_started_at ON runs(started_at);");
}

void SQLiteExperimentStore::apply_schema_v2() {
    execute(
        "CREATE TABLE IF NOT EXISTS telemetry_samples ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  run_id TEXT NOT NULL,"
        "  episode_index INTEGER NOT NULL DEFAULT 0,"
        "  step INTEGER NOT NULL,"
        "  mission_time_s REAL NOT NULL,"
        "  reward REAL NOT NULL,"
        "  control REAL,"
        "  value_estimate REAL,"
        "  sample_json TEXT NOT NULL,"
        "  created_at TEXT NOT NULL,"
        "  FOREIGN KEY(run_id) REFERENCES runs(run_id)"
        ");"
    );
    execute("CREATE INDEX IF NOT EXISTS idx_telemetry_run_step ON telemetry_samples(run_id, step);");
    execute("CREATE INDEX IF NOT EXISTS idx_telemetry_run_time ON telemetry_samples(run_id, mission_time_s);");

    execute(
        "CREATE TABLE IF NOT EXISTS run_artifacts ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  run_id TEXT NOT NULL,"
        "  artifact_type TEXT NOT NULL,"
        "  path TEXT NOT NULL,"
        "  checksum TEXT,"
        "  bytes INTEGER,"
        "  created_at TEXT NOT NULL,"
        "  FOREIGN KEY(run_id) REFERENCES runs(run_id)"
        ");"
    );
    execute("CREATE INDEX IF NOT EXISTS idx_run_artifacts_run_type ON run_artifacts(run_id, artifact_type);");

    execute(
        "CREATE TABLE IF NOT EXISTS run_configs ("
        "  run_id TEXT PRIMARY KEY,"
        "  config_json TEXT NOT NULL,"
        "  created_at TEXT NOT NULL,"
        "  FOREIGN KEY(run_id) REFERENCES runs(run_id)"
        ");"
    );

    execute(
        "CREATE TABLE IF NOT EXISTS model_registry_refs ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  run_id TEXT NOT NULL,"
        "  backend TEXT NOT NULL,"
        "  uri TEXT NOT NULL,"
        "  tag TEXT,"
        "  created_at TEXT NOT NULL,"
        "  FOREIGN KEY(run_id) REFERENCES runs(run_id)"
        ");"
    );
    execute("CREATE INDEX IF NOT EXISTS idx_model_registry_refs_run_backend ON model_registry_refs(run_id, backend);");
}

void SQLiteExperimentStore::apply_migrations() {
    constexpr int64_t kTargetSchemaVersion = 2;
    const int64_t version = current_schema_version();

    execute("BEGIN IMMEDIATE TRANSACTION;");
    try {
        if (version < 1) {
            apply_schema_v1();
            record_migration(1);
        }

        if (version < 2) {
            apply_schema_v2();
            record_migration(2);
        }

        execute("COMMIT;");
    } catch (...) {
        execute("ROLLBACK;");
        throw;
    }

    const int64_t applied_version = current_schema_version();
    if (applied_version < kTargetSchemaVersion) {
        throw std::runtime_error(
            "sqlite schema migration failed: expected version " + std::to_string(kTargetSchemaVersion) +
            ", got " + std::to_string(applied_version)
        );
    }
}

void SQLiteExperimentStore::initialize() {
    initialize_pragmas();
    ensure_migration_table();
    apply_migrations();
}

void SQLiteExperimentStore::insert_run_start(const RunStart& run) {
    Statement statement(
        db_,
        "INSERT INTO runs("
        "run_id, mode, environment, seed, started_at, status, artifact_dir, config_json"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?);"
    );

    check_bind_result(sqlite3_bind_text(statement.get(), 1, run.run_id.c_str(), -1, sqlite_transient()), db_, "bind run_id");
    check_bind_result(sqlite3_bind_text(statement.get(), 2, run.mode.c_str(), -1, sqlite_transient()), db_, "bind mode");
    check_bind_result(
        sqlite3_bind_text(statement.get(), 3, run.environment.c_str(), -1, sqlite_transient()),
        db_,
        "bind environment"
    );
    check_bind_result(sqlite3_bind_int64(statement.get(), 4, run.seed), db_, "bind seed");
    check_bind_result(
        sqlite3_bind_text(statement.get(), 5, run.started_at.c_str(), -1, sqlite_transient()),
        db_,
        "bind started_at"
    );
    check_bind_result(sqlite3_bind_text(statement.get(), 6, run.status.c_str(), -1, sqlite_transient()), db_, "bind status");
    check_bind_result(
        sqlite3_bind_text(statement.get(), 7, run.artifact_dir.c_str(), -1, sqlite_transient()),
        db_,
        "bind artifact_dir"
    );
    check_bind_result(
        sqlite3_bind_text(statement.get(), 8, run.config_json.c_str(), -1, sqlite_transient()),
        db_,
        "bind config_json"
    );

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

    check_bind_result(
        sqlite3_bind_text(statement.get(), 1, ended_at.c_str(), -1, sqlite_transient()),
        db_,
        "bind ended_at"
    );
    check_bind_result(sqlite3_bind_text(statement.get(), 2, status.c_str(), -1, sqlite_transient()), db_, "bind status");
    check_bind_result(
        sqlite3_bind_text(statement.get(), 3, summary_json.c_str(), -1, sqlite_transient()),
        db_,
        "bind summary_json"
    );
    check_bind_result(sqlite3_bind_text(statement.get(), 4, run_id.c_str(), -1, sqlite_transient()), db_, "bind run_id");

    const int rc = sqlite3_step(statement.get());
    check_sqlite_result(rc, db_, "finalize_run");
    if (sqlite3_changes(db_) == 0) {
        throw std::runtime_error("finalize_run: run_id not found: " + run_id);
    }
}

void SQLiteExperimentStore::insert_episode(const EpisodeTelemetry& episode) {
    Statement statement(
        db_,
        "INSERT INTO episodes("
        "run_id, phase, episode_index, env_steps, episode_return, episode_length, success, created_at"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?);"
    );

    check_bind_result(
        sqlite3_bind_text(statement.get(), 1, episode.run_id.c_str(), -1, sqlite_transient()),
        db_,
        "bind run_id"
    );
    check_bind_result(sqlite3_bind_text(statement.get(), 2, episode.phase.c_str(), -1, sqlite_transient()), db_, "bind phase");
    check_bind_result(sqlite3_bind_int64(statement.get(), 3, episode.episode_index), db_, "bind episode_index");
    check_bind_result(sqlite3_bind_int64(statement.get(), 4, episode.env_steps), db_, "bind env_steps");
    check_bind_result(
        sqlite3_bind_double(statement.get(), 5, static_cast<double>(episode.episode_return)),
        db_,
        "bind episode_return"
    );
    check_bind_result(sqlite3_bind_int64(statement.get(), 6, episode.episode_length), db_, "bind episode_length");
    check_bind_result(sqlite3_bind_double(statement.get(), 7, static_cast<double>(episode.success)), db_, "bind success");
    check_bind_result(
        sqlite3_bind_text(statement.get(), 8, episode.created_at.c_str(), -1, sqlite_transient()),
        db_,
        "bind created_at"
    );

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

    check_bind_result(sqlite3_bind_text(statement.get(), 1, event.run_id.c_str(), -1, sqlite_transient()), db_, "bind run_id");
    check_bind_result(sqlite3_bind_text(statement.get(), 2, event.level.c_str(), -1, sqlite_transient()), db_, "bind level");
    check_bind_result(
        sqlite3_bind_text(statement.get(), 3, event.event_type.c_str(), -1, sqlite_transient()),
        db_,
        "bind event_type"
    );
    check_bind_result(sqlite3_bind_text(statement.get(), 4, event.message.c_str(), -1, sqlite_transient()), db_, "bind message");
    check_bind_result(
        sqlite3_bind_text(statement.get(), 5, event.payload_json.c_str(), -1, sqlite_transient()),
        db_,
        "bind payload_json"
    );
    check_bind_result(
        sqlite3_bind_text(statement.get(), 6, event.created_at.c_str(), -1, sqlite_transient()),
        db_,
        "bind created_at"
    );

    const int rc = sqlite3_step(statement.get());
    check_sqlite_result(rc, db_, "insert_event");
}

void SQLiteExperimentStore::insert_benchmark(const BenchmarkTelemetry& benchmark) {
    Statement statement(
        db_,
        "INSERT INTO benchmarks(benchmark_name, run_id, summary_json, created_at) VALUES (?, ?, ?, ?);"
    );

    check_bind_result(
        sqlite3_bind_text(statement.get(), 1, benchmark.benchmark_name.c_str(), -1, sqlite_transient()),
        db_,
        "bind benchmark_name"
    );
    check_bind_result(sqlite3_bind_text(statement.get(), 2, benchmark.run_id.c_str(), -1, sqlite_transient()), db_, "bind run_id");
    check_bind_result(
        sqlite3_bind_text(statement.get(), 3, benchmark.summary_json.c_str(), -1, sqlite_transient()),
        db_,
        "bind summary_json"
    );
    check_bind_result(
        sqlite3_bind_text(statement.get(), 4, benchmark.created_at.c_str(), -1, sqlite_transient()),
        db_,
        "bind created_at"
    );

    const int rc = sqlite3_step(statement.get());
    check_sqlite_result(rc, db_, "insert_benchmark");
}

}  // namespace nmc::infrastructure::persistence
