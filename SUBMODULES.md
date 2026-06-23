# Submodule pins

All module entries track `main` in `.gitmodules`. This super-repo can pin exact commits when true gitlink submodules are used, but current CI bootstraps fresh module checkouts from `main` with `scripts/bootstrap-modules.sh`.

## Current checkout set

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

`ocean-imu` is checkout-only for future BoatIMU/AHRS integration. It is cloned and verified as a git checkout, but it is not built by `scripts/build-linux-all.sh`, not compiled by `scripts/build-arduino-all.sh`, and its tests are not run by this umbrella repository.

## Historical initial pins

These are the initial pins recorded for the original build/test module set:

```text
pypilot-event-loop           d3eba4a8dccd10721de3d9682b1d4a022592e2c4
pypilot-syslib               fbd2d2d4c358009232d0ac586df2eb9c60c31630
pypilot-data-model           7cde4cea09a9471eccdefa5bcc733fe0831f448c
pypilot-servo-protocol       ba5d5ddc938a06ef1d226e575cea6e057ef1a297
pypilot-client-protocol      af3c23f9cbe4144e21b84ace83a98ba522ad91a0
pypilot-algorithms           0941a2fb6cb121b018a0081e9c864edfe78675d6
pypilot-boatimu              4db929c1726aa2af0318e0bdd92b2c6a3fcb3435
pypilot-nmea0183-connector   699b58031a5496678114417e28dc02cd19d48c66
pypilot-signalk-connector    185d50b892e5a71a746c1ac4b1c2ff5cd5b552c3
pypilot-sensors              c32892c8aa9ce85d02d4bd54ec59f9cfb46c93d4
pypilot-gps-adapter          072caaddd967e579d6b0fdee7bc0611905d5caee
pypilot-pilots-logic         983a07de94f484e31784fe164b1c27842f4330c6
pypilot-steering-signaling   33341e4ed8e2ab7d6cc044d2cc214c0a7dd1c322
```

## Later added checkout entries

These entries were added after the initial pin list and are also tracked on `main` by `.gitmodules` and bootstrap scripts. The commit values below are observed `main` heads at the time this document was updated; CI still refreshes them from `main`.

```text
pypilot-settings             0049f028cc65590a797992c0a4faad9986e20469
pypilot-mdns                 fbf84161cd456fbbb58392e93c7e1baf09df2fef
pypilot-runtime              2d58dd99a0f37e6a8c4a58cb4cde676bbb5b5fde
ocean-imu                    a5814ce94521f6037d1eda5780f2096e432b50a5
```
