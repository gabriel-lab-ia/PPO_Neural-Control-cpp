#pragma once

#include <filesystem>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "domain/types.h"

struct sqlite3;

namespace orbital::backend::persistence {

class SQLiteStore {
public:
    explicit SQLiteStore(std::filesystem::path db_path);
    ~SQLiteStore();

    SQLiteStore(const SQLiteStore&) = delete;
    SQLiteStore& operator=(const SQLiteStore&) = delete;

    std::vector<domain::RunRecord> list_runs(std::int64_t limit, std::int64_t offset) const;
    std::optional<domain::RunRecord> get_run(const std::string& run_id) const;

    std::vector<domain::EventRecord> list_events(
        const std::string& run_id,
        std::int64_t limit,
        std::int64_t offset
    ) const;

    std::vector<domain::BenchmarkRecord> list_benchmarks(std::int64_t limit, std::int64_t offset) const;
    std::optional<domain::BenchmarkRecord> get_benchmark(const std::string& benchmark_id_or_name) const;

private:
    std::filesystem::path db_path_;
    sqlite3* db_ = nullptr;
};

}  // namespace orbital::backend::persistence
