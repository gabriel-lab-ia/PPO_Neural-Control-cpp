#!/usr/bin/env python3
from __future__ import annotations

import shutil
from pathlib import Path


def main() -> int:
    project_root = Path(__file__).resolve().parents[1]
    artifacts = project_root / "artifacts"
    docs_demo = project_root / "docs" / "demo"
    docs_demo.mkdir(parents=True, exist_ok=True)

    files = [
        "neural_network_3d.html",
        "neural_network_3d.json",
        "learning_curve.svg",
        "learning_curve.csv",
        "live_rollout.csv",
        "benchmark_summary.json",
    ]

    missing = [name for name in files if not (artifacts / name).exists()]
    if missing:
        raise SystemExit(f"missing required artifacts: {', '.join(missing)}")

    for name in files:
        shutil.copy2(artifacts / name, docs_demo / name)

    print(docs_demo)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
