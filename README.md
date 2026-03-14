Neuro Motor CPP

`Neuro Motor CPP` is a C++20 reinforcement learning project that combines:

- `LibTorch` for neural policy/value networks
- `PPO` for on-policy optimization
- `MuJoCo` for continuous-control simulation
- `HTML` and `JSON` exports for interactive neural visualization

The current repository provides a formal PPO baseline in C++, supports a MuJoCo cart-pole environment, exports learning metrics, and generates a browser-based 3D viewer that renders the trained policy network and its live activations.

Highlights

- PPO implementation in modern C++ with `LibTorch`
- Optional MuJoCo integration through a clean `Environment` interface
- CSV metrics and SVG learning curves
- Live rollout capture from the trained policy
- Standalone 3D HTML viewer for policy structure and activations
- Static web publishing path through `docs/` for GitHub Pages

Repository Layout

- `src/app/` application entrypoints and orchestration
- `src/env/` environment interface and implementations
- `src/model/` PPO policy and value network
- `src/train/` rollout collection, GAE, and PPO updates
- `src/utils/` logging and export utilities
- `assets/mujoco/` project MuJoCo XML assets
- `tools/` local scripts for plotting, viewing, and site publishing
- `docs/` static web site for GitHub Pages
- `notebooks/` analysis notebooks

Requirements

- GCC 13+
- CMake 3.24+
- LibTorch 2.2.2
- Eigen
- MuJoCo 3.2.6 or newer for MuJoCo environments

Quick Start

This repository does not need to commit LibTorch binaries. If `lib/libtorch/` is missing, install the CPU package locally with:

```bash
bash tools/setup_libtorch_cpu.sh
```

Configure:

```bash
cmake --preset dev
```

Build:

```bash
cmake --build --preset build
```

Run the default PPO baseline:

```bash
./build/motor
```

Generate the learning-curve SVG:

```bash
python3 tools/plot_learning_curve.py artifacts/learning_curve.csv artifacts/learning_curve.svg
```

## MuJoCo Training

Build with MuJoCo support:

```bash
cmake --preset dev -DNMC_ENABLE_MUJOCO=ON -DNMC_MUJOCO_ROOT=$HOME/.local/mujoco-3.2.6
cmake --build --preset build
```

Train the PPO agent in MuJoCo:

```bash
NMC_ENV=mujoco_cartpole ./build/motor
```

Run a live policy rollout after training:

```bash
NMC_ENV=mujoco_cartpole NMC_LIVE_POLICY=1 NMC_LIVE_STEPS=64 ./build/motor
```

## Visualization

Open the generated 3D network viewer:

```bash
xdg-open artifacts/neural_network_3d.html
```

Open the MuJoCo viewer for the project cart-pole:

```bash
./tools/view_mujoco.sh
```

Web Publishing

This repository is prepared for static deployment through GitHub Pages.

Sync the current demo into `docs/`:

```bash
python3 tools/publish_demo.py
```

The committed web assets are expected under `docs/demo/`.

After pushing the repository to GitHub and enabling Pages, the generated site exposes:

- `/` landing page
- `/demo/neural_network_3d.html` direct 3D neural viewer
- `/demo/learning_curve.svg` exported learning curve

## Generated Outputs

- `artifacts/learning_curve.csv`
- `artifacts/learning_curve.svg`
- `artifacts/live_rollout.csv`
- `artifacts/neural_network_3d.html`
- `artifacts/neural_network_3d.json`

## Project Status

This is a serious research-engineering foundation, not a general-purpose RL framework yet. The current codebase is strongest as:

- a C++ PPO reference implementation
- a MuJoCo-ready training baseline
- a neural-visualization demo for policy inspection

## Development Notes

- The default CI build targets the non-MuJoCo baseline so public builds stay lightweight.
- MuJoCo support is optional and activated through `NMC_ENABLE_MUJOCO`.
- The static site is generated from local artifacts; `docs/` is the publishable output.

## License

This project is distributed under the MIT License. See [LICENSE](LICENSE).
