# Wine 11.20 user-mode fix sweep

[English](BACKPORTS-11.20-SWEEP.md) | [Simplified Chinese](BACKPORTS-11.20-SWEEP.zh-CN.md) | [Japanese](BACKPORTS-11.20-SWEEP.ja.md)

> This English document is authoritative. The Chinese and Japanese documents are complete translations.

Candidate patch set `cx26.3-wine11.0-runeon.23` adds `patches/upstream/0135-wine-11.20-targeted-fixes.patch` after `0134`. It squashes 37 WineHQ commits: 35 first released in `wine-11.20`, and two `wine-11.12` msvcp60 fixes that one of them depends on. The patch lists every commit with its author, and the manifest records the full commit IDs. Test-only hunks are omitted.

## Selection

All 342 commits of `wine-11.19..wine-11.20` were reviewed with the rules of the [previous sweep](BACKPORTS-11.19-SWEEP.md), against a tree made of the pinned CrossOver archive plus patches `0001`–`0134`:

- 50 commits change only tests.
- 80 commits change only `ntdll`, `server`, `win32u`, `winemac.drv`, the loader, `include`, the build tools or translations. Most are feature work: memory-reservation changes, EGL and D3DKMT work, ARM64EC relay support and the volume-unmount request. Each was read. Two small defects that the base demonstrably shares are taken (below). The rest are left out.
- Of the 212 user-mode candidates, the sweep takes crash, memory-safety, leak, hang and error-path fixes in DLLs that Steam, launchers, installers or games load. It leaves out feature series (the quartz color converter, DTD validation in msxml3, the dmime loop rework, `wminet_utils`, dsound mixing, `winebus` device identity, the jscript/mshtml cycle collector, vbscript parser changes, gdiplus antialiasing), version-resource bumps and the `cmd` word-iterator refactor.

Each taken commit applies to that tree in upstream order, which shows that the base still carries the unfixed code.

## Taken

| Area | Commits | Defect fixed |
|---|---|---|
| msvcp60 | `5e5ca9fefc`, `076438fa7c` (11.12), `794feb9289` | The wide-string `replace` functions moved and filled `sizeof(char)`-sized units, corrupting the string. The narrow and wide character `replace` also wrote the terminator through a NULL buffer when the result was empty. |
| msvcp60, msvcp90 | `3d27b08dde`, `c54fd1eac5`, `c9173a6c5d`, `fb997bf426`, `ad23bc1551` | `operator<<` and `operator>>` crash on a stream without a buffer instead of setting the bad bit. In msvcp60, `narrow` and `widen` took the locale from the stream buffer, so `std::endl` still crashed there; they now use the stream's own locale, as msvcp90 already does. |
| msvcp90 | `f011c203a0` | A bad locale name freed a pointer that was never allocated before throwing. |
| msvcp90 | `f0fef9fef7` | `_Strxfrm` passed the sort-key length as the source length, which over-read the source. |
| msvcp90 | `00469cf5cf` | `codecvt<char16_t>` produced a wrong surrogate pair for U+10000. |
| msvcp60 | `0740f54989` | `mbsrtowcs` never advanced the caller's source pointer. |
| msvcp90 | `bbf2ae736c`, `199cc0386e`, `19b112a917` | Correctness fixes found in the independent review: `codecvt<unsigned short>` facets ignored the requested locale; wide floating-point output keyed the decimal point on `showbase` instead of `showpoint`; the three `time_put` types reported `num_put` RTTI names. None is a crash fix; each is a one-line change whose defect the base shares. |
| vcruntime140_1 | `86b80d02b6` | C++ exception state was set up only in `DLL_THREAD_ATTACH`, so a thread created before the DLL loaded unwound with a wrong try level. |
| winegstreamer | `73cc740a01`, `d7c49769d9`, `33fba6fc26`, `52f8e6b5a5`, `774cb470c5`, `90e98d3d94` | A NULL transform dereference on drain and flush; a failed media-type conversion reported as success, leaving the transform unset; a failed WMA output type ignored; a leaked async result on finalize; an uninitialised stream count; an unchecked allocation. |
| d3d8 | `6f14a8ea45` | Releasing a texture with no references wrapped the count around instead of returning 0, as Windows does. Inferred consequence: a loop that releases until the count reaches zero would not end. |
| d3dx10, d3dx11 | `547e01858d`, `16c13088f9` | An explicit width or height was not rounded up to the block size of compressed formats, so texture creation failed. |
| dinput | `9eb4614ad7` | Stopping the input thread from another module's `DllMain` waited for the thread to exit, which the loader lock prevents: a hang at game exit. |
| wininet | `f564234cb2`, `436ec312c3` | Option queries dereferenced a NULL size pointer (Wine-Bug 58418). |
| crypt32 | `bf9b5e567c` | An invalid time returned from inside a `__TRY` block and left the exception frame registered. |
| dmband | `146fbbe78b` | A band item without a band added an uninitialised pointer to the track. |
| dmusic | `43b83b580f` | An instrument name conversion was given a byte count as its WCHAR capacity. |
| winmm | `0e7224e852` | When `midiStreamOpen` failed, it freed the stream but left it in the stream tree, so a later lookup read freed memory. |
| hnetcfg | `baa7c5d335` | `NetFwPolicy2` creation returned a freed object on failure. Steam's web helper uses this DLL; Runeon's Steam logs show its `fwpolicy2` FIXME lines. |
| wintypes | `07c33a0e92` | Clearing a map entry added a reference instead of releasing it. |
| kernelbase | `0875da63b7` | Error path: `UpdateProcThreadAttribute` failed for `PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY`, which Windows accepts. `CreateProcess` still ignores the attribute with a FIXME. No launcher is known to need it. |
| ntdll | `81e52628b2` | `RtlWakeAddressSingle` and `RtlWakeAddressAll` published the dequeued state (`entry->addr = NULL`) before removing the node, without ordering. A waiter woken by a timeout or a spurious alert reads the cleared address without the lock and returns, so the waker's `list_remove` and `entry->tid` read touch a dead stack frame. That can corrupt the wait queue (a lost wake-up, a hang or a crash) or wake the wrong thread. SRW locks, condition variables and `WaitOnAddress` are built on these functions. |
| winemac.drv | `48e2d50404` | `macdrv_GetWindowStyleMasks()` cleared its `ex_style` argument instead of the output mask, then OR'd bits into the uninitialised mask. |

`msvcp90/ios.c` and `msvcp90/locale.c` are also built into the `msvcp100`–`msvcp140` builtins, so the msvcp90 rows apply to them too.

`81e52628b2` and `48e2d50404` touch `ntdll` and `winemac.drv`, which this repository treats as ABI-sensitive and leaves out by default. The maintainer approved both as individual exceptions for this patch set on 2026-10-23; they do not widen the rule for later sweeps. Each is a small fix whose defect is present in the base: CrossOver's `RtlWaitOnAddress` and `macdrv_GetWindowStyleMasks()` match the upstream code before the fix. Neither changes a Unix-call ABI, the server protocol or a data structure shared across modules.

## Left out after review

| Commit | Reason |
|---|---|
| `365e610147` ddraw | Fixes a crash from an uninitialised `dwVertexCount`, but does so by moving execute-buffer staging from `SetExecuteData` to every `Execute` and copying the whole buffer. That is a behaviour change for every execute-buffer game, not a contained fix. |
| `8ab37b5ee3` user32 | Makes `CharToOemBuffW` write up to twice as many bytes, to match Windows. It changes how much of the caller's buffer is written and fixes no crash. |
| `bce98257e7`, `264434d93d` dmusic | Make band parsing fail where it used to continue, as Windows does. These are behaviour changes. |
| `d26a2d2deb`, `331cb04791` winegstreamer, `f6c606ea2c` dmusic, `70aeffabea` fusion | Remove dead assignments or use an equivalent macro, so behaviour does not change. |
| `3d6505bfa0`, `66f0fbf9aa`, `5e0589a44f`, `30a0d6cf0d` bcrypt | Fix the SymCrypt RSA path, which the base does not have; the base uses GnuTLS. |
| `4e02394aa1` comsvcs | The base's `holder_AllocResource` does not have the leaked allocation. |
| `adb2b0d59a` cmd | A leak on a `cmd` parse error that depends on the word-iterator refactor, which is left out. |
| `eba89375a0` ntdll | Changes the thread-affinity protocol between ntdll and the server. |
| `1fa894d7a4`, `ba392e9016`, `505f126cc3` | New stubs and exports. No Steam title is known to need them; the first is needed by the Microsoft remote debugger. |

## Known risk

`9eb4614ad7` dinput now waits for an event that the input thread sets on its way out, instead of the thread handle. If the input thread has already ended through its unexpected-termination path, the stop message cannot be delivered and the wait never returns. Before the change, the wait returned at once. Upstream has the same behaviour. That path is rare, so the commit is taken unchanged, and the risk is recorded here.

`48e2d50404` turns `ex_style_mask` from an uninitialised value into a defined one, which changes the visible-rectangle calculation for decorated windows. The runtime check therefore includes the geometry of a window with a title bar, such as Steam.

## Adaptations

| Commit | Adaptation |
|---|---|
| `146fbbe78b` dmusic | Applied to `dlls/dmband/bandtrack.c`. Upstream moved the band track to dmusic in `220b4e7ff6`, which is not taken. The base's code is otherwise identical. |
| `48e2d50404` winemac.drv | The same one-line change. In the base it sits below CrossOver's Quicken hack, so the hunk is offset. |

## Validation

`scripts/static-check.sh` and `scripts/integration-check.sh` pass for `.23`. Every commit applies in order without fuzz, except the two adapted ones. The complete build and the runtime checks are recorded in the T948 entry of the Runeon task list, and are required before `.23` is tagged.

`upstreamAuditThrough` stays at `wine-11.15`. The base manifest records `upstreamSweepThrough: wine-11.20`.
