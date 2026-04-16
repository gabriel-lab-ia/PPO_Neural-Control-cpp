# Architecture Overview

Orbital Neural Control CPP is intentionally split into:

1. **baseline runtime** (`src/`) with strict CI guarantees
2. **optional platform modules** (`backend`, `frontend`, `mlops`, `core/control/sim/rl`)

## 1) Baseline Runtime (`src/`)

The official executable baseline is `nmc`.

Layer boundaries:

- `domain/`
  - PPO stack, env interfaces, inference backend abstraction, typed config
- `application/`
  - orchestration runners (`TrainingRunner`, `EvaluationRunner`, `BenchmarkRunner`)
- `infrastructure/`
  - artifacts, checkpoint lifecycle, SQLite persistence, reporting
- `interfaces/`
  - CLI contract (`train`, `eval`, `benchmark`)
- `common/`
  - time and JSON utility primitives

Design intent: deterministic control workflows with explicit artifact and persistence outputs.

## 2) Optional Platform Stack

### Backend (`backend/`)

Layered backend modules:

- `common/`
- `domain/`
- `persistence/`
- `telemetry/`
- `replay/`
- `application/`
- `transport/`

The backend reads persisted artifacts and SQLite data; it does not own the PPO simulation loop.

### Frontend (`frontend/`)

- React + TypeScript + Vite
- typed client generated from OpenAPI contract
- mission replay and live stream UX over REST + WebSocket

## 3) Data and Contracts

- OpenAPI contract: `docs/openapi/orbital-api.yaml`
- Replay flow: `docs/replay/mission-replay.md`
- System dataflow: `docs/architecture/system-dataflow.md`
- SQLite schema and indexes: `docs/database/sqlite-telemetry.md`

## 4) Why This Separation Exists

- keeps baseline runtime credible and reproducible
- allows optional modules to evolve without destabilizing `nmc`
- keeps backend/frontend/API contract-driven instead of ad hoc
- supports deterministic benchmark evidence and replayability
