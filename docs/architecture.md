# Architecture Overview

This repository is now a CPU-first C++20 reinforcement learning systems baseline centered on PPO, reproducible runs, and local telemetry.

## Layered structure

- `src/domain/`
  - RL core behavior and domain contracts.
  - `config/`: explicit train/eval/benchmark config structs.
  - `env/`: environment interface, factory, and concrete environments.
  - `ppo/`: policy-value model, rollout/metrics types, PPO trainer.
  - `inference/`: backend abstraction for policy inference (`libtorch` active, TensorRT stub placeholder).
- `src/application/`
  - Use-case orchestration for `train`, `eval`, and `benchmark`.
  - Coordinates domain components and infrastructure concerns.
- `src/infrastructure/`
  - Persistence and I/O boundaries.
  - `artifacts/`: run/checkpoint/layout management and manifests.
  - `persistence/`: SQLite experiment store (`runs`, `episodes`, `events`, `benchmarks`).
  - `reporting/`: CSV/live-rollout exporters.
- `src/interfaces/`
  - CLI entrypoint and command parsing.
- `src/common/`
  - Shared utilities (timestamp/run-id generation, JSON escaping).

## Runtime modes

### Train mode

- Command: `./build/nmc train [options]`
- Creates run layout under `artifacts/runs/<run_id>/`
- Executes PPO updates through `domain::ppo::PPOTrainer`
- Persists telemetry to SQLite
- Saves checkpoint + metadata
- Writes training summary + manifest

### Eval mode

- Command: `./build/nmc eval [options]`
- Loads checkpoint metadata and inference backend
- Runs deterministic/stochastic evaluation episodes
- Persists evaluation episode telemetry and summary
- Writes evaluation manifest

### Benchmark mode

- Command: `./build/nmc benchmark --quick`
- Runs short training + evaluation pipeline
- Validates required artifacts are readable
- Writes benchmark JSON/CSV summary in `artifacts/benchmarks/`

## Artifact model

```
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
    <run_id>_policy_last.pt
    <run_id>_policy_last.meta
  reports/
    <run_id>_training_summary.json
    <run_id>_evaluation_summary.json
  benchmarks/
    <benchmark_id>.json
    <benchmark_id>.csv
    latest.json
    latest.csv
  latest/
    manifest.json
    checkpoint.pt
    checkpoint.meta
    training_summary.json
    evaluation_summary.json
    training_metrics.csv
    live_rollout.csv
  experiments.sqlite
```

## SQLite schema

Implemented in `src/infrastructure/persistence/sqlite_experiment_store.cpp`.

- `runs`: run metadata, status transitions, config JSON, summary JSON.
- `episodes`: per-episode telemetry from train/eval.
- `events`: run-level events (resume, lifecycle, etc.).
- `benchmarks`: benchmark summaries and linkage to run IDs.

## Inference extensibility

`domain::inference::PolicyInferenceBackend` decouples evaluation inference from trainer internals.

- Active now: `LibTorchPolicyBackend` (CPU, enabled by default).
- Deferred intentionally: `TensorRtPolicyBackendStub`.

This allows future TensorRT inference integration without changing CLI/app contracts.

## Determinism and reproducibility

- Seed control in config (`seed`) and `torch::manual_seed` usage.
- Stable artifact naming with generated run IDs and UTC timestamps.
- Every run writes `manifest.json` + structured summaries.
- Smoke benchmark path is automated in CI (`ctest -R nmc_smoke_benchmark`).

## Current limitations

- Training still uses in-process env vectorization (single process).
- TensorRT backend is a stub only.
- Distributed training and large-scale hyperparameter sweeps are deferred.
- MuJoCo support remains optional compile-time integration.
