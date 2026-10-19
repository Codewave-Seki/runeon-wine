# Upstream Maintenance Policy

[English](MAINTENANCE.md) | [Chinese](MAINTENANCE.zh-CN.md) | [Japanese](MAINTENANCE.ja.md)

> This English document is authoritative. The Chinese and Japanese documents are complete translations for convenience.

Runeon Wine accepts input from proactive upstream audits and diagnostics-driven investigation. Neither path may bypass the same validation gates.

## Proactive audits

1. A weekly workflow checks the latest Wine `11.x` tag.
2. `scripts/audit-upstream.sh --through latest` generates the commit list beginning at `wine-11.0`.
3. Automated risk labels only narrow the review set; they do not establish patch safety.
4. Review each commit for an equivalent CrossOver 26.3 implementation, prerequisite refactors, and concrete Runeon value.
5. Add a fix to the active series only when its provenance, dependencies, tests, user benefit, and regression surface are understood.

## Risk levels

- `candidate`: a localized DLL fix that still requires human review and tests.
- `subsystem-sensitive`: graphics, media, windowing, input, build-system, or multi-module changes.
- `abi-sensitive`: changes to `ntdll`, `server`, `wow64`, `loader`, `winemac.drv`, `win32u`, Unix library/server protocols, or D3DMetal interfaces. These are not ordinary backport candidates by default.

## Current Wine 11.x review baseline

Current stable source: [`cx26.3-wine11.0-runeon.21`](patchsets/cx26.3-wine11.0-runeon.21/manifest.json) (stable/latest), paired with seed `2026.10.18` (App `>=1.13 (10)`), MoltenVK `1.4.2` and SDL2 `2.32.10`. It preserves `.20` and adds `0121`, which extends the `0112` stack-bound check to the two collided-unwind branches of the x64 handler search; `0122`, which logs the driver, call code and status of a failed mmdevapi Unix call before its assertion; and `0123`, which skips writing an async I/O status block whose pages are no longer writable. `0121` and `0123` take effect without an opt-in and are listed in MAINTENANCE as individually approved exceptions; `0123` is a limited guard, and memory released after its check can still fault. A complete x86_64/WoW64 build, static and integration checks, and local and Dev acceptance passed; Production uses the exact signed archive verified in Dev. These results do not establish universal game compatibility. Immutable assets are recorded in [the release manifest](release-manifests/cx26.3-wine11.0-runeon.21.source-archive.json).

Previous stable source, superseded by `.21` and retained for rollback: [`cx26.3-wine11.0-runeon.20`](patchsets/cx26.3-wine11.0-runeon.20/manifest.json) (stable), paired with seed `2026.10.16.1`, which is no longer delivered. It preserves `.19` and adds `0089`, `0090` and `0120`. Immutable assets are recorded in [the release manifest](release-manifests/cx26.3-wine11.0-runeon.20.source-archive.json).

`cx26.3-wine11.0-runeon.19` (Pre-release) was the Dev candidate for Dev seed `2026.10.16`; it was superseded by `.20` and never used in Production. Immutable assets are recorded in [the release manifest](release-manifests/cx26.3-wine11.0-runeon.19.source-archive.json).

Retained stable source still delivered to App `>=1.12 (6)`, before `1.13 (10)`: [`cx26.3-wine11.0-runeon.18`](patchsets/cx26.3-wine11.0-runeon.18/manifest.json) (stable), paired with seed `2026.10.14`. It preserves `.17` and adds `0081`–`0087` and `0118`. Immutable assets are recorded in [the release manifest](release-manifests/cx26.3-wine11.0-runeon.18.source-archive.json).

`cx26.3-wine11.0-runeon.17` (stable) for seed `2026.10.10`, App `>=1.10 (3)` before `1.12 (6)`, is retained for rollback; it adds `0112`–`0117` to `.16`.

`cx26.3-wine11.0-runeon.16` (stable) for seed `2026.10.06`, App `>=1.10 (0)`, is retained for rollback; it adds `0111`, an opt-in Steam downlevel D3D12 preparation gate, to `.15`.

`cx26.3-wine11.0-runeon.15` (stable) for seed `2026.10.04`, App `>=1.10 (0)`, is retained for rollback; it adds the dwrite emoji system fallback (`0110`) and targeted `wine-11.19` fixes (`0079`) to `.14`.

`cx26.3-wine11.0-runeon.14` (stable) for seed `2026.10.02`, App `>=1.10 (0)`, is retained for rollback; it adds per-process graphics backend routing (`0109`) to `.13`.

Earlier Apps (`>=1.8 (0)`) keep [`cx26.3-wine11.0-runeon.13`](patchsets/cx26.3-wine11.0-runeon.13/manifest.json) (stable) for seed `2026.09.28.1`. `.13` adds the upstream `RtlVirtualUnwind2` NULL-output fix, twelve further ntdll fixes and the opt-in Rosetta self-modifying code policy (`0107`, `0108`) to `.11`; the user authorized it for Production on 2026-09-28. Seed `2026.09.28.1` rebuilds the same `.13` source with SDL enabled and ships SDL2 `2.32.10`; real-controller acceptance is still pending. `.12` (seed `2026.09.27`) is a superseded Dev candidate, retained unchanged.

Historical `.11` delivery (2026-09-26; retained rollback): [`cx26.3-wine11.0-runeon.11`](BACKPORTS-11.16-11.18-SWEEP.md) (stable/latest) for seed `2026.09.26`, App gate `>=1.8 (0)`. It adds the filtered Wine 11.16–11.18 sweep, the winegstreamer video-pool fallback and two opt-in hooks to `.10`. Build, signing, readiness and Dev/Production distribution checks passed; the user reported no issues in Dev and authorized Production on 2026-09-26. Production promotes the exact signed Dev archive without rebuilding.

Historical `.10` delivery (2026-09-23; retained rollback): [`cx26.3-wine11.0-runeon.10`](FLSGETVALUE2.md) (stable/latest), paired with seed `2026.09.22` and MoltenVK `1.4.2`, App gate `>=1.8 (0)`. Production promotes the exact signed Dev archive without rebuilding. Select the independent `patchsets/` definition explicitly; the default series remains the historical `.9`. Validation: API and isolated-prefix checks, signing and release readiness passed. The user reported no issues in Dev and authorized Production on 2026-09-23. This general acceptance does not establish independent coverage of every Steam/CEF, game or App interaction scenario. App installers and distribution verification are managed by the product repository.

Historical `.9` delivery and validation (2026-09-14; retained rollback): The active `.9` patch set preserves the `.8` baseline and its product patches. Its [targeted Wine 11.16/11.17 review](BACKPORTS-11.16-11.17.md) adds 10 patch files carrying 13 upstream commits; it does not extend the complete audit beyond `wine-11.15`. Seed `2026.09.13` uses `.9` with MoltenVK `1.4.2` in Dev and Production, with a minimum App gate of `1.8 (0)`. Production reuses the exact signed archive verified in Dev. Build, API regression, signing, readiness, authenticated feed/ticket and full-download verification are complete. The user confirmed component updates, runtime preparation and Steam startup with the current Xcode Dev source build. Distribution was checked for `1.8 (0-3)`; per-build game, stop/restart interaction and clean-prefix acceptance were not repeated. Published App installers remain `1.8 (3)`. The source Release is stable; tag, commit and all four asset bytes, sizes and digests are unchanged. `.8` / seed `2026.09.06` is retained history.

`cx26.3-wine11.0-runeon.8` carries `patches/runeon/0102-vuplex-accelerated-paint-policy.patch`. (`.7` carried an earlier form of the same patch that does not compile — it was tagged before a full build had been run. `static-check.sh` and `integration-check.sh` verify that patches apply, not that the result builds, so a product patch must go through a complete build before it is tagged.) It gives Wine the ability to make a Vuplex 3D WebView CEF host fall back from `OnAcceleratedPaint()` to CPU `OnPaint()`, and nothing more: the switch is only appended when the calling application has opted in through `HKCU\Software\Wine\AppDefaults\<image>\Runeon`, value `DisableVuplexAcceleratedPaint`. No game name or Steam AppID appears anywhere in Wine — the policy is written by the caller, so which titles need the fallback stays outside this repository. The host is matched on the image the command line will actually run ending in `.vuplex`, carrying `--vx-graphics-api=d3d11` and no `--type=`, so neither Chromium child processes nor a stray `.vuplex` path inside some other argument can trigger it. Falling back costs a CPU copy per painted frame, which is why it is opt-in rather than always on.

The shipped `cx26.3-wine11.0-runeon.8` patch set is based on a commit-by-commit review through `wine-11.15`; see [the complete audit](AUDIT-11.0-11.15.md) and the earlier [11.0 to 11.14 audit](AUDIT-11.0-11.14.md) it extends. The review selected 42 localized correctness and stability fixes in addition to the two previously accepted upstream backports. It did not attempt to copy every upstream commit. Feature work, broad refactors, ABI-sensitive changes, subsystem migrations, fixes already present in CrossOver 26.3, and fixes whose dependency or regression surface was not yet bounded remain deferred.

The audit is a point-in-time decision record, not a permanent allowlist. A deferred commit may enter a later immutable patch set after its dependency chain, affected tests, independent probe, and Runeon impact are established. Conversely, inclusion in the audit does not authorize publishing a runtime: the complete build and product smoke gates below still apply.

## Diagnostics-driven investigation

Real user logs may establish impact and priority. First pin down the call chain or reproduce the behavior with an independent probe, then correlate it with an upstream commit. Without local evidence, record only a candidate; do not treat an online issue as the Runeon root cause.

## Gates for entering the active series

- Record the full upstream commit SHA, first Wine release containing it, and original authorship.
- Cleanly apply to the pinned CrossOver source SHA, or document every adaptation.
- Backport the original upstream tests, or explain why they cannot be carried over.
- Pass affected-module tests and an independent probe.
- Complete a full Wine runtime build.
- Pass Steam CEF, stop/relaunch, D3DMetal overlay, and DXMT/DXVK/D3DMetal smoke tests.
- Validate both a clean prefix and an existing prefix.
- Synchronize the source bundle, notices, component metadata, and Runeon documentation.

## Product patches without an opt-in

Runeon product patches are opt-in by default, as `CLAUDE.md` requires. A specific fix may take effect without an opt-in only when the maintainer has approved that patch individually and the reason and the boundary are stated in this table or in its manifest entry. The approval covers that patch alone and is not a precedent for similar changes. These patches currently take effect without an opt-in:

| Patch | Why it is not opt-in | Boundary |
|---|---|---|
| `0100` | Middleware fix: the Steam and Rockstar CEF processes it names need these arguments. | Matched by middleware process name, never a game. Steam webhelper and SocialClubHelper get `--in-process-gpu` on the main process only; the SocialClubHelper SwiftShader switches go to every SocialClubHelper process. |
| `0101` | A new session of the product's managed runtime must pass launch validation; with a missing credential or a failed validation the server refuses to start rather than falling back to an unvalidated launch. | Only in a macOS build that defines `RUNEON_MANAGED_LAUNCH_VALIDATION` (any value, including 0); checked when a new wineserver starts, not again for each child process of a running session. Without the define, or on another platform, the check passes. |
| `0103` | Adds the `FlsGetValue2` export, which Windows provides and Wine lacks. | New export only; last error and FLS storage behave as before. |
| `0106` | Declines a winegstreamer video buffer pool whose allocation caps cannot be described, instead of offering a zero-size pool. | Only that case; allocation is left to GStreamer. |
| `0110` | Maps the characters Windows draws with Segoe UI Emoji to that family in the dwrite system fallback. | Fallback table entries, plus: when a mapping has no usable font, only the run sharing that mapping is reported. Without a Segoe UI Emoji font, every character resolves as before. |
| `0112`, `0121` | End the x64 handler search before an unwind reads past StackBase, as the existing establisher frame check does for a frame outside the stack. | The search ends with `EXCEPTION_STACK_INVALID`; nothing else in dispatch changes. |
| `0115`, `0116` | Code page 54936 (GB18030), which Windows implements in code and Wine lacks; msvcrt keeps refusing it in `setlocale()` and `_setmbcp()` as before. | Code page 54936 only, including `IsValidCodePage()` and `GetCPInfo()` now accepting it; other code pages are unchanged. |
| `0120` | Build option: dsound is compiled with SSE floating point math. | One module only: the i386 build mixes in single precision like the x64 build and uses the SSE resampler loops. |
| `0122` | Logs a failed mmdevapi driver call before the existing assertion. | Diagnostic only; the assertion is unchanged. |
| `0123` | Limited guard against a nested fault when an asynchronous I/O status block is no longer writable; it only replaces a fault that kills the thread or fails the wait. | Skips only a block seen to be unwritable when checked; a release, decommit or protection change before or while it is written still faults. A guard page is skipped and left armed. A completion inside a wait now completes instead of failing the wait. Windows has not been measured. |
| `0124` | Reports the macOS task footprint as the commit charge (`PagefileUsage`, and `PrivateUsage` through `ProcessVmCounters`), which is what Windows defines it as, instead of address space. | That counter and its peak only; `VirtualSize` and the working set are unchanged, and without `TASK_VM_INFO` or when the query fails the old values stay. The footprint approximates, not equals, the Windows private commit. |
| `0125` | A filtered `CredEnumerateW()` returns matching registry credentials when the macOS Keychain has none, as Windows returns stored credentials. | Only `ERROR_NOT_FOUND` from the host store is forgiven, as `ERROR_NOT_SUPPORTED` already was; `ERROR_NOT_FOUND` is still returned when nothing matches anywhere. |
| `0126` | Lets a signed app bundle hold the loader alone, which macOS needs to give Wine programs Game Mode: the loader first loads `ntdll.so` next to the symlink it was started through. | Only when the loader's resolved directory is an app bundle's `Contents/MacOS`; the packaged layout is the switch. Any other layout, and a failed lookup, take the unchanged path. |
| `0127` | Keeps the per-program application menu name (CrossOver Hack 13438) under the `0126` layout by copying it from the loader's embedded `Info.plist` into the main bundle's info dictionary before AppKit starts. | Only when the main bundle is an app bundle and the embedded copy can be read; nothing else changes. |

## Release policy

Patch sets use immutable identifiers such as `cx26.3-wine11.0-runeon.1`. The Git tag, source bundle, runtime component metadata, and Runeon release documentation must reference the same identifier. Publish a new artifact to Dev first and complete readiness, download, and product smoke tests. Production may promote only the exact verified bytes; do not rebuild directly for Production.

The repository and bundles are public. After creating a candidate tag, use `build-source-archive-manifest.sh` to record the public Release URL, asset SHA/size, and `prerelease` state, then upload the patch-set bundle, complete corresponding source, and both `.sha256` files. Change the same immutable Release and manifest to `stable` only after the matching runtime actually enters Production. Historical tags and assets are retained permanently and must not be moved, replaced, or deleted.

The public repository contains only the LGPL Wine/CrossOver Wine lineage, patches, and build scripts. The Runeon App, user diagnostics, Steam/game files, Developer ID or service secrets, and D3DMetal/GPTK/Apple private components must never enter Git history, Actions artifacts, or Release assets. Before every public update, inspect the complete history, Release asset list, and Actions logs.
