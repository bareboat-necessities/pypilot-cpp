# pypilot-cpp

Umbrella repository and top-level C++ pypilot application/daemon module.

## Architecture

One shared `pypilot-data-model` state is used by the app, runtime, input services, control loop, and servo backend. Input services and connectors update this model from event-loop callbacks. The control task reads the model and drives control/servo output. `pypilot-runtime` exposes the same model through the pypilot TCP protocol; it does not maintain a second runtime-value model.

## Algorithmic methods reference

[Algorithmic Methods in Open Source Marine Autopilots](https://github.com/bareboat-necessities/ocean-imu/releases/download/vTest/autopilots-methods.pdf)

## Build

```bash
bash scripts/bootstrap-modules.sh
bash scripts/build-linux-all.sh
bash scripts/run-linux-integration-tests.sh
```
