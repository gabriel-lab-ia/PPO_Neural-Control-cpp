# Python Bindings (`py_orbital_core`)

Build the pybind11 module against the C++ orbital core:

Status: optional binding path for experimentation and analysis workflows.

```bash
cmake -S training/bindings -B build/pybind -Dpybind11_DIR=$(python3 -m pybind11 --cmakedir)
cmake --build build/pybind -j
```

Then run a deterministic rollout export:

```bash
PYTHONPATH=build/pybind python3 training/run_mission_rollout.py
```
