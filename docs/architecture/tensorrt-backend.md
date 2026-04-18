# TensorRT Backend Architecture

This repository keeps inference backends under `src/domain/inference` and routes runtime selection via existing `--backend` CLI flag (`libtorch`, `tensorrt`, `tensorrt_fp16`, `tensorrt_int8`).

## Recommended Structure

```text
src/domain/inference/
  policy_inference_backend.h          # stable backend contract
  inference_backend_factory.*         # backend selection from CLI/config
  libtorch_policy_backend.*           # baseline backend
  tensorrt_policy_backend.*           # runtime controller + fallback policy
  tensorrt_native_backend.*           # native TensorRT runtime (build/load/infer)
  tensorrt_policy_backend_stub.*      # compatibility emulation (legacy/testing)
  inference_benchmark.*               # latency benchmark utilities (avg/p95/min/max)
```

## TensorRT Native Flow

1. `tensorrt_policy_backend` receives `checkpoint_path`.
2. If path is `.onnx`, `.engine`, or `.plan`, it tries native TensorRT first.
3. `.onnx` path:
   - parse ONNX
   - configure precision (`FP16` preferred, fallback to `FP32`; `INT8` with simple entropy calibrator)
   - configure optimization profile for dynamic batch shape
   - build and serialize engine
4. `.engine/.plan` path:
   - deserialize and execute directly
5. If native load/build fails, fallback is automatic to LibTorch (`.pt`) when available.

## Runtime Metadata

Evaluation summary reports:

- `backend`
- `backend_capabilities.runtime`
- `backend_capabilities.uses_cuda`
- `backend_capabilities.is_emulated`
- `avg_inference_latency_ms`
- `p95_inference_latency_ms`

This supports reproducible latency comparisons between LibTorch and TensorRT paths.
