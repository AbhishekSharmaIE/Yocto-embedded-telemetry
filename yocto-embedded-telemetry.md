# Technical documentation

System design, component reference, and integration guide for **yocto-embedded-telemetry**.

---

## Table of contents

1. [Overview](#overview)
2. [System architecture](#system-architecture)
3. [Components](#components)
4. [Boot and runtime](#boot-and-runtime)
5. [API reference](#api-reference)
6. [Host development](#host-development)
7. [Yocto integration](#yocto-integration)
8. [Operations](#operations)
9. [Troubleshooting](#troubleshooting)

---

## Overview

**bootinfo-server** is a single-threaded C process that listens on TCP port **8000** and answers HTTP `GET /bootinfo` with a JSON document of live system metrics. It is intended for embedded Linux products where a small, dependency-free diagnostics endpoint is useful for bring-up, monitoring, or factory test.

The service is delivered in two forms:

| Delivery | Use case |
|----------|----------|
| **Native build** (`native-test/`) | Develop and validate on the host without Poky |
| **Yocto package** (`meta-mysoftware/`) | Install into `core-image-minimal` (or custom image) for target deployment |

---

## System architecture

### End-to-end

```
┌─────────────────────┐         ┌─────────────────────────────────┐
│  Build host         │         │  Target device                   │
│  (BitBake / gcc)    │         │                                  │
│                     │         │  Linux kernel                    │
│  meta-mysoftware    │──build─►│       ↓                          │
│    .bb + sources    │         │  /sbin/init (SysVinit)           │
│                     │         │       ↓                          │
│  native-test/       │──dev───►│  /etc/init.d/bootinfo-server     │
│    (optional)       │         │       ↓                          │
└─────────────────────┘         │  /usr/bin/bootinfo-server :8000  │
                                │       ↓                          │
                                │  GET /bootinfo → JSON            │
                                └─────────────────────────────────┘
```

### Request handling (daemon)

```
accept loop
    └── read HTTP request line
            ├── GET /bootinfo  → build_json() → HTTP 200 + JSON body
            └── otherwise      → HTTP 404
```

`build_json()` collects:

| Data | Linux API |
|------|-----------|
| Kernel release | `uname(2)` |
| Uptime, memory, load | `sysinfo(2)` |
| Hostname | `gethostname(2)` |
| Timestamp | `time(2)` + `gmtime` + `strftime` (UTC ISO 8601) |

Load average uses `si.loads[0] / 65536.0` per `sysinfo` documentation.

---

## Components

### Custom layer: `meta-mysoftware`

```
meta-mysoftware/
├── conf/layer.conf
├── recipes-core/images/core-image-minimal.bbappend
└── recipes-myapps/bootinfo-server/
    ├── bootinfo-server.bb
    └── files/
        ├── bootinfo-server.c
        └── bootinfo-server.init
```

| File | Role |
|------|------|
| `layer.conf` | Registers the layer; `BBFILE_PRIORITY_mysoftware = "10"`; Kirkstone compatibility |
| `bootinfo-server.bb` | Fetches sources, cross-compiles, installs binary and init script |
| `core-image-minimal.bbappend` | `IMAGE_INSTALL:append = " bootinfo-server"` |
| `bootinfo-server.c` | Daemon implementation |
| `bootinfo-server.init` | SysVinit start/stop/status via `start-stop-daemon` |

### BitBake recipe summary

- **License:** MIT
- **Init:** `inherit update-rc.d` — start priority **99** in runlevel **S**
- **Install paths:** `${bindir}/bootinfo-server`, `${sysconfdir}/init.d/bootinfo-server`
- **Compile:** `${CC} ${CFLAGS} ${LDFLAGS}` on `bootinfo-server.c` (no hardcoded `gcc`)

### Native test harness

| File | Role |
|------|------|
| `native-test/Makefile` | Builds daemon from the same `.c` used in the recipe |
| `native-test/run.sh` | Build, background start, `curl` smoke test, clean shutdown |
| `scripts/verify.sh` | Validates layer file presence + runs `run.sh` |

---

## Boot and runtime

On a Yocto `core-image-minimal` target with SysVinit:

```
kernel mount rootfs
    → /sbin/init
    → … network (Required-Start: $network)
    → S99bootinfo-server
    → /usr/bin/bootinfo-server (background, PID file /var/run/bootinfo-server.pid)
```

**Service control on target:**

```bash
/etc/init.d/bootinfo-server start|stop|restart|status
```

**Verify on target:**

```bash
wget -qO- http://127.0.0.1:8000/bootinfo
# or
curl -s http://127.0.0.1:8000/bootinfo
```

---

## API reference

### `GET /bootinfo`

**Success — 200 OK**

```http
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: <n>
Connection: close

{
  "kernel_version": "<string>",
  "uptime_seconds": <int>,
  "uptime_human": "<string>",
  "memory_total_mb": <int>,
  "memory_free_mb": <int>,
  "load_avg_1m": <float>,
  "hostname": "<string>",
  "timestamp": "<ISO8601 UTC>"
}
```

**Not found — 404**

Any path other than `/bootinfo` returns plain text: `Not Found. Try GET /bootinfo`.

### Field reference

| Field | Type | Source |
|-------|------|--------|
| `kernel_version` | string | `uname(2).release` |
| `uptime_seconds` | integer | `sysinfo(2).uptime` |
| `uptime_human` | string | Derived `Hh Mm Ss` |
| `memory_total_mb` | integer | `totalram * mem_unit` |
| `memory_free_mb` | integer | `freeram * mem_unit` |
| `load_avg_1m` | float | `loads[0] / 65536` |
| `hostname` | string | `gethostname(2)` |
| `timestamp` | string | Server UTC time at response |

---

## Host development

### Prerequisites

```bash
sudo ./scripts/setup-native.sh   # build-essential, curl, python3, make
```

### Build and test

```bash
./native-test/run.sh             # automated smoke test
./scripts/verify.sh              # layer check + smoke test
```

### Manual run

```bash
cd native-test && make && ./bootinfo-server
curl -s http://127.0.0.1:8000/bootinfo | python3 -m json.tool
```

The repository intentionally does not include Poky or build artifacts. Host development validates logic, HTTP behaviour, and JSON output before target integration.

---

## Yocto integration

Perform these steps on a build host with [Yocto Project](https://www.yoctoproject.org/) **Kirkstone** (Poky) installed.

### 1. Register the layer

Add the absolute path to `meta-mysoftware` in `build/conf/bblayers.conf`:

```bitbake
BBLAYERS ?= " \
  /path/to/yocto-embedded-telemetry/meta-mysoftware \
  ${TOPDIR}/../poky/meta \
  ${TOPDIR}/../poky/meta-poky \
  ${TOPDIR}/../poky/meta-yocto-bsp \
  "
```

Verify:

```bash
bitbake-layers show-layers | grep mysoftware
```

### 2. Build the package

```bash
source oe-init-build-env build
bitbake bootinfo-server
```

### 3. Build an image (optional)

```bash
# build/conf/local.conf
MACHINE ?= "qemux86-64"

bitbake core-image-minimal
```

Deploy artifacts appear under `tmp/deploy/images/<MACHINE>/`.

### 4. QEMU validation (optional)

```bash
runqemu qemux86-64 nographic
# guest login: root (empty password on minimal image)
wget -qO- http://127.0.0.1:8000/bootinfo
```

Host port forward (optional):

```bash
runqemu qemux86-64 nographic slirp "hostfwd=tcp::8000-:8000"
curl -s http://127.0.0.1:8000/bootinfo
```

### Host toolchain packages (full Yocto builds)

For machines that run BitBake, use `scripts/setup-host.sh` to install the Yocto recommended host packages. This is separate from `scripts/setup-native.sh`, which only supports host-side daemon development.

---

## Operations

| Item | Value |
|------|--------|
| Listen address | `0.0.0.0` |
| Port | `8000` |
| Process model | Single-threaded, blocking `accept` |
| Connections | One client per accept; connection closed after response |
| Logging | `printf` on bind/listen (stdout) |

**Resource profile:** Suitable for minimal embedded images — one binary, no linked libraries beyond libc.

---

## Troubleshooting

| Symptom | Cause | Resolution |
|---------|-------|------------|
| `bind: Address already in use` | Port 8000 occupied | `fuser -k 8000/tcp` or stop existing instance |
| `curl: connection refused` | Daemon not running | `./native-test/run.sh` or start binary manually |
| BitBake: no recipe `bootinfo-server` | Layer not in `BBLAYERS` | Add `meta-mysoftware` path; run `bitbake-layers show-layers` |
| `do_compile`: source not found | `SRC_URI` / `files/` mismatch | Ensure `files/bootinfo-server.c` exists beside `.bb` |
| QA install path errors | Incorrect `${D}` usage | Use `${D}${bindir}`, `${D}${sysconfdir}/init.d` |
| Service missing after boot | Init permissions / rc.d | Recipe must `install -m 0755`; confirm `update-rc.d` inherit |
| `LIC_FILES_CHKSUM` mismatch | Wrong MIT checksum | Use OE-Core `COMMON_LICENSE_DIR` MIT md5 |
| QEMU: cannot reach service from host | No port forward | Use `hostfwd` or test inside guest |

Additional notes: [docs/troubleshooting.md](docs/troubleshooting.md).
