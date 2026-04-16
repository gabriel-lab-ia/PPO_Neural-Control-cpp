# Roadmap

## Implemented Baseline

- CPU-first PPO runtime (`nmc`) in C++20 + LibTorch.
- Layered runtime boundaries under `src/`.
- Reproducible `train`, `eval`, `benchmark` artifacts and manifests.
- SQLite persistence with run/episode/event/benchmark contracts and extended forward-compatible tables.
- Deterministic smoke benchmark path in CI.
- Optional backend API with OpenAPI contract and WebSocket replay envelopes.
- Optional frontend mission console with typed API client and 3D orbital replay UX.

## Near-term (30 days)

1. Add first-class config file ingestion (`--config`) with deterministic override precedence.
2. Add benchmark regression thresholds (reward floor + runtime budget).
3. Add backend API integration tests and OpenAPI contract validation in CI.
4. Add frontend replay virtualization for very large mission histories.
5. Add stronger replay snapshot caching strategy for sub-second reloads.

## Mid-term (90 days)

1. Introduce richer orbital dynamics interfaces (toward 6DOF adapters).
2. Add standardized PPO vs LQR/PID comparison harness with confidence intervals.
3. Expand safety telemetry (corridor violations, control saturation, divergence alarms).
4. Add ARM-focused deterministic inference profiling.
5. Add LibTorch vs ONNX inference parity checks.

## Long-term Direction

1. Mission objective packs (station-keeping, rendezvous, collision-avoidance).
2. Hierarchical control policies for tactical vs strategic maneuvers.
3. Stronger control-theoretic validation and auditability workflows.
4. Optional TensorRT path with strict parity gates and fallback.
5. Hardware-in-the-loop replay and mission certification tracks.

## Deferred by Design

- CUDA acceleration in baseline path
- active TensorRT dependency in default CI path
- distributed training infrastructure before dynamics fidelity maturity
