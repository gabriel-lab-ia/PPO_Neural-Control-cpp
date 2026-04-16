# Security Notes

## Baseline Security Posture

- The default runtime is CPU-first and local-first.
- Build artifacts and common secret patterns are ignored in `.gitignore`.
- Linux hardening flags are enabled in baseline CMake (`relro/now`, stack protection, fortify).
- Optional sanitizers are available through the `debug-sanitized` CMake preset.

## Sensitive Data Hygiene

Do not commit:

- `.env` files with real credentials
- private keys (`*.pem`, `*.key`, `*.crt`, `*.p12`)
- production telemetry dumps containing sensitive mission parameters

## Reporting

If you discover a vulnerability or unsafe default behavior, open a private security report when possible and include:

- affected component (`src/`, `core/`, `backend/`, etc.)
- reproducible steps
- impact scope
- mitigation proposal

## Operational Guidance

- Keep optional integrations explicitly disabled unless required (`MuJoCo`, backend, MLflow services).
- Prefer deterministic smoke runs before publishing benchmark claims.
- Validate generated artifacts before sharing externally.
