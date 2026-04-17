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
| `libtorch` | `libtorch_cpu` | no | 48.0350 | 0.10 | 0.0305768 | 0.0358060 |
| `tensorrt_fp16` | `tensorrt_stub_emulation` | yes | 48.0350 | 0.10 | 0.0397559 | 0.0440470 |
| `tensorrt_int8` | `tensorrt_stub_emulation` | yes | 48.2105 | 0.05 | 0.0474782 | 0.0544430 |

Source summaries are generated under:

- `artifacts/runs/<eval_run_id>/evaluation_summary.json`

## Interpretation

- Policy quality remains aligned under backend swap for this smoke-scale test.
- TensorRT-compatible modes currently report emulated runtime; no native TensorRT kernel acceleration is claimed here.
- This benchmark is intended for parity/contract validation, not final GPU throughput claims.
