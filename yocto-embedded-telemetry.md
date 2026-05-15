# Lightweight Yocto Embedded Telemetry

> **Repo:** `yocto-embedded-telemetry`  
> **Stack:** Yocto layer (Kirkstone-compatible), C, BitBake recipes, native Linux test, optional QEMU  
> **Goal:** A small, credible embedded-Linux portfolio project — custom layer + C telemetry daemon — without keeping 50+ GB of Poky build artifacts on your machine.

---

## Table of Contents

1. [What This Project Is](#1-what-this-project-is)
2. [Lightweight Workflow](#2-lightweight-workflow)
3. [Architecture](#3-architecture)
4. [Repository Layout](#4-repository-layout)
5. [Phase 0 — Bootstrap the Repo](#phase-0--bootstrap-the-repo)
6. [Phase 1 — C Telemetry Daemon](#phase-1--c-telemetry-daemon)
7. [Phase 2 — Yocto Layer & Recipe](#phase-2--yocto-layer--recipe)
8. [Phase 3 — Native Test (No Full Image Build)](#phase-3--native-test-no-full-image-build)
9. [Phase 4 — Optional Full Build & QEMU](#phase-4--optional-full-build--qemu)
10. [API Reference](#10-api-reference)
11. [Troubleshooting](#11-troubleshooting)
12. [README Outline](#12-readme-outline)

---

## 1. What This Project Is

You ship a **custom Yocto layer** (`meta-mysoftware`) that packages a tiny **C HTTP server** (`bootinfo-server`). On boot (in a real image), it listens on port **8000** and serves system diagnostics as JSON:

```http
GET /bootinfo HTTP/1.1
```

```json
{
  "kernel_version": "6.1.0-yocto-standard",
  "uptime_seconds": 142,
  "uptime_human": "0h 2m 22s",
  "memory_total_mb": 512,
  "memory_free_mb": 387,
  "load_avg_1m": 0.12,
  "hostname": "qemux86-64",
  "timestamp": "2026-05-15T09:41:03Z"
}
```

**What you prove (without a local monster build):**

| Skill | How |
|-------|-----|
| Yocto / BitBake | Valid `layer.conf`, `.bb` recipe, image `.bbappend` |
| Embedded C | Sockets, `sysinfo(2)`, `uname(2)`, minimal HTTP |
| Linux deployment | Init script or systemd unit, auto-start at boot |
| Debugging | Native compile + `curl` before cross-build |

**Disk budget (tracked in Git):** ~10–30 MB.  
**Not in Git:** `poky/`, `build/`, `downloads/`, `sstate-cache/` (tens of GB if you run a full image build).

---

## 2. Lightweight Workflow

Default path — use this unless you have a cloud VM with plenty of disk:

```
┌─────────────────────────────────────────────────────────────┐
│  ON YOUR LAPTOP (always)                                     │
│                                                              │
│  1. meta-mysoftware/     ← layer, recipe, C source, init     │
│  2. native-test/         ← Makefile, gcc, curl validation    │
│  3. docs/                ← architecture, expected bitbake    │
│  4. README.md            ← build story + API                 │
└─────────────────────────────────────────────────────────────┘
                              │
                              │ optional, once
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  CLOUD VM / LAB MACHINE (Codespaces, EC2, NCI, …)            │
│                                                              │
│  shallow clone poky → bitbake core-image-minimal             │
│  runqemu + screenshots → delete VM / rm build/tmp            │
└─────────────────────────────────────────────────────────────┘
```

| Approach | Disk on laptop | When to use |
|----------|----------------|-------------|
| **Native test only** | &lt; 50 MB | Daily dev, interviews, GitHub |
| **Shallow Poky + single recipe** | ~2–5 GB temp | `bitbake bootinfo-server` only (no image) |
| **Full image + QEMU** | 30–80 GB | One-time proof screenshots in cloud |

**Do not** commit `poky/`, `build/tmp`, or deploy images. Put them in `.gitignore`.

---

## 3. Architecture

### 3.1 End-to-end (target system)

```
Host (Ubuntu)                         Target (QEMU or board)
────────────────                      ─────────────────────
  meta-mysoftware/                          Linux + init
       │                                        │
       ▼                                        ▼
  BitBake recipe ──cross-compile──►  /usr/bin/bootinfo-server
       │                             /etc/init.d/bootinfo-server
       │                                        │
       └────────────────────────────────────────┘
                              │
                    GET :8000/bootinfo → JSON
```

### 3.2 Layer layout

```
meta-mysoftware/
├── conf/
│   └── layer.conf
├── recipes-core/images/
│   └── core-image-minimal.bbappend    # IMAGE_INSTALL:append = " bootinfo-server"
└── recipes-myapps/bootinfo-server/
    ├── bootinfo-server.bb
    └── files/
        ├── bootinfo-server.c
        └── bootinfo-server.init       # SysVinit (matches core-image-minimal)
```

### 3.3 Daemon internals

```
bootinfo-server (port 8000, single-threaded)
├── socket → bind → listen → accept loop
├── handle_client()
│   ├── GET /bootinfo  → build_json() → HTTP 200
│   └── else           → HTTP 404
└── build_json()
    ├── uname(2)      → kernel_version
    ├── sysinfo(2)    → uptime, memory, load_avg_1m
    ├── gethostname() → hostname
    └── time/strftime → timestamp (UTC ISO 8601)
```

### 3.4 Boot (when running a real image)

```
kernel → /sbin/init → … → S99bootinfo-server → /usr/bin/bootinfo-server &
```

---

## 4. Repository Layout

```
yocto-embedded-telemetry/
├── README.md
├── .gitignore
├── meta-mysoftware/              # tracked — your Yocto layer
├── native-test/
│   ├── Makefile
│   └── run.sh                    # build, run, curl /bootinfo
├── docs/
│   ├── architecture.md           # copy/summary of §3
│   ├── expected-build-output.md  # sample bitbake lines (no tmp/)
│   ├── troubleshooting.md
│   └── screenshots/              # curl, ps, optional QEMU
└── scripts/
    └── setup-host.sh             # apt deps for native + optional Yocto
```

**.gitignore (minimum):**

```gitignore
poky/
build/
downloads/
*.sstate-cache/
.DS_Store
.vscode/
*.log
native-test/bootinfo-server
```

---

## Phase 0 — Bootstrap the Repo

**Time:** ~15 min

```bash
mkdir -p yocto-embedded-telemetry/{meta-mysoftware/conf,native-test,docs/screenshots,scripts}
cd yocto-embedded-telemetry
git init && git branch -M main
# add .gitignore (above)
git add . && git commit -m "chore: initialise lightweight yocto telemetry repo"
```

---

## Phase 1 — C Telemetry Daemon

**Time:** ~1–2 h  
**Location:** `meta-mysoftware/recipes-myapps/bootinfo-server/files/bootinfo-server.c`

Implement a minimal TCP server:

- Port **8000**, `SO_REUSEADDR`
- Only **`GET /bootinfo`** → JSON body, `Content-Type: application/json`
- Diagnostics via **`sysinfo(2)`**, **`uname(2)`**, **`gethostname(2)`**
- Load average: `si.loads[0] / 65536.0`

**SysVinit script** (`bootinfo-server.init`): start at runlevel S after `$network`, using `start-stop-daemon`, PID file under `/var/run/`.

**Quick native sanity check** (before any Yocto):

```bash
cd meta-mysoftware/recipes-myapps/bootinfo-server/files
gcc -Wall -o bootinfo-server bootinfo-server.c
./bootinfo-server &
curl -s http://127.0.0.1:8000/bootinfo | python3 -m json.tool
kill %1
```

Commit: `feat: add bootinfo-server C daemon and init script`

---

## Phase 2 — Yocto Layer & Recipe

**Time:** ~1 h

### `conf/layer.conf`

```bitbake
BBPATH .= ":${LAYERDIR}"
BBFILES += "${LAYERDIR}/recipes-*/*/*.bb \
            ${LAYERDIR}/recipes-*/*/*.bbappend"
BBFILE_COLLECTIONS += "mysoftware"
BBFILE_PATTERN_mysoftware = "^${LAYERDIR}/"
BBFILE_PRIORITY_mysoftware = "10"
LAYERDEPENDS_mysoftware = "core"
LAYERSERIES_COMPAT_mysoftware = "kirkstone"
```

### `bootinfo-server.bb` (essentials)

- `SRC_URI` = `file://bootinfo-server.c` + `file://bootinfo-server.init`
- `inherit update-rc.d`
- `INITSCRIPT_NAME = "bootinfo-server"`
- `INITSCRIPT_PARAMS = "start 99 S . stop 10 0 6 ."`
- `do_compile`: `${CC} ${CFLAGS} ${LDFLAGS} ${S}/bootinfo-server.c -o ${S}/bootinfo-server`
- `do_install`: binary → `${D}${bindir}`, init → `${D}${sysconfdir}/init.d`
- `LICENSE = "MIT"` + correct `LIC_FILES_CHKSUM`

### `core-image-minimal.bbappend`

```bitbake
IMAGE_INSTALL:append = " bootinfo-server"
```

Commit: `feat: add meta-mysoftware layer and bootinfo-server recipe`

You do **not** need a successful `bitbake core-image-minimal` on your laptop for this commit to be meaningful — the layer structure and recipe are the artifact.

---

## Phase 3 — Native Test (No Full Image Build)

**Time:** ~30 min  
**Purpose:** Prove the daemon and API on real Linux APIs.

### `native-test/Makefile`

```makefile
SRC = ../meta-mysoftware/recipes-myapps/bootinfo-server/files/bootinfo-server.c
CC ?= gcc
CFLAGS += -Wall -Wextra

bootinfo-server: $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

run: bootinfo-server
	./bootinfo-server

clean:
	rm -f bootinfo-server
```

### `native-test/run.sh`

```bash
#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
make -q bootinfo-server 2>/dev/null || make
./bootinfo-server &
PID=$!
trap 'kill "$PID" 2>/dev/null' EXIT
sleep 0.5
curl -sf http://127.0.0.1:8000/bootinfo | python3 -m json.tool
echo "OK: /bootinfo returned valid JSON"
```

```bash
chmod +x native-test/run.sh
./native-test/run.sh
```

Save sample output under `docs/sample-bootinfo.json`. Optional screenshot of `curl` → `docs/screenshots/`.

Commit: `test: add native build and curl smoke test`

---

## Phase 4 — Optional Full Build & QEMU

Only when you have **~50 GB free** and **several hours** (use a cloud VM if your laptop is tight).

### One-time host setup

```bash
./scripts/setup-host.sh   # gawk, python3, chrpath, etc. — see Yocto manual
```

### Shallow Poky (saves clone size)

```bash
git clone --depth=1 --branch kirkstone https://git.yoctoproject.org/poky
cd poky && source oe-init-build-env ../build
```

### `build/conf/local.conf` (minimal)

```bitbake
MACHINE ?= "qemux86-64"
BB_NUMBER_THREADS ?= "${@oe.utils.cpu_count()}"
PARALLEL_MAKE ?= "-j ${@oe.utils.cpu_count()}"
INHERIT += "rm_work"
```

### Register layer

In `build/conf/bblayers.conf`, add the **absolute** path to `meta-mysoftware`, plus standard `meta`, `meta-poky`, `meta-yocto-bsp`.

```bash
bitbake-layers show-layers    # must list mysoftware
```

### Build options (lightest → heaviest)

| Command | Purpose |
|---------|---------|
| `bitbake bootinfo-server` | Cross-compile package only |
| `bitbake core-image-minimal` | Full image (large) |

### QEMU smoke test

```bash
runqemu qemux86-64 nographic
# login: root (no password on minimal image)
wget -qO- http://127.0.0.1:8000/bootinfo
```

Port forward from host (optional):

```bash
runqemu qemux86-64 nographic slirp "hostfwd=tcp::8000-:8000"
curl -s http://127.0.0.1:8000/bootinfo
```

### Clean up after proof

```bash
rm -rf build/tmp build/sstate-cache build/downloads
# optional: rm -rf poky
```

Document commands and **expected** log snippets in `docs/expected-build-output.md` — not multi-GB `tmp/` folders.

---

## 10. API Reference

### `GET /bootinfo`

| Field | Type | Source |
|-------|------|--------|
| `kernel_version` | string | `uname(2)` release |
| `uptime_seconds` | int | `sysinfo(2).uptime` |
| `uptime_human` | string | derived |
| `memory_total_mb` | int | `sysinfo(2)` RAM |
| `memory_free_mb` | int | `sysinfo(2)` free RAM |
| `load_avg_1m` | float | `loads[0] / 65536` |
| `hostname` | string | `gethostname(2)` |
| `timestamp` | string | UTC ISO 8601 |

Any other path → **404** with plain text body.

---

## 11. Troubleshooting

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| `No recipes available for bootinfo-server` | Layer not in `bblayers.conf` | Add path; `bitbake-layers show-layers` |
| `do_compile`: file not found | Wrong `SRC_URI` / missing `files/` | Match recipe dir layout |
| QA: installed to wrong directory | Bad `${D}` paths | Use `${D}${bindir}`, `${D}${sysconfdir}/init.d` |
| Server not running after boot | Init not executable / rc.d | `install -m 0755`; check `update-rc.d` |
| `curl` fails on host | No port forward | Test inside guest or use `hostfwd` |
| `LIC_FILES_CHKSUM` error | MIT checksum | Use md5 from `${COMMON_LICENSE_DIR}/MIT` |
| Native `bind: Address in use` | Old process on 8000 | `fuser -k 8000/tcp` or change `PORT` |

---

## 12. README Outline

Use this structure for `README.md` (keep it short; link to this file for phases):

1. **One-liner** — Yocto layer + C JSON telemetry on `/bootinfo`
2. **Quick start** — `./native-test/run.sh` (no Poky required)
3. **Architecture** — small diagram (layer → recipe → binary)
4. **Optional full build** — link to Phase 4; note disk/RAM needs
5. **API table** — fields from §10
6. **License** — MIT

### Suggested commit history

```
chore: initialise lightweight yocto telemetry repo
feat: add bootinfo-server C daemon and init script
feat: add meta-mysoftware layer and bootinfo-server recipe
test: add native build and curl smoke test
docs: architecture, sample JSON, optional build notes
```

Tag `v1.0.0` when native test passes and layer/recipe are complete — full QEMU proof is optional.

---

## Interview talking points

- **Why lightweight:** Yocto proves BSP workflow; native test proves C and Linux APIs without 60 GB on disk.
- **`${CC}` in recipes:** never hardcode `gcc` — BitBake sets the cross-compiler.
- **`${D}` vs `${S}`:** `${S}` is build dir; `${D}` is staged rootfs for packaging.
- **sstate-cache:** after one cloud build, incremental recipe rebuilds are fast — delete `tmp` when done.
- **Production angle:** same pattern as ECU/edge diagnostics — small daemon, JSON over HTTP, starts at boot.

---

## Resource limits (honest)

| Resource | Native-only repo | Full `core-image-minimal` |
|----------|------------------|---------------------------|
| Git repo size | ~10–30 MB | +0 (don't commit images) |
| Local disk | &lt; 50 MB | 30–80 GB typical |
| RAM | 2 GB OK | 8–16 GB recommended |
| Time | minutes | hours (first build) |

**Bottom line:** Treat the **layer + recipe + working C daemon + native test** as the product. Treat a full Poky image build as optional evidence, usually done once in the cloud.
