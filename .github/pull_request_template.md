## Summary

- What changed and why.

## Scope

- [ ] baseline runtime (`src/`)
- [ ] backend (`backend/`)
- [ ] frontend (`frontend/`)
- [ ] docs/contracts (`docs/`)
- [ ] CI/docker/build tooling

## Validation

Provide exact commands executed and key outputs.

```bash
cmake --preset dev
cmake --build --preset build
./build/nmc benchmark --quick --name pr_smoke --seed 7
./build/nmc train --quick --run-id pr_train --seed 7
./build/nmc eval --checkpoint artifacts/latest/checkpoint.pt --episodes 5 --backend libtorch --run-id pr_eval --seed 7
```

If backend/frontend changed:

```bash
cd frontend && npm run typecheck && npm run build
# backend optional build if Boost available
cmake --preset orbital-stack
cmake --build --preset build-orbital-backend
```

## Contract Checks

- [ ] OpenAPI updated when API changed (`docs/openapi/orbital-api.yaml`)
- [ ] typed frontend API surface kept in sync (`frontend/src/shared/api/generated/`)
- [ ] artifact schema behavior unchanged or migration documented

## Risk Notes

- Runtime correctness risk:
- Determinism/reproducibility risk:
- Performance risk:
