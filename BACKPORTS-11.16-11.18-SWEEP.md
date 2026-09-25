# Wine 11.16–11.18 user-mode fix sweep

[English](BACKPORTS-11.16-11.18-SWEEP.md) | [Simplified Chinese](BACKPORTS-11.16-11.18-SWEEP.zh-CN.md) | [Japanese](BACKPORTS-11.16-11.18-SWEEP.ja.md)

> This English document is authoritative. The Chinese and Japanese documents are complete translations for convenience.

Candidate `cx26.3-wine11.0-runeon.11` adds four upstream patches after `0106`:

| Patch | Contents |
|---|---|
| `0062-wine-11.16-targeted-fixes.patch` | 20 WineHQ commits first released in `wine-11.16` |
| `0063-wine-11.17-targeted-fixes.patch` | 107 commits from `wine-11.17` |
| `0064-wine-11.18-targeted-fixes.patch` | 76 commits from `wine-11.18` |
| `0065-wine-11.18-imm32-ime-keydown-lparam.patch` | `a6b3099042` adapted to the CrossOver IME code |

Each squashed patch lists every upstream commit with its author. The manifest records the full commit IDs. The 13 commits already carried by `0045`–`0061` are not repeated. The only production adaptation is in `0064`: `d22810d47b` includes `intsafe.h`, which this base lacks, so `bmpdecode.c` defines a local `UIntMult()` with the same overflow contract. The full product build found this; clean patch application does not.

## Selection

All 1,009 commits in `wine-11.15..wine-11.18` were considered. A commit qualified only if it satisfied all of these rules:

- It changes user-mode DLLs that Steam, launchers, installers or games load. These include media, input, audio, COM, networking and TLS, crypto, WMI, text, image and D3DX, the D3D9 path, installers and UI controls.
- It changes nothing in `ntdll`, `server`, `win32u`, `winemac.drv`, `wow64`, `loader`, `configure`, `tools` or `libs`. It also leaves the server protocol header alone.
- It applies without fuzz, in upstream order, on top of patches `0001`–`0106`. The complete series reproduces the reviewed tree exactly.

Of the 271 commits that applied, the following were then removed because a textually clean apply would still change behaviour incorrectly on this base:

| Removed | Reason |
|---|---|
| NUMA (`073a4edad7`, `6ea56eb539` and FIXME companions) | They call `SystemNumaProcessorMap`, which this base's ntdll does not implement. A stub that returned success would start failing. |
| UI-language reimplementation (`5b4f8aa35b`, `480b8e4601`, `fa7d1b1571`, `6c91a3f4e7`, `74858bee70`, `065f1ad633`), console input rewrite `8126bd7f9c` | Global locale/console behaviour built on excluded ntdll work. Game language detection must not change silently. |
| PnP identity series (`hidclass.sys`, `winebus.sys` compatible IDs, `mmdevapi`/`coreaudio` instance IDs, `IRP_MJ_CREATE` handlers, setupapi→cfgmgr32 rewrites) | Depends on excluded `ntoskrnl`/`mountmgr` changes. It changes the device identities that controllers and games enumerate. |
| `msvcrt` std-handle series, `user32` object security descriptors | Process-wide stdio/object creation behaviour, partly dependent on server changes. |
| D3D11 `Discard*`/resource-sharing and related wined3d refactors, `WINED3D_TEXTURE_GENERATE_MIPMAPS` and the dependent d3d9 fix, D3DX10 sprite implementation, quartz fullscreen emulation, WMA decoder and wg_parser flag/PTS series | Feature or refactor series that change graphics, video or windowing behaviour without a Runeon need, or depend on parts of their chain that were excluded. |
| secur32 LSA/negotiate changes | Belongs to the excluded msv1_0/kerberos/lsass work. The SChannel and generic fixes in secur32 are kept. |
| `mfreadwrite` `4d22860530` | Belongs to the async-command refcount series deferred in `.9`. |
| `PackageFullNameFromId` (`ef6c413c68`) | Needs package-length constants from a later `appmodel.h`; found by the full build. UWP package APIs are not used by Steam games. |
| Minor toolbar, shell view and volume-label changes | No benefit to Steam or games. |

The previously deferred CoreAudio period, WGI initialisation, XAudio2 unlock and MF drain/lifecycle commits stay deferred for the reasons in [BACKPORTS-11.16-11.17.md](BACKPORTS-11.16-11.17.md).

Upstream revert `e5abb7c03a` would remove the x64 `RaiseException` path that preserves non-volatile registers. That path exists in this base, and its comment notes that some DRM depends on it. The revert is motivated by Windows 11 25H2 test results, not by a Wine defect, so it is not carried.

## Not applicable to this base

Several fixes target code that does not exist in CrossOver 26.3. Examples are the msxml3 XPath engine, WinHTTP `WINHTTP_OPTION_SERVER_CBT`, the SymCrypt-based bcrypt/rsaenh paths, the newer d2d1 command-list and dsound resampler code, and cfgmgr32. The wined3d swapchain mutex fix and the wg_parser initial-gap fix repair failure modes this base does not have. None of these were adapted.

## Validation

`scripts/static-check.sh` and `scripts/integration-check.sh` pass. Rebuilding the tree from the pinned archive plus the series matches the reviewed tree file for file. Every added low-level call was checked: only generic heap and critical-section functions are introduced. A full product Wine build is part of the local seed candidate. This sweep does not claim that any particular game issue is fixed.

`upstreamAuditThrough` stays at `wine-11.15`, because this is a filtered sweep and not a complete audit. The base manifest records `upstreamSweepThrough: wine-11.18` instead, so the weekly upstream watch alerts again only when a newer tag appears.
