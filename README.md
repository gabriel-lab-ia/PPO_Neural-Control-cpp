<p align="center">
  <img src="docs/assets/orbital-hero-banner.svg" alt="Orbital Neural Control CPP hero" width="100%" />
</p>

### 2. Run smoke benchmark (CI-equivalent baseline)

```bash
./build/nmc benchmark --quick --name smoke_local --seed 7
```

### 3. Run train and eval

```bash
./build/nmc train --env point_mass --seed 7 --updates 30 --run-id train_cpu_001
./build/nmc eval --checkpoint artifacts/latest/checkpoint.pt --episodes 10 --backend libtorch --run-id eval_cpu_001
```

### Example output (smoke benchmark)

```json
{
  "run_id": "smoke_local",
  "env": "point_mass",
  "updates": 30,
  "backend": "libtorch_cpu",
  "deterministic": true,
  "status": "ok"
}
```

Generated at:

`artifacts/benchmarks/latest.json`

### Baseline environment

The default environment is a lightweight continuous-control surrogate: `point_mass`.

Properties:

- continuous state space
- continuous action space
- deterministic CPU execution
- fast iteration cycle
- used to validate PPO training loop integrity
- minimal dynamics suitable for CI smoke validation

This environment is a scaffold before higher-fidelity orbital simulation layers.

### Validated operational baseline

| component | current implementation | purpose |
|---|---|---|
| primary language | C++20 | autonomy core and runtime |
| RL backend | LibTorch CPU | baseline training and inference |
| reference environment | `point_mass` | fast continuous-control validation |
| benchmark layer | smoke benchmark + CTest | pipeline integrity |
| persistence | SQLite | runs, episodes, events, benchmarks |
| experiment tracking | MLflow | metrics, artifacts, experiment lineage |
| real-time backend | C++ REST + WebSocket | operational telemetry |
| frontend | Next.js 15 + R3F + Recharts | live 3D mission visualization |

### Architecture expansion roadmap

| track | status | note |
|---|---|---|
| classical controllers (`LQR` / `PID`) | present | baseline comparison against RL |
| ONNX export | present | portable deployment contract |
| MuJoCo | optional | outside the default baseline |
| TensorRT | future | planned inference backend |
| orbital fidelity upgrade | roadmap | evolution from 3DOF to 6DOF |
| safety envelopes / disturbances | roadmap | more realistic mission conditions |

## Frontend Mission Replay (MVP)

The repository includes a React + TypeScript + Vite mission replay UI under `frontend/`.

Current scope:

- typed mock-data orbital replay (no live backend required)
- 3D orbital viewport + telemetry sidebar + replay controls
- run explorer page for replay dataset inspection

Run locally:

```bash
cd frontend
npm install
npm run dev
```

Future integration path:

- `VITE_BACKEND_HTTP` / `VITE_BACKEND_WS` for backend telemetry/runtime artifact wiring

## Architecture

### Baseline Runtime (Implemented Today)

The executable baseline lives in `src/` and is intentionally layered. This is the path validated by CI.

- `src/domain/`: PPO, env contracts/adapters, inference backend abstraction, config objects
- `src/application/`: orchestration (`TrainingRunner`, `EvaluationRunner`, `BenchmarkRunner`)
- `src/infrastructure/`: artifacts, checkpoints, SQLite persistence, reporting
- `src/interfaces/`: CLI entrypoint and argument parsing
- `src/common/`: JSON/time/run-id helpers

### Expansion Modules (Optional, Not Required for Baseline Build)

Expansion modules represent research surfaces and integration paths.
They are intentionally decoupled from the baseline runtime.
Some modules provide scaffolding for future experiments:

- `core/`: orbital dynamics/control primitives
- `control/`: baseline LQR/PID controllers
- `sim/`: perturbation model interfaces
- `rl/`: runtime mode + reproducibility primitives
- `training/`: Python orchestration + pybind11 bridge
- `mlops/`: MLflow tracking + ONNX export + registry scripts
- `backend/`: C++ REST/WebSocket telemetry service
- `frontend/`: React + Vite mission replay UI foundation

### Future Tracks (Roadmap)

- richer orbital dynamics and control safety validation
- tighter RL vs classical-control benchmark suites
- deployment/runtime hardening for embedded targets

See [Architecture Docs](docs/architecture.md) and UML under `docs/uml/`.

## Mathematical Foundations (PPO + Continuous Control)

This repository keeps the control-learning math explicit, compact, and engineering-readable for PPO-based optimization in continuous-control and orbital-autonomy settings.

### Policy ratio and clipped improvement

- `r_t(θ) = π_θ(a_t | s_t) / π_θ_old(a_t | s_t)`
- `L_clip(θ) = E_t [ min( r_t(θ) Â_t , clip(r_t(θ), 1 - ε, 1 + ε) Â_t ) ]`

Interpretation:

- `r_t(θ)` measures how strongly the updated policy reweights sampled actions;
- clipping prevents destructive policy jumps;
- `Â_t` determines whether the sampled action should be reinforced or suppressed.

### Value and entropy terms

- `L_value = (V_θ(s_t) - V_target)^2`
- `L_total = L_clip(θ) - c_v L_value + c_e H(π_θ(· | s_t))`

Interpretation:

- the critic provides a learned baseline that reduces gradient variance;
- the value term stabilizes training;
- the entropy term preserves exploratory behavior and delays premature collapse.

### Generalized Advantage Estimation

- `δ_t = r_t + γ V(s_{t+1}) - V(s_t)`
- `Â_t = δ_t + γ λ Â_{t+1}`
- `Â_t = Σ_l (γ λ)^l δ_{t+l}`

Interpretation:

- `δ_t` is the one-step temporal-difference residual;
- GAE smooths the policy-improvement signal across time;
- `λ` controls the bias-variance trade-off in advantage estimation.

### Continuous Gaussian control

- `a_t ~ N( μ_θ(s_t), σ_θ(s_t)^2 )`

Implementation view:

- mean head: `μ_θ(s_t)`
- scale head: `σ_θ(s_t)` or `log σ_θ(s_t)`

Interpretation:

- the actor emits a continuous control distribution rather than discrete logits;
- bounded variance improves numerical stability;
- Gaussian policies support smooth actuation and trajectory correction.

### Orbital-control objective shaping

A representative control-oriented reward may be structured as:

- `r_t = -w_pos ||e_pos||^2 - w_vel ||e_vel||^2 - w_ctrl ||u_t||^2 - w_risk C_t`

Where:

- `e_pos` is orbital position error;
- `e_vel` is velocity error;
- `u_t` is control effort / thrust command;
- `C_t` is a mission-risk or constraint-violation penalty.

Interpretation:

- the learned policy optimizes long-horizon orbital behavior rather than one-step greedy corrections;
- clipped PPO updates improve control stability;
- GAE helps identify whether recent control actions improved mission state over time.

### Why this math matters here

This mathematical structure is what turns the project into an engineering control system rather than a generic RL demo:

- PPO supplies stable policy improvement;
- actor-critic decomposition supports scalable training;
- GAE improves signal quality for long-horizon control;
- Gaussian policies make continuous actuation practical;
- reward shaping links learning directly to orbital mission objectives.

## Reproducible Artifacts

Each run writes structured outputs under `artifacts/`:

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

## Persistence and Experiment Tracking

### SQLite (baseline runtime)

`src/infrastructure/persistence/sqlite_experiment_store.*` tracks:

- `runs`
- `episodes`
- `events`
- `benchmarks`

### MLflow + ONNX (optional MLOps track)

```bash
python3 -m pip install -r mlops/requirements.txt
./mlops/start_mlflow.sh
python3 mlops/train_with_mlflow.py --tracking-uri http://localhost:5000 --experiment orbital_ppo --run-id mlflow_orbital_001 --seed 7 --updates 30 --num-envs 16 --env point_mass --export-onnx
```

## Running the full stack with Docker Compose
From the repository root directory, run:

```bash
cd Orbital-Neural-Control-CPP
docker compose up --build -d mlflow backend frontend
docker compose run --rm training
docker compose logs -f backend frontend
# Orbital Neural Control CPP

**C++20 autonomy engineering baseline for PPO-based continuous control, reproducible experiments, telemetry persistence, and benchmarkable evaluation.**

`nmc` is the primary CLI binary (`train`, `eval`, `benchmark`). The project is CPU-first by default.

- Active backend: LibTorch (CPU)
- Optional integration: MuJoCo (`NMC_ENABLE_MUJOCO=ON`)
- Future path (stub only): TensorRT inference backend

Execution scope:

- baseline (implemented + CI-gated): `src/` runtime and `nmc`
- expansion modules (implemented, optional): `core`, `control`, `sim`, `rl`, `training`, `mlops`, `backend`, `frontend`
- roadmap (not shipped as baseline): higher-fidelity orbital dynamics and advanced autonomy stacks

## Stack and Engineering Identity

<p align="center">
  <span style="display:inline-flex; margin:8px 10px;"><img alt="Linux First" src="https://img.shields.io/badge/Linux-First-2563EB?style=for-the-badge&amp;labelColor=333333&amp;color=2563EB" /></span>
  <span style="display:inline-flex; margin:8px 10px;"><img alt="C++ 20" src="https://img.shields.io/badge/C%2B%2B-20-2563EB?style=for-the-badge&amp;labelColor=333333&amp;color=2563EB" /></span>
  <span style="display:inline-flex; margin:8px 10px;"><img alt="Reinforcement Learning PPO" src="https://img.shields.io/badge/Reinforcement%20Learning-PPO-2563EB?style=for-the-badge&amp;labelColor=333333&amp;color=2563EB" /></span>
</p>

<p align="center">
  <span style="display:inline-flex; margin:8px 10px;"><img alt="Backend LibTorch CPU" src="https://img.shields.io/badge/Backend-LibTorch%20CPU-1F9D68?style=for-the-badge&amp;labelColor=333333&amp;color=1F9D68" /></span>
  <span style="display:inline-flex; margin:8px 10px;"><img alt="Tracking SQLite" src="https://img.shields.io/badge/Tracking-SQLite-0EA5B7?style=for-the-badge&amp;labelColor=333333&amp;color=0EA5B7" /></span>
</p>

<p align="center">
  <span style="display:inline-flex; margin:8px 10px;"><img alt="Simulation Orbital Mission Direction" src="https://img.shields.io/badge/Simulation-Orbital%20Mission%20Direction-2563EB?style=for-the-badge&amp;labelColor=333333&amp;color=2563EB" /></span>
</p>

<p align="center">
  <span style="display:inline-flex; margin:8px 10px;"><img alt="Telemetry Run Episode Event" src="https://img.shields.io/badge/Telemetry-Run%2FEpisode%2FEvent-2563EB?style=for-the-badge&amp;labelColor=333333&amp;color=2563EB" /></span>
  <span style="display:inline-flex; margin:8px 10px;"><img alt="Benchmarking Reproducible" src="https://img.shields.io/badge/Benchmarking-Reproducible-1F9D68?style=for-the-badge&amp;labelColor=333333&amp;color=1F9D68" /></span>
</p>

<p align="center">
  <span style="display:inline-flex; margin:8px 10px;"><img alt="UML Architecture Discipline" src="https://img.shields.io/badge/UML-Architecture%20Discipline-6D28D9?style=for-the-badge&amp;labelColor=333333&amp;color=6D28D9" /></span>
  <span style="display:inline-flex; margin:8px 10px;"><img alt="Systems Engineering" src="https://img.shields.io/badge/Systems-Engineering-2563EB?style=for-the-badge&amp;labelColor=333333&amp;color=2563EB" /></span>
</p>

<p align="center">
  <span style="display:inline-flex; margin:8px 10px;"><img alt="CI Smoke Benchmark" src="https://img.shields.io/badge/CI-Smoke%20Benchmark-1F9D68?style=for-the-badge&amp;labelColor=333333&amp;color=1F9D68" /></span>
  <span style="display:inline-flex; margin:8px 10px;"><img alt="Baseline CPU Only" src="https://img.shields.io/badge/Baseline-CPU%20Only-9CA3AF?style=for-the-badge&amp;labelColor=333333&amp;color=9CA3AF" /></span>
</p>

<p align="center">
  <span style="display:inline-flex; margin:8px 10px;"><img alt="Optional MuJoCo" src="https://img.shields.io/badge/Optional-MuJoCo-C97316?style=for-the-badge&amp;labelColor=333333&amp;color=C97316" /></span>
  <span style="display:inline-flex; margin:8px 10px;"><img alt="Future TensorRT Path" src="https://img.shields.io/badge/Future-TensorRT%20Path-D4A017?style=for-the-badge&amp;labelColor=333333&amp;color=D4A017" /></span>
</p>

## Core Capabilities

<p align="center">
  <img src="docs/assets/mission-capabilities-strip.svg" alt="Core capabilities chart" width="100%" />
</p>


## Frontend Mission Control Interface

The repository includes a **React + TypeScript mission-control visualization layer** designed to inspect reinforcement learning runs, telemetry signals, and orbital trajectories in a structured engineering interface.

The frontend is not a marketing UI or demo landing page.  
It is an architectural component intended to evolve into a **visual inspection and replay interface for autonomy experiments, simulation outputs, and control-system telemetry**.

Design direction:
- dark mission-control interface
- technical telemetry panels
- replayable trajectory inspection
- structured engineering UX
- layered software architecture aligned with backend runtime artifacts

The UI is intentionally aligned with the system architecture of the C++ runtime and experiment artifacts.

## Frontend Stack
Core technologies:

- React.js
- TypeScript
- Vite
- Three.js
- React Three Fiber
- modular CSS architecture
- typed domain contracts
- layered frontend architecture

Supporting libraries:

- Zustand (lightweight state management)
- drei utilities for Three.js
- strict TypeScript typing for telemetry contracts

The frontend is CPU-light and focuses on visualization, not training.

<img width="1852" height="807" alt="Screenshot_2026-04-15_22-38-08" src="https://github.com/user-attachments/assets/ad7f9f4f-db6f-4dbf-91c9-11c772d2de7b" />

## CI Baseline

GitHub Actions validates a meaningful CPU-first path:

1. validate required baseline source set
2. configure with CI preset (`cmake --preset ci`)
3. build `nmc`
4. execute `nmc_smoke_benchmark` via CTest
5. verify required artifacts (`benchmark/latest`, `latest/manifest`, `latest/checkpoint`)
6. cross-compile orbital core tests for ARM64 portability

## Full Stack Demo (Optional)

```bash
docker compose up --build -d mlflow backend frontend
docker compose run --rm training
docker compose logs -f backend frontend
```

Endpoints:

- dashboard: `http://localhost:3000`
- backend health: `http://localhost:8080/health`
- MLflow: `http://localhost:5000`

## Documentation

- [docs/architecture.md](docs/architecture.md)
- [docs/build.md](docs/build.md)
- [docs/roadmap.md](docs/roadmap.md)
- [docs/uml/component-diagram.md](docs/uml/component-diagram.md)
- [docs/uml/class-diagram.md](docs/uml/class-diagram.md)
- [docs/uml/sequence-training.md](docs/uml/sequence-training.md)
- [CONTRIBUTING.md](CONTRIBUTING.md)
- [CHANGELOG.md](CHANGELOG.md)

## Current Status vs Roadmap

Implemented now:

- production baseline CLI (`nmc`) with train/eval/benchmark
- reproducible artifact model and checkpoint flow
- SQLite persistence for runs/episodes/events/benchmarks
- CI smoke validation and ARM64 core cross-compile path
- optional orbital expansion modules (`core/control/sim/rl`, backend/frontend, mlops)

Deferred intentionally:

- CUDA acceleration
- active TensorRT runtime dependency
- full 6DOF mission dynamics in baseline CLI
