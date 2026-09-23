# FlsGetValue2 candidate

[English](FLSGETVALUE2.md) | [Simplified Chinese](FLSGETVALUE2.zh-CN.md) | [Japanese](FLSGETVALUE2.ja.md)

`cx26.3-wine11.0-runeon.10` preserves `.9` and adds one downstream patch. Its immutable stable/latest Release and corresponding source support Dev and Production seed `2026.09.22`, with App gate `>=1.8 (0)`. Production reuses the exact signed Dev archive; tag, commit and all four assets are unchanged. The default source definition remains the historical `.9`; select `.10` explicitly.

## Cause and implementation

The shipped x86 and x64 kernelbase libraries lack `FlsGetValue2`. Callers that require that export cannot resolve it. This establishes an API gap, not the cause of a particular Runeon game crash.

[The corrected downstream implementation](https://github.com/dappermint/winecx/commit/e0aa380780b73e20fabcfe78fd42713b94929a53) uses `RtlFlsGetValue` without changing last error. The earlier alias to `FlsGetValue` is insufficient: that API clears last error on success and sets it on failure. Patch `0103` adds the kernelbase implementation, the kernel32 import and the public declaration. Existing FLS allocation, storage and callbacks are reused; no ntdll, server, WoW64 or graphics ABI changes are included. This is adapted downstream code, not a WineHQ backport.

## Validation

`tests/flsgetvalue2-probe.c` resolves both public exports and checks non-null/null values, invalid indices, last-error preservation, unchanged legacy behavior, thread/fiber isolation and cleanup callbacks. Compile it with both `x86_64-w64-mingw32-gcc` and `i686-w64-mingw32-gcc`, using `-O2 -Wall -Wextra`. Run each executable in an isolated clean prefix with the candidate runtime. The old `.9` runtime must fail on missing exports; `--legacy-getter` must fail on last-error assertions, demonstrating why aliasing is not a fix.

Run `scripts/static-check.sh` and `scripts/integration-check.sh <unpatched-source>` before building. The product repository owns the full runtime build and managed launch grant harness. Local evidence is recorded under product task T784; this document does not claim release acceptance.

Local validation on 2026-09-22: the complete Wine build passed with the corrected product pipe2 configuration. Both x86 and x64 passed the API probe in a clean prefix and in a copy of an old test prefix upgraded by the candidate. The old runtime reproduced both missing exports; all legacy-getter negative controls failed as expected.

Native Windows equivalence and an affected game have not been independently tested. The user reported no issues in Dev and authorized Production on 2026-09-23. This general feedback does not establish that every Steam/CEF or game scenario has been independently tested. The product repository records packaging, readiness and distribution evidence. No per-game exception or new compatibility claim is introduced. The product build must retain its macOS pipe2 feature override: SDK 27 can otherwise weak-link this unavailable symbol on older supported systems before Windows DLLs load.

```sh
export RUNEON_WINE_PATCHSET_DEFINITION=patchsets/cx26.3-wine11.0-runeon.10
scripts/static-check.sh
scripts/integration-check.sh /absolute/path/to/unpatched/wine
```
