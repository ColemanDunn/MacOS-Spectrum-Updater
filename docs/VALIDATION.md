# Validation evidence and current limits

[← README](../README.md)

## Software validation

The existing stored results describe the packaged 1.0 build 15 source and prior unchanged protocol implementations. They were produced with offline simulators, test fixtures, stubbed CLI transports, and offscreen views. No monitor operation was performed to produce that publication package. Build 16 adds ten native USB access-policy scenarios in `tests/test_macos_access.c`, using a fake USB vtable and no device access; see the separate live read-only observations below.

Build 17 adds a read-only EF4018 scaler profile. Its protocol simulator reads the complete 4 MiB FW2 and validates the result, rejects scaler write setup without sending setup commands, independently rejects program/restore even with a forced ready flag, and rejects neighboring unrecognized IDs. The stored build 15 assertion snapshot below remains unchanged; current runs are written under gitignored `build/`.

Build 18 corrects the VIA JEDEC transaction to `C0/C1`, value `009F`, index zero, four-byte USB reply. The VIA simulator includes the measured successful `C1/9F` and zero-filled legacy `C8/9F` responses, plus zero/FF identification rejection and register cleanup (1,536,978 assertions in the focused run).

| Area | Recorded result | Evidence |
| --- | --- | --- |
| MStar scaler protocol | 21,058,882 assertions | [protocol-tests.json](validation/protocol-tests.json) |
| VIA USB/SPI protocol and planner | 1,536,900 assertions | [usb-protocol-tests.json](validation/usb-protocol-tests.json) |
| Scaler CLI automatic model authorization | 22 scenarios, 215 assertions | [auto-model-tests.json](validation/auto-model-tests.json) |
| USB CLI orchestration | 15 scenarios, 128 assertions | [usb-cli-tests.json](validation/usb-cli-tests.json) |
| Original vendor package import | 9 ZIPs, 38 assertions | [vendor-import-tests.json](validation/vendor-import-tests.json) |
| Supported model/profile rules | 16 checks | [model-profile-tests.txt](validation/model-profile-tests.txt) |
| Private backup/receipt behavior | 19 assertions | [managed-backup-tests.json](validation/managed-backup-tests.json) |
| Visible log formatter | 13 assertions | [update-log-tests.json](validation/update-log-tests.json) |
| Stock scaler image validation | 7 original scaler containers | [image-tests.json](validation/image-tests.json) |
| Fixed-page light/dark UI and step progression | 524 checks in the packaged build | [release-checks.json](validation/release-checks.json) |

Assertion counts include repeated low-level checks and fault-injection loops; they are not counts of physical devices or independent real-world update sessions.

The new documentation renderer and release archive are checked separately during repository preparation. The stored release evidence is retained as the original sanitized snapshot rather than presented as newly rerun hardware testing.

## Failure handling covered offline

- Corrupt/truncated files, wrong hashes, unknown releases, and incompatible target models.
- Installed-image authorization before write.
- Unsupported silicon/flash geometry and protected SPI state.
- Failed/short setup and transport operations, bounded busy waits, and readback mismatches.
- Dirty erase results, corrupt programming, stale backup comparisons, and invalid shared headers/layout.
- Writes outside a component's permitted range.
- Component entry-chunk/header commit ordering.
- Backup/receipt collisions and durability before writes.
- Partial log lines, path omission, semantic success/error display, stable scrolling, theme contrast, fixed-page fit, and current-step-only emphasis.

CLI tests replace native USB transports with memory-only stubs. ASan/UBSan applies to the relevant C protocol/CLI suites. The UI harness does not launch AppDelegate or discover a monitor.

## What has been physically observed

| Route / unit | Current evidence |
| --- | --- |
| ES07D03 scaler | Prior user-reported successful physical flash, incorporating corrected chip sizing and sequence |
| ES07DC9, ES07E30, ES07D02 scaler | Exact stock images/profile logic validated offline; real update sessions not yet documented |
| VIA paired-hub access | Build 16: read-only native PD header reads succeeded on two directly connected ES07D03 units, Apple M4 Pro, macOS 27.0.1. The unpatched transport failed on both with `kIOReturnExclusiveAccess` (`0xe00002c5`). No firmware was written. |
| VIA PD programming | Build 18 installed PD 19.02 from 17.02 on two ES07D03 units, one at a time over direct USB-C, M4 Pro/macOS 27.0.1. Each durable 262,144-byte backup and receipt was verified; both programming sessions and complete shared-SPI readbacks succeeded. The user confirmed both pictures after the required monitor DC power cycles, and passive header reads confirmed `0A.89.19.02` on both. |
| Post-update display standby/wake | Both units passed two short user-assisted display-sleep/wake cycles at 120 Hz. The right display subsequently failed after a 9-minute 24-second display-sleep interval; both units still reported PD 19.02. Its USB hub remained visible but the video link was inactive. The stock PD update did not resolve this intermittent wake issue. |
| VIA hub programming | Native access, complete reads, and offline planning/programming tests passed; a physical hub firmware update is not yet documented |
| ES07D03 with scaler EF4018 | Build 17 completed the 4 MiB scaler read and exact installed-model proof. It then stopped before erase at a zero-filled shared-SPI ID response. No firmware was changed. |
| VIA identification and complete reads | Build 18 request: three read-only `C1/9F` queries returned `EF301200` on a directly USB-C-connected ES07D03; legacy `C4/90` returned `EF11EF11`. The real VIA engine then identified 262,144 bytes, read the entire shared SPI twice with identical results, and validated the PD 19.02 plan against the installed factory header and PD CRC/layout. The probe prohibited flash write-enable/erase/program commands, restored all four setup registers, and checked their original values. |
| Power-loss factory fallback | Not certified by these tests |
| Custom firmware behavior improvement | Not included in this stock release |

A prior D03 update does not establish coverage for every revision or original factory image. A final flash readback does not prove boot success, application of secondary components, calibration, wake timing, or improved HDR/refresh transitions.

The native access observation used `sp_macos_open_paired_hub` and one fixed vendor IN request (`C0/C4`, value `0203`, index `0010`, length 32, timeout 3000 ms) per unit. Both returned a valid VIA `2109:0103` PD header with version `0A.89.17.02`, matching the independent libusb read. No scaler ISP, SPI setup-register changes, erase, or programming was performed. This does not establish that vendor OUT requests or a complete firmware update work on the shared path.

The later build 18 read/plan probe also exercised eight native vendor OUT configuration writes (four setup and four restoration requests). Every restored register read matched its original value. Firmware write-enable, erase and programming were explicitly blocked by that probe's transport wrapper and were not tested by it.

## Outstanding work

1. Extend PD programming and restart validation beyond the two observed ES07D03 units. Investigate the wake failure that recurred after the successful updates.
2. Document physical hub firmware updates, including backup/readback/restart outcomes.
3. Document scaler updates on each additional model/controller/flash profile.
4. Establish original firmware evidence before adding D01/DCA or other targets.
5. Investigate recovery behavior with model-specific evidence rather than simulator assumptions.
6. Obtain maintainer Developer ID signing/notarization if desired for future public packages.

Use the [hardware-validation issue template](https://github.com/Golps/MacOS-Spectrum-Updater/issues/new/choose) to contribute precise positive or negative results.
