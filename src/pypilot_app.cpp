#include "pypilot_app.hpp"

namespace pypilot {

PypilotApp::PypilotApp()
    : imu_(0),
      servo_(0),
      config_(),
      status_(),
      next_tick_us_(0),
      servo_command_(0.0f) {}

bool PypilotApp::begin(IBoatImuBackend& imu, IServoBackend& servo, const PypilotAppConfig& config) {
    imu_ = &imu;
    servo_ = &servo;
    config_ = config;
    status_ = PypilotAppStatus();
    servo_command_ = 0.0f;
    next_tick_us_ = 0;

    if (config_.loop_period_us == 0) {
        set_fault("loop period is zero", 0);
        return false;
    }
    if (config_.max_abs_command < 0.0f) {
        set_fault("max command is negative", 0);
        return false;
    }

    status_.state = PypilotAppState::disabled;
    servo_->disable(0);
    status_.servo_seen = true;
    return true;
}

void PypilotApp::stop(uint64_t now_us) {
    if (servo_) {
        servo_->disable(now_us);
        status_.servo_seen = true;
    }
    status_.state = PypilotAppState::stopped;
    status_.last_tick_us = now_us;
}

bool PypilotApp::set_enabled(bool enabled, uint64_t now_us) {
    if (!imu_ || !servo_) {
        set_fault("app backends are not configured", now_us);
        return false;
    }
    if (status_.state == PypilotAppState::fault && enabled) {
        return false;
    }
    status_.state = enabled ? PypilotAppState::enabled : PypilotAppState::disabled;
    if (!enabled) {
        servo_->disable(now_us);
        status_.servo_seen = true;
    }
    return true;
}

void PypilotApp::set_servo_command(float normalized_command) {
    servo_command_ = clamp_command(normalized_command);
}

void PypilotApp::tick(uint64_t now_us) {
    if (!imu_ || !servo_) {
        set_fault("app backends are not configured", now_us);
        return;
    }
    if (status_.state == PypilotAppState::stopped || status_.state == PypilotAppState::fault) {
        return;
    }
    if (next_tick_us != 0 && now_us < next_tick_us_) {
        return;
    }

    next_tick_us_ = now_us + config_.loop_period_us;
    status_.last_tick_us = now_us;
    ++status_.ticks;

    BoatImuSample imu_sample;
    const bool imu_ok = imu_->read_sample(imu_sample) && imu_sample.valid;
    status_.imu_seen = status_.imu_seen || imu_ok;

    if (status_.state != PypilotAppState::enabled) {
        servo_->disable(now_us);
        status_.servo_seen = true;
        return;
    }

    if (!imu_ok) {
        servo_->disable(now_us);
        status_.servo_seen = true;
        set_fault("imu sample unavailable", now_us);
        return;
    }

    if (!servo_->write_normalized(servo_command_, now_us)) {
        set_fault("servo command rejected", now_us);
        return;
    }
    status_.servo_seen = true;
}

float PypilotApp::clamp_command(float command) const {
    const float limit = config_.max_abs_command;
    if (command > limit) return limit;
    if (command < -limit) return -limit;
    return command;
}

void PypilotApp::set_fault(const char* message, uint64_t now_us) {
    if (servo_) {
        servo_->disable(now_us);
        status_.servo_seen = true;
    }
    status_.state = PypilotAppState::fault;
    status_.fault = message ? message : "fault";
    status_.last_tick_us = now_us;
}

} // namespace pypilot
