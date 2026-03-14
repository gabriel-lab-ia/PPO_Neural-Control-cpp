#include "model/ppo_agent.h"

#include <cmath>

namespace nmc {
namespace {

constexpr float kMinLogStd = -1.2f;
constexpr float kMaxLogStd = 0.35f;
constexpr double kLogTwoPi = 1.8378770664093453;

void init_linear(const torch::nn::Linear& layer, double gain) {
    torch::NoGradGuard no_grad;
    torch::nn::init::orthogonal_(layer->weight, gain);
    torch::nn::init::constant_(layer->bias, 0.0);
}

torch::Tensor gaussian_log_prob(
    const torch::Tensor& actions,
    const torch::Tensor& mean,
    const torch::Tensor& std
) {
    const auto variance = std.pow(2);
    const auto centered = actions - mean;
    const auto log_prob =
        -0.5 * ((centered.pow(2) / variance) + 2.0 * std.log() + kLogTwoPi);
    return log_prob.sum(-1);
}

torch::Tensor gaussian_entropy(const torch::Tensor& std) {
    const auto entropy = 0.5 + 0.5 * kLogTwoPi + std.log();
    return entropy.sum(-1);
}

}  // namespace

PPOAgentImpl::PPOAgentImpl(int64_t observation_dim, int64_t action_dim, int64_t hidden_dim) {
    observation_dim_ = observation_dim;
    action_dim_ = action_dim;
    hidden_dim_ = hidden_dim;

    encoder_input_ = torch::nn::Linear(observation_dim, hidden_dim);
    encoder_hidden_ = torch::nn::Linear(hidden_dim, hidden_dim);
    actor_mean_ = torch::nn::Linear(hidden_dim, action_dim);
    critic_ = torch::nn::Linear(hidden_dim, 1);
    log_std_ = register_parameter("log_std", torch::full({action_dim}, -0.10f));

    register_module("encoder_input", encoder_input_);
    register_module("encoder_hidden", encoder_hidden_);
    register_module("actor_mean", actor_mean_);
    register_module("critic", critic_);

    init_linear(encoder_input_, std::sqrt(2.0));
    init_linear(encoder_hidden_, std::sqrt(2.0));
    init_linear(actor_mean_, 0.01);
    init_linear(critic_, 1.0);
}

PolicyOutput PPOAgentImpl::act(const torch::Tensor& observations, bool deterministic) {
    const auto latent = encode(observations);
    const auto mean = torch::tanh(actor_mean_->forward(latent));
    const auto std = torch::exp(torch::clamp(log_std_, kMinLogStd, kMaxLogStd)).expand_as(mean);
    torch::Tensor action = mean;

    if (!deterministic) {
        action = mean + std * torch::randn_like(mean);
    }

    action = torch::clamp(action, -1.0f, 1.0f);
    const auto log_prob = gaussian_log_prob(action, mean, std);
    const auto value = critic_->forward(latent).squeeze(-1);

    return {action, log_prob, value, mean, std};
}

std::pair<torch::Tensor, torch::Tensor> PPOAgentImpl::evaluate_actions(
    const torch::Tensor& observations,
    const torch::Tensor& actions
) {
    const auto [mean, std] = policy_distribution(observations);
    const auto log_prob = gaussian_log_prob(actions, mean, std);
    const auto entropy = gaussian_entropy(std);
    return {log_prob, entropy};
}

torch::Tensor PPOAgentImpl::values(const torch::Tensor& observations) {
    return critic_->forward(encode(observations)).squeeze(-1);
}

torch::Tensor PPOAgentImpl::policy_std(const torch::Tensor& observations) {
    return policy_distribution(observations).second;
}

int64_t PPOAgentImpl::parameter_count() const {
    int64_t total = 0;
    for (const auto& parameter : parameters()) {
        total += parameter.numel();
    }
    return total;
}

std::pair<torch::Tensor, torch::Tensor> PPOAgentImpl::policy_distribution(
    const torch::Tensor& observations
) {
    const auto latent = encode(observations);
    const auto mean = torch::tanh(actor_mean_->forward(latent));
    const auto std = torch::exp(torch::clamp(log_std_, kMinLogStd, kMaxLogStd)).expand_as(mean);
    return {mean, std};
}

torch::Tensor PPOAgentImpl::encode(const torch::Tensor& observations) {
    const auto hidden_1 = torch::silu(encoder_input_->forward(observations));
    const auto hidden_2 = torch::silu(encoder_hidden_->forward(hidden_1) + hidden_1);
    return hidden_2;
}

std::vector<std::string> PPOAgentImpl::visualization_layer_names() const {
    return {
        "observation",
        "hidden_1",
        "hidden_2",
        "policy_mean",
        "value"
    };
}

std::vector<int64_t> PPOAgentImpl::visualization_layer_sizes() const {
    return {
        observation_dim_,
        hidden_dim_,
        hidden_dim_,
        action_dim_,
        1
    };
}

std::vector<torch::Tensor> PPOAgentImpl::visualization_weights() {
    return {
        encoder_input_->weight.detach().to(torch::kCPU).clone(),
        encoder_hidden_->weight.detach().to(torch::kCPU).clone(),
        actor_mean_->weight.detach().to(torch::kCPU).clone(),
        critic_->weight.detach().to(torch::kCPU).clone()
    };
}

std::vector<torch::Tensor> PPOAgentImpl::visualization_activations(const torch::Tensor& observations) {
    auto batch = observations;
    if (batch.dim() == 1) {
        batch = batch.unsqueeze(0);
    }

    const auto input = batch.squeeze(0).detach().to(torch::kCPU).clone();
    const auto hidden_1 = torch::silu(encoder_input_->forward(batch)).squeeze(0);
    const auto hidden_2 = torch::silu(encoder_hidden_->forward(hidden_1.unsqueeze(0)) + hidden_1.unsqueeze(0)).squeeze(0);
    const auto policy = torch::tanh(actor_mean_->forward(hidden_2.unsqueeze(0))).squeeze(0);
    const auto value = critic_->forward(hidden_2.unsqueeze(0)).squeeze(0);

    return {
        input,
        hidden_1.detach().to(torch::kCPU).clone(),
        hidden_2.detach().to(torch::kCPU).clone(),
        policy.detach().to(torch::kCPU).clone(),
        value.detach().to(torch::kCPU).clone()
    };
}

float PPOAgentImpl::policy_std_scalar() {
    return torch::exp(torch::clamp(log_std_, kMinLogStd, kMaxLogStd)).mean().item<float>();
}

}  // namespace nmc
