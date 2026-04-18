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

- Full TensorRT production deployment pipeline (advanced calibration datasets + multi-profile packaging)
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

### 5) Optional TensorRT backend validation

The baseline remains CPU-first (`libtorch`). TensorRT backend names are accepted at runtime and can run in:

- native TensorRT mode (`.onnx` / `.engine` / `.plan`)
- LibTorch fallback mode (automatic fallback when TensorRT runtime/init fails)

```bash
# native build from ONNX -> engine (first run builds, next runs reuse engine cache)
./build/nmc eval --checkpoint artifacts/latest/checkpoint.onnx --backend tensorrt_fp16 --episodes 10 --seed 7 --run-id eval_trt_fp16_native

# explicit engine path
./build/nmc eval --checkpoint artifacts/latest/checkpoint.fp16.engine --backend tensorrt_fp16 --episodes 10 --seed 7 --run-id eval_trt_fp16_engine

# fallback mode (if TensorRT unavailable, falls back to .pt)
./build/nmc eval --checkpoint artifacts/latest/checkpoint.pt --backend tensorrt_fp16 --episodes 10 --seed 7 --run-id eval_trt_fp16_fallback
```

## TensorRT Integration Status + Measured Data

Current implementation separates two states:

- **Shipped now**: native TensorRT runtime path for `.onnx`/`.engine` with builder + serialization + dynamic profile + precision fallback and runtime metadata in `evaluation_summary.json`.
- **Not shipped yet**: full production deployment flow (representative INT8 calibration corpus management and fleet-level engine packaging).

Measured comparison on **April 17, 2026** (`point_mass`, same quick-trained checkpoint, `episodes=20`, `seed=7`):

| Backend CLI | Runtime reported | Emulated | Avg episode return | Avg inference latency (ms) | P95 latency (ms) | Summary file |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `libtorch` | `libtorch_cpu` | `false` | `48.0350` | `0.0290420` | `0.0327260` | `artifacts/runs/<eval_run_id>/evaluation_summary.json` |
| `tensorrt_fp16` | `tensorrt_fallback_libtorch` | `true` | `48.0350` | `0.0290775` | `0.0391200` | `artifacts/runs/<eval_run_id>/evaluation_summary.json` |
| `tensorrt_int8` | `tensorrt_fallback_libtorch` | `true` | `48.0350` | `0.0281963` | `0.0317110` | `artifacts/runs/<eval_run_id>/evaluation_summary.json` |

Interpretation:

- policy quality remains statistically aligned across backends for this smoke-scale workload.
- fallback mode preserves policy behavior and availability; latency delta reflects fallback overhead, not TensorRT kernel speed.
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
- `docs/architecture/tensorrt-backend.md`
- `docs/performance/backend-performance.md`
- `docs/performance/inference-backend-comparison.md`
- `docs/database/sqlite-telemetry.md`
- `docs/openapi/orbital-api.yaml`
- `docs/roadmap.md`

## Honest Constraints

- baseline runtime is CPU-first
- TensorRT native path requires build with `NMC_ENABLE_TENSORRT=ON` and local TensorRT + CUDA runtime libraries
- if TensorRT initialization fails, runtime automatically falls back to LibTorch to preserve pipeline availability
- backend/frontend remain optional stack modules
- frontend 3D globe is mission-UI oriented and not a full geospatial GIS engine
