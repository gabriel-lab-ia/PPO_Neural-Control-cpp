# Roadmap

## Implemented Baseline

- CPU-first PPO `nmc` CLI in C++20 + LibTorch.
- Layered runtime architecture under `src/`.
- Reproducible run artifacts with manifests, checkpoints, and benchmark reports.
- SQLite persistence for runs, episodes, events, and benchmark summaries.
- Optional MuJoCo adapter path, disabled by default.
- Inference backend abstraction with active LibTorch backend and TensorRT stub.
- CI baseline with configure/build/smoke benchmark/artifact checks.
- Optional integration modules for telemetry/backend/frontend and MLOps workflows.

## Next 30 Days

1. Add `--config <json>` ingestion to `nmc` with deterministic override precedence.
2. Add focused unit tests for artifact schema and SQLite invariants.
3. Add benchmark regression thresholds (return floor + latency ceiling).
4. Improve benchmark report schema with environment/runtime metadata.
5. Wire run-to-MLflow URI linkage into run manifest metadata.

## Next 90 Days

1. Introduce richer orbital dynamics interfaces (toward 6DOF environment adapters).
2. Add standardized RL vs LQR/PID comparison harness with confidence intervals.
3. Add control-safety checks (actuation limits, trajectory divergence flags, residual bounds).
4. Add ARM profiling scripts focused on deterministic inference latency.
5. Add LibTorch vs ONNX parity tests for deployment contracts.

## Long-Term Direction

1. Mission objective packs (station-keeping, rendezvous, collision avoidance).
2. Hierarchical policy structure for tactical vs strategic orbital maneuvers.
3. Deeper control-theoretic validation and stability evidence workflows.
4. Optional TensorRT integration with strict parity and fallback mechanisms.
5. Mission replay and audit tooling integrated with telemetry/event persistence.

## Deferred by Design

- CUDA acceleration in baseline path.
- Active TensorRT runtime dependency.
- Distributed training infrastructure before dynamics fidelity justifies it.
