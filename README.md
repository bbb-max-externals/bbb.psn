# bbb.psn

Max/MSP external package for receiving PosiStageNet (PSN) tracking data.

## Object

### `PosiStageNet`

Receives PSN 2.x UDP packets and outputs decoded tracker data.

Default settings follow the PSN defaults:

- Port: `56565`
- Multicast group: `236.10.10.10`

#### Messages

- `start` — open UDP socket and begin receiving.
- `stop` — stop receiving and close the socket.
- `restart` — reopen with current attributes.
- `bang` — output current status.

#### Attributes

- `@port` — UDP port.
- `@multicast` — multicast group address. Empty string disables multicast join.
- `@autostart` — start on object creation.

#### Outlets

1. Tracker data: `tracker <id> <name> <x> <y> <z> <yaw> <pitch> <roll> <status> <timestamp>`
2. Info/status: `server <name>`, `name <id> <name>`, `status <0|1>`, `error <message>`

## Build

```bash
git submodule update --init --recursive
cmake -B build
cmake --build build --config Release
```

Outputs:

- macOS: `externals/PosiStageNet.mxo` (universal x86_64 + arm64)
- Windows: `externals/PosiStageNet.mxe64` (x64)

## Sources

PSN decoding uses the official VYV header-only C++ implementation in `deps/psn-cpp`.
