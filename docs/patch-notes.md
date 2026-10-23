# Patch notes

Design notes for downstream patches whose behaviour needs more than a patch header. Newer patches carry their rationale in the patch header and manifest entry instead.

<a id="dxgi-driver-version"></a>

## Optional DXGI driver version correction

Candidate `.11` includes independent LGPL-2.1-or-later `runeondxgi.dll` in `0105-dxgi-driver-version.patch`. It uses public COM/SetupAPI and Wine's GPU LUID property; it does not incorporate the unlicensed external shim.

Only successful `CheckInterfaceSupport` results equal to -1 are corrected, using one exact LUID match and the DXGI `VendorId` entry in the pinned Wine `win32u::driver_vendor_to_version` table. Other versions, HRESULTs, null outputs, ambiguous or unknown devices retain provider behavior. A bounded process-lifetime LUID/vendor cache avoids repeated device enumeration. Original COM objects are returned; provider tables are registered under a lock, the module is pinned, and page protection retains any execute permission while installing the hook.

**The product defaults this experimental correction off.** A local persistent preference may explicitly enable it; disabling restores the prefix's original DXGI and does not enable it again on subsequent preparation. The local key is `DXGIDriverVersionCorrectionEnabled` (absent/false means disabled); the unpublished negative key is no longer read. Intel `8086`, AMD `1002` and NVIDIA `10de` use `35.0.101.6314`, `35.0.21025.1024` and `35.0.15.6094` respectively. Unknown vendors keep -1. These Wine table values are synthesized compatibility versions, not actual driver/package versions; target-game acceptance has not been validated.

The App leaves the shared runtime `dxgi.dll` unchanged and installs the wrapper/provider pair only in the selected prefix. The wrapper retains its `runeondxgi.dll` export-library identity so Wine resolves the optional builtin. Ownership, destinations and required source files are checked before preparation, and the dependency is published before the wrapper in the existing prefix transaction. Initialization or repair by wineboot may replace the wrapper, so the App reapplies policy before per-game launch preparation only when enabled. Disabled first boot does not prepopulate DLLs. Disabling an installed wrapper restores the original on the next Steam launch, even when the seed no longer includes the optional module; subsequent disabled launches do not resync. Sync without a new fingerprint preserves the existing identity for the same runtime, avoiding false Steam web-cache invalidation. The unpublished runtime-wide prototype is restored from its preserved original.

The user-local Apple provider is renamed to `rxdg.dll` and uses the local matching unix alias. It is loaded at module attachment to preserve original provider initialization before a caller creates a D3D device. Provider load failure tries the bare module name and emits an explicit debug message; bare-name fallback did not resolve a missing prefix provider in the tested runtime and is not the safety boundary. Loading under the loader lock is a Wine-only compatibility constraint, not generally safe DllMain practice. Local checks validated sequential factory and device-first initialization on Apple 4.0b2 / M3 Max; a provider that starts and waits for loader-dependent threads could deadlock. Target-game validation must include concurrent device creation before enabling. Seed packaging excludes private providers and backups. This repository contains no Apple private binaries or per-game matching.

Build through the normal Wine build with the explicit `.11` definition. Policy check: `x86_64-w64-mingw32-gcc -std=c11 -I /path/to/patched/wine/dlls/runeondxgi tests/dxgi-driver-version.c -o /tmp/dxgi-policy.exe -lsetupapi -ldxguid -luuid`; also build with i686 and run through the product's authorized isolated Wine probe. It covers Intel/AMD/NVIDIA values, unknown vendors, LUID/vendor cache isolation, duplicate/missing devices, unchanged returns and executable page protection. App checks cover disabled prepare/launch/prepare fingerprints, retained htmlcache, first-boot no-op and real fingerprint-change invalidation.

Local checks: x64/i386 module builds, policy checks, App compilation, prefix scope, persistent disable, rollback and original/fixed/restored API checks pass. Factory calls and EnumAdapterByLuid are corrected. D3D11 device-first creation succeeds and preserves its original -1 result; it is not a covered correction entry point. EnumWarpAdapter preserves the original unsupported result. A delayed-provider-initialization regression was reproduced and fixed. No seed is published, and no game compatibility is claimed.

<a id="flsgetvalue2"></a>

## FlsGetValue2 candidate

`cx26.3-wine11.0-runeon.10` preserves `.9` and adds one downstream patch. Its immutable stable/latest Release and corresponding source support Dev and Production seed `2026.09.22`, with App gate `>=1.8 (0)`. Production reuses the exact signed Dev archive; tag, commit and all four assets are unchanged. The default source definition remains the historical `.9`; select `.10` explicitly.

### Cause and implementation

The shipped x86 and x64 kernelbase libraries lack `FlsGetValue2`. Callers that require that export cannot resolve it. This establishes an API gap, not the cause of a particular Runeon game crash.

[The corrected downstream implementation](https://github.com/dappermint/winecx/commit/e0aa380780b73e20fabcfe78fd42713b94929a53) uses `RtlFlsGetValue` without changing last error. The earlier alias to `FlsGetValue` is insufficient: that API clears last error on success and sets it on failure. Patch `0103` adds the kernelbase implementation, the kernel32 import and the public declaration. Existing FLS allocation, storage and callbacks are reused; no ntdll, server, WoW64 or graphics ABI changes are included. This is adapted downstream code, not a WineHQ backport.

### Validation

`tests/flsgetvalue2-probe.c` resolves both public exports and checks non-null/null values, invalid indices, last-error preservation, unchanged legacy behavior, thread/fiber isolation and cleanup callbacks. Compile it with both `x86_64-w64-mingw32-gcc` and `i686-w64-mingw32-gcc`, using `-O2 -Wall -Wextra`. Run each executable in an isolated clean prefix with the candidate runtime. The old `.9` runtime must fail on missing exports; `--legacy-getter` must fail on last-error assertions, demonstrating why aliasing is not a fix.

Run `scripts/static-check.sh` and `scripts/integration-check.sh <unpatched-source>` before building. The product repository owns the full runtime build and managed launch grant harness. This document does not claim release acceptance.

Local validation on 2026-09-22: the complete Wine build passed with the corrected product pipe2 configuration. Both x86 and x64 passed the API probe in a clean prefix and in a copy of an old test prefix upgraded by the candidate. The old runtime reproduced both missing exports; all legacy-getter negative controls failed as expected.

Native Windows equivalence and an affected game have not been independently tested. The user reported no issues in Dev and authorized Production on 2026-09-23. This general feedback does not establish that every Steam/CEF or game scenario has been independently tested. The product repository records packaging, readiness and distribution evidence. No per-game exception or new compatibility claim is introduced. The product build must retain its macOS pipe2 feature override: SDK 27 can otherwise weak-link this unavailable symbol on older supported systems before Windows DLLs load.

```sh
export RUNEON_WINE_PATCHSET_DEFINITION=patchsets/cx26.3-wine11.0-runeon.10
scripts/static-check.sh
scripts/integration-check.sh /absolute/path/to/unpatched/wine
```

<a id="gstreamer-video-pool"></a>

## GStreamer video pool fallback

Candidate `cx26.3-wine11.0-runeon.11` adds downstream patch `0106-winegstreamer-unfixed-caps-pool.patch`.

### Cause and implementation

`wg_video_buffer_pool_create` ignored the result of `gst_video_info_from_caps`. An allocation query may carry caps that cannot be described yet; the zeroed video info was then used and the query was answered with a pool advertising a size of zero, so a decoder could wait for a buffer that can never be allocated. The patch checks the result, releases the pool and returns NULL. The existing caller already treats NULL as "no pool offered" and falls back to GStreamer's default allocation.

This is [dappermint/winecx `9fcbfbe`](https://github.com/dappermint/winecx/commit/9fcbfbe12bd9e3d03787909d9488009dcdba0a90), an LGPL-2.1-or-later change to a Wine source file, adapted only in context: the 11.0 base lacks the later `output_plane_stride` argument. WineHQ master still ignores this return value, so it is not a release backport. No game-specific logic is included.

### Validation

The patch applies to the pinned base and is covered by `scripts/static-check.sh` and `scripts/integration-check.sh`. The winegstreamer unix library is compiled by the full Wine build. There is no local reproducer for undescribable caps, so this candidate only claims that the unchanged path keeps working; product validation should play existing video paths without regression. It does not claim that any game's video is fixed.

<a id="launch-prepare"></a>

## Steam launch preparation

Candidate `cx26.3-wine11.0-runeon.18` adds `patches/runeon/0118-steam-launch-route-preparation.patch` after `0117`.

### Behaviour

The patch adds the opt-in `RUNEON_LAUNCH_PREPARE_V1` to the Steam middleware selector that `0104` and `0111` use. With it set to `1`, every x64 or i386 executable created by `steam.exe` with a valid session grant and a nonzero `SteamAppId` runs the fixed helper `C:\runeon-input-source\runeon-input-source-launch.exe` in `launch-prepare` mode first. The helper only prepares; Wine waits for it to exit and then creates the original child with the same command line, creation flags and handles.

The caller uses the step to settle state that a child reads when it starts, such as its graphics route. The first process of a game then already sees the new state, instead of the state the session started with.

- The D3D12on7 preparation from `0111` is split into a directory lock and a helper run. One lock covers both preparations and the original creation, and route preparation runs first.
- The NW.js hand-off from `0104` replaces the child and keeps priority, so it gets no route preparation.
- The NW.js and D3D12on7 gates stay x64-only.
- Without the opt-in, or when the parent is not `steam.exe`, the grant or `SteamAppId` is missing, a token or extended startup information is passed, the image is neither x64 nor i386, or the helper is missing, creation is unchanged. A helper that cannot be started is skipped.
- Wine reads no game names, AppIDs or executable names.
- Steam's creating thread waits while the helper runs: up to 3 s when the caller does not take the request up, and up to the helper's own 75 s deadline when it does. This applies to every child `steam.exe` creates with a `SteamAppId`, not only a game's main executable. Wine ends a helper still running after 120 s; only a helper that can be neither waited for nor ended fails the creation, so a live writer is never passed.

### Validation

`tests/launch-prepare-check.py` compiles the patched selector and preparation functions with stubs. It covers the opt-in, x64 and i386 images, the NW.js and D3D12on7 interplay, command preservation, failure fallback, and that a helper runs only while the lock is held. `scripts/integration-check.sh` runs it for any tree with the opt-in; the earlier `tests/d3d12on7-launch-check.py` still covers patch sets without it. The check does not run Wine, the helper or a real Steam launch. Compilation is shown only by a complete i386/x86_64 WoW64 build.

<a id="nwjs-auto-launch"></a>

## Automatic NW.js launch candidate

Candidate `cx26.3-wine11.0-runeon.11` inherits `.10` and adds `0104-steam-nwjs-automatic-launch.patch`. Select it with `RUNEON_WINE_PATCHSET_DEFINITION=patchsets/cx26.3-wine11.0-runeon.11`. Historical definitions and the default patch set are unchanged.

Only process creation by `steam.exe` is considered, with the App session flag `RUNEON_NWJS_AUTO_V1=1`, a grant, a nonzero SteamAppId, an x64 PE and adjacent `nw.dll`, `nw_elf.dll` and `package.json`. The original command is routed to the fixed App-owned helper. Debug launches are left alone. If the first command argument differs from the actual executable, the wrapped command would exceed the Windows length limit, or allocation fails, creation is left untouched. No game ID or game executable name is matched.

The matching App/helper authenticates the local request and first checks actual runtime eligibility read-only, then checks the account, installation, command, protection, component, saves and user settings only for eligible games. It either launches the managed NW.js, explicitly passes through an ineligible original command, or reports preparation failure. The hook does not modify game files or live Steam configuration. Old Apps never set the capability flag; new Apps require the capability marker in both kernelbase architectures of the actual selected runtime, plus an installed helper and ready receiver, before enabling the flag and removing the old guidance.

Local development checks: the modified process translation unit compiles for i686 and x86_64; the App/helper protocol check covers managed launch, pass-through, denial, timeout and session cancellation. These are not a full Wine build or a real Steam/game launch. Build and distribute the matching runtime and App together through the existing release process; this candidate is not a published runtime.
