#!/usr/bin/env python3
from __future__ import annotations

import json
import shutil
from pathlib import Path


def copy_required_artifacts(project_root: Path, docs_demo: Path) -> list[str]:
    required = {
        project_root / "artifacts" / "benchmarks" / "latest.json": "benchmark_latest.json",
        project_root / "artifacts" / "benchmarks" / "latest.csv": "benchmark_latest.csv",
        project_root / "artifacts" / "latest" / "manifest.json": "latest_manifest.json",
        project_root / "artifacts" / "latest" / "checkpoint.meta": "latest_checkpoint.meta",
    }

    copied: list[str] = []
    missing = [str(path) for path in required if not path.exists()]
    if missing:
        raise SystemExit("missing required artifacts:\n- " + "\n- ".join(missing))

    for src, dst_name in required.items():
        dst = docs_demo / dst_name
        shutil.copy2(src, dst)
        copied.append(dst_name)

    return copied


def copy_optional_artifacts(project_root: Path, docs_demo: Path) -> list[str]:
    optional = {
        project_root / "artifacts" / "latest" / "training_metrics.csv": "latest_training_metrics.csv",
        project_root / "artifacts" / "latest" / "training_summary.json": "latest_training_summary.json",
        project_root / "artifacts" / "latest" / "evaluation_summary.json": "latest_evaluation_summary.json",
        project_root / "artifacts" / "latest" / "live_rollout.csv": "latest_live_rollout.csv",
    }

    copied: list[str] = []
    for src, dst_name in optional.items():
        if not src.exists():
            continue
        dst = docs_demo / dst_name
        shutil.copy2(src, dst)
        copied.append(dst_name)

    return copied


def write_manifest(docs_demo: Path, copied_required: list[str], copied_optional: list[str]) -> None:
    manifest = {
        "required": copied_required,
        "optional": copied_optional,
    }
    (docs_demo / "published_artifacts.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )


def main() -> int:
    project_root = Path(__file__).resolve().parents[1]
    docs_demo = project_root / "docs" / "demo"
    docs_demo.mkdir(parents=True, exist_ok=True)

    copied_required = copy_required_artifacts(project_root, docs_demo)
    copied_optional = copy_optional_artifacts(project_root, docs_demo)
    write_manifest(docs_demo, copied_required, copied_optional)

    print(docs_demo)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
