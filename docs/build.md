# Build and Run Guide

This document describes the supported local and CI-equivalent build flows for **Orbital Neural Control CPP**.

## Prerequisites

- Linux (primary target)
- CMake >= 3.24
- Ninja
- C++20 compiler (GCC/Clang)
- SQLite runtime/devel package (`libsqlite3-dev`)

LibTorch CPU is bootstrapped with:

```bash
bash tools/setup_libtorch_cpu.sh
```

## Baseline Configure and Build

```bash
cmake --preset dev
cmake --build --preset build
```

Binary:

```bash
./build/nmc
```

## CI-Equivalent Local Flow

```bash
cmake --preset ci
cmake --build --preset build-ci
ctest --test-dir build-ci --output-on-failure --verbose -R nmc_smoke_benchmark
```

Expected smoke artifacts:

- `artifacts/benchmarks/latest.json`
- `artifacts/latest/manifest.json`
- `artifacts/latest/checkpoint.pt`

## Optional Presets

Debug + sanitizers:

```bash
cmake --preset debug-sanitized
cmake --build --preset build-debug
```

Orbital core targets:

```bash
cmake --preset orbital-core-only
cmake --build --preset build-orbital
```

Backend target (requires Boost):

```bash
cmake --preset orbital-stack
cmake --build --preset build-orbital-backend
```

## Runtime Commands

Train:

```bash
./build/nmc train --env point_mass --seed 7 --updates 30 --run-id train_local_001
```

Evaluate:

```bash
./build/nmc eval --checkpoint artifacts/latest/checkpoint.pt --episodes 10 --backend libtorch --run-id eval_local_001
```

Benchmark:

```bash
./build/nmc benchmark --quick --name smoke_local --seed 7
```

## Optional Feature Flags

- `NMC_ENABLE_MUJOCO=ON`: enable MuJoCo environment adapter (requires local MuJoCo install)
- `NMC_BUILD_ORBITAL_BACKEND=ON`: build telemetry backend target
- `NMC_BUILD_ORBITAL_CORE=ON`: build orbital core library/tests/benchmarks

TensorRT remains a stub path and is intentionally disabled in the baseline.
