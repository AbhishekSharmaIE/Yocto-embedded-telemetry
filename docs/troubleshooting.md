# Troubleshooting

## Native development

| Symptom | Resolution |
|---------|------------|
| `bind: Address already in use` | Stop stale process: `fuser -k 8000/tcp` |
| `curl: connection refused` | Start server: `./native-test/run.sh` or `cd native-test && ./bootinfo-server` |
| `gcc: command not found` | Run `sudo ./scripts/setup-native.sh` |
| Smoke test hangs | Check firewall; confirm nothing else blocks port 8000 |

## Yocto / target

| Symptom | Resolution |
|---------|------------|
| Recipe not found | Add `meta-mysoftware` to `bblayers.conf`; verify with `bitbake-layers show-layers` |
| `LIC_FILES_CHKSUM` error | Use MIT checksum from `${COMMON_LICENSE_DIR}/MIT` in OE-Core |
| Service not running after boot | Confirm init script mode `0755` and `inherit update-rc.d` in recipe |
| `do_compile` cannot find `.c` | Keep sources under `files/` next to `bootinfo-server.bb` |
| QEMU: no response from host | Test inside guest, or use `hostfwd=tcp::8000-:8000` with `runqemu` |

## Verification

Run the automated check:

```bash
./scripts/verify.sh
```

See also [yocto-embedded-telemetry.md § Troubleshooting](../yocto-embedded-telemetry.md#troubleshooting).
