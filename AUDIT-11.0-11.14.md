# Wine 11.0 to 11.14 Stability Review

[English](AUDIT-11.0-11.14.md) | [Chinese](AUDIT-11.0-11.14.zh-CN.md) | [Japanese](AUDIT-11.0-11.14.ja.md)

> This English document is authoritative. The Chinese and Japanese documents are complete translations for convenience.

## Scope

The fixed comparison range is `wine-11.0..wine-11.14` against CrossOver 26.3 source archive SHA-256 `ac99c8ca4b3848f3e81784135f023df266b61c2345726ea55a50b3e030dd6872`.

The inventory contains 3,143 upstream commits: 2,373 automated candidates, 315 subsystem-sensitive commits, and 455 ABI-sensitive commits. Automated labels only route review. They do not establish safety or justify inclusion.

The `.2` review used two passes:

1. every non-ABI crash, use-after-free, out-of-bounds access, overflow, race, deadlock, null dereference, and resource leak signal;
2. 134 non-ABI correctness commits in Runeon-relevant runtime, network, crypto, COM, device, audio, input, codec, UI, and launcher modules.

Each shortlisted commit was checked with the same forward/reverse patch algorithm used by the release scripts. A commit entered the active series only after its CrossOver implementation, prerequisites, test changes, and regression surface were reviewed.

## Included in `cx26.3-wine11.0-runeon.2`

The patch set inherits the three VC14 commits, the adapted Escape `cfgmgr32` shutdown fix, and the Runeon CEF process patch from `.1`. It adds these 33 stability backports:

| First release | Upstream commit | Area | Result |
| --- | --- | --- | --- |
| 11.1 | `0706ffc06c18` | WindowsCodecs | Correct truncated GIF palette initialization |
| 11.1 | `e33a8cc4ba7d` | QASF | Guard unconnected pins and carry the upstream test |
| 11.2 | `a0a82c471b09` | QASF | Avoid a stopped-reader callback use-after-free race |
| 11.3 | `56a4347acac1` | CoreAudio | Reject a zero period instead of dividing by zero |
| 11.3 | `bb4ef1fbf7d8` | RichEdit | Bound tab leader indexing while copying text |
| 11.3 | `45190d46646b` | WindowsCodecs | Correct interlaced GIF output-buffer bounds |
| 11.3 | `352d6c94fd2a` | Winsock | Close duplicated socket handles correctly with the upstream test |
| 11.12 | `89b35d3f7079` | WindowsCodecs | Preserve a full GIF LZW table |
| 11.14 | `af25fb409cec` | WinINet | Prevent an empty-file-URL out-of-bounds read with upstream tests |
| 11.1 | `95fa88630ee4` | WindowsCodecs | Correct 64bpp RGBA to 32bpp BGRA conversion |
| 11.3 | `5f241ebda259` | UCRT | Correct scanf scanset ranges with the upstream test |
| 11.3 | `91905171217d` | Advapi32 | Release ReportEventA conversion buffers on failure |
| 11.4 | `8ca440beead4` | MSVCRT | Reject `_time32` overflow |
| 11.4 | `8845229a35a9` | MSVCP140 | Match native `_Schedule_chore` callback behavior with the upstream test |
| 11.4 | `a955417b8f7b` | MSVCR | Release the previous ExceptionPtr object |
| 11.6 | `9c7d674aa96c` | VBScript | Avoid ReDim null SAFEARRAY crashes with the upstream test |
| 11.7 | `1ed184e29eb9` | D3D11 | Release a video-device interface on decoder creation failure |
| 11.7 | `8d1c07977dc1` | Script Runtime | Correct the create-folder BSTR length |
| 11.7 | `8f15e858e311` | Quartz | Close the filter-graph completion event |
| 11.8 | `08dbf01aaa36` | NSI proxy | Handle `if_nameindex()` failure |
| 11.8 | `965a00c4b479` | NSI proxy | Avoid zero-sized interface allocation |
| 11.8 | `4781a7128e53` | IP Helper | Avoid an out-of-bounds read with no interfaces |
| 11.8 | `452d82e997fa` | ADO | Handle a provider returning no update status |
| 11.10 | `cb3de0f05be4` | WinINet | Release temporary authorization data |
| 11.12 | `c293e14307e7` | OleView | Clear selection before reload frees the tree |
| 11.12 | `fe4f614de6d5` | Regedit | Bound REG_MULTI_SZ scanning by value size |
| 11.13 | `3f4bff72fa8e` | URLMon | Fix cache-path character counts and terminator space |
| 11.13 | `632963fc4fbb` | MSHTML | Release Gecko attributes after creation |
| 11.13 | `3131eba5b352` | MSHTML | Release the collection reference on detach |
| 11.13 | `8e6d4b91001e` | MSHTML | Release Gecko attributes after DISPID lookup |
| 11.14 | `349d6af5b55b` | Crypt32 | Validate OID info sizes and carry the upstream test |
| 11.14 | `568aca4e2aad` | GDI+ | Reject point-count arithmetic overflow |
| 11.1 | `73e02c1e7f54` | CMD | Bound MKLINK paths; adapted to the legacy CrossOver parser |

The authoritative path, SHA-256, risk, full commit ID, adaptation note, and order for every patch are in [`patches/manifest.json`](patches/manifest.json).

## CrossOver equivalents or non-applicable defects

- `00f88cd59bb1` (`wineboot` null canonical name): CrossOver 26.3 already contains the exact macOS guard.
- `13a82c7468dd` (`joy.cpl` close hang): CrossOver 26.3 already contains the exact fix.
- `12154421e345` (MMDevAPI ASan copy): the CrossOver validator does not copy a short `WAVEFORMATEX` as a full extensible structure and validates `cbSize` before extended-field access; the upstream defect is absent in this layout.
- `e0663e8a283c` (SetupAPI ANSI buffer allocation): the CrossOver implementation uses a direct registry-query path and does not perform the affected wide-buffer allocation.
- `eef8e97dd335` (Crypt32 chain config): the CrossOver implementation validates `cbSize` before use and does not access the later exclusive fields on the affected path.

## Deferred, not silently dropped

- Media Foundation `f34dda8a56d0`: control flow and its regression test depend on later session changes not present in CrossOver 26.3.
- WinHTTP `a48b1ff36b61`: CrossOver uses a divergent stream implementation and the upstream commit adds no in-tree regression test; an applicable hunk alone is insufficient.
- SChannel `35b1e7eb9a7e`: the source hunk applies, but its matching test prerequisite does not; this batch does not accept a source-only backport.
- URI parser `8840f1dae9a7`: the 50-line safety fix depends on the surrounding URI canonicalization series and must be evaluated as one chain.
- Configuration Manager `b39db986571e`: upstream moved the implementation from SetupAPI to cfgmgr32; the legacy path requires a dedicated semantic test rather than a mechanical transplant.
- WinMM notification/leak fixes and the XInput controller series: both form ordered behavioral chains and need device reconnect and audio-session probes.
- Additional VBScript and MSXML crash fixes: several depend on parser/interpreter refactors; only the standalone ReDim fix is in `.2`.
- OpenGL synchronization and broad graphics/media changes: these require a dedicated D3DMetal/DXMT/DXVK regression batch.
- All ABI-sensitive changes to loader, server, WoW64, winemac, win32u, Unix protocols, or D3DMetal interfaces remain excluded by policy.
- WineAndroid, Wayland, X11, PulseAudio, ALSA, OSS, and BlueZ-only changes do not execute in Runeon's macOS runtime.

## Validation and release boundary

Static manifest/SHA checks, full sequential application, source markers, and the second-application fail-closed gate pass on the pinned CrossOver source. A complete Rosetta x86_64/WoW64 source build also passes, including compilation of the carried upstream test binaries. Direct execution of the affected Wine tests from the bare build did not reach their test cases: the test host stalled during Wine initialization without the seed runtime's MoltenVK and RPC service environment. That result is environmental and therefore neither a test pass nor a patch failure.

A candidate is not shipped merely because the source and compile gates pass. Affected runtime tests in the assembled seed, clean/existing-prefix probes, Steam lifecycle, VC14, Escape shutdown, audio/network, and graphics smoke must still pass through the Runeon product repository.

Until those product gates pass, `.2` is only a source candidate. It must not replace the Production `.0` source entry or be described as available to users.
