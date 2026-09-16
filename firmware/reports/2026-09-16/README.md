# Firmware build evidence — 2026-09-16

Real execution on the executor host (Docker `espressif/idf:v5.5.2`,
ESP-IDF v5.5.2 pinned via `sdkconfig.defaults` + CI image):

- `idf.py set-target esp32c3 && idf.py build`: **success** (exit 0).
  `gear_miles.bin` 0xcbe80 bytes (47% of the 1.5 MB app partition free);
  hashes in `bin_sha256.txt`.
- `make -C firmware/test/host test`: **19 tests, 0 failures** under
  `-Werror` + ASan + UBSan (output: `host_tests.txt`).
- Scope: host/simulation + build evidence only. **No run against real
  silicon** — panel command sequence, refresh timings, and current figures
  remain datasheet-derived until bench validation (issue #8).

Reproduce: see `.github/workflows/firmware.yml` (host-tests + idf-build).
