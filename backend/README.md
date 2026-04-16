# backend/

Mission telemetry backend with REST + WebSocket endpoints.

Status: optional module (not required for baseline `nmc` build/CI path).

## Endpoints

- `GET /health`
- `GET /api/telemetry/snapshot`
- `WS /ws/telemetry`

## Build

```bash
cmake -S backend -B build/backend
cmake --build build/backend
./build/backend/orbital_backend
```
