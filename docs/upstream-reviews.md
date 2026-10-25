# Upstream reviews

One section per upstream review, oldest first. Each records what was reviewed, what entered which patch set, what was excluded and why, and the limits of its validation. Patch sets themselves are listed in the [README](../README.md#release-history).

<a id="audit-11.0-11.14"></a>

## Wine 11.0 to 11.14 Stability Review

### Scope

The fixed comparison range is `wine-11.0..wine-11.14` against CrossOver 26.3 source archive SHA-256 `ac99c8ca4b3848f3e81784135f023df266b61c2345726ea55a50b3e030dd6872`.

The inventory contains 3,143 upstream commits: 2,373 automated candidates, 315 subsystem-sensitive commits, and 455 ABI-sensitive commits. Automated labels only route review. They do not establish safety or justify inclusion.

The `.2` review used two passes:

1. every non-ABI crash, use-after-free, out-of-bounds access, overflow, race, deadlock, null dereference, and resource leak signal;
2. 134 non-ABI correctness commits in Runeon-relevant runtime, network, crypto, COM, device, audio, input, codec, UI, and launcher modules.

Each shortlisted commit was checked with the same forward/reverse patch algorithm used by the release scripts. A commit entered the active series only after its CrossOver implementation, prerequisites, test changes, and regression surface were reviewed.

### Included in `cx26.3-wine11.0-runeon.2`

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

The authoritative path, SHA-256, risk, full commit ID, adaptation note, and order for every patch are in [`patchsets/cx26.3-wine11.0-runeon.9/manifest.json`](../patchsets/cx26.3-wine11.0-runeon.9/manifest.json).

### CrossOver equivalents or non-applicable defects

- `00f88cd59bb1` (`wineboot` null canonical name): CrossOver 26.3 already contains the exact macOS guard.
- `13a82c7468dd` (`joy.cpl` close hang): CrossOver 26.3 already contains the exact fix.
- `12154421e345` (MMDevAPI ASan copy): the CrossOver validator does not copy a short `WAVEFORMATEX` as a full extensible structure and validates `cbSize` before extended-field access; the upstream defect is absent in this layout.
- `e0663e8a283c` (SetupAPI ANSI buffer allocation): the CrossOver implementation uses a direct registry-query path and does not perform the affected wide-buffer allocation.
- `eef8e97dd335` (Crypt32 chain config): the CrossOver implementation validates `cbSize` before use and does not access the later exclusive fields on the affected path.

### Deferred, not silently dropped

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

### Validation and release boundary

Static manifest/SHA checks, full sequential application, source markers, and the second-application fail-closed gate pass on the pinned CrossOver source. A complete Rosetta x86_64/WoW64 source build also passes, including compilation of the carried upstream test binaries. Direct execution of the affected Wine tests from the bare build did not reach their test cases: the test host stalled during Wine initialization without the seed runtime's MoltenVK and RPC service environment. That result is environmental and therefore neither a test pass nor a patch failure.

A candidate is not shipped merely because the source and compile gates pass. Affected runtime tests in the assembled seed, clean/existing-prefix probes, Steam lifecycle, VC14, Escape shutdown, audio/network, and graphics smoke must still pass through the Runeon product repository.

Until those product gates pass, `.2` is only a source candidate. It must not replace the Production `.0` source entry or be described as available to users.

<a id="audit-11.0-11.15"></a>

## Wine 11.0 to 11.15 Stability Review

### Scope

The fixed comparison range is `wine-11.0..wine-11.15` against CrossOver 26.3 source archive SHA-256 `ac99c8ca4b3848f3e81784135f023df266b61c2345726ea55a50b3e030dd6872`.

The inventory contains 3,350 upstream commits: 2,520 automated candidates, 340 subsystem-sensitive commits, and 490 ABI-sensitive commits. Automated labels only route review. They do not establish safety or justify inclusion.

Decisions for `wine-11.0..wine-11.14` are unchanged and remain recorded in [`AUDIT-11.0-11.14.md`](#audit-11.0-11.14). This document adds the incremental `wine-11.14..wine-11.15` round, which the scheduled upstream watch surfaced when `wine-11.15` was published on 2026-08-08.

### The incremental range

`wine-11.14..wine-11.15` contains 207 commits: 147 automated candidates, 25 subsystem-sensitive commits, and 35 ABI-sensitive commits. 39 of them touch only test files.

Every commit was checked with the forward/reverse patch algorithm used by the release scripts. A forward-clean result proves the base still carries the unfixed code and that CrossOver has no equivalent; a reverse-clean result would prove the fix is already present. Commits that conflicted were read against the CrossOver implementation before any decision.

### Included in `cx26.3-wine11.0-runeon.6`

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

The authoritative path, SHA-256, risk, full commit IDs, and order for every patch are in [`patchsets/cx26.3-wine11.0-runeon.9/manifest.json`](../patchsets/cx26.3-wine11.0-runeon.9/manifest.json).

#### Test backports

Seven of the nine patches carry upstream test changes. Three needed the upstream test prerequisite as well:

- The listbox patch carries `ab21fbc49a25` and `dd7f58d774` — the commits that introduce `test_LB_SETTOPINDEX` in both test suites — because the base has no such test to un-`todo_wine`. Both prerequisites are inside the same release window and apply cleanly.
- The Crypt32 test was rewritten to the base test file layout: `test_PFXImportCertStore_trailing_zeros()` is inserted after `test_PFXImportCertStore()` rather than after the later `_sha256_signing` variant, which does not exist in the base, and is registered in `START_TEST` at the matching position. The test body is unchanged.
- The Win32u, MFReadWrite `9a26041b38d8`, Video Resizer, and CMD commits ship no upstream test. They are one- and two-line defect fixes in code with no in-tree test coverage at either revision.

#### Adaptations

Three patches deviate from the upstream text:

- `bb33d1d34204`: the helper is named `update_media_type_from_upstream()` in the base and `update_media_type()` upstream. The rename is the entire adaptation; the control flow, the media type copy, and its release are upstream's.
- `d4b32af24629`: `open_cert_store()` gained a `pgnutls_pkcs12_verify_mac()` call after 11.0, so the hunk context differs. The new `pfx_der_outer_length()` helper is transplanted verbatim inside the `SONAME_LIBGNUTLS` block and the two-line guard is placed at the same point relative to `pgnutls_pkcs12_import()`.
- `cd0d7910b23b`: the upstream commit does not touch `dlls/secur32/Makefile.in` because upstream secur32 already reaches `BCryptHash` through the `ncrypt` delay import that `f4a9724c89` added for mTLS. The pinned base has no such import, so the PE link fails with `undefined reference to BCryptHash`. Rather than pull in the unrelated mTLS commit, the patch adds `bcrypt` to `DELAYIMPORTS` directly. This is a build-graph adaptation only; the C code is upstream's.

#### ABI-sensitive exception

`702e44767a8e` touches `dlls/win32u`, which the risk policy marks ABI-sensitive and excludes by default. It is admitted as a documented exception: the change is a single initialiser, `ARRAY_SIZE(vsc_to_vk)` to `ARRAY_SIZE(vsc_to_vk) - 1`, in a static keyboard table. It alters no structure layout, no interface, and no Unix boundary, and it fixes an out-of-bounds read found by ASan. No other ABI-sensitive commit in this range is included.

### CrossOver equivalents or non-applicable defects

- `0fde54a3574a` (wined3d frame latency): CrossOver 26.3 does not use the upstream `frame_latency_semaphore` path at all. It carries its own `pending_presents` / `waiting_for_present` / `present_event` / `max_frame_latency` implementation in `dlls/wined3d/cs.c`, and that implementation already releases the wined3d mutex around the wait. The multi-threaded D3D9 stall the upstream commit removes does not exist in this base.
- `852e39a89c0d` (host window state lock): `pGetWindowStateUpdates` is implemented only by `winex11.drv` in the pinned base. `winemac.drv` provides no such callback and the unlock protocol the fix relies on lives in the X11 driver, so the race cannot occur on the macOS path.
- `95e5d76a1882` and `0aa733c17aff` (wined3d planar CPU blits): part of the Vulkan H.264 decoder chain, which Runeon does not execute because video decoding runs through D3DMetal and DXVK.
- `bfe591cbef5b` (WindowsCodecs bound check): the check it removes was added by `2e68942fce46` inside this same release window and never reached the pinned base. The pair is a no-op here.
- The four `winebus.sys` commits in this range drive `hidraw`, `inotify`, and `evdev` code paths. macOS controller enumeration runs through IOHID, so none of them execute in Runeon.
- The `winex11.drv`, `winewayland.drv`, glibc `sa_restorer`, and ARM64EC/ARM WoW64 commits do not run on the macOS x86_64 and arm64 targets.

### Deferred, not silently dropped

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

### Validation and release boundary

Static manifest and SHA-256 checks pass. The complete 46-patch series applies sequentially to the pinned CrossOver source, the source marker is written, and the second-application fail-closed gate holds.

A native arm64 build of the patched tree with tests enabled compiled every patched source file and every backported test file, and `SONAME_LIBGNUTLS` resolved to `libgnutls.30.dylib`, so patches 0040 and 0044 are live rather than inert. That build also caught the missing `bcrypt` delay import recorded above.

Three failures in that build are environmental and unrelated to this patch set: `dlls/winemac.drv/cocoa_window.m` cannot see `WineMetalLayer` without the product D3DMetal configuration, `dlls/win32u/vulkan.c` needs `SONAME_LIBVULKAN` from a MoltenVK install, and both belong to the product build environment. No patch in the series touches either file.

One verification item is open and must be closed by the build gate before `.6` advances: a full Rosetta x86_64/WoW64 build in the documented product configuration, including the D3DMetal and MoltenVK dependencies, has not been run for `.6`.

A candidate is not shipped merely because the source gates pass. Affected runtime tests in the assembled seed, clean and existing-prefix probes, Steam CEF, stop/relaunch, D3DMetal overlay, and DXMT/DXVK smoke must still pass through the Runeon product repository.

Until those product gates pass, `.6` is only a source candidate. It must not replace the Production source entry or be described as available to users.

<a id="backports-11.16-11.17"></a>

## Targeted Wine 11.16/11.17 backports

Review date: 2026-09-13. `cx26.3-wine11.0-runeon.9` is the stable public source Release used by Dev and Production seed `2026.09.13` (promoted on 2026-09-14), based on the same pinned CrossOver 26.3 / Wine 11.0 archive as `.8`. It preserves all 47 existing patch files and adds 10 files carrying 13 upstream commits: 57 files in total, comprising 54 upstream backports and three unchanged product patches. No ABI-sensitive or product-policy code is changed by this increment.

This is a targeted review of selected Wine 11.16/11.17 changes, not a complete review of the 628 commits since 11.15. The complete audit boundary remains `wine-11.15`; neither `upstreamAuditThrough` nor the historical audit reports are advanced to silence the upstream-watch workflow. Shipped runtime status remains in [README](../README.md).

### Candidate contents

Equivalent CrossOver implementations and prerequisites were checked against the existing patched baseline. Benefits below describe the repaired API contracts; they are not claims that a particular Runeon game issue has been reproduced or fixed. These changes apply only where the corresponding Wine DLL is used. In particular, the D3D10 changes concern the open Wine bridge and do not modify D3DMetal or DXMT.

| Patch | First Wine tag / upstream commits | Behavior |
|---|---|---|
| [0045](../patches/upstream/0045-wine-11.16-dinput-action-device-guid.patch) | `wine-11.16`: [`2a43c4b92910`](https://github.com/wine-mirror/wine/commit/2a43c4b9291083f8892f2100051131a87278e656), [`72ffda26fa71`](https://github.com/wine-mirror/wine/commit/72ffda26fa71d42ba01e6908f39f5972b238e1c0) | Match action app data by both device GUID and object ID; prevents keyboard and mouse actions with the same ID from sharing the wrong value. |
| [0053](../patches/upstream/0053-wine-11.16-d3d10-stateblock-disabled-byte-fields.patch) | `wine-11.16`: [`2d98b4fb0ae4`](https://github.com/wine-mirror/wine/commit/2d98b4fb0ae49aa22c735eda6f221da91b4bc3cb), [`a5c665c30fe3`](https://github.com/wine-mirror/wine/commit/a5c665c30fe3d1ba6748680e68e77a564d58a9d4) | Honor the low bit of all byte-sized state-block mask fields during Capture/Apply; disabled states stay disabled. This adds no D3D feature level. |
| [0054](../patches/upstream/0054-wine-11.17-winhttp-connection-query-buffer-length.patch) | `wine-11.17`: [`df71145f22f1`](https://github.com/wine-mirror/wine/commit/df71145f22f15e723697675e3beaf75094247c6c) | Give Connection and Proxy-Connection queries independent buffer capacities; the first oversized header cannot enlarge the capacity used for the second. |
| [0055](../patches/upstream/0055-wine-11.17-winhttp-pac-scheme-host-bounds.patch) | `wine-11.17`: [`03f4957cacee`](https://github.com/wine-mirror/wine/commit/03f4957caceeb8404a119f6b8bd8cc0e76dcb76e) | Bound the PAC lowercase-host buffer and defend the scheme buffer. Oversized schemes already fail earlier in this baseline; the reachable repair is the hostname copy. |
| [0056](../patches/upstream/0056-wine-11.17-secur32-schannel-invalid-context.patch) | `wine-11.17`: [`081d8b1dd9ff`](https://github.com/wine-mirror/wine/commit/081d8b1dd9ff4bdd067c54bb50d396e34c334c70) | Return SEC_E_INVALID_HANDLE for missing or wrong-type SChannel context objects before encryption/decryption. This does not solve arbitrary provider-pointer corruption or concurrent deletion. |
| [0057](../patches/upstream/0057-wine-11.17-windowscodecs-nonnull-stream-read-count.patch) | `wine-11.17`: [`111e5197390a`](https://github.com/wine-mirror/wine/commit/111e5197390aa008789b002222024229fa2b82cf) | Always pass a non-NULL read count to IStream and normalize successful short reads to S_FALSE. This covers the JPEG metadata path and the shared codec helper. |
| [0058](../patches/upstream/0058-wine-11.17-dwrite-ttc-header-read-size.patch) | `wine-11.17`: [`a6fc12e4a94b`](https://github.com/wine-mirror/wine/commit/a6fc12e4a94bf4dae2d5c3a297794107627dad0a) | Request the complete TTC header instead of pointer-sized data from DirectWrite font streams. This fixes fragment bounds, not text shaping. |
| [0059](../patches/upstream/0059-wine-11.17-d3d10-device-adapter-failure-reference.patch) | `wine-11.17`: [`5ae631904a4b`](https://github.com/wine-mirror/wine/commit/5ae631904a4b4a253e22408e5cfe107c8ebb33b9), [`0e2ee8ab3e8c`](https://github.com/wine-mirror/wine/commit/0e2ee8ab3e8c72dfcd798ba6d963b6f34625989d) | Acquire caller-supplied adapter references only after GetParent succeeds in D3D10 and D3D10.1 device creation, avoiding failure-path leaks. |
| [0060](../patches/upstream/0060-wine-11.17-d2d1-geometry-fill-brush-lifetime.patch) | `wine-11.17`: [`4334512575c7`](https://github.com/wine-mirror/wine/commit/4334512575c7be581ae9a2bd9d9d0d11e5038542) | Keep the first cloned brush owned by its command list when opacity-brush creation fails; list destruction must release it exactly once. |
| [0061](../patches/upstream/0061-wine-11.17-evr-video-window-swapchain-lifetime.patch) | `wine-11.17`: [`5d711b3bdc45`](https://github.com/wine-mirror/wine/commit/5d711b3bdc455addd7a720b553600abc869937be) | Clear the released swapchain pointer before EVR video-window replacement. A failed replacement must leave no dangling owner; same-window retry behavior is unchanged. |

### Dependencies and adaptations

- `0045` combines the original DirectInput test prerequisite and fix into one reversible final diff. The original assertions and authorship are retained without production-code adaptation.
- `0053` similarly includes the original D3D10 tests. The only extra test adaptation is an explicit skip if device creation fails, avoiding a NULL-device dereference. A skipped device test is not a graphics pass. All 14 byte-sized fields are covered by the production fix.
- `0059` groups the independent D3D10 and D3D10.1 adapter fixes. No production semantics were adapted in the selected backports. All other added upstream commits contain no original regression test; the independent checks below cover their specific contracts, with affected-module and product tests still required.
- Full commit IDs, patch digests and first Wine releases remain authoritative in [the manifest](../patchsets/cx26.3-wine11.0-runeon.9/manifest.json). Source bundles retain the patches, original attribution and [test sources](../tests).

### Deferred after deeper review

| Group / upstream commits | Reason to defer |
|---|---|
| CoreAudio periods: `ddefc3d4569f`, `07c7fcc86fe5`, `8e09602ca6e4`, `da0b08472b4b` | Sets the actual shared device buffer size and fails stream creation if setting that property fails. Real device periods, read-only/unsupported settings, differing concurrent streams and device changes need validation. The original test only adds an accepted default-period value; zero-fuzz application does not cover these risks. |
| WGI initialization: `b45d928aa12f` | Signaling before provider enumeration permits earlier GamepadAdded registration but does not guarantee it. Activation may return before the initial gamepad list is populated. Preconnected-controller and hotplug checks are missing; current event-registration tests do not prove this timing. |
| XAudio2 failure unlock: `b68ea3b878e2` | The pinned bundled FAudio_CreateSourceVoice has only a successful return, so the repaired HRESULT failure branch is not normally reachable in this build. The upstream unlock-before-free order also permits a voice-reuse race on effect_chain; re-evaluate when FAudio error behavior changes, with a real failure probe. |
| MF drain: `862453b70420` | One-at-a-time terminal output resumes in source_reader_get_read_result, but SOURCE_READER_ASYNC_SAMPLE_READY pops directly. At EOS, an already-pending async read can consume the sole queued sample and leave remaining decoder frames stranded; its next read can report EOS. This is a source-path counterexample, not a runtime reproduction, and needs a controlled multi-frame drain test. It does not depend on the deferred async Release change. |
| Unreleased MF lifecycle/seek: `2f73d9efc2a0`, `5fc7c7958be1`, `7edd71a8d76c` | Changing Release to decrement is unsafe while async commands start at refcount zero: queue AddRef followed by caller Release can free a queued object. Initial ownership, queue failure and read/seek/flush lifetimes need a coordinated fix. Earlier SEEKING assignment also needs failure cleanup review. These changes are not added to this release candidate. |

These decisions do not claim that upstream fixes are generally invalid. They reflect the pinned implementation and the validation available for this candidate. Broader macOS window-driver/ABI changes and an overall Wine version upgrade remain outside this increment.

### Validation status

| Check | Current result |
|---|---|
| Pinned-source replay and preservation | All 57 patch files replay with zero fuzz from the pinned archive; a comparison of 11,104 source files agrees. The 47 previous patch files and their manifest entries are unchanged. |
| Complete x86_64/WoW64 build | `configure`, `make` and `make install` passed. |
| Original upstream test compilation | The DirectInput and D3D10 test executables compile for x64 and x86: four executables. Complete upstream suite execution remains pending. |
| Independent Windows API execution | Passed in isolated prefixes on both x64 and x86: SChannel 11, adapter 6, stateblock 32, DirectWrite 6, WIC 11 and DirectInput 4 assertions, totaling 70 per architecture. WIC queries a real APP1/Exif value and exercises both four-byte metadata reads. |
| Previous-DLL API control | The same 70-check set reports 19 failures with `.8` DLLs: 6 SChannel, 2 adapter, 4 stateblock, 2 DirectWrite, 3 WIC and 2 DirectInput. This control replaces eight DLLs in a separate runtime using the candidate's same core and dependencies; it is not a complete old-product smoke test. |
| WinHTTP execution | Both `headers` and `pac` modes pass on candidate x64 and x86. Headers contain 100 A characters and 40 B characters, followed by the expected body and EOF. The normal PAC hostname returns DIRECT; the 1,093-character hostname is rejected. Fixture requests are confirmed on loopback, with no external traffic. |
| Previous-DLL WinHTTP control | The oversized-header and PAC paths reproduce stack-corruption page faults with `.8` DLLs after reaching their intended inputs. The header control first verifies the oversized headers; the PAC control first passes the normal hostname. Both enter fault/debugger handling and require a 25-second supervisor timeout with isolated server/debugger cleanup. They are not recorded as ordinary assertion failures or normal crash exits. The candidate eliminates these faults in these probes; this is not evidence of a game-level fix. |
| Direct2D/EVR ownership fault injection | [lifetime-check.py](../tests/lifetime-check.py) compiles the two real functions with dependency stubs: 51 candidate checks pass; `.8` produces six expected failures and two safely detected duplicate releases. Covers creation/replacement failure, recovery, success and destruction. This is function-level validation, not Wine or GPU execution. |
| WIC read-result fault injection | [stream-read-check.py](../tests/stream-read-check.py) runs nine scenarios with 38 assertions: candidate passes; `.8` produces 12 expected failures. Covers NULL/non-NULL count, full/short reads, S_FALSE normalization, and preserving E_FAIL without reading unwritten count. This is function-level validation, not real codec execution. |
| Repeat with new dependency build | Open Wine `.9` was fully rebuilt with MoltenVK `1.4.2` while retaining the existing GnuTLS/GStreamer dependency versions. It passes the same 70 API assertions per architecture and all four x64/x86 headers/PAC runs. Additional PAC tracing confirms one download followed by script-cache reuse and entry into the long-host path. These checks do not establish game compatibility or MoltenVK graphics capabilities. |
| Complete affected-module suites and product regression | Full suite execution, product existing-prefix checks, Steam CEF/stop/relaunch, and D3DMetal/DXMT/DXVK game smoke remain pending. Isolated API-prefix results do not replace them. |
| Source assets and runtime distribution | Seed `2026.09.13` uses `.9` with MoltenVK `1.4.2` in Dev and Production, with a minimum App gate of `1.8 (0)`. Production reuses the exact signed archive verified in Dev. Build, API regression, signing, readiness, authenticated feed/ticket and full-download verification are complete. The user confirmed component updates, runtime preparation and Steam startup with the current Xcode Dev source build. Distribution was checked for `1.8 (0-3)`; per-build game, stop/restart interaction and clean-prefix acceptance were not repeated. Published App installers remain `1.8 (3)`. The source Release is stable; tag, commit and all four asset bytes, sizes and digests are unchanged. `.8` / seed `2026.09.06` is retained history. |

Reproducible check commands and the managed-runtime launch boundary are in [BUILDING](../BUILDING.md). Compiling upstream tests is not running them; API and function-level passes do not replace the remaining product release gates in [MAINTENANCE](../MAINTENANCE.md). Update this status only when the corresponding evidence exists.

<a id="backports-11.16-11.18-sweep"></a>

## Wine 11.16–11.18 user-mode fix sweep

Candidate `cx26.3-wine11.0-runeon.11` adds four upstream patches after `0106`:

| Patch | Contents |
|---|---|
| `0062-wine-11.16-targeted-fixes.patch` | 20 WineHQ commits first released in `wine-11.16` |
| `0063-wine-11.17-targeted-fixes.patch` | 107 commits from `wine-11.17` |
| `0064-wine-11.18-targeted-fixes.patch` | 76 commits from `wine-11.18` |
| `0065-wine-11.18-imm32-ime-keydown-lparam.patch` | `a6b3099042` adapted to the CrossOver IME code |

Each squashed patch lists every upstream commit with its author. The manifest records the full commit IDs. The 13 commits already carried by `0045`–`0061` are not repeated. The only production adaptation is in `0064`: `d22810d47b` includes `intsafe.h`, which this base lacks, so `bmpdecode.c` defines a local `UIntMult()` with the same overflow contract. The full product build found this; clean patch application does not.

### Selection

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

The previously deferred CoreAudio period, WGI initialisation, XAudio2 unlock and MF drain/lifecycle commits stay deferred for the reasons in [BACKPORTS-11.16-11.17.md](#backports-11.16-11.17).

Upstream revert `e5abb7c03a` would remove the x64 `RaiseException` path that preserves non-volatile registers. That path exists in this base, and its comment notes that some DRM depends on it. The revert is motivated by Windows 11 25H2 test results, not by a Wine defect, so it is not carried.

### Not applicable to this base

Several fixes target code that does not exist in CrossOver 26.3. Examples are the msxml3 XPath engine, WinHTTP `WINHTTP_OPTION_SERVER_CBT`, the SymCrypt-based bcrypt/rsaenh paths, the newer d2d1 command-list and dsound resampler code, and cfgmgr32. The wined3d swapchain mutex fix and the wg_parser initial-gap fix repair failure modes this base does not have. None of these were adapted.

### Validation

`scripts/static-check.sh` and `scripts/integration-check.sh` pass. Rebuilding the tree from the pinned archive plus the series matches the reviewed tree file for file. Every added low-level call was checked: only generic heap and critical-section functions are introduced. A full product Wine build is part of the local seed candidate. This sweep does not claim that any particular game issue is fixed.

`upstreamAuditThrough` stays at `wine-11.15`, because this is a filtered sweep and not a complete audit. The base manifest records `upstreamSweepThrough: wine-11.18` instead, so the weekly upstream watch alerts again only when a newer tag appears.

<a id="backports-11.19-sweep"></a>

## Wine 11.19 user-mode fix sweep

Candidate `cx26.3-wine11.0-runeon.15` adds `patches/upstream/0079-wine-11.19-targeted-fixes.patch` after `0110`. It squashes 87 WineHQ commits first released in `wine-11.19`, lists every commit with its author, and records the full commit IDs in the manifest. Test-only hunks are omitted.

### Selection

All 353 commits in `wine-11.18..wine-11.19` were considered with the rules of [the previous sweep](#backports-11.16-11.18-sweep):

- 100 commits touch `ntdll`, `server`, `win32u`, `winemac.drv`, `wow64`, `loader`, `configure`, `tools`, `libs` or other host drivers. Most are an OpenGL/EGL context refactor and `winemac.drv` header and logging cleanups. None is taken. The small fixes among them were checked one by one: the IPv6 `IPV6_MTU_DISCOVER` level fix (`fc5dd34cc3`) is in a branch that macOS does not build, because it has `IPV6_DONTFRAG`; the EGL pbuffer return (`802d3d49fb`) is not on the macOS path; zero group affinity (`0998bfa77a`) only makes a rarely used call fail; the aliased-HKL IME check (`a07e981c59`) changes `winemac.drv` keyboard handling and needs its own validation; the `NtReleaseSemaphore` signedness change (`f89a7ca541`, `3e182735ea`) changes a syscall signature. Later, `cx26.3-wine11.0-runeon.17` carries the macOS driver effect of two of the context commits, `632068bd35` and `af79eeb0d0`, as the narrower patch `0080` without the refactor itself; its manifest `adaptation` lists what is and is not taken.
- 29 commits change tests only.
- Of the 224 user-mode candidates, crash, memory-safety, leak and error-path fixes in DLLs that Steam, launchers, installers or games load were selected. Left out: controller identity (`winebus.sys`, `hidclass.sys`, `xinput1_3`), CD/DVD and SCSI IOCTLs in `mountmgr.sys`, and feature work such as D3D12-backed Media Foundation buffers, GDI+ vertical text, the DNS cache and hosts file, VBScript parser changes, `RoResolveNamespace`, the D2D1 color-management stub and `GetNextAsyncId`.

Of the selected commits, three that applied cleanly were removed after review:

| Removed | Reason |
|---|---|
| `c9723c90f1` setupapi | Halving the `delete_multi_sz_value()` buffer leaves its `WCHAR` pointer arithmetic unchanged, so the copy writes past the allocation. The bug is present in `wine-11.19` itself. The change only saves memory. |
| `6880117619` oleaut32 | Changes decimal rounding semantics, not a crash fix. |
| `5ff02cf188` mfplat | `MFCalculateImageSize()` failing for zero height makes decoder type negotiation in the base fail where it used to succeed, which risks video playback. |

Thirteen commits do not apply because the base differs and were not ported: the `rsaenh` HMAC series, which needs a hash implementation the base does not have (a single member that applied was reverted rather than taking part of a series), `secur32` `770f0b9c7b`, the hosts-file follow-up `8f6cb19433`, the Media Foundation clock and session fixes whose code differs in the base, `msado15` `57cebf5711` and the D3DX10 sprite fixes.

### Adaptations

| Commit | Adaptation |
|---|---|
| `60551e1d7b` msvcrt | Overflow checks before both aligned allocations; the base lacks a later shrink-padding block that the upstream context includes. |
| `105ced5bc1` msvcrt | The same `ARRAY_SIZE` change with different surrounding lines. |
| `53b5345251`, `37fd158bfc` mf | Combined: `Flush` resets only an active renderer that still has an audio client; the base has no position/pts fields. |
| `ee38ff1725` mf | The base lacks `intsafe.h`; the index is compared with `~(DWORD)0` instead of `DWORD_MAX`. |
| `258ee951ab` windowscodecs | Completed: the new overflow check leaves through the common exit, so the decoder lock is released; upstream returns while holding it. |
| `ae807355d6` ieframe | Completed: the task allocation is checked, the task is freed when the callback cannot be created, and only a created callback is released; upstream still releases a NULL callback and leaks the task. |

### Validation

`scripts/static-check.sh` and `scripts/integration-check.sh` pass, and applying the series to the pinned archive reproduces the reviewed tree file for file. Every changed module, and every module that builds from its sources (`ucrtbase`, the `msvcr*` and `msvcp*` family, `d3dcompiler_*`, `d3dx9_*`, `d3dx10_43`, `d3dx11_43`, `comctl32_v6`), builds for i386 and x86_64. The only new low-level call is `RtlLookupFunctionEntry` in `cxx_frame_handler`, which the base implements. A complete product build and runtime smoke test are still required before tagging.

`upstreamAuditThrough` stays at `wine-11.15`. The base manifest records `upstreamSweepThrough: wine-11.19`.

<a id="backports-11.1-11.19-stubs"></a>

## Wine 11.1 to 11.19 compatibility stub review

Candidate `cx26.3-wine11.0-runeon.18` starts from `.17` and adds `patches/upstream/0081` to `0087`. Earlier reviews of this range selected crash, memory-safety and error-path fixes and left feature work out. That also left out small commits that turn a failing stub into a successful one. An application that treats the failure as fatal stops there, so this review looks at that class separately.

### Selection

Two groups in `wine-11.0..wine-11.19` were reviewed one commit at a time against the patched `.17` tree, using forward and reverse application:

- the 18 commits whose subject starts with a module name followed by "Stub";
- the 8 commits whose message says which application needed them ("Needed by", "Needed for").

### Taken

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

### Not taken

| Commits | Reason |
| --- | --- |
| `3f65f20cc2` ntdll, `339be36e64`, `94e5bbeb6f`, `2be7717684` win32u, `5b5e95719d` mountmgr.sys | ABI-sensitive modules, which these reviews do not change. |
| `8571f65844` ucrtbase, `5c281ca2b2` powrprof | Already carried by earlier patches in this series (`8571f65844` by `0079`, `5c281ca2b2` by `0063`). |
| `a33fbd48a0` wminet_utils | The base has no `wminet_utils` module. |
| `043bc5a0d4` windows.web | A new `JsonArray` class with no stated consumer; parsing arrived in later feature commits. |
| `fff6090795` webservices | Needed by a remote debugger, not by applications this runtime targets. |
| `ccdaa2b581`, `a164854c92` wined3d | Only change a Vulkan validation-layer message and a link dependency. |

### Validation

`scripts/static-check.sh` and `scripts/integration-check.sh` pass for `.18`, and the default definition still passes `scripts/static-check.sh`. Those checks prove that the series applies. A complete local i386/x86_64 WoW64 build of `.18` at this revision finished without errors on 2026-10-14; it has not been released.

<a id="backports-11.20-sweep"></a>

## Wine 11.20 user-mode fix sweep

Candidate patch set `cx26.3-wine11.0-runeon.23` adds `patches/upstream/0135-wine-11.20-targeted-fixes.patch` after `0134`. It squashes 37 WineHQ commits: 35 first released in `wine-11.20`, and two `wine-11.12` msvcp60 fixes that one of them depends on. The patch lists every commit with its author, and the manifest records the full commit IDs. Test-only hunks are omitted.

### Selection

All 342 commits of `wine-11.19..wine-11.20` were reviewed with the rules of the [previous sweep](#backports-11.19-sweep), against a tree made of the pinned CrossOver archive plus patches `0001`–`0134`:

- 50 commits change only tests.
- 80 commits change only `ntdll`, `server`, `win32u`, `winemac.drv`, the loader, `include`, the build tools or translations. Most are feature work: memory-reservation changes, EGL and D3DKMT work, ARM64EC relay support and the volume-unmount request. Each was read. Two small defects that the base demonstrably shares are taken (below). The rest are left out.
- Of the 212 user-mode candidates, the sweep takes crash, memory-safety, leak, hang and error-path fixes in DLLs that Steam, launchers, installers or games load. It leaves out feature series (the quartz color converter, DTD validation in msxml3, the dmime loop rework, `wminet_utils`, dsound mixing, `winebus` device identity, the jscript/mshtml cycle collector, vbscript parser changes, gdiplus antialiasing), version-resource bumps and the `cmd` word-iterator refactor.

Each taken commit applies to that tree in upstream order, which shows that the base still carries the unfixed code.

### Taken

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

### Left out after review

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

### Known risk

`9eb4614ad7` dinput now waits for an event that the input thread sets on its way out, instead of the thread handle. If the input thread has already ended through its unexpected-termination path, the stop message cannot be delivered and the wait never returns. Before the change, the wait returned at once. Upstream has the same behaviour. That path is rare, so the commit is taken unchanged, and the risk is recorded here.

`48e2d50404` turns `ex_style_mask` from an uninitialised value into a defined one, which changes the visible-rectangle calculation for decorated windows. The runtime check therefore includes the geometry of a window with a title bar, such as Steam.

### Adaptations

| Commit | Adaptation |
|---|---|
| `146fbbe78b` dmusic | Applied to `dlls/dmband/bandtrack.c`. Upstream moved the band track to dmusic in `220b4e7ff6`, which is not taken. The base's code is otherwise identical. |
| `48e2d50404` winemac.drv | The same one-line change. In the base it sits below CrossOver's Quicken hack, so the hunk is offset. |

### Validation

`scripts/static-check.sh` and `scripts/integration-check.sh` pass for `.23`. Every commit applies in order without fuzz, except the two adapted ones. A complete i386/x86_64 build, Wine's own tests for the affected modules compared against `.22`, and product acceptance passed before `.23` was tagged.

`upstreamAuditThrough` stays at `wine-11.15`. The base manifest records `upstreamSweepThrough: wine-11.20`.

<a id="msync-fix-series"></a>

## msync fix series from dappermint/winecx

Candidate patch set `cx26.3-wine11.0-runeon.25` adds `patches/runeon/0137`–`0143` after `0136`. msync is CrossOver's Mach-based synchronisation path, enabled only when `WINEMSYNC` is set; WineHQ has no msync, so there is no upstream fix to backport. The source is the `wine1119` branch of [dappermint/winecx](https://github.com/dappermint/winecx), a CrossOver 26.3 / Wine 11.0 tree like the pinned base. All seven commits are by millia ampora and apply in branch order, with zero fuzz, to the pinned archive plus `0001`–`0136`. No commit on that branch touched msync after 2026-08-25.

`ntdll` and `server` are ABI-sensitive. The maintainer approved the series as a whole on 2026-10-25; the approval does not widen the rule for other patches. Every changed line is in `server/msync.c` or `dlls/ntdll/unix/msync.c` and runs only when `do_msync()` is true, so without `WINEMSYNC` the runtime behaves as the base. The Mach message format and the Unix-call ABI are unchanged; `0141` changes the meaning of a shared object field (below). A wineserver and a client from different builds can still connect, so a mixed pair can hang; a later change to this layout must also rename the msync port.

### Defect reproduced on the base

`tests/msync-wait-probe.c` in leak mode has four threads wait for any of a never-signalled event and a semaphore that two threads keep releasing. On `runeon.24`, in a clean prefix with `WINEMSYNC=1`, the server node pool (`0x80000` nodes) ran out within two minutes, after which every registration printed `msync: warn: node memory pool exhausted` and fell back to `malloc`: wineserver grew from about 22 MB to 181 MB over the x86_64 and i686 runs, with 48 million warnings. With `WINEMSYNC=0` the same run printed none and wineserver stayed at about 18 MB.

### Taken

| Patch | Commit | Change |
|---|---|---|
| `0137` | `8df18268539a` | The pump unregistered already-added objects only when `i > 1`, so a wait whose second object was signalled left a node on the first. Defect fix. |
| `0138` | `9be392b3b4b8` | A wait-any that gave up on contention during registration never sent the unregister message, leaving the tid on every object and the alert counter above zero. Defect fix. |
| `0139` | `3a7a712d6616` | Pump queue limit 50 → `MACH_PORT_QLIMIT_MAX` (1024); `WINEMSYNC_QLIMIT` clamped. Performance. |
| `0140` | `307f90fdb1b3` | The client sleeps on its tid slot (value 3, 1 ms bound) instead of spinning while the pump acknowledges a registration. Performance. |
| `0141` | `620d8c542fcd` | Splits `int multiple_waiters` into two `short` counters and skips `__ulock_wake` when no thread waits on the object word. Object size and other field offsets are unchanged; server and client must come from the same build. Every signal path stores with a full barrier before it reads the count. Adapted: mutex abandonment stored the new owner with a plain store, which the count read may pass under TSO and skip the wake; it now stores with a full barrier. Performance. |
| `0142` | `ef72fdb55b92` | SetEvent on a set event returns after a read instead of an exchange. Adapted: a sequentially consistent fence before the read keeps SetEvent a full barrier, as on Windows; the argument relies on x86 TSO. Performance. |
| `0143` | `6d316146c2fa` | Registration spin count set to 0, as measured by the author; adds `WINEMSYNC_SPINS` to override it. Performance. |

The performance commits are not defect fixes and would normally stay out (§3 of `CLAUDE.md`). The maintainer took them because they run only in the opt-in msync path, which users enable for performance.

### Left out after review

| Change | Reason |
|---|---|
| `a7ef7b3b` (wake one waiter for auto events and mutexes) | Single-object waiters and wait-all waiters sleep in the same ulock queue on the object word. A wait-all thread woken alone moves on to its next unsignalled object without taking this one, so the object stays signalled while the remaining waiters sleep: a lost wake-up that Windows does not have, possibly a hang. Its gain was 68 → 50 ms in the author's benchmark together with `ef72fdb5`. |
| Highball `0014` | Carries the same two unregister fixes as `0137` and `0138`, plus rate-limiting the pool warning, which would hide the leak's symptom rather than fix it. |
| Highball `0005` (yield after setting an auto-reset event) | Also changes the server sync path every default user runs. Its msync half is a scheduling heuristic for programs that rely on Windows' priority boost, against the direction of this series, and was not reproduced here. |

### Measured on the candidate

A complete i386/x86_64 WoW64 build of this patch set, before the abandonment adaptation of `0141`, ran the same probe in a clean prefix (milliseconds and counts are x86_64/i686, single runs):

| | `runeon.24`, msync | `runeon.25`, msync | `runeon.24`, no msync | `runeon.25`, no msync |
|---|---|---|---|---|
| Pool warnings | 48,029,232 | 0 | 0 | 0 |
| wineserver RSS after the run | 181 MB | 22 MB, flat | 18 MB | 17 MB |
| Leak-mode waits in 120 s | 11.5M / 10.8M | 26.2M / 26.9M | 14.8M / 15.0M | 14.8M / 14.9M |
| ring | 810 / 893 | 814 / 852 | 1961 / 2212 | 1957 / 2155 |
| multi | 793 / 937 | 661 / 676 | 1216 / 1252 | 1225 / 1252 |

### Validation limits

Static and integration checks prove the series applies and its anchors are present. The table above shows the leak gone and no measurable change without `WINEMSYNC`. The abandonment adaptation is checked by code review only and must be in the build that is tagged. The probe covers auto-reset events with single-object waiters, wait-any registrations and semaphores in wait-any; it does not cover single-object waits on semaphores or manual events, `ReleaseMutex`, mutex abandonment, wait-all or alertable waits, and does not isolate each patch's effect. A lost wake-up shows only as a hang, so probe runs need an external timeout. Steam with representative games is still required before the patch set is tagged.

<a id="gl-present-on-swap"></a>

## OpenGL surface presentation on swap (Wine 11.3–11.4)

Candidate patch set `cx26.3-wine11.0-runeon.26` adds `patches/upstream/0144` after `0143`: WineHQ commits `62da9c02a7` (wine-11.3, Paul Gofman) and `b5ab3b155a` (wine-11.4, Rémi Bernon), applied unchanged as one patch because they decide one behaviour, which OpenGL surface a window shows. Upstream took them in this order and the second relies on the first. `.25` already ships as a built Dev runtime, so this goes into a new patch set rather than reopening it.

`winemac.drv` and `win32u` are ABI-sensitive (§3 of `CLAUDE.md`). The maintainer approved this patch individually on 2026-10-25; the approval does not widen the rule for other patches. Neither commit changes a structure, export or driver entry point: one flag argument in `context_sync_drawables()` and the bodies of `macdrv_surface_flush()` and `macdrv_surface_swap()`.

### Defect reproduced on the base

Amnesia: Memories (D3D9 through WineD3D) plays its opening videos through the DirectShow Video Mixing Renderer in windowless mode, clipped to the game's window. On `runeon.25`, skipping the video left the window on its last frame while the game kept running: menu sounds answered input and the main thread kept calling `Present`. A run with `WINEDEBUG=+quartz,+d3d9,+wgl,+macdrv` showed the mechanism:

- the game's device created its surface on the window when it started, and every later swap went to it;
- the first video's device created a second surface on the same window; `macdrv_client_surface_present()` made it the window's visible view and hid the game's; the next videos reused it through `get_window_unused_drawable()`;
- skipping stopped the graph and released the presenter, but that surface had no detach or destroy, and the game's swaps never presented its own surface again. In the base, `macdrv_surface_swap()` only flushes the context; the OpenGL path never calls `client_surface_present()` (the Vulkan path and CrossOver's D3DMetal presentation event do).

The same "skipping the intro video freezes the screen on a Mac" report exists for other games on CrossOver, which fits this mechanism, but no WineHQ bug is linked to these commits.

### Taken

| Patch | Commits | Change |
|---|---|---|
| `0144` | `62da9c02a7`, `b5ab3b155a` | `macdrv_surface_swap()` presents the swapped surface, and `macdrv_surface_flush()` presents it when opengl32 asks after front-buffer drawing; `context_sync_drawables()` no longer presents a drawable merely because a context was made current on it. Defect fix. |

Both apply with zero fuzz on top of `0143`; `0080`, `0114` and `0117` also touch `dlls/winemac.drv/opengl.c` and are unaffected. The pre-images in the base match upstream exactly, so there is no adaptation.

### Left out after review

| Change | Reason |
|---|---|
| `02d01544dd` (DC own drawable in `get_updated_drawable()` on flush) | Lets a context flush with no HDC find the DC's own drawable (pbuffer or D3D swapchain) through a new `owner_hdc`, reached only after the client surface was detached. It also inserts a field into `struct opengl_drawable`, changing the layout win32u shares with the drivers. The defect above does not need it. |
| `b3c408b3c0` (`flags |= GL_FLUSH_INTERVAL` in `opengl_drawable_flush()`) | Landed between the two taken commits. Without it, a flush that coincides with a swap-interval change drops `GL_FLUSH_PRESENT`, so that one front-buffer flush does not present; swaps always present. A separate win32u change that would need its own approval. |
| `ae1053abc3` (opengl32 reads the drawable after `wgl_context_flush()`) | The base has the same pattern in the `GL_FLUSH_FORCE_SWAP` branch, which is not reachable on macOS because the macdrv buffer map is the identity. |

### New behaviour and risks

Every swap now makes the swapped surface the window's visible view. Two devices that swap alternately on one window, such as a game that keeps calling `Present` while a windowless video plays, would switch the whole client view between them each frame; upstream behaves the same. A front-buffer `glFlush`/`glFinish` now flushes the surface, which for a double-buffered context is a buffer flip. On a window with one surface, presenting it again changes nothing.

CrossOver-specific, from reading only: `macdrv_DestroyWindow()` releases D3DMetal client surfaces while holding `win_data_mutex`, the reverse of the `surfaces_lock` → `win_data_mutex` order that `client_surface_present()` takes. Vulkan presents and window moves already take that order; this patch adds every OpenGL swap.

### Verification required before tagging

No self-contained check exists: the behaviour needs Cocoa views and CGL contexts. A complete WoW64 build and an application run must show:

- no flicker while the windowless video plays, and the game's own frames after it is skipped;
- unchanged presentation in at least one single-window WineD3D game and the Steam client;
- if available, a D3DMetal or DXMT game that plays a video and is then closed.

<a id="macos-fix-batch-1"></a>

## macOS fix batch 1 (Wine 11.0 to master, 2026-10-22)

Candidate patch set `cx26.3-wine11.0-runeon.27` adds `patches/upstream/0145` after `0144`: eleven WineHQ commits applied unchanged in upstream order, test-only hunks omitted. It is the first batch carried under the macOS-fix exception in §3 of `CLAUDE.md` (maintainer decision 2026-10-26). Before that rule, every sweep left `winemac.drv`, `win32u` and `ntdll` out, so most macOS fixes since `wine-11.0` were never carried.

### Selection

Candidates are every commit in `wine-11.0..master` (master at `e3c3d34867`, 2026-10-22) that changes `winemac.drv`, `winecoreaudio.drv`, the macOS preloader or `mountmgr.sys/diskarb.c`, names macOS, Darwin, Apple, Cocoa, MoltenVK or Rosetta in its message, or changes code guarded by `__APPLE__` or macdrv in `dlls`, `loader` or `server`: 167 commits. Each was checked against the pinned archive plus `0001`–`0144`:

- 8 are already carried or in the base.
- 42 apply with zero fuzz on their own. Most are clean-ups, deprecated-API replacements or ARM64-only changes; the defect fixes among them are taken below or left out with a reason.
- 93 do not apply. Almost all belong to the upstream rework of OpenGL contexts and client surfaces in `win32u` and `winemac.drv`, or to clean-ups; the fixes among them need that rework or an adaptation and are deferred.
- 24 more, found through the `__APPLE__` search, move CD-ROM and DVD ioctls from `ntdll` to `mountmgr`, add Linux syscall emulation, or reorder address-space setup: features and refactors.

For each taken commit, a script compared the pinned CrossOver files with `wine-11.0` and checked that no hunk falls within three lines of a CrossOver change; every later upstream commit that names it was searched for (§3).

### Taken

| Commit | Release | Change |
|---|---|---|
| `98a370e5e1`, `c8b7150680` | 11.2, 11.7 | `NtQueryVolumeInformationFile` reports the free space macOS makes available for important data, as Finder does, and falls back to `statfs` when that is 0 (non-APFS volumes). Taken together. |
| `7d29860d06` | 11.3 | Clipboard formats are registered through `NtUserRegisterWindowMessage` (WineHQ bug 59257). |
| `92df8e37e5` | 11.3 | gdiplus no longer assumes an enum type is unsigned (seen with clang on macOS). |
| `4f1ecabee9` | 11.5 | `GetKeyNameText` no longer matches keys whose scan code is 0. |
| `3b1ab64042` | 11.6 | Generated keyboard layouts keep the current user locale. |
| `1bfb493cd0` | 11.7 | A window that becomes invalid between `WM_QUERYENDSESSION` and `WM_ENDSESSION` is treated as having agreed, instead of blocking the session end. |
| `d14d0b22b4` | 11.10 | IME open status follows whether the keyboard layout has an IME. |
| `eefbbc07a8` | 11.10 | `ClipCursor` with the reset flag releases the clip instead of returning early. |
| `1a1d1f3f38` | 11.16 | A window-surface flush hides a client view left by an earlier D3D or OpenGL swapchain. Upstream fixes a regression of `b5ab3b155a`, carried in `0144`: a launcher that drew with D3D9 and then with GDI on the same window kept showing the last 3D frame. `0144` was carried without it; the search for later fixes in §3 was added because of this. |
| `53a91cdcbe` | 11.17 | Pasteboard data is released after import (leak). |

### Left out after review

| Commit | Reason |
|---|---|
| `e4683c4cff` (app icons through `CreateIconFromResourceEx`) | Removes two fields from `struct app_icon_entry`, shared by the PE and Unix sides of `winemac.drv`; a layout change for a cosmetic fix. |
| `82ad0ecd12` (no `.desktop` files outside XDG desktops) | Two later shell32 fixes (`93f3d8ae9f`, `648e636256`) repair trash-directory creation it broke; not worth carrying shell32 changes for it. |
| `07c7fcc86f` (Core Audio stream period) | The same crackling is already fixed differently by `0129`; both set `kAudioDevicePropertyBufferFrameSize`, to different values. Replacing `0129` with the upstream series (`8e09602ca6`, `da0b08472b`) is a later decision. |
| `4cf90548f2` (wineboot without mDNS) | Passes `AI_CANONNAME | AI_DNS_ONLY`, which the base `ws2_32` only treats as DNS-only when `AI_DNS_ONLY` is the sole flag; without the matching `ws2_32` change it has no effect. |
| `3ae8338d9f` (driver notified of capture reset on destruction) | Its `winemac.drv/window.c` hunk sits next to CrossOver hack 22435 (D3DMetal client surfaces in `macdrv_DestroyWindow`). |
| `3dcb1a565a` (display changes through `CGDisplayRegisterReconfigurationCallback`) | Larger change next to CrossOver hack 10912 in `cocoa_app.m`; deferred for a closer review. |
| `361b82c32e`, `a32e9be3d5` (mountmgr free space) | CrossOver changed the same file's includes and structure; the `ntdll` pair above covers the path games and installers use. |
| `d56f36c1aa` (revert of forced window activation before restore) | Changes `win32u` behaviour on every platform, so it is outside the macOS exception; pending individual approval. |
| `98d3e36f2b` (Core Audio `zero_bits` at wow64 attach) | Removes a Unix call entry: a unix-call table change and a refactor. |
| `ab9ab01153` (USB interface information in winebus) | A feature for device identity. |
| ARM64 commits (`569a7553b2`, `920240e73e`, `2a61baa185`, …) | Runeon runs x86_64 under Rosetta. |
| Clean-ups and deprecated-API replacements (`340872e8e5`, `a83c128ce6`, `f330e62e0a`, `5b649b7208`, `fc45e3f8f1`, `0e3f38cecc`, …) | Not defect fixes. |

### Deferred

Fixes that do not apply because they depend on the upstream rework, each to be adapted only if the fix alone fits the base without touching CrossOver changes: OpenGL core-profile contexts (`b7ce33f16d`, `1f6bdd6360`, `fe16726680`, `e7e7825b3c`, `4b70bebd89`, `cc91be7b65`), flushing GL commands when another context is activated (`1c144117a8`), the Core Audio period series, IME fixes (`3171b388e6`, `85ad031e9c`, `a07e981c59`, `8b2f29dfc2`), mouse-motion accumulation (`21f24279ee`) and address-space reservation (`fcd111ec5a`, `f8da67d3bc`).

### Verification required before tagging

No self-contained check. A complete WoW64 build and an application run must show that the Steam client and several games still present, take keyboard and mouse input and exit normally, and that free space reported to an installer matches Finder.
