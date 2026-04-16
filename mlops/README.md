# MLOps Pipeline

This folder provides experiment tracking and model packaging workflows for the C++ PPO baseline.

Status: optional module (baseline runtime does not require MLflow services).

## Features

- MLflow run tracking (params, metrics, tags, artifacts)
- mission-aware run tags (`orbital_dynamics`, `perturbation_level`, `reward_shaping`)
- ONNX export for deployment contracts
- model registration in MLflow registry

## Quick Start

```bash
python3 -m pip install -r mlops/requirements.txt
./mlops/start_mlflow.sh
./mlops/run_training_mlflow.sh
```

## Manual Run

```bash
python3 mlops/train_with_mlflow.py \
  --tracking-uri http://localhost:5000 \
  --experiment orbital_ppo \
  --run-id orbital_mission_001 \
  --seed 7 --updates 30 --num-envs 16 --env point_mass \
  --export-onnx --register-model
```
