# SQLite Telemetry Store

Primary database file:

- `artifacts/experiments.sqlite`

## Baseline Tables

- `runs`
- `episodes`
- `events`
- `benchmarks`

## Extended Tables (forward-compatible)

- `telemetry_samples`
- `run_artifacts`
- `run_configs`
- `model_registry_refs`
- `schema_migrations`

Schema is migrated incrementally:

- `v1`: `runs`, `episodes`, `events`, `benchmarks`
- `v2`: `telemetry_samples`, `run_artifacts`, `run_configs`, `model_registry_refs`

`schema_migrations(version, applied_at)` is written transactionally by the runtime, so startup fails fast if migrations do not complete.

## Index Strategy

- `idx_runs_started_at`
- `idx_episodes_run`
- `idx_episodes_run_phase_episode`
- `idx_events_run`
- `idx_events_run_created`
- `idx_benchmarks_name_created`
- `idx_telemetry_run_step`
- `idx_telemetry_run_time`
- `idx_run_artifacts_run_type`
- `idx_model_registry_refs_run_backend`

## Replay Query Patterns

- run list by recency: `ORDER BY started_at DESC`
- event stream by run: `WHERE run_id = ? ORDER BY id ASC`
- benchmark inspection by recency: `ORDER BY id DESC`

## Practical Notes

- WAL mode allows concurrent readers with active writer jobs.
- Foreign keys are enabled and expected to remain enabled in all runtime entrypoints.
- CSV remains authoritative for high-frequency rollout traces; DB keeps mission-level indexing and summaries.
