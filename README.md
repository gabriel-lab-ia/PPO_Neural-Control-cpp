<p align="center">
  <img src="docs/assets/orbital-hero-banner.svg" alt="Orbital Neural Control CPP hero" width="100%" />
</p>

# Orbital Neural Control CPP

C++20 orbital autonomy/control engineering platform with reproducible PPO workflows (`train`, `eval`, `benchmark`), strict artifact contracts, SQLite telemetry, and optional mission-control API/frontend stack.

## Technology Stack

<p>
  <img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white" />
  <img alt="LibTorch" src="https://img.shields.io/badge/LibTorch-CPU%20First-ee4c2c?logo=pytorch&logoColor=white" />
  <img alt="PPO" src="https://img.shields.io/badge/RL-PPO-1f6feb" />
  <img alt="SQLite" src="https://img.shields.io/badge/SQLite-Telemetry-003B57?logo=sqlite&logoColor=white" />
  <img alt="TensorRT" src="https://img.shields.io/badge/TensorRT-Optional-76B900?logo=nvidia&logoColor=white" />
  <img alt="TypeScript" src="https://img.shields.io/badge/TypeScript-Strict-3178c6?logo=typescript&logoColor=white" />
  <img alt="JavaScript" src="https://img.shields.io/badge/JavaScript-Runtime-f7df1e?logo=javascript&logoColor=black" />
  <img alt="React" src="https://img.shields.io/badge/React-Mission%20UI-61dafb?logo=react&logoColor=black" />
  <img alt="Linux" src="https://img.shields.io/badge/Linux-Primary%20Target-fcc624?logo=linux&logoColor=black" />
</p>

## Scope and Status

### Implemented baseline (CI-gated)

- `nmc` runtime (CPU-first) in layered C++20 architecture under `src/`.
- PPO actor-critic refactor with explicit components:
  - `GaussianPolicy`
  - `ValueNetwork`
  - `RolloutBuffer`
  - `PPOTrainer`
- Point-mass reward shaping upgrade with configurable orbital-friendly terms.
- Deterministic smoke benchmark path.
- Artifact integrity flow under `artifacts/` (`runs/`, `latest/`, `benchmarks/`).
- SQLite persistence (`runs`, `episodes`, `events`, `benchmarks`) with extended forward-compatible telemetry tables.

### Optional stack modules (not required for baseline `nmc`)

- `backend/` C++ REST + WebSocket service
- `frontend/` React + TypeScript mission replay console
- `mlops/` and `training/` orchestration with MLflow
- `core/`, `control/`, `sim/`, `rl/` expansion tracks

### Roadmap (not marketed as shipped)

- Full TensorRT engine build/deploy pipeline (calibration cache + engine serialization)
- CUDA-first training path
- richer control-safety formalism and hardware-in-the-loop integration

## Architecture (Baseline Runtime)

- `src/domain`: PPO, env contracts, inference contracts, config types
- `src/application`: train/eval/benchmark orchestrators
- `src/infrastructure`: artifacts, persistence, reporting
- `src/interfaces`: CLI interface and command parsing
- `src/common`: time/json utility primitives

Optional backend architecture:

- `backend/src/common`
- `backend/src/domain`
- `backend/src/persistence`
- `backend/src/telemetry`
- `backend/src/replay`
- `backend/src/application`
- `backend/src/transport`

## Quickstart (Repository Root)

All commands must run from repository root.

### 1) Bootstrap + configure + build baseline (vcpkg)

```bash
./tools/setup_vcpkg.sh
export VCPKG_ROOT="$HOME/.vcpkg"
cmake --preset dev
cmake --build --preset build
./build/nmc help
```

### 2) Smoke benchmark

```bash
./build/nmc benchmark --quick --name smoke_local --seed 7
```

### 3) Train

```bash
./build/nmc train --quick --run-id train_quick_001 --seed 7
```

### 4) Eval

```bash
./build/nmc eval --checkpoint artifacts/latest/checkpoint.pt --episodes 10 --backend libtorch --run-id eval_local_001 --seed 7
```

### 5) Optional TensorRT-compatible backend validation

The baseline remains CPU-first (`libtorch`). TensorRT backend names are already accepted by CLI and validated in parity mode:

```bash
# fp16-compatible path (parity/emulation mode)
./build/nmc eval --checkpoint artifacts/latest/checkpoint.pt --backend tensorrt_fp16 --episodes 10 --seed 7 --run-id eval_trt_fp16

# int8-compatible path (parity/emulation mode)
./build/nmc eval --checkpoint artifacts/latest/checkpoint.pt --backend tensorrt_int8 --episodes 10 --seed 7 --run-id eval_trt_int8
```

If you pass `.engine`, `.plan`, or `.onnx`, runtime returns an explicit error when native TensorRT support is not compiled.

## TensorRT Integration Status + Measured Data

Current implementation separates two states:

- **Shipped now**: TensorRT-compatible backend API with parity/evaluation path and runtime capability reporting in `evaluation_summary.json`.
- **Not shipped yet**: full native engine lifecycle (ONNX export + engine build + calibration cache + deployment runtime).

Measured comparison on **April 17, 2026** (`point_mass`, same quick-trained checkpoint, `episodes=20`, `seed=7`):

| Backend CLI | Runtime reported | Emulated | Avg episode return | Avg inference latency (ms) | P95 latency (ms) | Summary file |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `libtorch` | `libtorch_cpu` | `false` | `48.0350` | `0.0305768` | `0.0358060` | `artifacts/runs/<eval_run_id>/evaluation_summary.json` |
| `tensorrt_fp16` | `tensorrt_stub_emulation` | `true` | `48.0350` | `0.0397559` | `0.0440470` | `artifacts/runs/<eval_run_id>/evaluation_summary.json` |
| `tensorrt_int8` | `tensorrt_stub_emulation` | `true` | `48.2105` | `0.0474782` | `0.0544430` | `artifacts/runs/<eval_run_id>/evaluation_summary.json` |

Interpretation:

- policy quality remains statistically aligned across backends for this smoke-scale workload.
- latency is currently higher in TensorRT compatibility mode because this path is emulated, not native TensorRT kernels.
- these numbers are useful for parity confidence, not as claims of TensorRT acceleration.

Reproduce the table with one command:

```bash
./scripts/compare_inference_backends.sh
```

## API and Streaming Contracts (Optional Backend)

Contract source:

- `docs/openapi/orbital-api.yaml`

Implemented endpoints:

- `GET /health`
- `GET /version`
- `GET /runs`
- `GET /runs/{runId}`
- `GET /runs/{runId}/summary`
- `GET /runs/{runId}/telemetry`
- `GET /runs/{runId}/telemetry/window`
- `GET /runs/{runId}/events`
- `GET /runs/{runId}/artifacts`
- `GET /runs/{runId}/replay`
- `GET /benchmarks`
- `GET /benchmarks/{benchmarkId}`
- `POST /train/jobs`
- `POST /eval/jobs`
- `POST /benchmark/jobs`
- `GET /jobs/{jobId}`
- `GET /config/presets`

WebSocket:

- `/ws/telemetry/live`
- `/ws/runs/{runId}/stream`

Envelope fields:

- `type`
- `schema_version`
- `timestamp`
- `source`
- `run_id`
- `payload`

## Frontend Mission Console (Optional)

Current frontend stack is **React.js + TypeScript**

Implemented UX highlights:

- replay mode and live mode
- typed API client from OpenAPI contract (`frontend/src/shared/api/generated/orbital-api.ts`)
- 3D Earth mission viewport with OrbitControls, atmosphere glow, orbit lines, and animated satellite
- local high-quality Earth texture support (`frontend/public/textures/earth/*`) with safe procedural fallback
- orbit path + satellite trace + timeline scrubber + event markers
- technical telemetry and benchmark tables for engineering inspection
  
<img width="1817" height="791" alt="Screenshot_2026-04-16_13-23-23" src="https://github.com/user-attachments/assets/ef6b3c2f-7bf1-452c-a50f-ad7b4d8b1d91" />


## Docker / Compose

Bring up optional services:

```bash
docker compose up --build -d mlflow backend frontend
```

Run training service:

```bash
docker compose run --rm training
```

Logs:

```bash
docker compose logs -f mlflow backend frontend
```

Important hygiene:

- `.dockerignore` excludes host build outputs (`build*/`, `CMakeCache.txt`, `CMakeFiles/`, `artifacts/`) to prevent cache contamination.

## Artifact and Persistence Contract

```text
artifacts/
  runs/<run_id>/
    manifest.json
    training_metrics.csv
    training_summary.json
    evaluation_summary.json
    live_rollout.csv
    checkpoints/policy_last.pt
    checkpoints/policy_last.meta
  latest/
    manifest.json
    training_metrics.csv
    checkpoint.pt
    checkpoint.meta
  benchmarks/
    latest.json
    latest.csv
  experiments.sqlite
```

DB docs:

- `docs/database/sqlite-telemetry.md`

Replay docs:

- `docs/replay/mission-replay.md`

## Documentation Index

- `docs/build.md`
- `docs/architecture.md`
- `docs/architecture/backend-api.md`
- `docs/architecture/system-dataflow.md`
- `docs/performance/backend-performance.md`
- `docs/performance/inference-backend-comparison.md`
- `docs/database/sqlite-telemetry.md`
- `docs/openapi/orbital-api.yaml`
- `docs/roadmap.md`

## Honest Constraints

- baseline runtime is CPU-first
- TensorRT path in baseline is API-compatible emulation for parity testing; it is not a production TensorRT engine deployment yet
- backend/frontend remain optional stack modules
- frontend 3D globe is mission-UI oriented and not a full geospatial GIS engine
