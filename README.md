# Yocto Embedded Telemetry

Embedded Linux telemetry service packaged as a Yocto layer, with a minimal C HTTP daemon that exposes host diagnostics over REST/JSON.

Designed for resource-constrained targets: single-process, no external dependencies, SysVinit integration, and a host-side native test harness for fast iteration without a full image build.

## Features

- **REST endpoint** — `GET /bootinfo` returns kernel, uptime, memory, load average, hostname, and UTC timestamp as JSON
- **Yocto integration** — `meta-mysoftware` layer with BitBake recipe and `core-image-minimal` append
- **Boot-time service** — SysVinit script (`update-rc.d`) starts the daemon after network bring-up
- **Host validation** — `native-test/` cross-checks the same source on your development machine

## Quick start

**Prerequisites:** `gcc`, `make`, `curl`, `python3` (Debian/Ubuntu: `sudo ./scripts/setup-native.sh`)

```bash
git clone https://github.com/AbhishekSharmaIE/Yocto-embedded-telemetry.git
cd Yocto-embedded-telemetry

./scripts/verify.sh
```

Or run the smoke test only:

```bash
./native-test/run.sh
```

Example response:

```bash
curl -s http://127.0.0.1:8000/bootinfo | python3 -m json.tool
```

```json
{
  "kernel_version": "6.17.0-23-generic",
  "uptime_seconds": 2039,
  "uptime_human": "0h 33m 59s",
  "memory_total_mb": 15614,
  "memory_free_mb": 7060,
  "load_avg_1m": 0.62,
  "hostname": "kali",
  "timestamp": "2026-05-15T13:43:49Z"
}
```

A captured sample is in [docs/sample-bootinfo.json](docs/sample-bootinfo.json).

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│  Development host                                             │
│  native-test/  ──gcc──►  bootinfo-server  ──►  :8000/bootinfo │
└──────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│  Target (Yocto image)                                         │
│  BitBake (meta-mysoftware)                                    │
│    bootinfo-server.bb  →  /usr/bin/bootinfo-server            │
│    bootinfo-server.init → /etc/init.d/bootinfo-server         │
│  Boot  →  S99bootinfo-server  →  GET :8000/bootinfo  →  JSON │
└──────────────────────────────────────────────────────────────┘
```

See [docs/architecture.md](docs/architecture.md) and [yocto-embedded-telemetry.md](yocto-embedded-telemetry.md) for component-level detail.

## Repository structure

| Path | Description |
|------|-------------|
| `meta-mysoftware/` | Custom Yocto layer (Kirkstone-compatible) |
| `meta-mysoftware/recipes-myapps/bootinfo-server/` | Daemon source, init script, BitBake recipe |
| `meta-mysoftware/recipes-core/images/` | Image append to install the package |
| `native-test/` | Host build (`Makefile`) and HTTP smoke test (`run.sh`) |
| `scripts/setup-native.sh` | Minimal development dependencies |
| `scripts/verify.sh` | Layer structure check + native smoke test |
| `docs/` | Architecture, troubleshooting, sample JSON |
| `yocto-embedded-telemetry.md` | Full technical and integration reference |

## API

### `GET /bootinfo`

Returns `200 OK` with `Content-Type: application/json`.

| Field | Type | Description |
|-------|------|-------------|
| `kernel_version` | string | Linux kernel release (`uname`) |
| `uptime_seconds` | integer | Seconds since boot |
| `uptime_human` | string | Human-readable uptime |
| `memory_total_mb` | integer | Total RAM (MiB) |
| `memory_free_mb` | integer | Free RAM (MiB) |
| `load_avg_1m` | number | 1-minute load average |
| `hostname` | string | System hostname |
| `timestamp` | string | Response time (UTC, ISO 8601) |

Any other request path returns **404 Not Found**.

## Development

Build and run locally (same source as the Yocto recipe):

```bash
cd native-test
make
./bootinfo-server
```

In another terminal:

```bash
curl -s http://127.0.0.1:8000/bootinfo
make clean   # remove binary
```

Port **8000** is hard-coded in `bootinfo-server.c`. If the port is busy: `fuser -k 8000/tcp`.

## Yocto integration

Add `meta-mysoftware` to `bblayers.conf`, then build the package or image on a machine with Poky installed:

```bash
bitbake bootinfo-server          # package only
bitbake core-image-minimal       # full minimal image including the service
```

Layer compatibility: **Kirkstone** (`LAYERSERIES_COMPAT_mysoftware = "kirkstone"`).

Full integration steps, boot behaviour, and BitBake notes: [yocto-embedded-telemetry.md](yocto-embedded-telemetry.md).

> **Note:** This repository ships the layer and daemon source only. Poky, `build/`, and deploy artifacts are excluded via `.gitignore` and are not required for host-side development.

## Troubleshooting

See [docs/troubleshooting.md](docs/troubleshooting.md).

## License

MIT — see recipe `LICENSE` and `LIC_FILES_CHKSUM` in `bootinfo-server.bb`.
