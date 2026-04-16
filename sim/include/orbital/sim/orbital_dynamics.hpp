#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>

namespace orbital::sim {

struct OrbitalBodyState6DOF {
    std::array<double, 3> position_m{6'778'000.0, 0.0, 0.0};
    std::array<double, 3> velocity_mps{0.0, 7'670.0, 0.0};
    std::array<double, 4> attitude_wxyz{1.0, 0.0, 0.0, 0.0};
    std::array<double, 3> angular_rate_rps{0.0, 0.0, 0.0};
    double mission_time_s = 0.0;
};

struct OrbitalControlCommand6DOF {
    std::array<double, 3> thrust_body_n{0.0, 0.0, 0.0};
    std::array<double, 3> torque_body_nm{0.0, 0.0, 0.0};
};

struct OrbitalDynamicsConfig6DOF {
    double gravitational_mu = 3.986004418e14;
    double earth_radius_m = 6'378'136.3;
    double j2 = 1.08262668e-3;

    double vehicle_mass_kg = 300.0;
    double vehicle_ref_area_m2 = 2.5;
    double drag_cd = 2.15;
    double srp_cr = 1.35;

    double atmosphere_density_ref = 1.225;
    double atmosphere_scale_height_m = 8'500.0;
    double atmosphere_cutoff_altitude_m = 1'000'000.0;

    double solar_flux_w_m2 = 1361.0;
    double speed_of_light_mps = 299'792'458.0;

    std::array<double, 3> sun_position_m{149.6e9, 0.0, 0.0};
    std::array<double, 3> moon_position_m{384.4e6, 0.0, 0.0};
    double moon_mu = 4.9048695e12;
    double sun_mu = 1.32712440018e20;

    std::array<double, 3> inertia_diag_kgm2{120.0, 100.0, 80.0};
    double angular_damping = 2.0e-4;
};

class OrbitalDynamics {
public:
    virtual ~OrbitalDynamics() = default;

    [[nodiscard]] virtual OrbitalBodyState6DOF propagate(
        const OrbitalBodyState6DOF& state,
        const OrbitalControlCommand6DOF& command,
        double dt_s
    ) const = 0;
};

class HighFidelityOrbitalDynamics6DOF final : public OrbitalDynamics {
public:
    explicit HighFidelityOrbitalDynamics6DOF(OrbitalDynamicsConfig6DOF config = {}) : config_(config) {}

    [[nodiscard]] OrbitalBodyState6DOF propagate(
        const OrbitalBodyState6DOF& state,
        const OrbitalControlCommand6DOF& command,
        const double dt_s
    ) const override {
        if (dt_s <= 0.0) {
            return state;
        }

        struct Derivative {
            std::array<double, 3> dpos{};
            std::array<double, 3> dvel{};
            std::array<double, 4> dquat{};
            std::array<double, 3> domega{};
        };

        const auto derive = [&](const OrbitalBodyState6DOF& s) -> Derivative {
            Derivative derivative;
            derivative.dpos = s.velocity_mps;

            const auto acceleration = total_acceleration(s, command);
            derivative.dvel = acceleration;

            derivative.dquat = quaternion_rate(s.attitude_wxyz, s.angular_rate_rps);
            derivative.domega = angular_acceleration(s.angular_rate_rps, command.torque_body_nm);
            return derivative;
        };

        const Derivative k1 = derive(state);
        const Derivative k2 = derive(apply_delta(state, k1, dt_s * 0.5));
        const Derivative k3 = derive(apply_delta(state, k2, dt_s * 0.5));
        const Derivative k4 = derive(apply_delta(state, k3, dt_s));

        OrbitalBodyState6DOF next = state;
        for (std::size_t axis = 0; axis < next.position_m.size(); ++axis) {
            next.position_m[axis] += dt_s / 6.0 * (k1.dpos[axis] + 2.0 * k2.dpos[axis] + 2.0 * k3.dpos[axis] + k4.dpos[axis]);
            next.velocity_mps[axis] += dt_s / 6.0 * (k1.dvel[axis] + 2.0 * k2.dvel[axis] + 2.0 * k3.dvel[axis] + k4.dvel[axis]);
            next.angular_rate_rps[axis] += dt_s / 6.0 * (k1.domega[axis] + 2.0 * k2.domega[axis] + 2.0 * k3.domega[axis] + k4.domega[axis]);
        }

        for (std::size_t index = 0; index < next.attitude_wxyz.size(); ++index) {
            next.attitude_wxyz[index] +=
                dt_s / 6.0 * (k1.dquat[index] + 2.0 * k2.dquat[index] + 2.0 * k3.dquat[index] + k4.dquat[index]);
        }
        normalize_quaternion(next.attitude_wxyz);

        next.mission_time_s += dt_s;
        return next;
    }

private:
    struct Vec3 {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    };

    static Vec3 to_vec3(const std::array<double, 3>& value) {
        return {value[0], value[1], value[2]};
    }

    static std::array<double, 3> to_array(const Vec3& value) {
        return {value.x, value.y, value.z};
    }

    static Vec3 add(const Vec3& lhs, const Vec3& rhs) {
        return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
    }

    static Vec3 sub(const Vec3& lhs, const Vec3& rhs) {
        return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
    }

    static Vec3 mul(const Vec3& value, const double scale) {
        return {value.x * scale, value.y * scale, value.z * scale};
    }

    static double dot(const Vec3& lhs, const Vec3& rhs) {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    static double norm(const Vec3& value) {
        return std::sqrt(dot(value, value));
    }

    static Vec3 normalize(const Vec3& value) {
        const double n = std::max(norm(value), 1.0e-9);
        return mul(value, 1.0 / n);
    }

    static std::array<double, 4> quaternion_rate(
        const std::array<double, 4>& q,
        const std::array<double, 3>& omega
    ) {
        const double w = q[0];
        const double x = q[1];
        const double y = q[2];
        const double z = q[3];

        const double p = omega[0];
        const double qv = omega[1];
        const double r = omega[2];

        return {
            -0.5 * (x * p + y * qv + z * r),
             0.5 * (w * p + y * r - z * qv),
             0.5 * (w * qv + z * p - x * r),
             0.5 * (w * r + x * qv - y * p)
        };
    }

    static void normalize_quaternion(std::array<double, 4>& q) {
        const double n = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
        const double safe_n = std::max(n, 1.0e-12);
        for (double& value : q) {
            value /= safe_n;
        }
    }

    Vec3 rotate_body_to_inertial(const std::array<double, 4>& q, const Vec3& body) const {
        const double w = q[0];
        const double x = q[1];
        const double y = q[2];
        const double z = q[3];

        const double r00 = 1.0 - 2.0 * (y * y + z * z);
        const double r01 = 2.0 * (x * y - z * w);
        const double r02 = 2.0 * (x * z + y * w);
        const double r10 = 2.0 * (x * y + z * w);
        const double r11 = 1.0 - 2.0 * (x * x + z * z);
        const double r12 = 2.0 * (y * z - x * w);
        const double r20 = 2.0 * (x * z - y * w);
        const double r21 = 2.0 * (y * z + x * w);
        const double r22 = 1.0 - 2.0 * (x * x + y * y);

        return {
            r00 * body.x + r01 * body.y + r02 * body.z,
            r10 * body.x + r11 * body.y + r12 * body.z,
            r20 * body.x + r21 * body.y + r22 * body.z,
        };
    }

    Vec3 central_gravity(const Vec3& r) const {
        const double radius = std::max(norm(r), config_.earth_radius_m);
        const double inv_r3 = 1.0 / (radius * radius * radius);
        return mul(r, -config_.gravitational_mu * inv_r3);
    }

    Vec3 j2_perturbation(const Vec3& r) const {
        const double radius = std::max(norm(r), config_.earth_radius_m);
        const double z2 = r.z * r.z;
        const double r2 = radius * radius;
        const double factor = 1.5 * config_.j2 * config_.gravitational_mu *
            (config_.earth_radius_m * config_.earth_radius_m) / std::pow(radius, 5.0);

        const double common = 5.0 * z2 / r2;
        return {
            factor * r.x * (common - 1.0),
            factor * r.y * (common - 1.0),
            factor * r.z * (common - 3.0),
        };
    }

    double nrlmsise00_density_approx(const double altitude_m) const {
        const double clamped_altitude = std::clamp(altitude_m, 0.0, config_.atmosphere_cutoff_altitude_m);
        const double exponential = std::exp(-clamped_altitude / config_.atmosphere_scale_height_m);
        return config_.atmosphere_density_ref * exponential;
    }

    Vec3 drag_acceleration(const Vec3& r, const Vec3& v) const {
        const double altitude = norm(r) - config_.earth_radius_m;
        const double rho = nrlmsise00_density_approx(altitude);
        const double speed = std::max(norm(v), 1.0e-6);

        const double coeff = -0.5 * rho * config_.drag_cd * config_.vehicle_ref_area_m2 / config_.vehicle_mass_kg;
        return mul(v, coeff * speed);
    }

    Vec3 srp_acceleration(const Vec3& r) const {
        const Vec3 sun = to_vec3(config_.sun_position_m);
        const Vec3 direction = normalize(sub(sun, r));
        const double pressure = config_.solar_flux_w_m2 / config_.speed_of_light_mps;
        const double magnitude = pressure * config_.srp_cr * config_.vehicle_ref_area_m2 / config_.vehicle_mass_kg;
        return mul(direction, magnitude);
    }

    Vec3 third_body_acceleration(const Vec3& r, const Vec3& body_position, const double body_mu) const {
        const Vec3 body_minus_sc = sub(body_position, r);
        const double d1 = std::max(norm(body_minus_sc), 1.0);
        const double d2 = std::max(norm(body_position), 1.0);
        return add(
            mul(body_minus_sc, body_mu / (d1 * d1 * d1)),
            mul(body_position, -body_mu / (d2 * d2 * d2))
        );
    }

    std::array<double, 3> angular_acceleration(
        const std::array<double, 3>& omega,
        const std::array<double, 3>& torque
    ) const {
        return {
            torque[0] / config_.inertia_diag_kgm2[0] - config_.angular_damping * omega[0],
            torque[1] / config_.inertia_diag_kgm2[1] - config_.angular_damping * omega[1],
            torque[2] / config_.inertia_diag_kgm2[2] - config_.angular_damping * omega[2],
        };
    }

    std::array<double, 3> total_acceleration(
        const OrbitalBodyState6DOF& state,
        const OrbitalControlCommand6DOF& command
    ) const {
        const Vec3 r = to_vec3(state.position_m);
        const Vec3 v = to_vec3(state.velocity_mps);

        const Vec3 thrust_body = to_vec3(command.thrust_body_n);
        const Vec3 thrust_inertial = rotate_body_to_inertial(state.attitude_wxyz, thrust_body);
        const Vec3 control_accel = mul(thrust_inertial, 1.0 / config_.vehicle_mass_kg);

        Vec3 total = central_gravity(r);
        total = add(total, j2_perturbation(r));
        total = add(total, drag_acceleration(r, v));
        total = add(total, srp_acceleration(r));
        total = add(total, third_body_acceleration(r, to_vec3(config_.sun_position_m), config_.sun_mu));
        total = add(total, third_body_acceleration(r, to_vec3(config_.moon_position_m), config_.moon_mu));
        total = add(total, control_accel);

        return to_array(total);
    }

    template <typename DerivativeT>
    static OrbitalBodyState6DOF apply_delta(
        OrbitalBodyState6DOF base,
        const DerivativeT& derivative,
        const double dt
    ) {
        for (std::size_t axis = 0; axis < base.position_m.size(); ++axis) {
            base.position_m[axis] += derivative.dpos[axis] * dt;
            base.velocity_mps[axis] += derivative.dvel[axis] * dt;
            base.angular_rate_rps[axis] += derivative.domega[axis] * dt;
        }
        for (std::size_t i = 0; i < base.attitude_wxyz.size(); ++i) {
            base.attitude_wxyz[i] += derivative.dquat[i] * dt;
        }
        normalize_quaternion(base.attitude_wxyz);
        base.mission_time_s += dt;
        return base;
    }

    OrbitalDynamicsConfig6DOF config_{};
};

}  // namespace orbital::sim
