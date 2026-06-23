# Submodules

`pypilot-cpp` is an umbrella repository. The code lives in separate module repositories under the `bareboat-necessities` organization, and this repository ties those modules together for checkout, documentation, CI, and cross-module build validation.

Each module is listed in `.gitmodules` and represented under `modules/` so GitHub shows the module directory entries on the web site. CI also supports refreshing/checking out module repositories with `scripts/bootstrap-modules.sh`.

## Module roles

| Path | Umbrella role | Upstream repository | Build policy |
| --- | --- | --- | --- |
| `modules/pypilot-event-loop` | build/test | `bareboat-necessities/pypilot-event-loop` | Built and tested by Linux CI; Arduino examples are compiled where supported. |
| `modules/pypilot-settings` | build/test | `bareboat-necessities/pypilot-settings` | Built and tested by Linux CI; Arduino library is included. |
| `modules/pypilot-mdns` | build/test | `bareboat-necessities/pypilot-mdns` | Built and tested by Linux CI; ESP32 mDNS examples are compiled for ESP32-family targets. |
| `modules/pypilot-syslib` | build/test | `bareboat-necessities/pypilot-syslib` | Built and tested by Linux CI; shared by other modules. |
| `modules/pypilot-data-model` | build/test | `bareboat-necessities/pypilot-data-model` | Built and tested by Linux CI; header-only data model surface for downstream modules. |
| `modules/pypilot-servo-protocol` | build/test | `bareboat-necessities/pypilot-servo-protocol` | Built and tested by Linux CI; Arduino examples are compiled. |
| `modules/pypilot-client-protocol` | build/test | `bareboat-necessities/pypilot-client-protocol` | Built and tested by Linux CI; Arduino library is included. |
| `modules/pypilot-runtime` | build/test | `bareboat-necessities/pypilot-runtime` | Built and tested by Linux CI with settings and mDNS integration enabled. |
| `modules/pypilot-algorithms` | build/test | `bareboat-necessities/pypilot-algorithms` | Built and tested by Linux CI; pure algorithm layer for other modules. |
| `modules/pypilot-boatimu` | build/test | `bareboat-necessities/pypilot-boatimu` | Built and tested by Linux CI; backend-neutral BoatIMU/AHRS abstraction. |
| `modules/ocean-imu` | checkout-only | `bareboat-necessities/ocean-imu` | Checked out for future BoatIMU/AHRS integration; not built or tested by this umbrella CI. |
| `modules/pypilot-nmea0183-connector` | build/test | `bareboat-necessities/pypilot-nmea0183-connector` | Built and tested by Linux CI; Arduino library is included. |
| `modules/pypilot-signalk-connector` | build/test | `bareboat-necessities/pypilot-signalk-connector` | Built and tested by Linux CI with Signal K mDNS discovery support when available. |
| `modules/pypilot-sensors` | build/test | `bareboat-necessities/pypilot-sensors` | Built and tested by Linux CI; sensor arbitration and connector-adapter layer. |
| `modules/pypilot-gps-adapter` | build/test | `bareboat-necessities/pypilot-gps-adapter` | Built and tested by Linux CI; optional gpsd support is platform-specific. |
| `modules/pypilot-pilots-logic` | build/test | `bareboat-necessities/pypilot-pilots-logic` | Built and tested by Linux CI; pilot-mode and command logic. |
| `modules/pypilot-steering-signaling` | build/test | `bareboat-necessities/pypilot-steering-signaling` | Built and tested by Linux CI; steering safety and servo signaling layer. |

## Build policy

Normal `pypilot-*` modules are part of the umbrella validation flow unless a particular example is platform-gated. `ocean-imu` is intentionally different: it is available as a source/header dependency for future integration work, but this umbrella project does not build it or run its tests.

## Updating module checkouts

For day-to-day development and CI, use:

```bash
bash scripts/bootstrap-modules.sh
```

For recursive git submodule workflows, use:

```bash
git submodule update --init --recursive
```
