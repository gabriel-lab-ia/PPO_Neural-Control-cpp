# Backend Performance Notes

## Current Performance Posture

- SQLite opened with:
  - `journal_mode=WAL`
  - `synchronous=NORMAL`
  - `foreign_keys=ON`
  - `busy_timeout=5000`
- Read-path indexes are created for:
  - `runs(started_at)`
  - `events(run_id, created_at)`
  - `benchmarks(created_at)`
- Query layer uses prepared statements and typed row mappers.
- Replay endpoints support deterministic downsampling to cap payload size.
- WebSocket replay uses chunked transmission (`replay.chunk`) for large missions.

## Latency-sensitive Paths

1. `GET /runs` and `GET /benchmarks`
2. `GET /runs/{runId}/replay`
3. `WS /ws/runs/{runId}/stream`

## Operational Guidance

- Keep `artifacts/experiments.sqlite` on local SSD for replay-heavy sessions.
- For very large telemetry history, prefer `telemetry/window` + `downsample` over full raw pulls.
- Use frontend replay mode for historical analyses and WebSocket mode for operator-style viewing.

## Known Constraints

- CSV replay ingestion is currently file-based and single-process.
- Job execution orchestration is process-shell based and opt-in.
- No HTTP/2 or async I/O worker pool yet; current server is thread-per-connection.
