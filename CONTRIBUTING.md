# Contributing

Thanks for contributing to **Orbital Neural Control CPP**.

This repository is a C++20 RL systems baseline with an orbital-autonomy direction. Contributions should improve reliability, reproducibility, and maintainability.

## Engineering Principles

- Keep the CPU-first baseline (`nmc`) working.
- Prefer explicit interfaces and low coupling.
- Keep optional integrations optional (MuJoCo, backend/frontend, MLflow).
- Do not add hidden runtime dependencies.
- Keep documentation aligned with real behavior.

## Development Setup

```bash
bash tools/setup_libtorch_cpu.sh
cmake --preset dev
cmake --build --preset build
```

Primary executable:

```bash
./build/nmc help
```

## Validation Before PR

Run at least these checks locally:

```bash
./build/nmc benchmark --quick --name pre_pr_smoke --seed 7
ctest --test-dir build --output-on-failure --verbose -R nmc_smoke_benchmark
```

If you touch orbital core modules:

```bash
cmake --preset orbital-core-only
cmake --build --preset build-orbital
ctest --test-dir build-orbital-core --output-on-failure
```

## Code Conventions

- C++20, RAII, const-correctness, and explicit ownership.
- Keep `src/` layering intact:
  - `domain` for RL/environment logic
  - `application` for orchestration
  - `infrastructure` for persistence/artifacts/reporting
  - `interfaces` for CLI
- Keep names descriptive and avoid throwaway identifiers.
- Add comments only when clarifying intent or design constraints.

## Pull Request Guidelines

- Keep PR scope focused.
- Include a short problem statement and validation steps.
- Update docs when behavior/commands/paths change.
- If functionality is roadmap-only, mark it clearly (do not present as shipped).

## Reporting Issues

Please include:

- OS and compiler versions
- exact configure/build/run commands
- whether MuJoCo is enabled
- logs or stack traces
- generated artifact paths when relevant
