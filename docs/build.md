# Build and Run Guide

All commands are expected to run from repository root.

## Prerequisites

- Linux (primary target)
- CMake >= 3.24
- Ninja
- C++20 compiler (GCC/Clang)
- SQLite runtime/devel package (`libsqlite3-dev`)
- Node.js 20+ for optional frontend
- vcpkg (manifest mode)

Bootstrap vcpkg once:

```bash
./tools/setup_vcpkg.sh
export VCPKG_ROOT="$HOME/.vcpkg"
```

If `VCPKG_ROOT` is missing/invalid, CMake now fails early with an explicit error message instead of silently falling back.

## Baseline Runtime (`nmc`)

Configure + build:

```bash
cmake --preset dev
cmake --build --preset build
./build/nmc help
```

Smoke benchmark:

```bash
./build/nmc benchmark --quick --name smoke_local --seed 7
```

Train/eval quick validation:

```bash
./build/nmc train --quick --run-id train_quick_001 --seed 7
./build/nmc eval --checkpoint artifacts/latest/checkpoint.pt --episodes 10 --backend libtorch --run-id eval_local_001 --seed 7
```

## Optional Backend

```bash
cmake --preset orbital-stack
cmake --build --preset build-orbital-backend
./build-orbital/backend/orbital_backend
```

Environment variables:

- `ORBITAL_BACKEND_PORT` (default `8080`)
- `ORBITAL_ARTIFACT_ROOT` (default `artifacts`)
- `ORBITAL_SQLITE_PATH` (default `artifacts/experiments.sqlite`)
- `ORBITAL_REPO_ROOT` (default current directory)
- `ORBITAL_JOB_EXECUTOR=1` to allow backend-submitted jobs to execute `nmc`

## Optional Frontend

```bash
cd frontend
npm install
npm run dev
```

Validation:

```bash
npm run typecheck
npm run build
```

Default backend URLs (override with env):

- `VITE_BACKEND_HTTP=http://localhost:8080`
- `VITE_BACKEND_WS=ws://localhost:8080`

## Docker Compose (Optional Stack)

```bash
docker compose up --build -d mlflow backend frontend
docker compose run --rm training
docker compose logs -f mlflow backend frontend
```

## CI-equivalent Baseline Path

```bash
cmake --preset ci
cmake --build --preset build-ci
ctest --test-dir build-ci --output-on-failure --verbose -R nmc_smoke_benchmark
```

Expected smoke artifacts:

- `artifacts/benchmarks/latest.json`
- `artifacts/latest/manifest.json`
- `artifacts/latest/checkpoint.pt`
- `artifacts/experiments.sqlite`

## Optional TensorRT Configure Path

TensorRT is opt-in and requires SDK headers/libs available locally:

```bash
cmake --preset dev-tensorrt -DNMC_TENSORRT_ROOT=/opt/tensorrt
cmake --build --preset build-tensorrt
```

Without TensorRT SDK, keep using baseline `dev` preset; `tensorrt_*` backends remain available in compatibility mode for parity testing.

Quick backend parity + latency comparison table:

```bash
NMC_BIN=./build/nmc ./scripts/compare_inference_backends.sh
```
