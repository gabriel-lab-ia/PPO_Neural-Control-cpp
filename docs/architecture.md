# Architecture Overview

Orbital Neural Control CPP has one official runtime baseline and several optional expansion modules.

## 1) Official Runtime Baseline (`src/`)

The baseline executable is `nmc` and is built from `src/` only.

Layering:

- `src/domain/`
  - `ppo/`: policy/value model and PPO trainer implementation.
  - `env/`: environment interface and environment factory.
  - `inference/`: backend abstraction (`libtorch` active, TensorRT stub).
  - `config/`: typed train/eval/benchmark configs and JSON helpers.
- `src/application/`
  - `TrainingRunner`, `EvaluationRunner`, `BenchmarkRunner` orchestration.
- `src/infrastructure/`
  - `artifacts/`: run layout, manifests, checkpoint lifecycle.
  - `persistence/`: SQLite persistence (`runs`, `episodes`, `events`, `benchmarks`).
  - `reporting/`: CSV and rollout reporting.
- `src/interfaces/`
  - CLI parsing + commands (`train`, `eval`, `benchmark`).
- `src/common/`
  - JSON escaping, time, and run-id helpers.

This is the path validated in CI and used as the reproducible CPU-first contract.

## 2) Expansion Modules (Optional)

These modules are intentionally separated from the baseline runtime so they can evolve without destabilizing `nmc`.

- `core/`: orbital dynamics/control kernel and deterministic mission rollout API.
- `control/`: baseline LQR/PID controllers for comparison benchmarks.
- `sim/`: perturbation/disturbance model interfaces.
- `rl/`: runtime-mode and reproducibility primitives.
- `training/`: Python orchestration and pybind11 binding.
- `mlops/`: MLflow tracking, ONNX export, model registration scripts.
- `backend/`: C++ REST/WebSocket telemetry service.
- `frontend/`: Next.js mission dashboard prototype.

## 3) Artifact and Persistence Contract

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
  checkpoints/
  reports/
  experiments.sqlite
```

SQLite tables:

- `runs`
- `episodes`
- `events`
- `benchmarks`

## 4) Execution Flows

Baseline CLI:

- `./build/nmc train ...`
- `./build/nmc eval ...`
- `./build/nmc benchmark --quick ...`

Optional MLOps:

- `python3 mlops/train_with_mlflow.py ...`

Optional telemetry demo:

- `docker compose up --build -d mlflow backend frontend`

## 5) Build-Time Guardrails

- MuJoCo remains optional: `NMC_ENABLE_MUJOCO=ON` only when explicitly enabled.
- TensorRT backend remains a stub and is not a required dependency.
- Baseline CI path is CPU-only and validates artifact generation.
