# pypilot-cpp

Umbrella repository for the modular C++ pypilot port.

This repository does not duplicate module code. It defines the module set, tracks each module on `main`, bootstraps module checkouts for CI, documents the build graph, and runs Linux and Arduino builds across the full dependency chain.

## Original PyPilot project

This project is a modular C++ port inspired by and based on the original PyPilot project:

- Original repository: https://github.com/pypilot/pypilot
- Original author: Sean D'Epagnier

Credit and thanks go to Sean D'Epagnier and the PyPilot contributors for the original open-source marine autopilot implementation.

## Clone

```bash
git clone https://github.com/bareboat-necessities/pypilot-cpp.git
cd pypilot-cpp
bash scripts/bootstrap-modules.sh
```

If true gitlink submodules are later committed, this also works:

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

For a future gitlink submodule tree:

```bash
git submodule update --remote --recursive
git add modules
git commit -m "Update pypilot module pins"
```

## Modules

| Module | Role |
| --- | --- |
| `pypilot-event-loop` | portable task/timer/stream scheduling with Linux libevent backend |
| `pypilot-settings` | descriptor validation and persistent settings stores, including memory, Linux file, and ESP32 NVS backends |
| `pypilot-mdns` | mDNS/DNS-SD advertisement/discovery for `_pypilot._tcp` and Signal K lookup via `_http._tcp` TXT `swname=signalk-server` |
| `pypilot-syslib` | shared system helpers and logging |
| `pypilot-data-model` | shared pypilot data model |
| `pypilot-servo-protocol` | low-level servo packet protocol |
| `pypilot-client-protocol` | pypilot client/server protocol |
| `pypilot-runtime` | typed runtime values, pypilot TCP server, periodic watches, client helper, settings bridge, and mDNS helper |
| `pypilot-algorithms` | pure math, filters, GPSFilter, WMM, pilot math |
| `pypilot-boatimu` | backend-neutral BoatIMU / AHRS / heading sample abstraction |
| `pypilot-nmea0183-connector` | NMEA 0183 parsing and formatting |
| `pypilot-signalk-connector` | Signal K conversion layer and Signal K mDNS discovery helper |
| `pypilot-sensors` | sensor ingestion, source/device arbitration, data-model writes |
| `pypilot-gps-adapter` | gpsd/raw GPS adapter and GPS-to-sensor bridge |
| `pypilot-pilots-logic` | autopilot mode selection and pilot command logic |
| `pypilot-steering-signaling` | rudder/servo runtime, steering safety, servo signaling |

## Linux build

```bash
bash scripts/bootstrap-modules.sh
bash scripts/build-linux-all.sh
```

The Linux CI installs `libevent-dev` because `pypilot-event-loop` uses libevent as the Linux backend. The Linux umbrella build also builds and tests `pypilot-settings`, `pypilot-mdns`, `pypilot-runtime` with settings and mDNS enabled, and `pypilot-signalk-connector` with Signal K mDNS discovery enabled.

## Arduino build

```bash
bash scripts/bootstrap-modules.sh
bash scripts/build-arduino-all.sh
```

The Arduino build script targets `arduino:avr:mega` by default because it is a broad AVR compile target for CI. `pypilot-settings` is included as an Arduino library. `pypilot-mdns` and runtime server examples are compiled only for ESP32-family targets because they use ESP32 networking / mDNS support.

## Dependency order

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
pypilot-nmea0183-connector
pypilot-signalk-connector
pypilot-sensors
pypilot-gps-adapter
pypilot-pilots-logic
pypilot-steering-signaling
```

`pypilot-runtime` optionally consumes `pypilot-settings` and `pypilot-mdns` when those sibling checkouts are present. `pypilot-signalk-connector` depends on `pypilot-mdns` for Signal K discovery.

## Current project status

Completed module groups include GPS adapter, WMM/GPSFilter, BoatIMU sample abstraction, sensor arbitration, APB/NAV command handling, steering signaling, servo runtime, syslib logging, event-loop Linux/libevent phase, settings persistence backends, mDNS/DNS-SD service discovery, cross-module logging integration, and the initial `pypilot-runtime` typed TCP/value layer with settings and mDNS helpers.
