#pragma once

#include <stdint.h>

#include <pypilot_servo_protocol.hpp>
#include <pypilot_steering_signaling.hpp>

#include "pypilot_app.hpp"

namespace pypilot {

class PypilotServoBackend final : public IServoBackend {
public:
    explicit PypilotServoBackend(pypilot_servo_protocol::Transport& transport)
        : transport_(transport),
          host_(transport_),
          telemetry_(),
          servo_runtime_(),
          packet_count_(0) {}

    PypilotServoBackend(pypilot_servo_protocol::Transport& transport,
                        const pypilot_steering_signaling::ServoRuntimeConfig& config)
        : transport_(transport),
          host_(transport_),
          telemetry_(),
          servo_runtime_(config),
          packet_count_(0) {}

    pypilot_steering_signaling::ServoRuntime& runtime() { return servo_runtime_; }
    const pypilot_steering_signaling::ServoRuntime& runtime() const { return servo_runtime_; }

    pypilot_servo_protocol::Host& host() { return host_; }
    const pypilot_servo_protocol::Telemetry& telemetry() const { return telemetry_; }

    void reset_protocol() {
        telemetry_.reset();
        servo_runtime_.reset();
        packet_count_ = 0;
        host_.force_unsync();
        host_.reset_faults();
    }

    void disable(PypilotDataModel& model, uint64_t now_us) override {
        poll_telemetry(model, now_us);
        host_.neutral();
        host_.disengage();
        model.servo.engaged.value = false;
        model.servo_telemetry.controller_state.value = ship_data_model::ServoControllerState::idle;
    }

    bool apply(PypilotDataModel& model, uint64_t now_us) override {
        poll_telemetry(model, now_us);

        const pypilot_steering_signaling::ServoCommand command = command_from_model(model, now_us);
        const pypilot_steering_signaling::RudderAngle rudder = rudder_from_model(model, now_us);
        const pypilot_steering_signaling::ServoRuntimeOutput output =
            servo_runtime_.update_command(command, rudder, now_us);

        if (output.emit) {
            if (!transport_.write_bytes(output.protocol.raw_packet.bytes, sizeof(output.protocol.raw_packet.bytes))) {
                model.servo.engaged.value = false;
                model.servo_telemetry.controller_state.value = ship_data_model::ServoControllerState::disconnected;
                return false;
            }
            model.servo_telemetry.command_norm.set(command.value, now_us);
            model.servo_telemetry.raw_command_norm.set(output.protocol.allowed ? command.value : 0.0f, now_us);
        }

        model.servo.engaged.value = true;
        if (model.servo_telemetry.controller_state.value == ship_data_model::ServoControllerState::disconnected ||
            model.servo_telemetry.controller_state.value == ship_data_model::ServoControllerState::idle) {
            model.servo_telemetry.controller_state.value = ship_data_model::ServoControllerState::engaged;
        }
        return true;
    }

    void poll_telemetry(PypilotDataModel& model, uint64_t now_us) {
        bool any = false;
        for (unsigned i = 0; i < 32; ++i) {
            if (!host_.poll(telemetry_)) break;
            any = true;
            ++packet_count_;
        }
        if (!any) return;

        model.servo.has_controller = true;
        model.servo_telemetry.packet_count.set(packet_count_, now_us);
        servo_runtime_.update_feedback(telemetry_, now_us);

        if (telemetry_.has_current) {
            model.servo.current_a.set(telemetry_.current_a, now_us);
            model.servo_telemetry.current_a.set(telemetry_.current_a, now_us);
        }
        if (telemetry_.has_voltage) {
            model.servo.voltage_v.set(telemetry_.voltage_v, now_us);
            model.servo_telemetry.voltage_v.set(telemetry_.voltage_v, now_us);
        }
        if (telemetry_.has_controller_temp) {
            model.servo.controller_temp_c.set(telemetry_.controller_temp_c, now_us);
            model.servo_telemetry.controller_temp_c.set(telemetry_.controller_temp_c, now_us);
        }
        if (telemetry_.has_motor_temp) {
            model.servo.motor_temp_c.set(telemetry_.motor_temp_c, now_us);
            model.servo_telemetry.motor_temp_c.set(telemetry_.motor_temp_c, now_us);
        }
        if (telemetry_.has_flags) {
            model.servo.flags.value = telemetry_.flags;
            model.servo_telemetry.flags.set(telemetry_.flags, now_us);
            model.servo.engaged.value = telemetry_.engaged();
            model.servo_telemetry.controller_state.value = telemetry_.faulted()
                ? ship_data_model::ServoControllerState::fault
                : telemetry_.engaged() ? ship_data_model::ServoControllerState::engaged
                                       : ship_data_model::ServoControllerState::idle;
        }
        if (telemetry_.has_rudder && telemetry_.rudder_valid) {
            const float range = model.rudder.range_deg.value != 0.0f ? model.rudder.range_deg.value : 45.0f;
            const float angle = telemetry_.rudder * 2.0f * range;
            model.rudder.angle_deg.set(angle, now_us);
            model.servo.rudder_position_deg.set(angle, now_us);
            model.servo_telemetry.position_deg.set(angle, now_us);
            model.servo_telemetry.rudder_feedback_deg.set(angle, now_us);
        }
        if (telemetry_.has_flags && telemetry_.faulted()) {
            const uint32_t errors = model.servo_telemetry.error_count.valid
                ? model.servo_telemetry.error_count.value
                : 0u;
            model.servo_telemetry.error_count.set(errors + 1u, now_us);
        }
    }

private:
    static constexpr uint64_t ManualOverrideUs = 1000000ULL;
    static constexpr uint64_t PilotOutputTimeoutUs = 1000000ULL;

    pypilot_steering_signaling::ServoCommand command_from_model(const PypilotDataModel& model, uint64_t now_us) const {
        if (model.servo.command_norm.valid &&
            now_us >= model.servo.command_norm.last_external_set_us &&
            now_us - model.servo.command_norm.last_external_set_us <= ManualOverrideUs) {
            return pypilot_steering_signaling::ServoCommand(
                pypilot_steering_signaling::ServoCommandMode::Speed,
                model.servo.command_norm.value,
                now_us,
                true);
        }
        if (model.pilot_output.command_norm.valid && !model.pilot_output.command_norm.stale(now_us, PilotOutputTimeoutUs)) {
            return pypilot_steering_signaling::ServoCommand(
                pypilot_steering_signaling::ServoCommandMode::Speed,
                model.pilot_output.command_norm.value,
                now_us,
                true);
        }
        return pypilot_steering_signaling::servo_stop_command(now_us);
    }

    pypilot_steering_signaling::RudderAngle rudder_from_model(const PypilotDataModel& model, uint64_t now_us) const {
        if (model.rudder.angle_deg.valid) {
            return pypilot_steering_signaling::RudderAngle(model.rudder.angle_deg.value, model.rudder.angle_deg.last_update_us, true);
        }
        if (model.servo.rudder_position_deg.valid) {
            return pypilot_steering_signaling::RudderAngle(model.servo.rudder_position_deg.value, model.servo.rudder_position_deg.last_update_us, true);
        }
        return pypilot_steering_signaling::RudderAngle(0.0f, now_us, false);
    }

    pypilot_servo_protocol::Transport& transport_;
    pypilot_servo_protocol::Host host_;
    pypilot_servo_protocol::Telemetry telemetry_;
    pypilot_steering_signaling::ServoRuntime servo_runtime_;
    uint32_t packet_count_;
};

} // namespace pypilot
