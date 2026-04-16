# Changelog

All notable changes to this project are documented in this file.

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
