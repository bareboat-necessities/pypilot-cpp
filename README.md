# pypilot-cpp

Umbrella repository and top-level C++ pypilot application/daemon module.

## Architecture

The common app owns one shared `pypilot-data-model` state. Input services and connectors update that state from event-loop callbacks. The control task reads that state and drives control and servo output. `pypilot-runtime` exposes the same state over the pypilot TCP protocol instead of maintaining a second runtime-value model.

## Algorithmic methods reference

[Algorithmic Methods in Open Source Marine Autopilots](https://github.com/bareboat-necessities/ocean-imu/releases/download/vTest/autopilots-methods.pdf)

## Build

```bash
bash scripts/bootstrap-modules.sh
bash scripts/build-linux-all.sh
bash scripts/run-linux-integration-tests.sh
```
