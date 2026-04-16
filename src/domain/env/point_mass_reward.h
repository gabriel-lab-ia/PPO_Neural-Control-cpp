#pragma once

namespace nmc::domain::env {

// Reward weights for the point-mass control surrogate.
// These terms are designed to approximate orbital-control priorities:
// tracking, velocity alignment, fuel use, and corridor safety.
struct PointMassRewardConfig {
    float position_log_weight = 0.55f;
    float position_log_scale = 2.20f;
    float position_exp_weight = 1.10f;
    float position_exp_scale = 1.75f;

    float velocity_alignment_weight = 0.30f;
    float velocity_error_weight = 0.18f;
    float desired_velocity_scale = 0.90f;
    float desired_velocity_limit = 1.35f;

    float control_quadratic_weight = 0.035f;
    float control_soft_weight = 0.070f;
    float control_soft_threshold = 0.55f;
    float control_soft_scale = 6.00f;

    float corridor_weight = 0.22f;
    float corridor_half_width = 0.90f;
    float boundary_weight = 0.30f;
    float boundary_soft_margin = 0.22f;

    float efficiency_bonus_weight = 0.10f;
    float efficiency_velocity_weight = 0.35f;

    float lyapunov_weight = 0.15f;

    float safety_projection_gain = 0.65f;
    float safety_boundary_margin = 0.20f;

    bool potential_shaping_enabled = true;
    float potential_position_weight = 0.55f;
    float potential_velocity_weight = 0.22f;
    float potential_gamma = 0.99f;

    float success_bonus = 1.50f;
};

}  // namespace nmc::domain::env
