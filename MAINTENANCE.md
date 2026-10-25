# Upstream Maintenance Policy

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

## Review records

Release status lives in the [README](README.md) table, the GitHub Releases and `release-manifests/`; this file does not repeat it. Each upstream review is a section of [docs/upstream-reviews.md](docs/upstream-reviews.md), in English:

| Record | Scope | First patch set |
|---|---|---|
| [AUDIT-11.0-11.14](docs/upstream-reviews.md#audit-11.0-11.14) | Complete commit-by-commit review, wine-11.1 to 11.14 | `.2` |
| [AUDIT-11.0-11.15](docs/upstream-reviews.md#audit-11.0-11.15) | Extends the complete review through wine-11.15 | `.6` |
| [BACKPORTS-11.16-11.17](docs/upstream-reviews.md#backports-11.16-11.17) | Targeted wine-11.16/11.17 backports | `.9` |
| [BACKPORTS-11.16-11.18-SWEEP](docs/upstream-reviews.md#backports-11.16-11.18-sweep) | Filtered user-mode sweep, wine-11.16 to 11.18 | `.11` |
| [BACKPORTS-11.19-SWEEP](docs/upstream-reviews.md#backports-11.19-sweep) | Filtered user-mode sweep, wine-11.19 | `.15` |
| [BACKPORTS-11.1-11.19-STUBS](docs/upstream-reviews.md#backports-11.1-11.19-stubs) | Compatibility stubs, wine-11.1 to 11.19 | `.18` |
| [BACKPORTS-11.20-SWEEP](docs/upstream-reviews.md#backports-11.20-sweep) | Filtered user-mode sweep, wine-11.20 | `.23` |
| [MSYNC-FIX-SERIES](docs/upstream-reviews.md#msync-fix-series) | msync fixes from dappermint/winecx, opt-in through `WINEMSYNC` | `.25` |
| [GL-PRESENT-ON-SWAP](docs/upstream-reviews.md#gl-present-on-swap) | Show the OpenGL surface being swapped (wine-11.3/11.4), individually approved for `winemac.drv`/`win32u` | `.26` |
| [MACOS-FIX-BATCH-1](docs/upstream-reviews.md#macos-fix-batch-1) | First batch of upstream macOS fixes under the §3 exception (11 commits) | `.27` |

The complete audit boundary is `wine-11.15`; later releases were reviewed with the filtered sweep rules. An audit is a point-in-time decision record, not a permanent allowlist: a deferred commit may enter a later patch set once its dependency chain, tests, probe and Runeon impact are established, and inclusion in an audit does not authorize publishing a runtime.

Design notes for individual downstream patches are in [docs/patch-notes.md](docs/patch-notes.md) ([DXGI-DRIVER-VERSION](docs/patch-notes.md#dxgi-driver-version), [FLSGETVALUE2](docs/patch-notes.md#flsgetvalue2), [GSTREAMER-VIDEO-POOL](docs/patch-notes.md#gstreamer-video-pool), [LAUNCH-PREPARE](docs/patch-notes.md#launch-prepare), [NWJS-AUTO-LAUNCH](docs/patch-notes.md#nwjs-auto-launch)). New patches carry their rationale in the patch header and manifest entry instead.

Until 2026-10-24 each record and note was a separate root file with Chinese and Japanese translations; each section keeps its old file name as its anchor. Manifests of published patch sets still name the old root paths; those paths remain valid at each patch set's tag.

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
| `0124` | Reports the macOS task footprint as the commit charge (`PagefileUsage`, and `PrivateUsage` through `ProcessVmCounters`), which is what Windows defines it as, instead of address space. | That counter and its peak only; the peak is the kernel's footprint high-water mark where the kernel answers it (rev3), otherwise the current value. `VirtualSize` and the working set are unchanged; without `TASK_VM_INFO`, on failure or with an answer older than rev1 the old values stay. The footprint approximates, not equals, the Windows private commit. |
| `0125` | A filtered `CredEnumerateW()` returns matching registry credentials when the macOS Keychain has none, as Windows returns stored credentials. | Only `ERROR_NOT_FOUND` from the host store is forgiven, as `ERROR_NOT_SUPPORTED` already was; `ERROR_NOT_FOUND` is still returned when nothing matches anywhere. |
| `0126` | Lets a signed app bundle hold the loader alone, which macOS needs to give Wine programs Game Mode: the loader first loads `ntdll.so` next to the symlink it was started through. | Only when the loader's resolved directory is an app bundle's `Contents/MacOS`; the packaged layout is the switch. Any other layout, and a failed lookup, take the unchanged path. With `WINEDLLPATH` set and no preloader, CrossOver Hack 22144 starts children through a hard link outside the bundle; they run but get neither Game Mode nor the `0127` name. |
| `0127` | Keeps the per-program application menu name (CrossOver Hack 13438) under the `0126` layout by copying it from the loader's embedded `Info.plist` into the main bundle's info dictionary before AppKit starts. | Only when the main bundle is an app bundle and the embedded copy can be read; nothing else changes. Relies on that info dictionary being mutable in practice, which macOS does not document; if that changes, the call aborts instead of failing. |
| `0128` | Gives spatial audio static objects the channel order WAVEFORMATEXTENSIBLE defines, so a 7.1 or 7.1.4 bed no longer swaps its side and back channels or drops back centre. | The static object channel map only. |
| `0129` | Keeps a render stream's HAL I/O buffer under half its period, within the 10 ms period shared-mode clients are woken for, so a client keeping about one period queued is not played partly as silence. | Render streams whose buffer is larger than half a period. The buffer is per process and per device, so the same process's capture stream on a device that also renders gets it too, and several render streams leave the last value. Half a period is a measured choice that doubles the callback rate. Failure to read or set it leaves it unchanged. WineHQ 11.16 sets the whole period instead; a later backport of that replaces this patch. |
| `0130` | Answers the five GL_ARB_ES2_compatibility calls in contexts without the extension, as Windows drivers do in any context. | Only calls that returned nothing; a context that has the extension calls the same equivalents, and macOS lists no shader binary formats. |
| `0131` | Reports each running thread's own CPU times on macOS, as Windows does, instead of zero or the process's times. | Threads of the calling process; `ThreadTimes` skips exited threads, which report zero as before (Windows reports their final times), and other processes fail as before. The process list (`SystemProcessInformation`) now also reports the calling process's threads, without the exit guard. |
| `0132` | Answers app package lookups from the registry and registers the VCLibs framework package in `wine.inf`, as Windows has it. | The three lookup functions and the VCLibs entries, which do not replace existing values. |
| `0133` | Calls the application's media engine callback outside the engine lock, on the thread that raised the event and before its call returns, in the same order as before, so the callback cannot deadlock against the engine. | Event delivery only. An event raised under the lock is delivered when that call ends, so the callback sees the state at the end of the call; events after `Shutdown()` are dropped. Different threads' callbacks are no longer serialized and can run concurrently; `Shutdown()` does not wait for a callback running on another thread, and one past the shut-down check can still arrive. Events queued at destruction are freed undelivered. |
| `0134` | Stops winemac doubling `GetDeviceCaps(HORZRES/VERTRES)` in retina mode, so the screen DC reports the same size as `GetSystemMetrics`, `EnumDisplaySettings` and `DESKTOPHORZRES`, as on Windows. Its switch is Wine's own `RetinaMode` registry value, not one this repository defines. | Only while `RetinaMode` is on (`retina_on`); with it off, behaviour is unchanged. Only those two caps of the screen device; printer DCs do not go through winemac. |
| `0136` | msvcp100 `seekg()` keeps the state of a stream that has already failed, as native msvcp100 does, instead of clearing it to goodbit. | msvcp100 only, both overloads for char and wchar_t, only when the stream has already failed. msvcp60 to msvcp90 and msvcp110 and later are unchanged, and so are successful seeks. |

## Release policy

Patch sets use immutable identifiers such as `cx26.3-wine11.0-runeon.1`. The Git tag, source bundle, runtime component metadata, and Runeon release documentation must reference the same identifier. Publish a new artifact to Dev first and complete readiness, download, and product smoke tests. Production may promote only the exact verified bytes; do not rebuild directly for Production.

The repository and bundles are public. After creating a candidate tag, use `build-source-archive-manifest.sh` to record the public Release URL, asset SHA/size, and `prerelease` state, then upload the patch-set bundle, complete corresponding source, and both `.sha256` files. Change the same immutable Release and manifest to `stable` only after the matching runtime actually enters Production. Historical tags and assets are retained permanently and must not be moved, replaced, or deleted.

The public repository contains only the LGPL Wine/CrossOver Wine lineage, patches, and build scripts. The Runeon App, user diagnostics, Steam/game files, Developer ID or service secrets, and D3DMetal/GPTK/Apple private components must never enter Git history, Actions artifacts, or Release assets. Before every public update, inspect the complete history, Release asset list, and Actions logs.
