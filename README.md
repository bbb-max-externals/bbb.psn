# bbb.psn

Max/MSP external package for sending and receiving PosiStageNet (PSN) tracking data.

## Object

### `bbb.psn.receiver`

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

### `bbb.psn.sender`

Sends PSN 2.x UDP packets.

Default settings follow the PSN defaults:

- Destination: `236.10.10.10`
- Port: `56565`
- System name: `Max`

#### Messages

- `tracker <id> <x> <y> <z> [yaw pitch roll]` — set position and optional orientation.
- `pos <id> <x> <y> <z>` — set position.
- `ori <id> <yaw> <pitch> <roll>` — set orientation.
- `speed <id> <x> <y> <z>` — set speed.
- `accel <id> <x> <y> <z>` — set acceleration.
- `target <id> <x> <y> <z>` — set target position.
- `status <id> <value>` — set tracker status.
- `name <id> <symbol>` — set tracker name for info packets.
- `clear [id]` — clear one tracker, or all trackers with no args.
- `send` / `bang` — send one data frame.
- `info` — send one info frame.

#### Attributes

- `@destination` — IPv4 destination address.
- `@port` — UDP destination port.
- `@system` — PSN system/server name for info packets.

#### Outlet

1. Info/status: `sent <data|info> <packet-count>`, `error <message>`


## Latest CI build

GitHub Actions creates a downloadable package on every `main` push, pull request, release, and manual workflow run.

- For normal downloads, open **Releases** and download `<repository-name>-latest-build.zip` from the `latest` release.
- For per-run artifacts, open **Actions** → **Build & Package** → latest successful run, then download `<repository-name>-latest-build-zip`.
- The zip contains both macOS `.mxo` and Windows `.mxe64` externals plus help and package metadata.

## Build

```bash
git submodule update --init --recursive
cmake -B build
cmake --build build --config Release
```

Outputs:

- macOS: `externals/*.mxo` (universal x86_64 + arm64)
- Windows: `externals/*.mxe64` (x64)

## Sources

PSN encoding/decoding uses the official VYV header-only C++ implementation in `deps/psn-cpp`.
