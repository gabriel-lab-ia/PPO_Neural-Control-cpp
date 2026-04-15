#include "interfaces/cli/command_line.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include "application/benchmark_runner.h"
#include "application/evaluation_runner.h"
#include "application/training_runner.h"

namespace nmc::interfaces::cli {
namespace {

int64_t parse_int64(const std::string& name, const std::string& value) {
    try {
        return std::stoll(value);
    } catch (const std::exception&) {
        throw std::runtime_error("invalid integer for " + name + ": " + value);
    }
}

float parse_float(const std::string& name, const std::string& value) {
    try {
        return std::stof(value);
    } catch (const std::exception&) {
        throw std::runtime_error("invalid float for " + name + ": " + value);
    }
}

bool parse_bool(const std::string& name, const std::string& value) {
    if (value == "1" || value == "true" || value == "TRUE" || value == "on") {
        return true;
    }
    if (value == "0" || value == "false" || value == "FALSE" || value == "off") {
        return false;
    }
    throw std::runtime_error("invalid bool for " + name + ": " + value);
}

std::string require_value(const int argc, char** argv, int& index, const std::string& name) {
    if (index + 1 >= argc) {
        throw std::runtime_error("missing value for option: " + name);
    }
    ++index;
    return argv[index];
}

void print_usage() {
    std::cout
        << "Usage:\n"
        << "  nmc train [options]\n"
        << "  nmc eval [options]\n"
        << "  nmc benchmark [--quick|--full] [--seed N] [--name NAME]\n\n"
        << "Train options:\n"
        << "  --env <point_mass|mujoco_cartpole>\n"
        << "  --seed <int>\n"
        << "  --num-envs <int>\n"
        << "  --updates <int>\n"
        << "  --rollout-steps <int>\n"
        << "  --ppo-epochs <int>\n"
        << "  --minibatch-size <int>\n"
        << "  --hidden-dim <int>\n"
        << "  --learning-rate <float>\n"
        << "  --run-id <string>\n"
        << "  --resume-checkpoint <path>\n"
        << "  --live-steps <int>\n"
        << "  --deterministic-live <bool>\n"
        << "  --mujoco-model <path>\n\n"
        << "Eval options:\n"
        << "  --checkpoint <path>\n"
        << "  --env <point_mass|mujoco_cartpole>\n"
        << "  --episodes <int>\n"
        << "  --max-steps <int>\n"
        << "  --seed <int>\n"
        << "  --backend <libtorch|tensorrt>\n"
        << "  --deterministic <bool>\n"
        << "  --run-id <string>\n"
        << "  --mujoco-model <path>\n";
}

int run_train(const int argc, char** argv) {
    domain::config::TrainConfig config;

    for (int index = 2; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--env") {
            config.environment = require_value(argc, argv, index, arg);
        } else if (arg == "--seed") {
            config.trainer.seed = parse_int64(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--num-envs") {
            config.trainer.num_envs = parse_int64(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--updates") {
            config.trainer.total_updates = parse_int64(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--rollout-steps") {
            config.trainer.ppo.rollout_steps = parse_int64(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--ppo-epochs") {
            config.trainer.ppo.ppo_epochs = parse_int64(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--minibatch-size") {
            config.trainer.ppo.minibatch_size = parse_int64(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--hidden-dim") {
            config.trainer.hidden_dim = parse_int64(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--learning-rate") {
            config.trainer.ppo.learning_rate = parse_float(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--run-id") {
            config.run_id = require_value(argc, argv, index, arg);
        } else if (arg == "--resume-checkpoint") {
            config.resume_checkpoint = std::filesystem::path(require_value(argc, argv, index, arg));
        } else if (arg == "--live-steps") {
            config.live_rollout_steps = parse_int64(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--deterministic-live") {
            config.deterministic_live_rollout = parse_bool(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--mujoco-model") {
            config.mujoco_model_path = std::filesystem::path(require_value(argc, argv, index, arg));
        } else if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        } else {
            throw std::runtime_error("unknown train option: " + arg);
        }
    }

    application::TrainingRunner runner;
    static_cast<void>(runner.run(config));
    return 0;
}

int run_eval(const int argc, char** argv) {
    domain::config::EvalConfig config;

    for (int index = 2; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--checkpoint") {
            config.checkpoint_path = std::filesystem::path(require_value(argc, argv, index, arg));
        } else if (arg == "--env") {
            config.environment = require_value(argc, argv, index, arg);
        } else if (arg == "--episodes") {
            config.episodes = parse_int64(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--max-steps") {
            config.max_steps = parse_int64(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--seed") {
            config.seed = parse_int64(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--backend") {
            config.inference_backend = require_value(argc, argv, index, arg);
        } else if (arg == "--deterministic") {
            config.deterministic_policy = parse_bool(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--run-id") {
            config.run_id = require_value(argc, argv, index, arg);
        } else if (arg == "--mujoco-model") {
            config.mujoco_model_path = std::filesystem::path(require_value(argc, argv, index, arg));
        } else if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        } else {
            throw std::runtime_error("unknown eval option: " + arg);
        }
    }

    application::EvaluationRunner runner;
    static_cast<void>(runner.run(config));
    return 0;
}

int run_benchmark(const int argc, char** argv) {
    domain::config::BenchmarkConfig config;

    for (int index = 2; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--quick" || arg == "--smoke") {
            config.quick = true;
        } else if (arg == "--full") {
            config.quick = false;
        } else if (arg == "--seed") {
            config.seed = parse_int64(arg, require_value(argc, argv, index, arg));
        } else if (arg == "--name") {
            config.benchmark_name = require_value(argc, argv, index, arg);
        } else if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        } else {
            throw std::runtime_error("unknown benchmark option: " + arg);
        }
    }

    application::BenchmarkRunner runner;
    static_cast<void>(runner.run(config));
    return 0;
}

}  // namespace

int run_cli(const int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const std::string command = argv[1];
    if (command == "train") {
        return run_train(argc, argv);
    }
    if (command == "eval") {
        return run_eval(argc, argv);
    }
    if (command == "benchmark") {
        return run_benchmark(argc, argv);
    }
    if (command == "help" || command == "--help" || command == "-h") {
        print_usage();
        return 0;
    }

    throw std::runtime_error("unknown command: " + command);
}

}  // namespace nmc::interfaces::cli
