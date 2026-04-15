# PPO Neural Control C++

A disciplined C++20 reinforcement learning systems baseline for PPO.

This repository is now organized as a CPU-first, reproducible RL project with clear boundaries between domain logic, application orchestration, infrastructure, and interfaces. It keeps optional MuJoCo support and prepares a future inference backend path (TensorRT) without enabling it yet.

## What it is now

- PPO training baseline in modern C++20 + LibTorch (CPU).
- Explicit `train`, `eval`, and `benchmark` operational flows.
- Reproducible run artifacts with per-run manifests.
- Local SQLite experiment tracking and telemetry.
- CI smoke benchmark that compiles and validates generated artifacts.

## What it is not (yet)

- No active CUDA path.
- No active TensorRT runtime integration.
- No full orbital dynamics domain migration yet.

## Architecture

Source layout:

- `src/domain/`
  - `config/`: explicit train/eval/benchmark configuration models.
  - `env/`: `Environment` interface + concrete envs (`point_mass`, optional `mujoco_cartpole`).
  - `ppo/`: policy/value model, rollout types, PPO trainer.
  - `inference/`: inference backend interface + LibTorch backend + TensorRT stub.
- `src/application/`
  - training/evaluation/benchmark runners.
- `src/infrastructure/`
  - artifact layout + checkpoint management.
  - SQLite persistence (`runs`, `episodes`, `events`, `benchmarks`).
  - CSV/live rollout reporting.
- `src/interfaces/`
  - CLI and entrypoint.
- `src/common/`
  - shared utilities.

Detailed docs and UML:

- `docs/architecture.md`
- `docs/uml/component-diagram.md`
- `docs/uml/class-diagram.md`
- `docs/uml/sequence-training.md`
- `docs/roadmap.md`

## Requirements

- CMake 3.24+
- C++20 compiler (GCC 13+ recommended)
- LibTorch CPU (default helper script provided)
- SQLite runtime library (`libsqlite3`)
- Optional MuJoCo for `mujoco_cartpole`

## Setup

Install LibTorch CPU locally (if `lib/libtorch` is missing):

```bash
bash tools/setup_libtorch_cpu.sh
```

Configure and build:

```bash
cmake --preset dev
cmake --build --preset build
```

## CLI commands

### Train

```bash
./build/nmc train --env point_mass --seed 7 --updates 30
```

Example with explicit hyperparameters:

```bash
./build/nmc train \
  --env point_mass \
  --seed 7 \
  --num-envs 16 \
  --updates 30 \
  --rollout-steps 128 \
  --ppo-epochs 5 \
  --minibatch-size 192 \
  --hidden-dim 96 \
  --learning-rate 0.0003
```

### Evaluate

```bash
./build/nmc eval --checkpoint artifacts/latest/checkpoint.pt --episodes 10 --backend libtorch
```

### Smoke benchmark

```bash
./build/nmc benchmark --quick --name smoke
```

Script aliases:

```bash
./scripts/train.sh ...
./scripts/eval.sh ...
./scripts/benchmark_smoke.sh
```

## Artifact layout

Generated under `artifacts/`:

```text
artifacts/
  runs/<run_id>/
    manifest.json
    training_metrics.csv
    training_summary.json
    evaluation_summary.json
    live_rollout.csv
    checkpoints/
      policy_last.pt
      policy_last.meta
  checkpoints/
  reports/
  benchmarks/
  latest/
  experiments.sqlite
```

Each run writes structured metadata (`manifest.json`) and summary outputs.

## SQLite tracking

Database: `artifacts/experiments.sqlite`

Tables:

- `runs`: mode, status, config JSON, summary JSON.
- `episodes`: episode-level telemetry for train/eval.
- `events`: run lifecycle and important events.
- `benchmarks`: benchmark summary records.

## MuJoCo (optional)

Enable and build with MuJoCo:

```bash
cmake --preset dev -DNMC_ENABLE_MUJOCO=ON -DNMC_MUJOCO_ROOT=$HOME/.local/mujoco-3.2.6
cmake --build --preset build
```

Run with MuJoCo environment:

```bash
./build/nmc train --env mujoco_cartpole
```

## Future TensorRT direction

The inference abstraction (`domain/inference/PolicyInferenceBackend`) is already in place.

- Active now: `LibTorchPolicyBackend`.
- Placeholder: `TensorRtPolicyBackendStub`.

This keeps today’s setup simple and CPU-first while preserving a clean extension point for future TensorRT inference.

## CI

GitHub Actions pipeline (`.github/workflows/ci.yml`) performs:

1. Configure and build.
2. Run smoke benchmark (`ctest -R nmc_smoke_benchmark`).
3. Validate critical artifact outputs exist.

## Orbital/satellite direction

The long-term target remains autonomous orbital/satellite control.

This refactor intentionally focuses on reusable RL systems foundations so new environments (e.g., orbital dynamics) can be plugged into `domain/env` without rewriting the PPO core.
