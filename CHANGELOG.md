# Changelog

All notable changes to this project are documented in this file.

## 0.4.1 - TensorRT Readiness + Runtime Hardening

- Added explicit `TensorRtPolicyBackend` controller in the inference layer, keeping CLI compatibility while exposing runtime/emulation capability metadata.
- Expanded evaluation summaries with backend runtime metadata and inference latency statistics (`avg_inference_latency_ms`, `p95_inference_latency_ms`) for reproducible backend comparisons.
- Added optional TensorRT build hooks (`NMC_ENABLE_TENSORRT`) with clear configuration diagnostics and dedicated CMake presets (`dev-tensorrt`, `build-tensorrt`).
- Hardened configure-time bootstrap diagnostics when `VCPKG_ROOT` is missing or `CMAKE_TOOLCHAIN_FILE` is invalid.
- Recalibrated CI smoke benchmark quality gates to realistic deterministic envelope thresholds for current baseline dynamics.
- Replaced backend POST body regex parsing with structured JSON parsing using Boost.JSON.
- Introduced SQLite schema versioning/migrations scaffold for forward-safe persistence evolution.
- Applied frontend code-splitting for the 3D orbital scene to reduce initial bundle pressure.

## 0.4.0 - Deterministic PPO Runtime + 6DOF Foundations

- Added deterministic runtime bootstrap (`src/common/determinism.*`) and enforced seed propagation through train/eval orchestration.
- Introduced an explicit 6DOF orbital dynamics contract (`OrbitalDynamics`) with a high-fidelity baseline model (`RK4 + J2 + drag + SRP + Sun/Moon third-body`).
- Added `OrbitalSixDofEnv` with safety-aware action projection and Lyapunov-shaped reward terms.
- Expanded inference backend contract with capability introspection and precision modes (`fp32/fp16/int8`) plus parity coverage test (`nmc_inference_parity`).
- Added CMake presets for TSAN, coverage, and release PGO/LTO profiles; release presets now default to `LTO + PGO(auto)`.
- Added `vcpkg.json` manifest and aligned Docker/CI/docs to vcpkg-first dependency flow.
- Updated smoke/benchmark/test path to keep deterministic behavior under one process.

## 0.3.1 - Build + Documentation Harmonization

- Refactored top-level `CMakeLists.txt` into clearer engineering blocks (toolchain defaults, dependency roots, compile profiles, baseline target, optional modules).
- Kept `nmc` as the explicit runtime contract and aligned CMake status messaging with Orbital Neural Control CPP naming.
- Improved CI clarity with source/docs contract checks and explicit CLI contract validation (`nmc help`) before smoke benchmark.
- Rewrote README math section for cleaner GitHub rendering and better PPO/GAE notation readability.
- Tightened README/CONTRIBUTING/docs wording to explicitly separate baseline runtime, optional modules, and roadmap scope.

## 0.3.0 - Architecture Consolidation

- Promoted a single official runtime path around `nmc` (`train`, `eval`, `benchmark`).
- Removed legacy unused source tree (`src/main.cpp`, `src/app`, `src/env`, `src/model`, `src/train`, `src/utils`) that no longer participated in the build.
- Standardized top-level CMake project metadata to **OrbitalNeuralControlCPP**.
- Added explicit CI configure/build/test preset alignment (`ci`, `build-ci`, `test-ci`).
- Hardened CI source validation to enforce active baseline entrypoints and reject legacy file reintroduction.
- Reworked README and contributor documentation to match the actual architecture, commands, and artifact model.
- Added dedicated build/run documentation (`docs/build.md`).

## 0.2.0 - PPO Systems Baseline

- Established layered `src/` architecture (`domain`, `application`, `infrastructure`, `interfaces`, `common`).
- Added reproducible train/eval/benchmark workflow with structured artifacts.
- Added SQLite persistence for runs, episodes, events, and benchmark summaries.
- Added inference backend abstraction with CPU LibTorch backend and TensorRT stub.
- Added benchmark smoke path and CI artifact validation.

## 0.1.0 - Initial Prototype

- Initial C++ PPO prototype with point-mass environment.
- Optional MuJoCo cart-pole adapter.
- Early visualization/export scripts and static demo assets.
