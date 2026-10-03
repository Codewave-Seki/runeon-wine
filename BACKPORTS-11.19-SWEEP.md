# Wine 11.19 user-mode fix sweep

[English](BACKPORTS-11.19-SWEEP.md) | [Simplified Chinese](BACKPORTS-11.19-SWEEP.zh-CN.md) | [Japanese](BACKPORTS-11.19-SWEEP.ja.md)

> This English document is authoritative. The Chinese and Japanese documents are complete translations for convenience.

Candidate `cx26.3-wine11.0-runeon.15` adds `patches/upstream/0079-wine-11.19-targeted-fixes.patch` after `0110`. It squashes 87 WineHQ commits first released in `wine-11.19`, lists every commit with its author, and records the full commit IDs in the manifest. Test-only hunks are omitted.

## Selection

All 353 commits in `wine-11.18..wine-11.19` were considered with the rules of [the previous sweep](BACKPORTS-11.16-11.18-SWEEP.md):

- 100 commits touch `ntdll`, `server`, `win32u`, `winemac.drv`, `wow64`, `loader`, `configure`, `tools`, `libs` or other host drivers. Most are an OpenGL/EGL context refactor and `winemac.drv` header and logging cleanups. None is taken. The small fixes among them were checked one by one: the IPv6 `IPV6_MTU_DISCOVER` level fix (`fc5dd34cc3`) is in a branch that macOS does not build, because it has `IPV6_DONTFRAG`; the EGL pbuffer return (`802d3d49fb`) is not on the macOS path; zero group affinity (`0998bfa77a`) only makes a rarely used call fail; the aliased-HKL IME check (`a07e981c59`) changes `winemac.drv` keyboard handling and needs its own validation; the `NtReleaseSemaphore` signedness change (`f89a7ca541`, `3e182735ea`) changes a syscall signature.
- 29 commits change tests only.
- Of the 224 user-mode candidates, crash, memory-safety, leak and error-path fixes in DLLs that Steam, launchers, installers or games load were selected. Left out: controller identity (`winebus.sys`, `hidclass.sys`, `xinput1_3`), CD/DVD and SCSI IOCTLs in `mountmgr.sys`, and feature work such as D3D12-backed Media Foundation buffers, GDI+ vertical text, the DNS cache and hosts file, VBScript parser changes, `RoResolveNamespace`, the D2D1 color-management stub and `GetNextAsyncId`.

Of the selected commits, three that applied cleanly were removed after review:

| Removed | Reason |
|---|---|
| `c9723c90f1` setupapi | Halving the `delete_multi_sz_value()` buffer leaves its `WCHAR` pointer arithmetic unchanged, so the copy writes past the allocation. The bug is present in `wine-11.19` itself. The change only saves memory. |
| `6880117619` oleaut32 | Changes decimal rounding semantics, not a crash fix. |
| `5ff02cf188` mfplat | `MFCalculateImageSize()` failing for zero height makes decoder type negotiation in the base fail where it used to succeed, which risks video playback. |

Thirteen commits do not apply because the base differs and were not ported: the `rsaenh` HMAC series, which needs a hash implementation the base does not have (a single member that applied was reverted rather than taking part of a series), `secur32` `770f0b9c7b`, the hosts-file follow-up `8f6cb19433`, the Media Foundation clock and session fixes whose code differs in the base, `msado15` `57cebf5711` and the D3DX10 sprite fixes.

## Adaptations

| Commit | Adaptation |
|---|---|
| `60551e1d7b` msvcrt | Overflow checks before both aligned allocations; the base lacks a later shrink-padding block that the upstream context includes. |
| `105ced5bc1` msvcrt | The same `ARRAY_SIZE` change with different surrounding lines. |
| `53b5345251`, `37fd158bfc` mf | Combined: `Flush` resets only an active renderer that still has an audio client; the base has no position/pts fields. |
| `ee38ff1725` mf | The base lacks `intsafe.h`; the index is compared with `~(DWORD)0` instead of `DWORD_MAX`. |

## Validation

`scripts/static-check.sh` and `scripts/integration-check.sh` pass, and applying the series to the pinned archive reproduces the reviewed tree file for file. Every changed module, and every module that builds from its sources (`ucrtbase`, the `msvcr*` and `msvcp*` family, `d3dcompiler_*`, `d3dx9_*`, `d3dx10_43`, `d3dx11_43`, `comctl32_v6`), builds for i386 and x86_64. The only new low-level call is `RtlLookupFunctionEntry` in `cxx_frame_handler`, which the base implements. A complete product build and runtime smoke test are still required before tagging.

`upstreamAuditThrough` stays at `wine-11.15`. The base manifest records `upstreamSweepThrough: wine-11.19`.
