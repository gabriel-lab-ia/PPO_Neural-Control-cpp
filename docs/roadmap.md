# Roadmap

## Current baseline (implemented)

- CPU-first PPO stack in C++20 with LibTorch.
- Clean train/eval/benchmark CLI.
- Layered architecture (`domain`, `application`, `infrastructure`, `interfaces`).
- Structured artifacts with run manifests.
- SQLite telemetry persistence.
- CI smoke benchmark + artifact validation.
- Inference backend abstraction with TensorRT placeholder.

## Near-term improvements (next)

1. Add automated regression thresholds for smoke benchmark (return floor, latency ceiling).
2. Add unit tests for artifact persistence and SQLite schema operations.
3. Add richer evaluation reports (percentiles, variance, confidence intervals).
4. Add checkpoint resume tests and compatibility checks.
5. Add optional config-file ingestion (`--config path.json`) while keeping CLI overrides.

## Mid-term RL systems upgrades

1. Environment randomization and curriculum scheduling interfaces.
2. Batched evaluation workers for robust benchmark statistics.
3. Better fault tolerance for long-running experiments.
4. Experiment sweep tooling with deterministic run matrix generation.
5. Pluggable reward decomposition telemetry.

## TensorRT integration path (future, optional)

1. Implement concrete `TensorRtPolicyBackend` under `domain/inference/`.
2. Add ONNX/export pipeline from `PolicyValueModel` checkpoint.
3. Add backend-level parity tests against LibTorch inference.
4. Add runtime backend selection policy with fallback to LibTorch.
5. Keep TensorRT path optional and disabled by default.

## Orbital/satellite domain evolution path

1. Add `OrbitalDynamicsEnv` implementing `domain::env::Environment`.
2. Add mission-level success metrics (fuel efficiency, pointing, rendezvous constraints).
3. Introduce safety envelopes and constraint-aware reward shaping.
4. Add sim-to-policy validation harness specific to orbital autonomy tasks.
5. Keep PPO core reusable while domain complexity grows in environment modules.
