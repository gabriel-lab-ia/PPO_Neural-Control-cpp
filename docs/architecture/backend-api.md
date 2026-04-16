# Backend API Architecture

The backend module (`backend/`) is now organized as a strict layered service:

- `common/`: logging, time, JSON helpers, HTTP parsing utilities.
- `domain/`: stable DTOs (`RunRecord`, `TelemetrySample`, `BenchmarkRecord`, `JobRecord`).
- `persistence/`: SQLite read/query layer with prepared statements.
- `telemetry/`: rollout CSV ingestion + deterministic orbital projection transforms.
- `replay/`: replay-window assembly (`run + telemetry + events`).
- `application/`: mission queries and job orchestration services.
- `transport/`: HTTP routing, response serialization, WebSocket streaming.

## Runtime Wiring

`main.cpp` only performs composition:

1. resolve environment and paths (`ORBITAL_ARTIFACT_ROOT`, `ORBITAL_SQLITE_PATH`)
2. construct `SQLiteStore`
3. construct `CsvTelemetryStore`
4. construct `MissionService` and `JobService`
5. start `HttpServer`

No route logic or SQL is embedded in `main.cpp`.

## API Contracts

Contract source of truth:

- `docs/openapi/orbital-api.yaml`

Implemented REST surface includes:

- `/health`, `/version`
- `/runs`, `/runs/{runId}`, `/runs/{runId}/summary`
- `/runs/{runId}/telemetry`, `/runs/{runId}/telemetry/window`
- `/runs/{runId}/events`, `/runs/{runId}/artifacts`, `/runs/{runId}/replay`
- `/benchmarks`, `/benchmarks/{benchmarkId}`
- `/train/jobs`, `/eval/jobs`, `/benchmark/jobs`, `/jobs/{jobId}`
- `/config/presets`

WebSocket contracts:

- `/ws/telemetry/live`
- `/ws/runs/{runId}/stream`

All stream messages use envelope fields:

- `type`
- `schema_version`
- `timestamp`
- `source`
- `run_id`
- `payload`

## Determinism Notes

- Replay endpoints read persisted artifacts, not ephemeral simulator state.
- Downsampling is deterministic for identical inputs.
- Job execution is dry-run by default (`ORBITAL_JOB_EXECUTOR=0`) to avoid accidental background mutation in demo environments.
