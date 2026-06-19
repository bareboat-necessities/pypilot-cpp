# Submodule pins

All submodules track `main` in `.gitmodules`; this super-repo pins exact commits when true gitlink submodules are used. Current CI bootstraps modules from `main`.

Initial pins:

```text
pypilot-event-loop           38699d19e74dee174864e105af745f3f38c55423
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
