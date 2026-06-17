# Sound Transfer

Real-time, full-duplex voice over the network. Captures the microphone, encodes
it with [Opus](https://opus-codec.org/), streams it over UDP to a peer PC, which
decodes and plays it through the speakers — and vice versa. Built with Qt5
(Multimedia + Network + Widgets).

## Architecture

```
 mic ─▶ AudioCapture ─▶ Encoder(Opus) ─▶ UdpTransport ═══UDP═══▶ peer
peer ═══UDP═══▶ UdpTransport ─▶ Decoder(Opus) ─▶ AudioPlayback ─▶ speaker
```

- **`SoundCore`** static library (`src/core/`) — device-independent, fully testable:
  - `OpusCodec` — RAII Opus encoder/decoder (48 kHz, mono, VOIP, in-band FEC).
  - `UdpTransport` — `QUdpSocket` with a 4-byte sequence header per datagram.
  - `AudioCapture` / `AudioPlayback` — Qt Multimedia device I/O in 20 ms frames.
  - `AudioEngine` — wires capture→encode→send and recv→decode→playback.
- **`soundTransfer`** GUI (`src/app/`) — device pickers, peer IP/port, level meters.

Fixed audio parameters: **48 kHz, mono, 16-bit PCM, 20 ms (960-sample) frames.**

## Prerequisites

- CMake ≥ 3.16
- A C/C++17 compiler (MSVC, MinGW, GCC, or Clang)
- **Qt5** installed system-wide (Core, Network, Multimedia, Widgets, Test)
- Internet access on first configure (libopus is fetched via `FetchContent`)

## Build (CMake presets — recommended)

A `CMakePresets.json` is provided with an **`msvc`** preset that points CMake at the
Qt 5.15.2 MSVC kit (`C:/Qt/5.15.2/msvc2019_64`) and builds with Visual Studio 2022
(x64). It also disables the machine-wide vcpkg MSBuild integration for this project
(which otherwise breaks the build) and puts Qt's `bin` on `PATH` so the app and tests
find their DLLs.

```powershell
cmake --preset msvc          # configure
cmake --build --preset msvc  # build (Release)
ctest --preset msvc          # run tests
```

In **VS Code**: install the *CMake Tools* extension, open the folder, and pick the
**"Qt 5.15 MSVC 2019 64-bit"** configure preset when prompted — then use the
Build / Run Tests buttons in the status bar. (`.vscode/settings.json` sets
`cmake.useCMakePresets`, so it won't ask you to choose a compiler kit.)

If your Qt is installed elsewhere, edit the paths in `CMakePresets.json`.

### Manual build (without presets)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/msvc2019_64" `
  -DCMAKE_VS_GLOBALS="VcpkgEnabled=false;VcpkgApplocalDeps=false"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Three suites run without any audio hardware:

- `tst_opuscodec` — Opus encode/decode round-trip (energy + correlation, silence, PLC).
- `tst_udptransport` — UDP loopback delivers payloads byte-identical and in order.
- `tst_transmission` — full pipeline (encode → UDP loopback → decode) over 1 s of
  audio, plus dropped-packet recovery via packet-loss concealment.

## Run

### Single-PC loopback smoke test
Launch the app, set **Local port** `5000`, **Peer IP** `127.0.0.1`, **Peer port**
`5000`, press **Start**. Speaking into the mic should come back out the speakers
(use headphones to avoid feedback).

### Two-PC voice
On each PC, enter the *other* PC's LAN IP as **Peer IP** and use the same UDP port
on both ends, then press **Start** on both:

| Setting     | PC-A            | PC-B            |
|-------------|-----------------|-----------------|
| Local port  | 5000            | 5000            |
| Peer IP     | `<PC-B IP>`     | `<PC-A IP>`     |
| Peer port   | 5000            | 5000            |

Allow the chosen UDP port through both firewalls (on Windows the first run usually
prompts for network access — allow it for private networks).

## Modes

Pick a **Mode** in the GUI before pressing Start:

- **Opus (compressed)** — default. ~24 kbps audio, ~35 kbps on the wire per direction.
- **Raw PCM (uncompressed)** — no codec; sends 48 kHz/16-bit mono frames directly.
  Lossless and zero codec latency, but ~768 kbps audio (~790 kbps on the wire) per
  direction. Each frame is 1920 bytes + header, so datagrams exceed the typical 1500-byte
  MTU and get IP-fragmented — fine on a LAN, less robust over lossy links.
- **Zip PCM (zlib)** — PCM compressed per-frame with Qt's `qCompress` / `qUncompress`.
  Lossless. Note that 16-bit audio is high-entropy, so zlib gives little or no size
  reduction over Raw (and may even be slightly larger); it's offered as a simple,
  dependency-free option, not a substitute for Opus.

The mode is **not negotiated** — set the **same mode on both PCs**, or you'll hear noise.
It's intentionally the user's responsibility, so there's no handshake to wait on.

## Future enhancements

- Jitter buffer with reordering by sequence number.
- Configurable bitrate / DTX from the GUI.
- Optional payload encryption.
