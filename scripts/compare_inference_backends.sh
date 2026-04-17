#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NMC_BIN="${NMC_BIN:-${PROJECT_ROOT}/build/nmc}"
SEED="${SEED:-7}"
EPISODES="${EPISODES:-20}"
TRAIN_RUN_ID="${TRAIN_RUN_ID:-trt_compare_train_q1}"
if [[ "${TRAIN_RUN_ID}" == "trt_compare_train_q1" ]]; then
  TRAIN_RUN_ID="trt_compare_train_$(date +%Y%m%d_%H%M%S)"
fi
EVAL_RUN_PREFIX="${EVAL_RUN_PREFIX:-trt_compare_eval_$(date +%Y%m%d_%H%M%S)}"

if [[ ! -x "${NMC_BIN}" ]]; then
  echo "[compare_inference_backends] nmc binary not found at ${NMC_BIN}" >&2
  echo "Set NMC_BIN or build first with: cmake --build --preset build" >&2
  exit 1
fi

echo "[compare_inference_backends] training reference checkpoint"
"${NMC_BIN}" train --quick --run-id "${TRAIN_RUN_ID}" --seed "${SEED}"

CHECKPOINT="artifacts/runs/${TRAIN_RUN_ID}/checkpoints/policy_last.pt"
if [[ ! -f "${CHECKPOINT}" ]]; then
  echo "[compare_inference_backends] expected checkpoint not found: ${CHECKPOINT}" >&2
  exit 1
fi

declare -a BACKENDS=(
  "libtorch"
  "tensorrt_fp16"
  "tensorrt_int8"
)

for backend in "${BACKENDS[@]}"; do
  run_id="${EVAL_RUN_PREFIX}_${backend}"
  echo "[compare_inference_backends] running ${backend}"
  "${NMC_BIN}" eval \
    --checkpoint "${CHECKPOINT}" \
    --episodes "${EPISODES}" \
    --seed "${SEED}" \
    --backend "${backend}" \
    --run-id "${run_id}"
done

export EVAL_RUN_PREFIX

python3 - <<'PY'
import json
from pathlib import Path
import os

rows = []
run_prefix = os.environ["EVAL_RUN_PREFIX"]
for backend in ("libtorch", "tensorrt_fp16", "tensorrt_int8"):
    run_id = f"{run_prefix}_{backend}"
    path = Path("artifacts/runs") / run_id / "evaluation_summary.json"
    data = json.loads(path.read_text(encoding="utf-8"))
    caps = data.get("backend_capabilities", {})
    rows.append(
        {
            "backend": backend,
            "runtime": caps.get("runtime", "unknown"),
            "emulated": str(caps.get("is_emulated", True)).lower(),
            "avg_return": float(data.get("avg_episode_return", 0.0)),
            "avg_latency": float(data.get("avg_inference_latency_ms", 0.0)),
            "p95_latency": float(data.get("p95_inference_latency_ms", 0.0)),
            "summary": str(path),
        }
    )

print("| Backend CLI | Runtime reported | Emulated | Avg episode return | Avg inference latency (ms) | P95 latency (ms) | Summary file |")
print("| --- | --- | --- | ---: | ---: | ---: | --- |")
for row in rows:
    print(
        f"| `{row['backend']}` | `{row['runtime']}` | `{row['emulated']}` | "
        f"{row['avg_return']:.4f} | {row['avg_latency']:.7f} | {row['p95_latency']:.7f} | `{row['summary']}` |"
    )
PY
