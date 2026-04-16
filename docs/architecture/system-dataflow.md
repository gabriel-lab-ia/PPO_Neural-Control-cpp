# System Dataflow

```mermaid
flowchart LR
    CLI[nmc CLI train/eval/benchmark] --> ART[Artifact Writer]
    CLI --> DB[(SQLite experiments.sqlite)]
    CLI --> ROLLOUT[live_rollout.csv]

    DB --> API[Backend API Service]
    ROLLOUT --> API
    ART --> API

    API --> REST[REST JSON Contracts]
    API --> WS[WebSocket Stream Contracts]

    REST --> FE[Frontend Mission Console]
    WS --> FE
```

## Why this Design

- Baseline runtime (`nmc`) keeps deterministic training/eval/benchmark as the source of truth.
- Backend reads persisted artifacts instead of owning simulator state.
- Frontend consumes typed contracts generated from OpenAPI to avoid ad hoc payload drift.
- Replay and live visualizations share telemetry schema for consistent operational tooling.
