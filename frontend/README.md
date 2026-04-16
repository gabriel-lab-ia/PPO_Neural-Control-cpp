# Frontend Mission Replay

React + TypeScript + Vite mission-control frontend for orbital replay and telemetry inspection.

This UI is wired to typed API contracts and can run with backend telemetry or local mock fallback.

## What is Implemented

- Premium dark mission-control layout (desktop-first, responsive)
- 3D orbital viewport (React Three Fiber + Three.js + OrbitControls)
- Earth rendering with local high-quality textures when available
- Fallback procedural Earth textures if texture files are not present
- Live/replay modes with timeline scrubber and event markers
- Technical telemetry cards/tables and benchmark summary panel
- Run Explorer page (`/runs`) for run registry inspection

## Stack

- React 18 + TypeScript (strict)
- Vite
- Three.js + `@react-three/fiber` + `@react-three/drei`
- TailwindCSS
- OpenAPI-generated typed API client

## Local Run

```bash
cd frontend
npm install
npm run dev
```

Open `http://localhost:3000`.

## Earth Texture Setup (Recommended)

For realistic Earth material maps, place files in:

```text
frontend/public/textures/earth/
  earth_day_4k.jpg
  earth_night_4k.jpg
  earth_clouds_4k.png
  earth_normal_4k.jpg
  earth_specular_4k.jpg
```

If these files are missing, the frontend automatically falls back to procedural textures (no crash).

## Suggested Texture Sources

- NASA Visible Earth / Blue Marble
- NASA Earth Observatory public imagery
- Solar System Scope texture packs (license-check before commercial use)

Always verify license and attribution terms before shipping.

## Backend Wiring

Set Vite environment variables when using the backend:

```bash
VITE_BACKEND_HTTP=http://localhost:8080
VITE_BACKEND_WS=ws://localhost:8080
```
