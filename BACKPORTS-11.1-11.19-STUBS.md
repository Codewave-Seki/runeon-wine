# Wine 11.1 to 11.19 compatibility stub review

[English](BACKPORTS-11.1-11.19-STUBS.md) | [Simplified Chinese](BACKPORTS-11.1-11.19-STUBS.zh-CN.md) | [Japanese](BACKPORTS-11.1-11.19-STUBS.ja.md)

> This English document is authoritative. The Chinese and Japanese documents are complete translations for convenience.

Candidate `cx26.3-wine11.0-runeon.18` starts from `.17` and adds `patches/upstream/0081` to `0087`. Earlier reviews of this range selected crash, memory-safety and error-path fixes and left feature work out. That also left out small commits that turn a failing stub into a successful one. An application that treats the failure as fatal stops there, so this review looks at that class separately.

## Selection

Two groups in `wine-11.0..wine-11.19` were reviewed one commit at a time against the patched `.17` tree, using forward and reverse application:

- the 18 commits whose subject starts with a module name followed by "Stub";
- the 8 commits whose message says which application needed them ("Needed by", "Needed for").

## Taken

| Patch | Upstream commits | Change |
| --- | --- | --- |
| `0081` | `f19a7df7c0` (11.1) | `IUISettings2::get_TextScaleFactor()` returns 1.0 instead of `E_NOTIMPL`. C++/WinRT turns `E_NOTIMPL` into an exception, and an uncaught one ends the process at start-up. |
| `0082` | `0b0bef31ea` (11.14) | `IUISettings2::add_TextScaleFactorChanged()` returns the dummy cookie the other event stubs in the file already use. |
| `0083` | `caba5e68e2` (11.9) | WMI `Win32_DiskDrive` gains `Signature` and `TotalHeads`. |
| `0084` | `2873b4d077` (11.5) | VBScript calls a named item's default property when it is called with arguments (Wine-Bug 53889). |
| `0085` | `d30dcd75bc` (11.14) | `PdhGetFormattedCounterArrayA/W` are implemented instead of spec stubs. |
| `0086` | `fbd19baa4f`, `20e2458ab3` (11.16) | JScript accepts Unicode identifier characters. The first commit is the refactor the second depends on. |
| `0087` | `22aedfab35`, `3864ce7fcb`, `cefdd9ebe6`, `d95e15dfe8`, `44b0da29ad`, `88ceeada6d` (11.4 to 11.15) | WinHTTP accepts four newer options and answers two queries instead of failing with `ERROR_WINHTTP_INVALID_OPTION`. |

`0081` to `0086` carry upstream code unchanged; upstream test changes are omitted. `0087` is adapted:

- The session options follow the WebSocket buffer options, because the base lacks the upstream `WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL` session case they follow.
- `WINHTTP_OPTION_HTTP_VERSION` validates and reports `sizeof(HTTP_VERSION_INFO)`. Upstream, still in `wine-11.19`, uses the size of the pointer, so a 32-bit caller can pass a 4-byte buffer that then receives 8 bytes.
- `WINHTTP_OPTION_DECOMPRESSION` and `WINHTTP_OPTION_ENABLE_HTTP2_PLUS_CLIENT_CERT` check the buffer length before they read it, as the other boolean options do. Upstream reads a NULL buffer passed with a zero length.
- The base has no WinHTTP decompression. Accepting the option does not request compressed responses, so responses stay uncompressed.

## Not taken

| Commits | Reason |
| --- | --- |
| `3f65f20cc2` ntdll, `339be36e64`, `94e5bbeb6f`, `2be7717684` win32u, `5b5e95719d` mountmgr.sys | ABI-sensitive modules, which these reviews do not change. |
| `8571f65844` ucrtbase, `5c281ca2b2` powrprof | Already carried by earlier patches in this series (`8571f65844` by `0079`, `5c281ca2b2` by `0063`). |
| `a33fbd48a0` wminet_utils | The base has no `wminet_utils` module. |
| `043bc5a0d4` windows.web | A new `JsonArray` class with no stated consumer; parsing arrived in later feature commits. |
| `fff6090795` webservices | Needed by a remote debugger, not by applications this runtime targets. |
| `ccdaa2b581`, `a164854c92` wined3d | Only change a Vulkan validation-layer message and a link dependency. |

## Validation

`scripts/static-check.sh` and `scripts/integration-check.sh` pass for `.18`, and the default definition still passes `scripts/static-check.sh`. Those checks prove that the series applies. A complete local i386/x86_64 WoW64 build of `.18` at this revision finished without errors on 2026-10-14; it has not been released.
