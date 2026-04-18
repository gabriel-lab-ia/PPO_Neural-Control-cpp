# Inference Backend Comparison (Baseline vs TensorRT-Compatible Path)

Date: 2026-04-17  
Scenario: `point_mass`, same checkpoint, deterministic seed `7`, `episodes=20`

## Command Trace

```bash
NMC_BIN=./build-local/nmc ./scripts/compare_inference_backends.sh
```

## Results

| Backend | Runtime | Emulated | Avg return | Success rate | Avg latency (ms) | P95 latency (ms) |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| `libtorch` | `libtorch_cpu` | no | 48.0350 | 0.10 | 0.0290420 | 0.0327260 |
| `tensorrt_fp16` | `tensorrt_fallback_libtorch` | yes | 48.0350 | 0.10 | 0.0290775 | 0.0391200 |
| `tensorrt_int8` | `tensorrt_fallback_libtorch` | yes | 48.0350 | 0.10 | 0.0281963 | 0.0317110 |

Source summaries are generated under:

- `artifacts/runs/<eval_run_id>/evaluation_summary.json`

## Interpretation

- Policy quality remains aligned under backend swap for this smoke-scale test.
- TensorRT-compatible modes in this sample report fallback/emulated runtime; native TensorRT runs should report runtime `tensorrt_native`.
- This benchmark is intended for parity/contract validation, not final GPU throughput claims.
