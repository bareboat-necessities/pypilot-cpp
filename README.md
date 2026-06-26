# pypilot-cpp

Umbrella repository and top-level C++ pypilot application/daemon module.

This repository owns the common application composition root in `src/`, a short Linux daemon entrypoint in `linux/`, and short MCU-specific Arduino sketches under `mcu/`. The common app composes a `pypilot-event-loop` event loop with the `pypilot-runtime` service facade and a shared `pypilot-data-model` state. Runtime values, TCP handling, watches, settings, and value publication remain the responsibility of `pypilot-runtime`. Connector/input services register event-loop callbacks and update the shared data model from their own protocol handlers; the periodic app control task reads that latest model state and drives control/servo output.

The reusable library code still lives in the separate `modules/` repositories and is pulled together here for checkout, documentation, CI, and cross-module build validation.

Some repositories are **build/test modules** in this umbrella project. Other repositories are **checkout-only modules** used as source/header dependencies for later integration work and are intentionally not built or tested by this umbrella CI.

## Original PyPilot project

This project is a modular C++ port inspired by and based on the original PyPilot project:

- Original repository: https://github.com/pypilot/pypilot
- Original author: Sean D'Epagnier

Credit and thanks go to Sean D'Epagnier and the PyPilot contributors for the original open-source marine autopilot implementation.

## Algorithmic methods reference

For background on the control and estimation methods considered in this port, see [Algorithmic Methods in Open Source Marine Autopilots](https://github.com/bareboat-necessities/ocean-imu/releases/download/vTest/autopilots-methods.pdf).

## Repository layout

```text
src/                 common pypilot application/daemon source
linux/               short Linux-specific entrypoints, including pypilotd.cpp
mcu/                 short MCU-specific Arduino sketches
mcu/atomS3R/         AtomS3R sketch entrypoint
modules/             git submodule checkouts for reusable pypilot libraries
scripts/             bootstrap, verification, and CI build scripts
```

The common app core uses `pypilot-event-loop` for scheduling and platform polling, stores latest state in `pypilot-data-model`, lets input services update that state from event-driven connector callbacks, and delegates pypilot-compatible TCP values, watches, settings, and value publication to `pypilot-runtime`. Platform-specific BoatIMU and servo backends must still be wired before closed-loop control is enabled.

## Clone

```bash
git clone https://github.com/bareboat-necessities/pypilot-cpp.git
cd pypilot-cpp
bash scripts/bootstrap-modules.sh
```

Recursive submodule checkout also works:

```bash
git clone --recurse-submodules https://github.com/bareboat-necessities/pypilot-cpp.git
cd pypilot-cpp
git submodule update --init --recursive
```

## Updating modules to latest main

Each module tracks `main`. CI bootstraps fresh module checkouts with `scripts/bootstrap-modules.sh`.

```bash
bash scripts/bootstrap-modules.sh
```

For a git submodule tree:

```bash
git submodule update --remote --recursive
git add modules
git commit -m "Update pypilot modules"
```

## Effective checkout set

`pypilot-cpp` currently bootstraps these repositories under `modules/`:

```text
pypilot-event-loop
pypilot-settings
pypilot-mdns
pypilot-syslib
pypilot-data-model
pypilot-servo-protocol
pypilot-client-protocol
pypilot-runtime
pypilot-algorithms
pypilot-boatimu
ocean-imu
pypilot-nmea0183-connector
pypilot-signalk-connector
pypilot-sensors
pypilot-gps-adapter
pypilot-pilots-logic
pypilot-steering-signaling
```

All `pypilot-*` entries above are normal umbrella build/test modules unless a build script explicitly gates an example by platform. `ocean-imu` is checkout-only: it is cloned and verified as a git checkout, but it is not passed to CMake, CTest, or `arduino-cli` by this umbrella repository.

## Modules

| Module | Umbrella role | Description |
| --- | --- | --- |
| `pypilot-event-loop` | build/test | portable task/timer/stream scheduling with Linux libevent backend |
| `pypilot-settings` | build/test | descriptor validation and persistent settings stores, including memory, Linux file, and ESP32 NVS backends |
| `pypilot-mdns` | build/test | mDNS/DNS-SD advertisement/discovery for `_pypilot._tcp` and Signal K lookup via `_http._tcp` TXT `swname=signalk-server` |
| `pypilot-syslib` | build/test | shared system helpers and logging |
| `pypilot-data-model` | build/test | shared pypilot data model |
| `pypilot-servo-protocol` | build/test | low-level servo packet protocol |
| `pypilot-client-protocol` | build/test | pypilot client/server protocol |
| `pypilot-runtime` | build/test | typed runtime values, pypilot TCP server, periodic watches, client helper, settings bridge, and mDNS helper |
| `pypilot-algorithms` | build/test | pure math, filters, GPSFilter, WMM, pilot math |
| `pypilot-boatimu` | build/test | backend-neutral BoatIMU / AHRS / heading sample abstraction |
| `ocean-imu` | checkout-only | header/source library for marine IMU/AHRS/INS algorithms; not built or tested by this umbrella CI |
| `pypilot-nmea0183-connector` | build/test | NMEA 0183 parsing and formatting |
| `pypilot-signalk-connector` | build/test | Signal K conversion layer and Signal K mDNS discovery helper |
| `pypilot-sensors` | build/test | sensor ingestion, source/device arbitration, data-model writes |
| `pypilot-gps-adapter` | build/test | gpsd/raw GPS adapter and GPS-to-sensor bridge |
| `pypilot-pilots-logic` | build/test | autopilot mode selection and pilot command logic |
| `pypilot-steering-signaling` | build/test | rudder/servo runtime, steering safety, servo signaling |

## Linux build

```bash
bash scripts/bootstrap-modules.sh
bash scripts/build-linux-all.sh
```

The Linux CI installs `libevent-dev` because `pypilot-event-loop` uses libevent as the Linux backend. The Linux umbrella build also builds the root `pypilot-cpp` app core and `linux/pypilotd.cpp`, then builds and tests the module set. `pypilot-settings`, `pypilot-mdns`, and `pypilot-runtime` are built with settings and mDNS enabled, and `pypilot-signalk-connector` is built with Signal K mDNS discovery enabled.

`ocean-imu` is bootstrapped and checked out for future BoatIMU/AHRS integration, but it is intentionally excluded from the umbrella Linux build and test script. Build or test `ocean-imu` from its own repository when needed.

## Arduino build

```bash
bash scripts/bootstrap-modules.sh
bash scripts/build-arduino-all.sh
```

The Arduino build script targets `arduino:avr:mega` by default because it is a broad AVR compile target for CI. `pypilot-settings` is included as an Arduino library. `pypilot-mdns`, runtime server examples, and `mcu/atomS3R/pypilot.ino` are compiled only for ESP32-family targets because they use ESP32 networking / platform support.

`ocean-imu` is not passed to `arduino-cli` by the umbrella Arduino build. ESP32-S3 sketches that use `ocean-imu` should include it explicitly when those integrations are added.

## Dependency tree

This is a deduplicated dependency-layer tree. Each module appears exactly once; shared lower-level dependencies are not repeated under every consumer.

```text
pypilot-cpp
├── application/runtime layer
│   ├── pypilot-runtime
│   ├── pypilot-sensors
│   ├── pypilot-gps-adapter
│   ├── pypilot-pilots-logic
│   └── pypilot-steering-signaling
├── sensor, navigation, and IMU layer
│   ├── pypilot-boatimu
│   └── ocean-imu
├── protocol and connector layer
│   ├── pypilot-client-protocol
│   ├── pypilot-servo-protocol
│   ├── pypilot-nmea0183-connector
│   └── pypilot-signalk-connector
└── platform, data, and support layer
    ├── pypilot-event-loop
    ├── pypilot-settings
    ├── pypilot-mdns
    ├── pypilot-syslib
    ├── pypilot-data-model
    └── pypilot-algorithms
```

Important cross-dependencies are intentionally described here instead of duplicating nodes in the tree: `pypilot-runtime` uses the event loop, data model, settings, mDNS, NMEA 0183 connector, and Signal K connector; `pypilot-sensors` uses the data model, algorithms, syslib, servo protocol, NMEA 0183 connector, and Signal K connector; `pypilot-gps-adapter` feeds the sensors/data-model path; `pypilot-pilots-logic` uses the data model and algorithms; `pypilot-steering-signaling` uses the data model, servo protocol, and syslib; `pypilot-boatimu` uses the data model, algorithms, and `ocean-imu`.

`pypilot-runtime` optionally consumes `pypilot-settings` and `pypilot-mdns` when those sibling checkouts are present. `pypilot-signalk-connector` depends on `pypilot-mdns` for Signal K discovery. `ocean-imu` is a checkout-only dependency for future BoatIMU/AHRS integration and is not part of the current umbrella build graph.

## Current project status

Completed module groups include GPS adapter, WMM/GPSFilter, BoatIMU sample abstraction, sensor arbitration, APB/NAV command handling, steering signaling, servo runtime, syslib logging, event-loop Linux/libevent phase, settings persistence backends, mDNS/DNS-SD service discovery, cross-module logging integration, the initial `pypilot-runtime` typed TCP/value layer with settings and mDNS helpers, checkout-only `ocean-imu` availability for future marine IMU/AHRS integration, and the root `pypilot-cpp` composition root for Linux and MCU entrypoints.
