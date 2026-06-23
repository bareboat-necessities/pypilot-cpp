#include "pypilot_app.hpp"

namespace pypilot {

PypilotApp::PypilotApp()
    : loop_(),
      autopilot_values_(),
      boatimu_values_(),
      sensor_values_(),
      servo_values_(),
      pilot_values_(),
      gps_values_(),
      wind_values_(),
      runtime_state_{autopilot_values_, boatimu_values_, sensor_values_, servo_values_, pilot_values_, gps_values_, wind_values_},
      runtime_protocol_(runtime_state_),
      runtime_server_(loop_, runtime_protocol_),
      imu_backend_(0),
      servo_backend_(0),
      config_(),
      status_(),
      control_tick_handle_() {}

bool PypilotApp::begin(IBoatImuBackend* imu_backend, IServoBackend* servo_backend, const PypilotAppConfig& config) {
    imu_backend_ = imu_backend;
    servo_backend_ = servo_backend;
    config_ = config;
    status_ = PypilotAppStatus();

    if (!loop_.valid()) {
        set_fault("event loop backend is not valid");
        return false;
    }
    if (config_.control_period_us == 0) {
        set_fault("control loop period is zero");
        return false;
    }

    runtime_state_.sensors.server_version.set("pypilot-cpp");
    runtime_state_.servo.state.set(servo_backend_ ? "idle" : "no_servo_backend");
    runtime_state_.servo.engaged.set(false);

    if (config_.enable_runtime_tcp && !start_runtime_server()) {
        return false;
    }

    control_tick_handle_ = loop_.on_repeat_us(config_.control_period_us, [this]() { control_tick(); });
    if (!control_tick_handle_.assigned()) {
        set_fault("failed to register control loop task");
        return false;
    }

    status_.state = PypilotAppState::running;
    publish_runtime();
    return true;
}

void PypilotApp::tick() {
    loop_.tick();
}

void PypilotApp::run_forever() {
    loop_.run_forever();
}

void PypilotApp::request_exit() {
    loop_.request_exit();
}

void PypilotApp::stop() {
    if (servo_backend_) {
        servo_backend_->disable(runtime_state_, loop_.clock().micros());
    }
    runtime_state_.autopilot.enabled.set(false);
    runtime_state_.servo.engaged.set(false);
    runtime_state_.servo.state.set("stopped");
    runtime_server_.close();
    if (control_tick_handle_.assigned()) {
        loop_.remove(control_tick_handle_);
        control_tick_handle_ = pypilot_event_loop::EventHandle{};
    }
    status_.state = PypilotAppState::stopped;
    publish_runtime();
}

bool PypilotApp::start_runtime_server() {
    pypilot_runtime::PypilotRuntimeServerOptions options;
    options.max_output_bytes = config_.max_runtime_output_bytes;
    options.udp_watch_port = config_.runtime_udp_watch_port;
    if (!runtime_server_.configure(options)) {
        set_fault("failed to configure runtime server");
        return false;
    }
    if (!runtime_server_.listen(config_.runtime_host, config_.runtime_port)) {
        set_fault("failed to listen on runtime TCP port");
        return false;
    }
    status_.runtime_listening = true;
    status_.runtime_port = runtime_server_.port();
    return true;
}

void PypilotApp::control_tick() {
    const uint64_t now_us = loop_.clock().micros();
    status_.last_control_tick_us = now_us;
    ++status_.control_ticks;

    runtime_state_.sensors.server_uptime.set(static_cast<double>(now_us) / 1000000.0);

    if (imu_backend_) {
        if (!imu_backend_->poll(runtime_state_, now_us)) {
            runtime_state_.status_warnings.set(runtime_state_.sensors.status_warnings.get() + 1.0);
        }
    }

    const bool enabled = runtime_state_.autopilot.enabled.get();
    if (!enabled) {
        runtime_state_.servo.engaged.set(false);
        if (servo_backend_) {
            servo_backend_->disable(runtime_state_, now_us);
            runtime_state_.servo.state.set("disabled");
        } else {
            runtime_state_.servo.state.set("no_servo_backend");
        }
        publish_runtime();
        return;
    }

    if (!imu_backend_) {
        runtime_state_.servo.engaged.set(false);
        runtime_state_.servo.state.set("no_imu_backend");
        runtime_state_.sensors.status_faults.set(runtime_state_.sensors.status_faults.get() + 1.0);
        publish_runtime();
        return;
    }

    if (!servo_backend_) {
        runtime_state_.servo.engaged.set(false);
        runtime_state_.servo.state.set("no_servo_backend");
        runtime_state_.sensors.status_faults.set(runtime_state_.sensors.status_faults.get() + 1.0);
        publish_runtime();
        return;
    }

    runtime_state_.servo.engaged.set(true);
    if (!servo_backend_->apply(runtime_state_, now_us)) {
        runtime_state_.servo.engaged.set(false);
        runtime_state_.servo.state.set("servo_rejected_command");
        runtime_state_.sensors.status_faults.set(runtime_state_.sensors.status_faults.get() + 1.0);
    } else {
        runtime_state_.servo.state.set("active");
    }
    publish_runtime();
}

void PypilotApp::publish_runtime() {
    runtime_state_.sensors.runtime_published_value_count.set(
        runtime_state_.sensors.runtime_published_value_count.get() + 1.0);
    runtime_server_.publish_changed_values();
}

void PypilotApp::set_fault(const char* message) {
    status_.state = PypilotAppState::fault;
    status_.fault = message ? message : "fault";
    runtime_state_.sensors.status_faults.set(runtime_state_.sensors.status_faults.get() + 1.0);
    runtime_state_.servo.engaged.set(false);
    runtime_state_.servo.state.set(status_.fault);
}

} // namespace pypilot
