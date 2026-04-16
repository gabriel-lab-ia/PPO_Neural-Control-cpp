# Mission Replay Pipeline

This repository supports two replay data paths:

1. **Persisted replay** (`GET /runs/{runId}/replay`)
2. **Stream replay** (`WS /ws/runs/{runId}/stream`)

## Replay Sequence

```mermaid
sequenceDiagram
    participant FE as Frontend
    participant API as Backend Transport
    participant APP as ReplayService
    participant DB as SQLite
    participant CSV as live_rollout.csv

    FE->>API: GET /runs/{runId}/replay?start_step&end_step&downsample
    API->>APP: build_window(run_id,...)
    APP->>DB: fetch run + events
    APP->>CSV: load telemetry samples
    APP->>APP: clip window + downsample
    APP-->>API: ReplayWindow
    API-->>FE: { run, telemetry[], events[] }
```

## Streaming Sequence

```mermaid
sequenceDiagram
    participant FE as Frontend
    participant WS as WebSocket Server
    participant APP as MissionService

    FE->>WS: connect /ws/runs/{runId}/stream
    WS->>APP: build replay window
    loop chunked
      WS-->>FE: replay.chunk envelope
    end
    WS-->>FE: mission.event (replay.completed)
```

## Frontend Modes

- **Replay mode**: uses `GET /runs/{runId}/replay`.
- **Live mode**: uses WebSocket stream and progressively appends telemetry samples.

Both modes are rendered through the same timeline and orbital viewport primitives to keep interaction semantics consistent.
