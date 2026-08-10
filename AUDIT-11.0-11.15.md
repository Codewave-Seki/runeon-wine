# Wine 11.0 to 11.15 Stability Review

[English](AUDIT-11.0-11.15.md) | [Chinese](AUDIT-11.0-11.15.zh-CN.md) | [Japanese](AUDIT-11.0-11.15.ja.md)

> This English document is authoritative. The Chinese and Japanese documents are complete translations for convenience.

## Scope

The fixed comparison range is `wine-11.0..wine-11.15` against CrossOver 26.3 source archive SHA-256 `ac99c8ca4b3848f3e81784135f023df266b61c2345726ea55a50b3e030dd6872`.

The inventory contains 3,350 upstream commits: 2,520 automated candidates, 340 subsystem-sensitive commits, and 490 ABI-sensitive commits. Automated labels only route review. They do not establish safety or justify inclusion.

Decisions for `wine-11.0..wine-11.14` are unchanged and remain recorded in [`AUDIT-11.0-11.14.md`](AUDIT-11.0-11.14.md). This document adds the incremental `wine-11.14..wine-11.15` round, which the scheduled upstream watch surfaced when `wine-11.15` was published on 2026-08-08.

## The incremental range

`wine-11.14..wine-11.15` contains 207 commits: 147 automated candidates, 25 subsystem-sensitive commits, and 35 ABI-sensitive commits. 39 of them touch only test files.

Every commit was checked with the forward/reverse patch algorithm used by the release scripts. A forward-clean result proves the base still carries the unfixed code and that CrossOver has no equivalent; a reverse-clean result would prove the fix is already present. Commits that conflicted were read against the CrossOver implementation before any decision.

## Included in `cx26.3-wine11.0-runeon.6`

The patch set inherits every patch from `.5` and adds nine backports covering twelve upstream commits.

| First release | Upstream commit | Area | Result |
| --- | --- | --- | --- |
| 11.15 | `702e44767a8e` | Win32u | Stop the US layout table from reporting one scan code past the end of `vsc_to_vk` |
| 11.15 | `bb33d1d34204` | MFReadWrite | Keep the caller media type untouched during transform negotiation, with the upstream test |
| 11.15 | `9a26041b38d8` | MFReadWrite | Drain more than one decoder sample when another transform is downstream |
| 11.15 | `0e3bbc87cf61` | Video Resizer | Register the media types the DMO actually supports |
| 11.15 | `cd0d7910b23b` | SChannel | Hash the server certificate with bcrypt so ECDSA algorithms work, with the upstream test |
| 11.15 | `1fc89cc934e9` | GDI+ | Stop `convert_pixels()` rounding the per-row byte count down, with the upstream test |
| 11.15 | `27ee4f5db28c`, `2f66b17d51de` | ComCtl32, User32 | Return `LB_ERR` for out-of-bounds `LB_SETTOPINDEX` in both listbox implementations |
| 11.15 | `d8186e465ad4` | CMD | Initialise the `node_builder_parse` loop flag before it is read |
| 11.15 | `d4b32af24629` | Crypt32 | Ignore bytes past the outer DER SEQUENCE when importing a PFX blob, with the upstream test |

The authoritative path, SHA-256, risk, full commit IDs, and order for every patch are in [`patches/manifest.json`](patches/manifest.json).

### Test backports

Seven of the nine patches carry upstream test changes. Three needed the upstream test prerequisite as well:

- The listbox patch carries `ab21fbc49a25` and `dd7f58d774` — the commits that introduce `test_LB_SETTOPINDEX` in both test suites — because the base has no such test to un-`todo_wine`. Both prerequisites are inside the same release window and apply cleanly.
- The Crypt32 test was rewritten to the base test file layout: `test_PFXImportCertStore_trailing_zeros()` is inserted after `test_PFXImportCertStore()` rather than after the later `_sha256_signing` variant, which does not exist in the base, and is registered in `START_TEST` at the matching position. The test body is unchanged.
- The Win32u, MFReadWrite `9a26041b38d8`, Video Resizer, and CMD commits ship no upstream test. They are one- and two-line defect fixes in code with no in-tree test coverage at either revision.

### Adaptations

Three patches deviate from the upstream text:

- `bb33d1d34204`: the helper is named `update_media_type_from_upstream()` in the base and `update_media_type()` upstream. The rename is the entire adaptation; the control flow, the media type copy, and its release are upstream's.
- `d4b32af24629`: `open_cert_store()` gained a `pgnutls_pkcs12_verify_mac()` call after 11.0, so the hunk context differs. The new `pfx_der_outer_length()` helper is transplanted verbatim inside the `SONAME_LIBGNUTLS` block and the two-line guard is placed at the same point relative to `pgnutls_pkcs12_import()`.
- `cd0d7910b23b`: the upstream commit does not touch `dlls/secur32/Makefile.in` because upstream secur32 already reaches `BCryptHash` through the `ncrypt` delay import that `f4a9724c89` added for mTLS. The pinned base has no such import, so the PE link fails with `undefined reference to BCryptHash`. Rather than pull in the unrelated mTLS commit, the patch adds `bcrypt` to `DELAYIMPORTS` directly. This is a build-graph adaptation only; the C code is upstream's.

### ABI-sensitive exception

`702e44767a8e` touches `dlls/win32u`, which the risk policy marks ABI-sensitive and excludes by default. It is admitted as a documented exception: the change is a single initialiser, `ARRAY_SIZE(vsc_to_vk)` to `ARRAY_SIZE(vsc_to_vk) - 1`, in a static keyboard table. It alters no structure layout, no interface, and no Unix boundary, and it fixes an out-of-bounds read found by ASan. No other ABI-sensitive commit in this range is included.

## CrossOver equivalents or non-applicable defects

- `0fde54a3574a` (wined3d frame latency): CrossOver 26.3 does not use the upstream `frame_latency_semaphore` path at all. It carries its own `pending_presents` / `waiting_for_present` / `present_event` / `max_frame_latency` implementation in `dlls/wined3d/cs.c`, and that implementation already releases the wined3d mutex around the wait. The multi-threaded D3D9 stall the upstream commit removes does not exist in this base.
- `852e39a89c0d` (host window state lock): `pGetWindowStateUpdates` is implemented only by `winex11.drv` in the pinned base. `winemac.drv` provides no such callback and the unlock protocol the fix relies on lives in the X11 driver, so the race cannot occur on the macOS path.
- `95e5d76a1882` and `0aa733c17aff` (wined3d planar CPU blits): part of the Vulkan H.264 decoder chain, which Runeon does not execute because video decoding runs through D3DMetal and DXVK.
- `bfe591cbef5b` (WindowsCodecs bound check): the check it removes was added by `2e68942fce46` inside this same release window and never reached the pinned base. The pair is a no-op here.
- The four `winebus.sys` commits in this range drive `hidraw`, `inotify`, and `evdev` code paths. macOS controller enumeration runs through IOHID, so none of them execute in Runeon.
- The `winex11.drv`, `winewayland.drv`, glibc `sa_restorer`, and ARM64EC/ARM WoW64 commits do not run on the macOS x86_64 and arm64 targets.

## Deferred, not silently dropped

- UCRT `b0150eda77d7` (`exp(NAN)`): the source fix is three lines, but its only regression test needs `8bed5ce602`, which also rewrites unrelated `test_cexp` and `test_expf` assertions whose outcome on this base depends on 11.x math fixes that are not being backported. Consistent with the `35b1e7eb9a7e` decision in the previous round, a source-only backport is not accepted.
- KernelBase `5e5fea8b9117` (debug port instead of the PEB `BeingDebugged` flag): a behavioural change on the unhandled-exception path. Titles that set or clear `BeingDebugged` for anti-tamper purposes would take a different branch than they do today. This needs product evidence before acceptance, not a source review.
- Win32u `3ec143785905` (detached-source DPI, Wine-Bug 59970): upstream converted the monitor DPI helpers to fractional ratios after 11.0, so the two `DISPLAY_DEVICE_ATTACHED_TO_DESKTOP` guards need a hand-written adaptation in ABI-sensitive display code, and the commit ships no test.
- MFReadWrite `4534f6cd0471` (1:1 pixel aspect ratio): adds new output behaviour rather than repairing a defect, and no Runeon title is known to need anamorphic correction.
- Win32u `a1bae27f21b5` (raw mouse input, Wine-Bug 59986): repairs a regression in the batched raw input mechanism introduced after 11.0. The base has no `info->raw_mouse` path, so there is nothing to fix without first porting that refactor.
- WinHTTP `f27d14a21cfe` and `93ef2498b391`: both fix `WINHTTP_OPTION_SERVER_CERT_CHAIN_CONTEXT`, an option that does not exist in the pinned base.
- Win32u `a37867ddf841` (initial monitor DPI in the window creation request): changes `server/protocol.def` and `include/wine/server_protocol.h`.
- The 24-commit `server` `init()` object-operation series: an ABI-sensitive refactor, deferred as one chain.
- Feature work in MSXML3, WMI .NET utilities, WinRT metadata, JScript, the NTLM and auth-identity series across msv1_0, sspicli and secur32, DNS-SD stubs, D2D1 sprite batches, D3DX9 intersection, bcrypt HKDF and TLS1 KDF, Shell32 CSIDL handling, and conhost beep.
- Header, build tool, symcrypt alignment, translation, and debugger changes carry no runtime value for this patch set.

## Validation and release boundary

Static manifest and SHA-256 checks pass. The complete 46-patch series applies sequentially to the pinned CrossOver source, the source marker is written, and the second-application fail-closed gate holds.

A native arm64 build of the patched tree with tests enabled compiled every patched source file and every backported test file, and `SONAME_LIBGNUTLS` resolved to `libgnutls.30.dylib`, so patches 0040 and 0044 are live rather than inert. That build also caught the missing `bcrypt` delay import recorded above.

Three failures in that build are environmental and unrelated to this patch set: `dlls/winemac.drv/cocoa_window.m` cannot see `WineMetalLayer` without the product D3DMetal configuration, `dlls/win32u/vulkan.c` needs `SONAME_LIBVULKAN` from a MoltenVK install, and both belong to the product build environment. No patch in the series touches either file.

One verification item is open and must be closed by the build gate before `.6` advances: a full Rosetta x86_64/WoW64 build in the documented product configuration, including the D3DMetal and MoltenVK dependencies, has not been run for `.6`.

A candidate is not shipped merely because the source gates pass. Affected runtime tests in the assembled seed, clean and existing-prefix probes, Steam CEF, stop/relaunch, D3DMetal overlay, and DXMT/DXVK smoke must still pass through the Runeon product repository.

Until those product gates pass, `.6` is only a source candidate. It must not replace the Production source entry or be described as available to users.
