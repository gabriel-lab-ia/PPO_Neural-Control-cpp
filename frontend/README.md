# Frontend Mission Replay

React + TypeScript + Vite mission-control frontend for orbital replay and telemetry inspection.

This module is a serious UI foundation for the orbital autonomy platform, with typed replay contracts and modular architecture. It currently runs on mock datasets (no live backend requirement).

## Current Scope (MVP)

- Mission Replay screen with 3D orbital viewport
- Telemetry sidebar with engineering fields
- Replay timeline controls (play/pause/reset/speed/scrub)
- Run selection and benchmark summary cards
- Run Explorer screen (`/runs`) for replay dataset inspection

## Architecture

```text
frontend/src/
  app/
    providers/
    layout/
    router/
  pages/
    MissionReplayPage/
    RunExplorerPage/
  widgets/
    MissionTopbar/
    OrbitalCanvas/
    TelemetrySidebar/
    ReplayTimeline/
    BenchmarkCard/
  features/
    run-selection/
    orbital-view/
    telemetry-inspection/
    replay-controls/
    benchmark-summary/
  entities/
    run/
    telemetry/
    orbit/
    replay/
    episode/
  shared/
    ui/
    hooks/
    lib/
    config/
    mock/
```

## Local Run

```bash
cd frontend
npm install
npm run dev
```

Open `http://localhost:3000`.

## Optional Backend Wiring (future integration path)

The current MVP is mock-data-first. When backend telemetry integration is added, use Vite public env names:

```bash
VITE_BACKEND_HTTP=http://localhost:8080
VITE_BACKEND_WS=ws://localhost:8080
```

## Notes

- Design direction: dark, spatial, mission-control UX (Google-Earth-at-night inspiration)
- This frontend intentionally avoids claiming real-time backend telemetry before that integration is implemented
