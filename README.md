# bbb.psn

> [!WARNING]
> This repository is published as AI-assisted, insufficiently tested work in progress ("AI slop"). Treat it as experimental. Correctness, stability, compatibility, and fitness for production use are not guaranteed.

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
- `@multicast` — multicast group address. Use `none`, `off`, `false`, `0`, or `unicast` to disable multicast join for unicast receiving.
- `@localaddr` — local IPv4 address used when joining a multicast group. Use `any` or `0.0.0.0` for OS default.
- `@autostart` — start on object creation.


#### Shared receiver instances

`bbb.psn.receiver` uses one shared UDP socket/thread/decoder per `@port + @multicast + @localaddr` tuple. Multiple Max objects with the same tuple subscribe to the same internal receiver instead of binding duplicate sockets.

This is intentional: duplicate UDP binds are OS-dependent and unreliable, especially for unicast. If you need the same PSN stream in multiple places, create multiple `bbb.psn.receiver` objects with the same attributes or split one receiver output in the patch; both avoid duplicate socket ownership.

The `bang` message reports `status` and current `subscribers` count.

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

- `@destination` — IPv4 destination address. Use `236.10.10.10` for standard PSN multicast.
- `@port` — UDP destination port.
- `@localaddr` — local IPv4 address used as the multicast output interface. Use `any` or `0.0.0.0` for OS routing.
- `@system` — PSN system/server name for info packets.

#### Outlet

1. Info/status: `sent <data|info> <packet-count>`, `error <message>`




## Multicast to grandMA3

For standard PSN multicast, send to `236.10.10.10:56565` and explicitly select the NIC that is on the grandMA3 network. On this machine, if the grandMA3 network address is `192.168.0.222`, use:

```max
bbb.psn.sender @destination 236.10.10.10 @port 56565 @localaddr 192.168.0.222 @system MaxPSN
```

Then send at least one tracker definition and continuous data:

```max
name 1 performer
info
tracker 1 1. 2. 3. 0. 0. 0.
```

Drive `bang`/`send` continuously with a `metro`, for example 30 Hz. A single packet is a weak test for PSN receivers.

grandMA3 onPC must also listen on the NIC connected to this network and allow UDP `56565` through Windows Firewall. Wireshark seeing packets does not prove grandMA3 onPC is allowed to receive them.

For local multicast receive tests on this same machine, use matching receiver interface selection:

```max
bbb.psn.receiver @multicast 236.10.10.10 @localaddr 192.168.0.222 @port 56565
```

## Local loopback test

Use `patchers/bbb.psn.loopback-test.maxpat` to test `bbb.psn.sender` and `bbb.psn.receiver` on one machine.

Basic sequence:

1. Build the externals and make sure Max can find this package folder.
2. Open `patchers/bbb.psn.loopback-test.maxpat`. It uses unicast loopback: sender `@destination 127.0.0.1`, receiver `@multicast none`.
3. Click `start` on `bbb.psn.receiver`. You should see `status 1` in `print psn-rx-info`.
4. Click `name 1 performer`, then `info`. You should see `server LocalMax` and `name 1 performer`.
5. Click `tracker 1 1. 2. 3. 10. 20. 30.`, then click the bang connected to sender. You should see a `tracker ...` message in `print psn-rx-data`.

Important: `bbb.psn.sender` does not send any data packet until at least one tracker exists. If you only bang a fresh sender, it reports `sent data 0` and the receiver correctly outputs nothing.

If Wireshark shows UDP packets but Max prints nothing, check these in order:

- Receiver is actually started and prints `status 1`.
- Sender and receiver use the same port.
- You sent `tracker ...` before `bang`/`send`.
- The packet is PSN, not merely arbitrary UDP to the port.
- For unicast tests, set sender `@destination` to `127.0.0.1` or your local interface IP, and set receiver `@multicast none`.
- For multicast tests, keep sender destination `236.10.10.10` and receiver multicast `236.10.10.10`; some systems or network setups may suppress multicast loopback.

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
