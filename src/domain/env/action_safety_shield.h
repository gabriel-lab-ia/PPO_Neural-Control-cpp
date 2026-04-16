#pragma once

#include <algorithm>
#include <cmath>

namespace nmc::domain::env {

struct ActionSafetyConfig {
    float max_action_magnitude = 1.0f;
    float boundary_margin = 0.20f;
    float projection_gain = 0.65f;
};

class ActionSafetyShield {
public:
    explicit ActionSafetyShield(ActionSafetyConfig config = {}) : config_(config) {}

    [[nodiscard]] float project_1d(
        const float raw_action,
        const float position,
        const float velocity,
        const float position_limit
    ) const {
        float bounded = std::clamp(raw_action, -config_.max_action_magnitude, config_.max_action_magnitude);

        const float boundary_distance = position_limit - std::abs(position);
        if (boundary_distance >= config_.boundary_margin) {
            return bounded;
        }

        const float outward_direction = position >= 0.0f ? 1.0f : -1.0f;
        const float outward_velocity = velocity * outward_direction;
        const float outward_action = bounded * outward_direction;

        if (outward_velocity > 0.0f || outward_action > 0.0f) {
            const float risk = std::clamp((config_.boundary_margin - boundary_distance) / config_.boundary_margin, 0.0f, 1.0f);
            bounded -= outward_direction * config_.projection_gain * risk * (outward_action + std::max(outward_velocity, 0.0f));
        }

        return std::clamp(bounded, -config_.max_action_magnitude, config_.max_action_magnitude);
    }

private:
    ActionSafetyConfig config_{};
};

}  // namespace nmc::domain::env
