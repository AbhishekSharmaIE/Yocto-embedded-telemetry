# Architecture

## Data flow

```
meta-mysoftware (BitBake)
    → compile bootinfo-server.c for target ABI
    → install /usr/bin/bootinfo-server
    → install /etc/init.d/bootinfo-server
    → package included via core-image-minimal.bbappend

Target runtime
    → init starts bootinfo-server after network
    → TCP :8000
    → GET /bootinfo → JSON diagnostics
```

## Layer map

| Path | Responsibility |
|------|----------------|
| `conf/layer.conf` | Layer registration, priority, Kirkstone compatibility |
| `recipes-myapps/bootinfo-server/bootinfo-server.bb` | Build, install, SysVinit integration |
| `recipes-myapps/bootinfo-server/files/*` | Source and init script |
| `recipes-core/images/core-image-minimal.bbappend` | Adds package to image |

## Host vs target

| Environment | Entry point | Purpose |
|-------------|-------------|---------|
| Development host | `native-test/` | Fast compile and HTTP validation |
| Yocto target | `/etc/init.d/bootinfo-server` | Production-like boot and networking |

Full reference: [yocto-embedded-telemetry.md](../yocto-embedded-telemetry.md).
