# Class Diagram

```mermaid
classDiagram
    class PolicyValueModel {
        +act(observations, deterministic) PolicyOutput
        +evaluate_actions(observations, actions)
        +values(observations)
    }

    class PPOTrainer {
        -TrainerConfig config_
        -PolicyValueModel model_
        -vector~unique_ptr~Environment~~ environments_
        +train() vector~TrainingMetrics~
        +run_live_episode(max_steps, deterministic)
        +consume_completed_episodes() vector~EpisodeRecord~
        +model() PolicyValueModel&
    }

    class Environment {
        <<interface>>
        +reset() Tensor
        +step(action) StepResult
        +observation_dim() int64
        +action_dim() int64
        +success_signal(result) float
    }

    class PointMassEnv
    class MuJoCoCartPoleEnv

    class PolicyInferenceBackend {
        <<interface>>
        +load_checkpoint(path)
        +infer(observation, deterministic) InferenceOutput
    }

    class LibTorchPolicyBackend
    class TensorRtPolicyBackendStub

    class TrainingRunner {
        +run(TrainConfig) TrainingRunOutput
    }

    class EvaluationRunner {
        +run(EvalConfig) EvaluationRunOutput
    }

    class BenchmarkRunner {
        +run(BenchmarkConfig) BenchmarkRunOutput
    }

    class SQLiteExperimentStore {
        +insert_run_start(...)
        +finalize_run(...)
        +insert_episode(...)
        +insert_event(...)
        +insert_benchmark(...)
    }

    class ArtifactLayout

    Environment <|.. PointMassEnv
    Environment <|.. MuJoCoCartPoleEnv

    PolicyInferenceBackend <|.. LibTorchPolicyBackend
    PolicyInferenceBackend <|.. TensorRtPolicyBackendStub

    PPOTrainer o-- PolicyValueModel
    PPOTrainer o-- Environment

    TrainingRunner --> PPOTrainer
    TrainingRunner --> SQLiteExperimentStore
    TrainingRunner --> ArtifactLayout

    EvaluationRunner --> PolicyInferenceBackend
    EvaluationRunner --> Environment
    EvaluationRunner --> SQLiteExperimentStore

    BenchmarkRunner --> TrainingRunner
    BenchmarkRunner --> EvaluationRunner
```
