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

Candidate patch set `cx26.3-wine11.0-runeon.27` adds `patches/upstream/0145` after `0144`: seven WineHQ commits in upstream order, test-only hunks omitted, the first batch carried under the macOS-fix exception in §3 of `CLAUDE.md` (maintainer decision 2026-10-26); `d56f36c1aa` was also approved individually. All are applied unchanged except one Runeon adaptation, described with `98a370e5e1` below. Before that rule, every sweep left `winemac.drv`, `win32u` and `ntdll` out, so most macOS fixes since `wine-11.0` were never carried.

### Selection

Candidates are every commit in `wine-11.0..master` (master at `e3c3d34867`, 2026-10-22) that changes `winemac.drv`, `winecoreaudio.drv`, the macOS preloader or `mountmgr.sys/diskarb.c`, names macOS, Darwin, Apple, Cocoa, MoltenVK or Rosetta in its message, or changes code guarded by `__APPLE__` or macdrv in `dlls`, `loader` or `server`: 167 commits. Each was checked against the pinned archive plus `0001`–`0144`:

- 8 are already carried or in the base.
- 42 apply with zero fuzz on their own. Most are clean-ups, deprecated-API replacements or ARM64-only changes; the defect fixes among them are taken below or left out with a reason.
- 93 do not apply. Almost all belong to the upstream rework of OpenGL contexts and client surfaces in `win32u` and `winemac.drv`, or to clean-ups; the fixes among them need that rework or an adaptation and are deferred.
- 24 more, found through the `__APPLE__` search, move CD-ROM and DVD ioctls from `ntdll` to `mountmgr`, add Linux syscall emulation, or reorder address-space setup: features and refactors.

For each candidate, a script compared the pinned CrossOver files with `wine-11.0` and checked that no hunk falls within three lines of a CrossOver change; every later upstream commit that names it was searched for (§3). No later macOS fix had to come with this batch; `d56f36c1aa` has a later fix, `852e39a89c`, for the X11 driver only.

### Taken

| Commit | Release | Change |
|---|---|---|
| `98a370e5e1`, `c8b7150680` | 11.2, 11.7 | `NtQueryVolumeInformationFile` reports the free space macOS makes available for important data, as Finder does, and falls back to `statfs` when that is 0 (non-APFS volumes). Taken together. Runeon adaptation: CoreFoundation allows `CFURLCopyResourcePropertyForKey()` to succeed without a value, and upstream then passes NULL to `CFNumberGetValue()`; the result is now also checked, so that case falls back to `statfs`. No volume returning that was observed. |
| `7d29860d06` | 11.3 | Registered clipboard formats (RTF, HTML, application formats; not the fixed-number ones such as `CF_UNICODETEXT`) are registered through `NtUserRegisterWindowMessage` (WineHQ bug 59257); the base registered them in the user atom table while winemac looked them up with `NtAddAtom`. |
| `4f1ecabee9` | 11.5 | `GetKeyNameText` no longer matches keys whose scan code is 0. |
| `d56f36c1aa` | 11.8 | A window restored from the Dock is activated before `SC_RESTORE`, so it gets `WM_ACTIVATE` first as on Windows (upstream: Warhammer: Vermintide 2 could not return to fullscreen after losing focus). It also reverts `win32u` and `winex11.drv` code that macOS does not reach: `winemac.drv` does not implement `GetWindowStateUpdates`, so `handle_internal_message` returns early. For that reason it is outside the macOS exception; it was approved individually on 2026-10-26 after confirming upstream master still carries it, and is applied unchanged. Its later fix `852e39a89c` relies on an unlock call only `winex11.drv` implements. |
| `eefbbc07a8` | 11.10 | `ClipCursor` with the reset flag releases the clip instead of returning early. |
| `53a91cdcbe` | 11.17 | `get_format_entries()`, used when a drag enters a window (`query_drag_drop_enter()`), releases the pasteboard data it imports (leak). |

### Left out after review

| Commit | Reason |
|---|---|
| `3b1ab64042`, `d14d0b22b4` (keyboard layouts keep the user locale; IME open status follows the layout) | `3b1ab64042` reworks how HKLs are generated, and its companions (`c0312c4743`, `d519ce4345`) are not in the base. It also brings an upstream bug still present on master: `macdrv_ActivateKeyboardLayout` tests `HIWORD(hkl) == layout->layout_id ? layout->layout_id : layout->lang`, which is true for every layout, so an application asking for a layout gets the first enabled input source. `d14d0b22b4` needs the follow-ups `a07e981c59` (aliased HKLs are not IMEs) and `85ad031e9c` (Roman input modes report a closed IME). Chinese and Japanese input matter most to Runeon's users, so the whole group waits for its own evaluation. |
| `1a1d1f3f38` (window-surface flush hides a client view left by a D3D or OpenGL swapchain) | Upstream fixes a regression of `b5ab3b155a` (carried in `0144`): a launcher that drew with D3D9 and then with GDI kept showing the last 3D frame. Every top-level window has a window surface, so with this change each GDI flush hides the 3D view until the next present. On CrossOver that present can come from the D3DMetal path (hack 22435), which re-shows the view only when the window thread handles an event; upstream never ran this combination, and DXMT and D3DMetal carry most Runeon games. The base also lacks `2293b0e8ca`, which keeps unused client surfaces alive, so whether the regression occurs on the base at all is unproven. Evaluated separately with a test. |
| `1bfb493cd0` (invalid windows between `WM_QUERYENDSESSION` and `WM_ENDSESSION`) | Only changes the last argument of `quit_callback(..., WM_ENDSESSION, ...)`, which that branch never reads; the quit decision still comes from `qi->result`. For an invalid window, another error, and earlier agreement or refusal, the decision is the same before and after; only the log differs. |
| `92df8e37e5` (gdiplus no longer assumes an enum is unsigned) | An ordinary gdiplus parameter check, not macOS code; its upstream tests are the useful check and are not carried. Left to a general user-mode sweep. |
| `e4683c4cff` (app icons through `CreateIconFromResourceEx`) | Removes two fields from `struct app_icon_entry`, shared by the PE and Unix sides of `winemac.drv`; a layout change for a cosmetic fix. |
| `82ad0ecd12` (no `.desktop` files outside XDG desktops) | Two later shell32 fixes (`93f3d8ae9f`, `648e636256`) repair trash-directory creation it broke; not worth carrying shell32 changes for it. |
| `07c7fcc86f` (Core Audio stream period) | The same crackling is already fixed differently by `0129`; both set `kAudioDevicePropertyBufferFrameSize`, to different values. Replacing `0129` with the upstream series (`8e09602ca6`, `da0b08472b`) is a later decision. |
| `4cf90548f2` (wineboot without mDNS) | Passes `AI_CANONNAME | AI_DNS_ONLY`, which the base `ws2_32` only treats as DNS-only when `AI_DNS_ONLY` is the sole flag; it needs `724d5ccdbb` (wine-11.19, `ws2_32` accepts `AI_CANONNAME | AI_DNS_ONLY`), a change outside the macOS exception. Candidates for a later batch together. |
| `3ae8338d9f` (driver notified of capture reset on destruction) | Moves the capture reset into `win32u` for every driver: a cross-driver change, not a macOS fix. |
| `3dcb1a565a` (display changes through `CGDisplayRegisterReconfigurationCallback`) | Larger change next to CrossOver hack 10912 in `cocoa_app.m`; deferred for a closer review. |
| `361b82c32e`, `a32e9be3d5` (mountmgr free space) | CrossOver changed the same file's includes and structure; the `ntdll` pair above covers the path games and installers use. |
| `98d3e36f2b` (Core Audio `zero_bits` at wow64 attach) | Removes a Unix call entry: a unix-call table change and a refactor. |
| `ab9ab01153` (USB interface information in winebus) | A feature for device identity. |
| ARM64 commits (`569a7553b2`, `920240e73e`, `2a61baa185`, …) | Runeon runs x86_64 under Rosetta. |
| Clean-ups and deprecated-API replacements (`340872e8e5`, `a83c128ce6`, `f330e62e0a`, `5b649b7208`, `fc45e3f8f1`, `0e3f38cecc`, …) | Not defect fixes. |

### Deferred

To be adapted only if the fix alone fits the base without touching CrossOver changes: the keyboard and IME group above together with `a07e981c59`, `85ad031e9c`, `d519ce4345`, `3171b388e6` and `8b2f29dfc2`; `1a1d1f3f38`; OpenGL core-profile contexts (`b7ce33f16d`, `1f6bdd6360`, `fe16726680`, `e7e7825b3c`, `4b70bebd89`, `cc91be7b65`); flushing GL commands when another context is activated (`1c144117a8`); the Core Audio period series; mouse-motion accumulation (`21f24279ee`); and address-space reservation (`fcd111ec5a`, `f8da67d3bc`).

### Verification required before tagging

No self-contained check. A complete WoW64 build and an application run must show that the Steam client and several games (WineD3D, DXMT, D3DMetal) still present, take keyboard and mouse input and exit normally; rich text (RTF or HTML) copies with formatting both ways between a macOS app and a Windows program that reads registered formats; files or text dragged from Finder reach a Windows program; a game that confines the cursor releases it; a fullscreen game minimised to the Dock and restored returns to fullscreen; and free space Steam reports for its library drive matches Finder. The adaptation's branch (a volume without the property) is not covered by any test.

<a id="game-graphics-input-fixes"></a>

## Game-facing graphics and input fixes (Wine 11.1 to 11.20)

Candidate patch set `cx26.3-wine11.0-runeon.27` also adds `patches/upstream/0146` (37 WineHQ commits, applied unchanged in upstream order) and `0147` (`1c144117a8`, adapted) after `0145`. Earlier sweeps reviewed these releases but took only crash, leak and hang fixes in user-mode DLLs; the maintainer decided on 2026-10-26 that the graphics and input DLLs games use also take rendering and input correctness fixes (§3 of `CLAUDE.md`).

### Selection

Every commit in `wine-11.0..master` that changes `wined3d`, `d3d8`, `d3d9`, `d3d10*`, `d3d11`, `dxgi`, `ddraw`, `dinput`, `xinput1_3`/`xinput1_4`, `winebus.sys`, `hidclass.sys`, `hid` or `windows.gaming.input`: 262 commits, checked one by one against the pinned archive plus `0001`–`0145` (56 change only tests, 22 are carried, 1 is in the base, about 100 apply with zero fuzz and the rest do not). Defect fixes games reach were taken; then the unapplied ones were tried again on top of the taken set, which brought in `c6e627b6d1`, `b0642a4c6e` and `78e5d7a1b6`. For each taken commit: the CrossOver files were compared with `wine-11.0` (no hunk is near a CrossOver change); later upstream commits naming it were searched for (only `88e775dbdc` has one, `4377bdebd6`, taken with it); and every identifier the added lines use was looked up in the patched tree, because a clean apply does not prove the result compiles.

### Taken in 0146

| Area | Commits |
|---|---|
| wined3d (OpenGL renderer) | `dec0864562` sphere-map texture coordinates in GLSL; `76f152e01f`, `98a441ddf9` backup DC state after a reset; `24df29db09` the right pixel format in `swapchain_gl_present()`; the resolve series `0ff2188fb1`, `824d95cae5` (helper), `b0642a4c6e` (the raw blitter requires matching sample counts), `78e5d7a1b6`, `58af62eb90` — without `b0642a4c6e`, `0ff2188fb1` could send a multisample-to-single-sample copy to the raw blitter; `77d56ec16f` self-blits with FX transforms; `b58c221630` no `texelFetchOffset()` on buffers and multisample textures (GLSL compile failure); `5279200712` locking multisampled back buffers is refused, as on Windows |
| ddraw | `f4626afec7` the ramp device is enumerated (WineHQ bug 1201); `7df175a66f` material activation uses the active device; `303b790376` no rendering to the clipping window in exclusive fullscreen; `6b486ec783` render-target flags on v1 swapchain surfaces; `365e610147` execute-buffer vertices staged at execute time; `6b17d73c04` system-memory depth fills done on the CPU (the base has `5942da6c6c`, which this completes) |
| dxgi | `020cbce1a9` fullscreen descriptions reported as created; `c6e627b6d1`, `94f82f44e9` an invalid fullscreen scanline order or scaling gives a windowed swapchain, as Windows does (Titan Quest Anniversary Edition upstream); `c5cb1e789e` `EnumWarpAdapter()` returns an adapter |
| dinput, HID | `62bf2b0545` `Escape` returns `DIERR_UNSUPPORTED` (Thrustmaster wheels); `a7277ba021` opening a disconnected joystick fails; `5335bc6551`, `da5cdc1bde` link collections and separate hat-switch collections; `0e1616f1f6` HID device objects report the right device type; `135ae82919` `IOCTL_HID_WRITE_REPORT` returns the full report length (WineHQ bug 59891) |
| xinput | `ea50539dae` read events kept across device changes (also a handle leak); `4241f00fb7` no wait after a failed `CancelIoEx`; `1b79d86f73`, `2b11697164` controllers disabled on read errors and removal |
| winebus, winexinput, windows.gaming.input | `8df796f0f8`, `88e775dbdc` with `4377bdebd6` axis report sizes for the SDL bus Runeon uses, and the winexinput inversion that assumed 32-bit axes (logical ranges are unchanged, so calibration is not); `7f29964c15` HID axis usage mapping (DualSense); `b45d928aa1` gamepad-added callbacks registered before the provider starts |

Known differences: with a wined3d `SampleCount` registry override, a lockable back buffer becomes unlockable (`5279200712` validates the original description); `0e1616f1f6` makes hidclass answer `FileFsDeviceInformation` itself, which reports `Characteristics` 0 because the base lacks `b606598745` and `d2b0290d6a`; after `b45d928aa1`, a game that reads `Gamepads` once right after activation may see an empty list, as on Windows.

### Taken in 0147

`1c144117a8` (wine-11.18) flushes the current OpenGL context when the thread makes another context current or releases its context. `winemac.drv` is ABI-sensitive, so it is taken under the macOS-fix exception: it fixes a defect the base shares (switching between shared contexts on one thread, releasing a context before another thread samples what it drew, and the internal context `wglBindTexImageARB` pushes; the framebuffer-surface case upstream also covers is not built on macOS), touches Wine code CrossOver keeps unchanged and changes no protocol or structure. Upstream puts the line in `macdrv_context_activate()`, which a later refactor split out of the base's `macdrv_make_current()` and which upstream also reaches on release; here it is the first statement of `macdrv_make_current()`, before the release path. wined3d already flushes before switching its own contexts, so WineD3D games pay one extra empty flush per switch; same-context rebinds return early in `win32u` and never reach the driver.

### Left out after review

| Commits | Reason |
|---|---|
| `c7720f8698` (xinput state from the previous state) | Calls `get_current_state()`, added by `02d1858d8d`, which the base lacks; it would not compile. `dwPacketNumber` already advances in the base. |
| `1ec446b9ae` (swapchain latency left as set) | The tested behaviour is `Set/GetMaximumFrameLatency` (`fae70b64c6`, `7b605f9348`), which the base lacks; on the base it only raises the default frame latency from 2 to 3, one more frame of input lag. |
| `6481c20603` (d3d9 vtbl initialization sequence) | An i386 data marker that a signature scan in one game's launcher looks for, not a defect fix. |
| `2647661759` (frame latency through a semaphore), `0aa733c17a` (CPU blit helper) | Refactors that apply only after other commits. |
| `da9f8090bf`, `9787affece` (d3d8 lock boxes) | Upstream: they only silence a warning wined3d already ignores. |
| `13897c1d39` (dxgi fullscreen state) | An efficiency change. |
| Vulkan renderer, SPIR-V and HLSL back ends (`170c4c9083`, `bdc2fc0c8f`, `0f3858a83e`, `ccdaa2b581`, …) | wined3d runs on OpenGL on macOS. |
| Linux device back ends (`5c991013a8`, `8b8cdc9e68`, `fe9533f93a`, `cf5d043dc1`, …) | hidraw, evdev and inotify do not exist on macOS. |
| Performance work (`975b8929de`, `b3f63962dd`, `5b3196138b`, `90e5a9c436`, `7586644cad`) | Not defect fixes. |
| Features (`3d6a212b68` `DiscardResource()`, `34d085e3e0` video support, `1de2dcda90`/`e980a92009` shared resource flags, PnP and container IDs) | New behaviour rather than fixes. |
| GPU descriptions, clean-ups, tracing and FIXME changes | Not defect fixes. |
| OpenGL core-profile contexts (`b7ce33f16d`, `1f6bdd6360`, `fe16726680`, `e7e7825b3c`, `4b70bebd89`, `cc91be7b65`) | They rewrite context creation in `winemac.drv` and `win32u`, where `0080`, `0114` and `0130` already change the same code; adapting them is a rewrite, not a backport. |
| `ea527b0a9d` (raw resolve flag) | Fixes `2505de4d1f` (Vulkan, not in the base) and changes a wined3d export. |
| The commits that do not apply | Not adapted in this batch. |

### Verification required before tagging

No self-contained check. A complete WoW64 build and an application run must show that several WineD3D games (D3D8, D3D9, D3D11, and a DirectDraw title if one is available) render, present and enter and leave fullscreen as before, and that a game controller over USB or Bluetooth is detected, reads axes, buttons and the hat switch correctly, and survives being unplugged and replugged.

<a id="ntdll-win32u-server-fixes"></a>

## ntdll, win32u and server fixes (Wine 11.1 to master)

Candidate patch set `cx26.3-wine11.0-runeon.27` also adds `patches/upstream/0148`: twenty-eight WineHQ commits applied unchanged in upstream order, test-only hunks omitted, carried under the `ntdll`/`win32u`/`server` exception in §3 of `CLAUDE.md` (maintainer decision 2026-10-26). One, `06c0cb5c5e`, is on master after `wine-11.20` and not yet in a release.

### Selection

Of the 753 commits in `wine-11.0..master` that change `dlls/ntdll`, `dlls/win32u` or `server`, 202 apply with zero fuzz on top of `0001`–`0147`; 62 of those read as defect fixes from their message. The maintainer approved the behaviour fixes games and launchers reach together with the smaller ones; each was then checked:

- no hunk within three lines of a CrossOver change (compared with `wine-11.0`) or of an earlier Runeon patch;
- no later upstream commit names it, and no later commit to the same function fixes the same behaviour;
- no change to `protocol.def`, `server_protocol.h`, a unix-call table, an exported prototype or a structure shared between modules (`93939c0659` only adds a status name to the server's trace table);
- the added lines compile against the patched tree: every identifier exists, every call has the argument count of its definition, and every field accessed belongs to the structure it is accessed through. A presence check alone missed `MAKE_FNID` and `thread->is_system`; the independent review caught both;
- the changed code is reached on macOS x86_64.

### Taken

| Area | Commits |
|---|---|
| ntdll | `f6c53a242a` threadpool timers across a system time change; `1ebcc3fb9b` anonymous files; `c403790475` empty volume information (label, serial) instead of failure; `e8781e7c8d` Unix paths without a drive (WineHQ bug 60021); `2ecc2f84b4` `NtQueryFullAttributesFile` status; `dab7d8deda`, `5f881b67ea` directory entries fully initialized; `8c1dcfc77d` `LdrLoadDll` leaves the output handle alone on error; `06c0cb5c5e` semaphore errors ignored when releasing a critical section |
| win32u | `5aa9e20efe` `TransparentBlt` to a 32-bit DIB goes through 24 bits, so destination alpha is 0 as on Windows; `66a1d04995` glyph outline size (WineHQ bug 58314); `3e132b3cbb` `NtUserGetAtomName` overflow; `27aad48fa8` class registration thread safety (a deadlock upstream saw); `4d6a6cd036`, `65a3d0cf1c` window shape and surface flushing; `9ad182656c` D3D present rect in screen coordinates; `336df56a67` tool-window minimum width (WineHQ bug 60276); `e45f9ad9bd` signed drag-and-drop coordinates (winemac packs them the same way); `1c4cb6c6bf` monitors without an id; `da3b638c85` DirectX registry value sizes; `b4622e972d` `get_locale_data()` table lookup |
| wineserver | `93939c0659` `NtUnlockFile` on an unlocked range (WineHQ bug 59711); `bf98c93e00` file mode mask comparison; `63f8091369` freeing unlinked windows; `5bb70f23d1` kernel notification events' initial state; `864ca426a3` AFD connect failure events (WineHQ bug 53574); `51f87a11a1` d3dkmt leak; `1fe3600b16` a sizeof fix |

### Left out after review

| Commits | Reason |
|---|---|
| `b1a9794b14` (menu class id) | Calls the one-argument `MAKE_FNID()` from the FNID rework (`0541c1dfb0`, `e8508add15`); the base macro takes two, so it would not compile, and the base's window private data depends on the old layout. It also changes `user32`. |
| `720ea07775`, `ca175f340c` (system threads) | Use `thread->is_system`, added by `5155018f6f` with a protocol change; it would not compile, and the base has no system threads, so the defect is not there. |
| `5f47658c5c` (fiber data starts at 0x1e00) | Needs `ee656e1a1f`, which makes kernelbase stop treating a non-NULL `FiberData` as "already a fiber"; without it `ConvertThreadToFiber` fails on every thread. `ee656e1a1f` touches CrossOver's hack in `ConvertThreadToFiberEx`. |
| `32324bf1ef` (`LOCALE_CUSTOM_UI_DEFAULT`) | Uses `user_ui_languages_default` from the preferred-UI-language series (`c1197354df` … `f91f4404dd`), a feature the base lacks; it would not compile. |
| `bfae6eb8b4`, `9c027fac91` (time zones with inverse DST) | Upstream's later `e339429215` removes two `mktime()` calls per `get_timezone_info()` call that `bfae6eb8b4` adds, and it does not apply to the base; the wineserver half is left out with it to keep the two consistent. |
| `79e718d7bf` (`WM_PRINT` clip for children) | Clips away the non-client area of printed children until `d36039e588` fixes it; left out with it. |
| `730d06d7b7` (DPI contexts when reparenting) | Windows 10 1803 behaviour, and the base's `SetThreadDpiHostingBehavior` that lets programs opt out is a stub. |
| `c69173c460`, `fc5dd34cc3`, `0405e200e2` | Not reached on macOS: the `process_vm_readv` path (macOS uses Mach), the IPv4 branch macOS skips because it has `IPV6_DONTFRAG`, and EGL devices. |
| `e44e721ca9` (menu name ANSI conversion) | Its `user32/class.c` hunk sits next to `0063`. |
| `5e50024ec7`, `bf2a7ffd7a` (`LdrLoadDll` and `NtFlushVirtualMemory` prototypes) | Change prototypes in `winternl.h` and spec files shared between modules. |
| `f5e390911f` | ARM64EC only. |
| `f35078da93` (d3dkmt keyed-mutex wait handle) | Relies on newer in-process synchronisation caching, and D3DKMT is near CrossOver's D3DMetal path. |
| `e5abb7c03a` (revert of a `RaiseException` register change) | Upstream reverted because Windows 11 25H2 behaves differently; the behaviour depends on the Windows version. |
| `3e182735ea` (`NtReleaseSemaphore` argument check) | In `ntdll/unix/sync.c`, next to CrossOver's synchronisation code. |
| `ec61f02ccf` (normal window position) | Its hunks overlap code earlier Runeon patches change. |
| `4ac0555e55`, `67a26bddad`, `60c5ce0263`, `4c18d96e9b`, `96b01aea7a` (address-space reservation and DLL mapping layout) | Layout changes under Rosetta; reviewed one by one later. |
| `c50fc02868`, `399898d604`, `e5968847f5` | Compiler warnings or OpenBSD only. |
| `88f02a2c45`, `93442cbaee`, `9606d8c53a` | EGL and Linux dma-buf, not used on macOS. |
| `ce72727a65`, `a081733d48` | Old-WoW64 mode only. |
| The 140 applying commits whose message does not read as a fix, and the 479 that do not apply | Not reviewed one by one in this batch. |

### Verification required before tagging

No self-contained check. A complete WoW64 build and an application run must show that the Steam client logs in, downloads and launches several games of each graphics route; a launcher with child and tool windows draws them; text renders in a GDI-drawn game or launcher; and files dragged from Finder onto a window on a second display left of the main one land where dropped.

### Local candidate results for `.27` (2026-10-26)

Candidate built from `94dd4e2` in empty source and build directories (complete WoW64 build).

- Thread-pool and waitable timers (a probe that does not change the system time): pass on `.26` and `.27`, x64 and x86, within 2 ms.
- `ddraw1` upstream tests, x86, compared with `.26`: no new failure; the four `test_execute_buffer_updates` failures on `.26` pass on `.27` (`365e610147`). Both versions stall at the same point in `test_caps` during wined3d adapter initialisation; swapping in the `.26` `winemac.drv`, `wined3d` and `ddraw` does not remove it, so it predates `.27`.
- Maintainer's application run: the Steam client logs in, downloads and launches games; Heroes of Might and Magic: Olden Era (D3DMetal), Survival Log (DXMT), Dying Light and Telma-H (graphics route not recorded) run and present; a wired controller reads correctly in Steam's controller test; cursor release, minimise to the Dock and restore, launcher windows and text drawing look as before; Steam's free space matches Finder.
- Not checked: a game confirmed on the WineD3D route in this run (WineD3D is covered only by the `ddraw1` tests above), rich-text clipboard both ways, Finder drag and drop (including onto a display left of the main one), controller unplug and replug.

<a id="recall-fix-series"></a>

## Stall and correctness fixes from AsherJN/recall

Candidate patch set `cx26.3-wine11.0-runeon.29` adds `patches/runeon/0150`–`0154` after `0149`. The source is [AsherJN/recall](https://github.com/AsherJN/recall) at commit `a436e4cd4fac`, whose Wine patches are LGPL-2.1-or-later and target the same CrossOver 26.3.0 source. All changes taken are by AsherJN. WineHQ and CodeWeavers had not fixed any of them as of this base.

`ntdll`, `server` and `winemac.drv` are ABI-sensitive. The maintainer approved the series patch by patch on 2026-10-31; the approval does not widen the rule for other patches. None of them changes the wineserver protocol or matches a process. `0154` adds a unix call to the shared audio driver table, so mmdevapi and the driver must come from the same build; the runtime is replaced as a whole.

### Taken

| Patch | Change | Evidence on `runeon.28` |
|---|---|---|
| `0150` | Thread exit no longer close()s the msync alert index as a file descriptor. | `tests/msync-alert-fd-probe.c`, x86_64, `WINEMSYNC=1`: 2 failed reads of 6000 in each of 3 runs; 0 without msync. The alert indices seen in a server trace ranged from 115 to 6835. |
| `0151` | The periodic registry save runs in a forked child on macOS. | `tests/registry-save-stall-probe.c`, no msync, about 10 MB in HKCU: one 813.9 ms stall at the 30-second save. |
| `0152` | `dlopen()` of a builtin module's Unix library runs outside `virtual_mutex`. | `tests/unixlib-load-lock-probe.c`, warm Rosetta cache: slowest VirtualAlloc/VirtualFree 2.9 ms during the loads, 0.0 ms idle (3 runs). Small; kept only if the patched runtime shows a smaller stall. |
| `0153` | The displays-changed event on activation only follows a real resign, a screen change or a permission dialog. | Not reproduced: `tests/activation-click-probe.c` with `WINEDEBUG=+event`, 43 left clicks in a window gave one `DISPLAYS_CHANGED`, at the first activation. Adopted by maintainer decision on the recall project's report for full-screen games; to be checked in a full-screen game. |
| `0154` | mmdevapi follows Core Audio device and default device changes and notifies clients. | `tests/audio-device-change-probe.c`: switching the Mac output device several times in 60 seconds gave no notification. |

Adapted: `0151` forks only on macOS and closes inherited descriptors in the child; `0152` checks the library path again before recording the handle; `0154` places one `create_stream` hunk by hand, pins mmdevapi and the driver for its watch thread, and adds the unix call as not implemented to the ALSA, PulseAudio and OSS drivers.

### Left out

| Change | Reason |
|---|---|
| `WINE_SYSCALL_LOG`, `WINESERVER_REGISTRY_LOG` and the display log patches | Diagnostics only. |
| The Bluetooth microphone policy in `wine-audio-devices.patch` | Overrides the microphone the user chose in macOS. |
| The rest of `wine-winemac-activation.patch`, mouselook, the canvas patch | Features or behaviour changes beyond the defects above. |
| `wine-portable-directory-boolean.patch`, `wine-service-security.patch` | Outside the five defects the maintainer selected; not reviewed. |
| The DXMT patches | DXMT is maintained outside this repository. |

### Not verified yet

Behaviour on the patched runtime: each probe above is rerun on a complete i386/x86_64 WoW64 build, with Steam and representative games, with and without msync and Retina mode, and `0153` in a full-screen game.

<a id="rosetta-cpu-fidelity"></a>

## Rosetta CPU fidelity

Candidate patch set `cx26.3-wine11.0-runeon.30` adds `patches/runeon/0157`–`0160` after `0156`. These are downstream changes, not WineHQ commits; WineHQ and CodeWeavers have no equivalent on this base.

Rosetta's x86 differs from a real CPU in ways a program can observe: CPUID leaf 7 omits CLFLUSHOPT, which Rosetta executes; CLWB and RDPID are missing; there are no debug registers; and a non-canonical access is reported at its address instead of as #GP. Copy protection that checks the CPU this way faults endlessly under Rosetta. CrossOver 26.3 fails the same way.

`0157`–`0159` change `ntdll`, which is ABI-sensitive. The maintainer approved them as one opt-in group on 2026-11-02; the approval covers these patches only and is no precedent. They take effect only under Rosetta and only when `HKCU\Software\Wine\AppDefaults\<app.exe>\Runeon` has DWORD `RosettaCpuFidelity` = 1. Wine never decides by game name, Steam AppID or executable name; a launcher sets the value for a program it has decided needs it. The changes make the emulated CPU report and behave as Intel and Windows define it, and hide the trap flag Wine sets for stepping; while a breakpoint is armed, a trap flag the program sets with popf is hidden too and not delivered, one it sets in the context it continues with is delivered. They do not read, change or skip program code, licence data or activation results.

`0160` is a data-only Windows-behaviour fix without an opt-in, approved individually on the same date and listed in `MAINTENANCE.md`.

### Taken

| Patch | Change | Evidence |
|---|---|---|
| `0157` | Opt-in policy; execution breakpoints a thread sets on itself (current-thread pseudo handle) are emulated by single-stepping, with DR6, read-back, a hidden trap flag, the program's own trap flag set in its context, breakpoints on faulting instructions and the resume flag. | Local experiments on `runeon.29`: a copy-protected game armed one execution breakpoint per thread and stopped before graphics initialisation without it. With this emulation and `0159`, 173 breakpoint checks hit and the game reached gameplay. `tests/rosetta-cpu-fidelity-probe.c`. |
| `0158` | With the policy, a non-canonical access is reported as #GP (address -1). | The same game reads non-canonical addresses on purpose; Windows reports them with address -1. `tests/rosetta-cpu-fidelity-probe.c`. |
| `0159` | With the policy, CPUID leaf 7 adds CLFLUSHOPT, CLWB and RDPID in this process; CLWB and RDPID are emulated; one marker per process. | Without it the game stopped in its CPU checks even with `0157`; with it the checks passed. On macOS 27.0 CPUID inside `ntdll.so` does not see the table change, so the self-check runs CPUID from code generated at run time. |
| `0160` | Builtin `vcruntime140` and `msvcp140_1` report 14.50.35719, as the other VC14 builtins do since `0001`. As with `msvcp140` since `0001`, an MSI redistributable older than that no longer replaces the builtin copies in `system32`; with default load order the builtin was loaded before as well, so only a native override of these two DLLs sees the difference. | The game reads the version of each loaded VC14 runtime DLL; with no version on these two it failed its check and exited. With Microsoft's 14.51 DLLs it passed. `tests/vc14-version-probe.c`. |

The breakpoint on a faulting instruction was not reached in the runs that passed; it is kept because it is how x86 orders a debug fault before the instruction's own fault.

`0159` changes a row only when exactly one matches. The page also holds Rosetta's code; on a future layout macOS might reject the changed page and terminate the process instead of `0159` logging `table_not_found`. The scan runs before Wine's signal handlers are installed.

### Left out

| Change | Reason |
|---|---|
| Showing Windows segment selectors (CS 0x33, SS 0x2b) in exception contexts | Tried during the investigation; it did not change the result. |
| Diagnostic logging of each breakpoint hit, the exceptions after it, and NULL heap callers | Diagnostics only. |
| Reading back debug registers without emulating them | Tried before `0119`; it does not help a program that needs the breakpoint to fire. |

### Verified

On a minimal i386/x86_64 WoW64 build of the patched tree (no GStreamer, Vulkan, SDL or FreeType), fresh prefix, macOS 27.0 under Rosetta: `tests/rosetta-cpu-fidelity-probe.c` passes with the policy off, on, and set to 2 (treated as off), including the program's own trap flag while a breakpoint is armed (DR6 `0xffff4ff0` with the policy on), and with it on logs `RUNEON_ROSETTA_CPUID7 active rows=1`, leaf 7 EBX `0` to `0x01800000`, ECX `0` to `0x400000`. `tests/vc14-version-probe.c` passes for x86_64 and i386; in the `runeon.29` runtime `vcruntime140` and `msvcp140_1` carry no version resource for either architecture.

### Not verified yet

The complete product build, Wine's ntdll exception tests, the game with the policy on, and Steam and other games with it off. The CPUID table layout is checked only on macOS 27.0; on another layout `0159` logs `table_not_found` and CPUID is unchanged.

<a id="backports-11.21-sweep"></a>

## Wine 11.21 user-mode fix sweep

Candidate patch set `cx26.3-wine11.0-runeon.31` adds `patches/upstream/0164-wine-11.21-targeted-fixes.patch` after `0163`. It squashes 30 WineHQ commits first released in `wine-11.21`. The patch lists every commit with its author, and the manifest records the full commit IDs. Test-only hunks are omitted.

### Selection

All 261 commits of `wine-11.20..wine-11.21` were reviewed with the rules of the [previous sweep](#backports-11.20-sweep), against a tree made of the pinned CrossOver archive plus patches `0001`–`0163`:

- 32 commits change only tests.
- 77 commits change only `ntdll`, `server`, `win32u`, `winemac.drv`, `wow64`, `include`, the build tools, bundled libraries, translations or the X11 and Wayland drivers. Most are feature work: d3dkmt swapchains and sync objects, memory-reservation changes, ARM64EC/noentry build support and token creation. Each was read. Three small fixes are carried with the maintainer's individual approval (below); the rest are left out. `06c0cb5c5e` (ntdll critical sections) is already carried in `0148`.
- Of the 152 user-mode commits, the sweep takes crash, memory-safety, leak, hang and error-path fixes in DLLs that Steam, launchers, installers or games load, and two d3d11 fixes under the graphics rule. It leaves out feature series (mfwmaaec, ncrypt keys and padding, bcrypt DES, wbemprox services, msxml3 DTD and notation nodes, dmusic compressed waves, the winecoreaudio SpatialMixer, wpnapps stubs, the advapi32 user name rework), stubs and clean-ups.

For each taken commit the defect listed below was confirmed in the base source, not inferred from a clean apply; each applies to that tree in upstream order, except the adapted ones. No later master commit names a taken one (checked up to `7692c6213e`).

### Taken

| Area | Commits | Defect fixed |
|---|---|---|
| dplayx | `81db815a81`, `dd586289a4` | The Unicode `IDirectPlay4::GetSessionDesc` returned ANSI strings, and the Unicode `IDirectPlayLobby3::Connect` returned an ANSI interface; a Unicode caller reads them as WCHAR strings. |
| crypt32 | `a358d7ecf1` | Signature verification re-sorted the authenticated attributes before hashing them, so a valid signature whose attributes are not in DER order was rejected (`TRUST_E_CERT_SIGNATURE` through wintrust). A launcher or installer checking its own signature then refuses to start (Wine-Bug 60273). |
| user32 | `cd3e9ba0a7` | `DialogBox` truncated a 64-bit `EndDialog` result to 32 bits; a caller using it as a pointer crashes. |
| winebus.sys | `e90cb1c257` | `SDL_Init` installed SDL's own handler for termination signals that winedevice leaves at the default, such as SIGTERM, so the signal became an SDL quit event that nothing handles and no longer stopped the process. |
| wininet | `3fef3b5d3a`, `70b5903fde` | An uncommitted cache file was deleted while its handle was still open, so the delete failed and the file stayed (Wine-Bug 60560). `HttpSendRequestExA` set the converted buffer's `dwStructSize` to a pointer size. |
| comctl32 | `4b2e49fb5a` | ComboBoxEx passed a NULL item text to `lstrcpynW` when the dropdown closed: a crash (Wine-Bug 60495). |
| mmdevapi | `4751826ecd` | The allocation check in `MMDevCol_Create` tested the count instead of the allocation. |
| winecoreaudio.drv | `5a66c3e869` | The QPC position of a stream was computed as `counter * 10000000 / frequency`, which overflows after about 25.6 hours of host uptime; on macOS the counter runs from boot, so clock positions are wrong on most long-running Macs. The frequency is already 10 MHz, so the counter is returned as is. Also changes winealsa, wineoss and winepulse, which are not built on macOS. |
| dbghelp | `bcff0d4155` | `MiniDumpWriteDump` on its own process ran on a new thread; a crash handler that has suspended its other threads first deadlocked. It now runs on the calling thread, as on Windows (Wine-Bug 60547). |
| gdiplus | `21ffe25da3`, `7c7438bddb`, `088fedbdbc`, `8de6d2d2a5` | An uninitialised path was freed on an error path; flattening a Bezier with non-finite points or zero flatness looped forever (Wine-Bug 60041); a negative offset in `GdipDrawCurve3` read before the point array; an open figure followed by another left one edge on its scanlines and hit an assertion in software filling. |
| dmusic | `0568158eca`, `c56f0edd92`, `0738d528db` | A second `fmt ` chunk leaked the first format; a `fmt ` chunk shorter than `WAVEFORMATEX` was read past its allocation; the chunk size was never checked against the format. |
| wmp | `d52e0901b5` | `IOleObject::GetClientSite` returned the site without a reference, so the caller's release freed it. |
| d3d11 | `55719ae5f1`, `ca0bb83d63` | `FinishCommandList` wrote through a NULL output pointer instead of returning `E_UNEXPECTED` (Wine-Bug 60467); `ID3D10Device1::SetExceptionMode` returned `E_NOTIMPL` where Windows returns success. Taken under the graphics rule; only wined3d-backed D3D10/11 reaches them. |
| comsvcs | `c85f71ce3f` | `guid_from_string` validated only the first character of the second GUID group and indexed its conversion table with the unchecked rest. |
| d2d1 | `6f981b85d7`, `9b1eb46383`, `9e375e9f93`, `535d68f02a` | A leaked readback texture on WIC target failure; the `Inputs` property pointer used after the property array may have been reallocated; `GetPropertyNameLength` returned an error code as a length; a failed effect creation released the context instead of the effect. |
| dwrite | `7219e03eff` | A font set property lookup could return an uninitialised pointer. |
| ntdll | `c37cb58e4c` | Idle threadpool workers exited after 5 seconds; Windows 10 keeps them for about 67. Upstream reports that a copy-protected game crashes when a worker exits while its code is still being decrypted. The base has 5000 ms. |
| win32u | `45f00f925f` | `release_display_manager_ctx()` released the display device mutex before `cleanup_devices()`, so another process preparing devices could have its GPU registry keys deleted (Wine-Bug 55346). The base has the pre-fix order. |
| winemac.drv | `98ebfcd75f` | Single-buffered pixel formats did not report `PFD_SUPPORT_GDI`, so a program that asks for it found no format. The base describes pixel formats as upstream did before the fix. |

### Left out after review

| Commit | Reason |
|---|---|
| `01a9c6c8ac` dnsapi, `d7f953859d` winhttp, `5f5b46ccd2` crypt32, `fdbe8914d7` msv1_0, `224a61be55` wintab32 | The base does not have the defect: the code is absent (no hosts-file reader, PFX export or credential attribute query) or already correct (CrossOver's winhttp option size; wintab32 registers and unregisters its class with the same NULL instance). |
| `3630b2d151`, `0408d15bef`, `cbb82a7d8e`, `038eccf05e`, `d0e2a7d200`, `b132754ce5`, `8ac1870e39`, `c3c5a9d190`, `cfde576623`, `20f41e857a` msxml3 | Fix the msxml3 implementation that replaced libxml2 after 11.0; the base still uses libxml2. |
| `66caeb46ae` cmd | The HELP use-after-free is in the word-iterator code that the base does not have. |
| `569e6b6a75` winhttp | Raises the reply line limit to 64 KB (Wine-Bug 60562). CrossOver's `read_reply` differs, so it needs a hand adaptation; no product failure is known. |
| `cdc6f6c942` wininet | Depends on the `write_cache_file()` helper the base does not have. |
| `a3e7d10b56` comsvcs | The base's `holder_Close` also releases a driver reference it never took, but its holder predates upstream's rewrite, which moved where the reference is released, so the fix is not mechanical there; comsvcs is not on a common game path. |
| `244a31a814` kernelbase | Lets `SHRegGetUSValueW` run with NULL data and size; with default data that then dereferences the NULL size in `SHRegQueryUSValueW`. A behaviour change, not a crash fix. |
| `e3c3d34867` oleaut32, `8023acff4a` rpcrt4, `a0f1ef41f4` msvcrt, `c1f55d60f3` wininet, `70577c3e3c` dwrite, `93b006bfe4`, `3b8a6501a2`, `8bb2a57669`, `191766ac0a`, `9af1cb53eb`, `7832417985` gdiplus, `1e3979fd3c` comctl32, `3c3421b563` advapi32 | Correctness or rendering changes with no crash, leak, hang or error path; `3c3421b563` changes a debug string only. |
| `6c11b196e2`, `a7ca6ed5cb`, `d45ac538c0` user32/comctl32 | Reorder control state flags around `SetCapture()`; a behaviour change. |
| `2cdb2c4128`, `e6d169b724` userenv | The calls already succeed in Wine; they only match Windows' stricter parameter rules. |
| `1f8d7f1cd3` winegstreamer, `1695ab626a` mf | Change which media streams are selected; a playback behaviour change that needs its own testing. |
| `b315cdff0a` mfplat, `bfdb48f4e1` d3d12, `f2f287c927` wmp, `943fb73d5f`, `4f7d045312` infosoft, `d658978505` hhctrl | Stubs and QueryInterface changes, or DLLs games do not load. |
| `3493088b1c`, `2a662c61b5` setupapi | Device driver installation only; the first restructures `SetupCopyOEMInf`. |
| `5b39e109cb`, `c99f5c7b03` appwiz.cpl | The addon installer, not loaded by games. |
| `bf225d4305` xinput | A performance fix for a commit the base does not carry. |
| `8891603b59` mmdevapi | Follows `da0b08472b` (11.16), which the base does not carry. |
| `42809af4b3` wined3d | Vulkan renderer only. |

### ABI-sensitive commits

`c37cb58e4c` (ntdll), `45f00f925f` (win32u) and `98ebfcd75f` (winemac.drv) are in ABI-sensitive modules. The maintainer approved carrying them individually on 2026-11-04; the approval covers these three only and does not widen the rule. None changes a unix-call or server protocol or a shared structure. The thread pool timeout differs between Windows versions; 60 seconds is upstream's value.

The rest stay out:

| Commit | Area | Reason |
|---|---|---|
| `5fcf25652c` | ntdll virtual memory | Fixes the alignment check in upstream's `map_reserved_area()` after its post-11.0 reservation rewrite. The base's `map_reserved_area()` goes through `find_reserved_free_area_outside_preloader()` and has no such check to fix (Wine-Bug 60534). |
| `96283c8170` | ntdll virtual memory | Changes `virtual_free_thread_data()`, which the base does not have; it frees threads through `virtual_free_teb()` and has no `process_exiting` flag (Wine-Bug 60545). |
| `6725f8d94f` | winemac.drv | Switches the IME view back to `NSTextInput`; no defect is stated and it touches CrossOver window code. Held until a CJK IME test. |
| `9f4469dbfb`, `1698588a0c`, `ef95772aee`, `9bc785a86e`, `5273b891fe`, `5a29cf122a` | wow64, ntdll, server | Token creation and `NtTerminateProcess(NULL)`; change the server protocol or wow64 thunks. |

### Adaptations

| Commit | Adaptation |
|---|---|
| `a358d7ecf1` crypt32 | The new declaration goes after `cng_prepare_signature()` in `crypt32_private.h`; the upstream context names a declaration added after 11.0. The code changes apply unchanged. |
| `e90cb1c257` winebus.sys | The base already loads `SDL_SetHint` for CrossOver's rumble hints, so only the `SDL_HINT_NO_SIGNAL_HANDLERS` line is added, before `SDL_Init`. |
| `4751826ecd` mmdevapi | The same one-line change; CrossOver's branch also leaves the device lock. |
| `0568158eca`, `c56f0edd92`, `0738d528db` dmusic | The same lines in the base's `fmt ` case, which upstream reordered in `b8b7181ec9` (not taken). |
| `45f00f925f` win32u | The same two lines move above the mutex release; the base frees its GPU lists differently below them, so the hunk context differs. |

### Known risk

`bcff0d4155` includes the calling thread in the dump and reads its context with `GetThreadContext` on itself, without suspending it; the dump then shows that thread inside `MiniDumpWriteDump`, as on Windows. A crash reporter that wrote dumps through the helper thread before will now write them on its own stack.

`cd3e9ba0a7` widens `dialog_box_loop` in user32's `wow_handlers32`; `user.exe16` keeps its own `INT` declaration, as upstream does. Both are 32-bit there, so the 16-bit path is unchanged.

### Downstream fixes in the same patch set

`0162` (wined3d) and `0163` (ntdll) are downstream, approved individually by the maintainer on 2026-11-04 and listed in `MAINTENANCE.md`. For `0162`, `wined3d_stateblock_state_cleanup()` and `wined3d_stateblock_reset()` in WineHQ master (`7692c6213e`) still match the base, so there is no upstream commit to take.

### Validation

`scripts/static-check.sh` and `scripts/integration-check.sh` pass for `.31`. Every taken commit applies in order without fuzz, except the adapted ones. Not done yet: a complete i386/x86_64 build, Wine's tests for the affected modules against `.30`, and product acceptance.

`upstreamAuditThrough` stays at `wine-11.15`. The base manifest records `upstreamSweepThrough: wine-11.21`.

<a id="metal-shader-cache-expand"></a>

## Metal shader cache expansion

Candidate patch set `cx26.3-wine11.0-runeon.32` adds `patches/runeon/0165` after `0164`. It is a downstream change, not a WineHQ commit; WineHQ and CodeWeavers have no equivalent on this base.

Metal keeps the GPU binaries it compiles in a per-process disk cache. Its `MTLCompilerFSCache` constructor reads `MTL_SHADER_CACHE_SIZE` and `FS_CACHE_SIZE` with a 1 GiB default and clamps any larger value back to 1 GiB, so the variables can only lower the size. Once the cache is full, new binaries are not kept. A program whose pipelines compile to more than 1 GiB therefore compiles the remainder again on every start, even though its own caches and the translation layer's caches are intact.

`0165` changes `ntdll`, which is ABI-sensitive. The maintainer approved it individually on 2026-11-05; the approval covers this patch only and is no precedent. It takes effect only under Rosetta and only when `HKCU\Software\Wine\AppDefaults\<app.exe>\Runeon` has DWORD `MetalShaderCacheExpand` = 1, which a launcher sets per program. It is never keyed on a program in Wine. It does not touch any file on disk, the program's own code or data, unix-call tables or the wineserver protocol.

### Taken

| Patch | Change | Evidence |
|---|---|---|
| `0165` | Opt-in policy; the `0157` AppDefaults reader takes the value name. A dyld add-image callback finds the 45-byte clamp sequence in Metal's `__TEXT,__text` and, only if it occurs exactly once, raises the default, the comparison and the clamp to 3 GiB in this process's copy; one marker per process. | Disassembly of x86_64 and arm64 Metal on macOS 27.0.1 shows the same clamp. A standalone test program with 7,600 distinct compute pipelines: at the default limit the cache stayed at 1.14 GB and the second run took about 116 s; with the clamp raised in-process under Rosetta the cache reached 1.53 GB and the second run took 4.8 s. Running without the change afterwards, Metal trimmed the cache back to 1 GiB, with no failed pipeline. |

The size field is 32 bits, so 3 GiB leaves room below the 4 GiB maximum. A first start with the policy still compiles everything once.

### Left out

| Change | Reason |
|---|---|
| Setting `MTL_SHADER_CACHE_SIZE` above 1 GiB | Clamped by Metal; tried, the cache did not grow. |
| Patching the size field of each cache object instead of the code | Needs a hook on a private constructor; the code change has the same effect with one match check. |
| Enabling it for every program | Not needed for programs whose binaries fit in 1 GiB; kept opt-in until more programs are checked. |

### Validation

Not done yet at the time of writing: a complete i386/x86_64 build of `.32` and a program run through Wine with the policy on.

<a id="concurrent-vector-spatial-mixer-batch"></a>

## Concurrent vector, SpatialMixer and D3DMetal diagnostics batch

Candidate patch set `cx26.3-wine11.0-runeon.33` adds `patches/upstream/0166`–`0168` and `patches/runeon/0169`–`0170` after `0165`. The maintainer approved `0169` and `0170` as named items of this batch on 2026-11-06; both change `ntdll`, which is ABI-sensitive. The approval covers these patches only and is no precedent.

The batch follows public reports of a game that runs on D3DMetal and crashes or loses audio there. The reports name the defects; each one below was checked against the base source, not inferred from the reports. Nothing in Wine is keyed on a program.

### Taken

| Patch | Source | Change | Defect in the base |
|---|---|---|---|
| `0166` | WineHQ `d5c7ea9089`, `de43f5434f` (wine-11.8) | `concurrent_vector` in msvcp90 (exported by msvcp100, msvcp110 and msvcp120) and concrt140: `_Internal_capacity()` no longer counts a segment that another thread is still allocating; concrt140 no longer multiplies segment sizes by the element size twice or does element arithmetic on `BYTE **`. `de43f5434f` also restores the NULL callback check in msvcp90's `threadpool_callback()`. | Both files test only `!this->segment[i]`, so `_Internal_reserve()` returns early while the marker (`SEGMENT_ALLOC_MARKER`, 1) is in place and `_Internal_push_back()` returns the marker plus an offset as an element address. concrt140 places segments 1 to `first_block - 1` eight times too far into segment 0. |
| `0167` | WineHQ `199ae1d33c`, `d20f6c71e7`, `3e29c3639e`, `8cf640803b`, `0c61795bfd` (wine-11.21) | A shared-mode render stream with more than two channels and a channel mask that differs from the device's goes through a SpatialMixer audio unit (equal-power panning) in front of HALOutput. | HALOutput maps channels but does not downmix, so a 7.1.4 or 5.1 bed on a 2-channel output loses its centre, surround and height channels; the base has no mixer. Public report: [highball#313](https://github.com/gauthierpiarrette/highball/pull/313) (dialogue and most engine audio missing on 2-channel outputs). |
| `0168` | WineHQ `19d5fbd382` (wine-11.19) | `WSAIoctl(SIO_UDP_NETRESET)` succeeds as a stub, like `SIO_UDP_CONNRESET`. | The base's `WSAIoctl()` reaches its default case and fails with `WSAEOPNOTSUPP` (10045). |
| `0169` | Downstream | Opt-in policy `D3DMetalReportNVIDIA` (below). | D3DMetal reports a vendor of its own choosing; see below. |
| `0170` | highball-engine patch `0028` (Andrii Rozhanskyi, LGPL-2.1, commit `0e9cb560a4`) | With `WINE_CRASH_ON_FOREIGN_FAULTS=1`, a fault on a thread without a TEB restores the signal's default action so the process ends with it. | `segv_handler`, `trap_handler` and `fpe_handler` read the TEB first (`save_context()`, the `0107` step check), which a thread Wine did not create does not have. |

`0167` reverses the [11.21 sweep](#backports-11.21-sweep), which left the SpatialMixer out as a feature series: the missing downmix loses dialogue and engine audio on 2-channel outputs, so the maintainer now treats it as a defect fix. It does not meet the macOS exception in §3 of `CLAUDE.md`, whose fourth condition leaves out upstream refactors and any fix that only works with one: `199ae1d33c` moves code, the other helper commits extract code, and two hunks had to be placed by hand. It is carried on the maintainer's approval of this batch, which names these prerequisites, and that approval widens no rule. The other conditions hold: CrossOver changes only the capture authorisation in that file, which the batch does not touch, and no unix-call or structure shared with mmdevapi changes. Like upstream, it has no opt-in, so every stream it selects changes path.

### Upstream follow-ups

Searched up to `c42c1532a8` for later commits that name a taken commit or touch the same code:

| Commit | Decision |
|---|---|
| `7c1d8456e1`, `1ea7620d9d` | A change to `_Internal_reserve()` and its revert; together they change nothing and are not carried. |
| `8845229a35` (wine-11.4) | Already carried as `0016`; it removed the callback check that `de43f5434f` restores, and upstream master has the check in both DLLs. |
| `07c7fcc86f`, `8e09602ca6`, `da0b08472b` (wine-11.16) | Period frame size and sample-rate helpers that only provide context for two hunks of `0167`; not needed by the fix, and `0129` already covers the period problem. Not carried. |
| `5a66c3e869` | Already carried in `0164`. |
| `8891603b59` | Fixes `da0b08472b` in `mmdevapi/spatialaudio.c`; not applicable, since `da0b08472b` is not carried. |

No later commit names `0c61795bfd` or its prerequisites, `19d5fbd382`, `d5c7ea9089` or `de43f5434f`.

### Adaptations

| Commit | Adaptation |
|---|---|
| `199ae1d33c` | Removes the old copy of `dev_id_from_device()`. The function is identical in the base, but upstream context shows `get_device_period_frame_range()` after it (`07c7fcc86f`, not carried), so the removal is placed by hand. |
| `8cf640803b` | Replaces the channel probing in `unix_get_mix_format()` with `get_device_channel_mask()`. Upstream's context reads the sample rate through `get_device_sample_rate()` (`8e09602ca6`, not carried); the base's inline read is kept, and the property scope and element it relied on are now set right before it. |
| `0c61795bfd` | `use_mixer()` also requires each channel mask to have one bit per channel, for the stream and the device; otherwise the previous path is kept. Upstream sends such a stream to the mixer, whose empty channel bitmap (`get_channel_mask()` above 8 channels) fails with -10851, so `IsFormatSupported()` and stream creation return `E_FAIL` for every input on an output with more than 8 channels, and for streams of 9 or more channels with mask 0, where HALOutput only warned. In offline runs 8, 16 and 32-channel and 2-channel mask-0 outputs failed at the output layout for all inputs; non-zero masks succeeded. |
| `0c61795bfd` | Zeroes the `AudioComponentDescription` with `memset(&comp_desc, ...)`. Upstream writes `memset(&desc, 0, sizeof(comp_desc))`, which leaves `componentFlags` and `componentFlagsMask` uninitialised and can make the SpatialMixer lookup fail, failing stream creation with `AUDCLNT_E_DEVICE_INVALIDATED`. Upstream master still has the line. |

Every other commit applies in upstream order without fuzz. `0168` omits its test hunk.

### Behaviour of the `0167` fold-down

- A HALOutput loopback of a 7.1.4 stream on a 2-channel output measured centre 0.250/0.250, rear left 0.250/0 and top front left 0.250/0, but top back left and right 0/0 and LFE 0/0. Centre, surrounds and front heights are retained; back heights and LFE remain lost.
- The fold-down has no limiter. The per-channel gain sum reaches 2.41 for 5.1, 3.12 for 7.1 and 3.83 for 7.1.4, so loud mixes may clip; this has not been verified by ear.
- A mono format without `WAVEFORMATEXTENSIBLE` now asks HALOutput for layout bitmap `0x4` (accepted) instead of `0x3` (rejected, with a warning), with the same output.

### D3DMetal adapter identity (`0169`)

D3DMetal fills in the DXGI adapter description itself. `D3DM_VENDOR_ID`, `D3DM_DEVICE_ID` and `D3DM_DEVICE_DESCRIPTION` are read from the process environment on the Unix side; this is not yet confirmed end to end on D3DMetal 4.0 beta 3. `SetEnvironmentVariable()` changes only the Windows environment. Public reports show programs that take a vendor-specific path on D3DMetal's default and crash there.

With `HKCU\Software\Wine\AppDefaults\<app.exe>\Runeon`, DWORD `D3DMetalReportNVIDIA` = 1, read with the `0157` reader, `signal_init_process()` sets vendor `0x10de`, device `0x2786` and the description `NVIDIA GeForce RTX 4070` and logs one `RUNEON_D3DMETAL_ADAPTER` line. If the caller has set any of the three, it sets none, so an identity is never mixed. This runs before any Windows code and before D3DMetal is loaded. The values are fixed in Wine; a launcher can only turn the policy on for a program. A child process inherits the Unix environment, so `RUNEON_D3DM_SET` records that the policy set the variables and every process drops them at start; a program's policy does not reach its children. Reporting NVIDIA can open NVIDIA-only paths such as DLSS or Reflex in a program; that is why it stays per program.

### Foreign-thread faults (`0170`)

Apple's libdispatch workers run Metal and D3DMetal code inside a Wine process on threads Wine did not create. Highball's report: when one of them faults, Wine's handler faults again reading the missing TEB while the signal is blocked, and the thread spins while the rest of the program keeps running, so the program looks hung and no log shows the fault. `0170` is opt-in through `WINE_CRASH_ON_FOREIGN_FAULTS=1` because it turns that hang into a process exit; it is a diagnostic, not a fix of the fault itself. Code is unchanged from the original except for the opt-in and the trap handler check, which goes before the `0107` step check because that check also reads the TEB.

### Validation

`scripts/static-check.sh` and `scripts/integration-check.sh` pass for `.33`, and the series reproduces the reviewed tree exactly. A complete i386/x86_64 WoW64 build succeeds with the configuration in `BUILDING.md`, with GStreamer, SDL2, FreeType, GnuTLS and MoltenVK present. It does not define the managed launch validation and has no signing, so it runs directly and is not the product build. On that build, in a fresh prefix, on macOS 27 under Rosetta:

| Patch | Result |
|---|---|
| `0166` | `tests/concurrent-vector-probe.c` (x86_64) passes 4 of 4 runs for concrt140 and msvcp120. With the two commits reverted and the DLLs rebuilt, it fails 3 of 3 runs for both: `push_back()` returns addresses such as `0x1`, `0x9` and `0x11`, and concrt140 asks the allocator for 64 elements instead of 16 and places segments 1 to 3 eight times too far. |
| `0167` | A shared-mode 12-channel 7.1.4 float stream on the 2-channel default output: `IsFormatSupported()` and `Initialize()` return `S_OK`, the trace shows a 12-channel mixer input in front of a 2-channel HALOutput format, and a second of silence drains. A 2-channel stream (`tests/coreaudio-io-buffer-probe.c`) creates no mixer. |
| `0168` | `WSAIoctl(SIO_UDP_NETRESET)` returns 0 on a UDP socket, for x86_64 and i386. |
| `0169` | A dylib injected into the processes calls `getenv()` from a native image. With the DWORD set to 1 the program reads the three fixed values and logs the marker, and a child without the policy reads none. With any one variable set by the caller the policy sets none, and that value stays in both. A child with its own policy gets the values again. With the key absent, the DWORD set to 2 or a `REG_SZ` value, nothing is set. |
| `0170` | A pthread injected into a Wine process with a dylib reads address `0x14` after three seconds; its `%gs:0x30` reads NULL. Without the variable, and with it set to 2, the process used about 100% CPU at eight seconds while the program carried on, which reproduces the defect; with `WINE_CRASH_ON_FOREIGN_FAULTS=1` it ended with `SIGSEGV`. |

### Not verified yet

- The product build and signing, Wine's own tests for the affected modules against `.32`, and product acceptance.
- `0166` on i386 and in msvcp100 and msvcp110; the restored chore callback check.
- `0167` against `.32` with the same stream, by ear or by measuring the output, on multichannel outputs, with 5.1 or integer sources, and across device changes.
- `0168` with an online program.
- `0169` end to end: no adapter description was read back through D3DMetal. The effect on the program it is meant for is unknown.
- `0170` for `SIGTRAP` and `SIGFPE` on such threads, and with a real Metal or D3DMetal fault.

<a id="boolean-syscall-args-msync-cleanup"></a>

## BOOLEAN system call arguments and msync node cleanup

Candidate patch set `cx26.3-wine11.0-runeon.35` adds `patches/upstream/0173` and `patches/runeon/0174`–`0175` after `0172`. All three change ABI-sensitive code (`ntdll`, `server`). On 2026-11-10 the maintainer approved fixing them on the condition that code review confirms a defect; this review confirmed each one (reproduced on `.34` for `0173` and `0175`, by code reading for `0174`). The approval covers these patches only and is no precedent. Nothing is keyed on a program.

Four candidates were checked against the base source; online reports were leads only.

### Taken

| Patch | Source | Change | Defect in the base |
|---|---|---|---|
| `0173` | WineHQ `f43402cde3` (wine-11.1, Alexandre Julliard), x86_64 part | The 36 system calls with a `BOOLEAN` argument go through a stub that zero-extends it before jumping to the real function. Built only with clang on x86_64. | clang assumes the caller extended a `BOOLEAN` argument to 32 bits, which neither the x64 convention nor Windows requires; Windows reads only the low byte. In the `.34` `ntdll.so`, `NtQueryDirectoryObject()` tests `restart` with `testl %r8d`, and `NtDelayExecution()`, `NtContinue()`, `NtDuplicateToken()`, `NtQueryDefaultLocale()` and `NtSetDefaultLocale()` also test 32 bits. |
| `0174` | Downstream | When an msync index drops to refcount 0, the pump runs `signal_all_internal()` on it, waking and freeing the nodes left on its list. | See below. |
| `0175` | Downstream | The node pool exhaustion warning is written once per wineserver. | `pool_alloc()` wrote it to unbuffered stderr on every registration after the pool ran out, on the thread that serves all msync waits. |

`0173` fixes the cases clang currently compiles with a 32-bit test, and also the others, because the code generation can change with any rebuild. A fix for only the functions that read 32 bits today would not hold. The stubs touch only the syscall table entries, so calls between Wine's own Unix modules are unchanged. The WoW64 path converts arguments in C and is not affected.

### Adaptation

`f43402cde3` adds the wrappers to `dlls/ntdll/unix/syscall.c`, which `d1415ab24e` (same day, wine-11.1) created by moving the syscall table out of `loader.c`. That move is not carried: the block goes right before the table in `loader.c`, with the `wine/asm.h` include. The ARM64 branch is left out. No later function in `loader.c` uses a redirected name; the file compiles to an object with clang (x86_64) against the patched headers. CrossOver's `GPT_ABI_WRAPPER` symbols, such as `NtCreateEvent`, check the return address; the stub jumps without a call, so that address is unchanged.

### Upstream follow-ups

Searched up to `78e52205d9` (wine-11.21-164): no commit names `f43402cde3`. `2a61baa185` adds win32u wrappers for ARM64 only and is not carried. `9e03f3324c` changes only includes in `syscall.c`. The test hunk `565091afa4` is replaced by `tests/syscall-boolean-args-probe.c`.

### msync cleanup (`0174`, `0175`)

msync is CrossOver-only, so WineHQ has nothing to backport. No branch of dappermint/winecx changes `destroy_all_internal()` or `pool_alloc()`, and CodeWeavers has published no source archive after 26.3.0 (26.3.1, 26.4.0 and 27.0.0 are absent).

A thread killed while it waits, by `TerminateThread()` or when its process exits, never sends the unregister message, so its node stays on every object it waited on. Client references are not released for a dead process either, so most of these objects never reach refcount 0, and `0174` does not reclaim their nodes. A thread's alert object, used in every alertable wait, is different: only the server's thread object references it, and the client gets its index without a reference. When that thread object goes, the index reaches refcount 0 and `msync_alloc_shm()` reuses it first, with the dead thread's node still on its list. The node leaked, or woke whichever thread then had that id when the new object was signalled. `0174` drains the list when the refcount reaches 0. Waits on the next object at that index are registered on the same pump thread after this message. A live waiter holds a reference for the length of its wait, so it cannot be on the list. Waking instead of only freeing keeps it from hanging if one ever is; a woken thread rechecks its objects, as after any wake.

`tests/msync-wait-probe.c` gains an `exit` mode. With 120 rounds, a child whose 100 threads wait alertably for 63 events and then exits, the `.34` runtime with `WINEMSYNC=1` in a fresh prefix ran out of nodes and wrote the warning 243757 times. The probe shows the general leak through killed waiters and the warning flood; it cannot show the alert-object share that `0174` reclaims.

### Left out

| Candidate | Decision |
|---|---|
| win32u `FT_FixedFromFIXED()` | The expression yields an `unsigned int`, so a negative MAT2 component becomes a large positive `FT_Fixed` on the 64-bit Unix side. WineHQ master still has it. Not reachable in this runtime: its only use is `FT_Matrix_Multiply()`, and the bundled FreeType reads matrix elements as 32-bit (`movslq`), which restores the sign. `GetGlyphOutlineW()` with eM22 = ±0.3125 and eM11 = -1 gave the same 34×13 box and 468-byte buffer on the `.34` runtime, x86_64 and i686. Revisit if the FreeType build changes. |
| ntdll `get_full_path()` with a Unix path outside every drive | Already fixed by WineHQ `e8781e7c8d` (wine-11.14), carried in `0148`. |

### Not verified yet

- No build: `0173`–`0175` are checked by patch application and, for `0173`, an object compile of `loader.c`. A complete i386/x86_64 WoW64 build is required.
- `tests/syscall-boolean-args-probe.c` on a build with `0173`; Wine's `ntdll` tests against `.34`.
- `0174` has no probe; it is code review only.
- `0175` on a build: the `exit` mode must write the warning once.


<a id="host-mapping-query"></a>

## Host mappings in macOS x86_64 memory queries

Candidate patch set `cx26.3-wine11.0-runeon.36` adds `patches/runeon/0176` after `0175`. It changes `ntdll`, which is ABI-sensitive; the maintainer approved it individually on 2026-11-12. It is a downstream fix; no WineHQ commit addresses it, and its shape is meant to be submittable upstream.

### Defect in the base

`get_memory_region_size()` in `dlls/ntdll/unix/virtual.c` sizes a free range only from Wine's views. The check that treats space outside the reserved areas as allocated is built for i386 only. On macOS x86_64 the loader is linked at `0x200000000`, directly above the reserved low 8 GB, and Rosetta and the host allocator map next to it, all without a view. An independent probe on `.35` saw `VirtualQuery(0x150000000)` report one free range of about 112 TB, `0x200000000` reported free, and `VirtualAlloc(0x150000000, 0xd5c00000, MEM_RESERVE)` fail with `ERROR_INVALID_ADDRESS` while the same size at `0x300000000` succeeded. A program that picks a large block from the query results keeps choosing the occupied range.

### Change

Outside the reserved areas, the query asks the host for the mapping at or above the address with `mach_vm_region()`, as `reserve_area()` already does. A host mapping is reported `MEM_RESERVE`/`PAGE_NOACCESS` through the existing `fake_reserved` path; a free range ends at the next host mapping or reserved area. Inside a reserved area only the end of a free range changes: it stops at the area's end instead of running on to the next view; state and protection there stay as before. Allocation and the unix-call and server protocols are unchanged; no process is matched. Because host mappings now reach the reserved branch of `get_memory_region_info()`, that branch writes `CommitSize` only when the caller's buffer holds it, as the view branch does; it wrote 8 bytes past a 24-byte buffer, which Wine's own tests use. A query at address 0 now reports the host's page-zero mapping as reserved where Windows reports it free; a walk from 0 still advances.

### Verification

`tests/host-mapping-query-probe.c` fails on `.35` (the loader at `0x200000000` is reported free, and a 4 GB reserve at `0x150000000` fails) and passes on a complete WoW64 build of `.36`: `0x150000000` is free up to `0x200000000`, the host mappings above it are reserved, and a 4 GB reserve succeeds from the first 64K boundary past them. The probe only walks `0x150000000`–`0x400000000`, and WoW64 32-bit processes are not covered. After a later review it also fails on a failed query during the walk, requires one full 4 GB reservation, and checks that a 24-byte `MemoryRegionInformation` query at the loader succeeds and writes nothing past the buffer; that version has not been run on a `.36` build yet.

### Not verified yet

Whether programs that scan for a large block then run.


<a id="application-menu-name"></a>

## Application menu name

Patch set `cx26.3-wine11.0-runeon.36` also adds `patches/runeon/0177` after `0176`. It changes the loader, which is ABI-sensitive; the maintainer approved it individually on 2026-11-12. It is product branding, not a Wine fix, and is not meant for upstream.

CrossOver embeds `CFBundleName` "CrossOver-Hosted Application" in the loader's `__info_plist` section (Hack 10913), and `apple_override_bundle_name()` (Hack 13438) rewrites it in place from `WINEPRELOADERAPPNAME`, which ntdll's `replace_wineloader_path_with_link()` sets from the exe name with the preloader or when `WINEDLLPATH` is set, or from `argv[1]`. The embedded name becomes Runeon and the rewrite, its call and its includes are removed, so every program shows Runeon. `CFBundleIdentifier` stays `com.codeweavers.CrossOver.wineloader`: Metal keys its shader cache directory on it.

The loader builds without warnings and its `__info_plist` section carries the new name. Not checked at run time yet; why programs started from Steam kept the CrossOver name before, rather than their exe name, was not established.


<a id="shm-surface-flush-under-lock"></a>

## Shared-memory surface flush under the USER lock

Candidate patch set `cx26.3-wine11.0-runeon.37` adds `patches/runeon/0178` after `0177`. It changes `win32u`, which is ABI-sensitive, and CrossOver's own Hack 23950; the maintainer approved it individually on 2026-11-14. The approval covers this patch alone and is not a precedent. It is a downstream fix, not a WineHQ backport: upstream Wine has no shared-memory surface.

### Defect in the base

Hack 23950 gives a window whose parent belongs to another process a shared-memory surface. `shm_surface_flush()` in `dlls/win32u/dce.c` delivers it by sending `WM_WINE_FLUSHSHMSURFACE` to the parent. `apply_window_pos()` in `dlls/win32u/window.c` holds the window pointer, and with it the USER lock, while it calls `update_surface_region()`, which reaches `window_surface_set_shape()` and then `window_surface_flush()`. Sending from there hits `user_check_not_lock()` and the process aborts. A windowed WebView2 browser process takes this path when it reparents a window into the one it hosts in the application's window.

### Change

`shm_surface_flush()` returns failure when the calling thread holds the USER lock, with a TRACE. `window_surface_flush()` only resets the dirty bounds after a successful flush, so they are kept. Reading the source, the surface is registered in the flush list after `apply_window_pos()` releases the lock, and the message loop's wait and idle paths call `flush_window_surfaces()` without the lock. A small `user_lock_held()` helper reads the existing lock-owner record; locking itself is unchanged.

### Verification

`tests/shm-surface-lock-probe.c` reproduces the abort on `.36`: the child process exits with `EXCEPTION_WINE_ASSERTION` right after the reparent, while the earlier unlocked flush still delivers its content. On a complete local i386/x86_64 WoW64 build of `.37` it exits 0: the blue drawn before the reparent reaches the parent without further drawing.

### Not verified yet

A windowed WebView2 game presenting and updating was checked only together with `0179`–`0181` (see the next section). If the thread that owns the window stops pumping messages, nothing is flushed until it does, as before.


<a id="foreign-window-surface"></a>

## Foreign window DC surface and cursors

Candidate patch set `cx26.3-wine11.0-runeon.37` adds `patches/runeon/0179`, `0180` and `0181` after `0178`. They change `win32u`, which is ABI-sensitive, and `0179` builds on CrossOver's own Hack 23950. The maintainer approved each of the three patches individually on 2026-11-14; the approvals cover these patches alone, are not a precedent, and were not derived from the approval of `0178`. They are downstream changes, not WineHQ backports: upstream Wine has no shared-memory surface. All three are opt-in through `RUNEON_FOREIGN_WINDOW_SURFACE=1`, which the caller sets for every process of a session; without it behaviour is that of the base, and the receiving side of each new request does not answer.

### Defects in the base

- `update_visible_region()` in `dlls/win32u/dce.c` takes a surface only from a top window of the current process. A DC another process obtains for a window hosted inside a third process's window gets an empty device rect, and its output is lost; on Windows it draws into the window. A Chromium GPU process in software compositing draws a WebView2 browser window this way, with `GetDC()`, `BitBlt()` and `ReleaseDC()`.
- A cursor set by a window hosted in another process's window is shown by the host process. `macdrv_SetCursor()` cannot read a cursor object of another process, logs `GetIconInfoExW failed` and keeps the previous cursor, which may be none; Windows shows the hosted window's cursor.

### Change

- `0179`: such a cache DC of a client area gets a per-DC Hack 23950 shared-memory surface. The top window's process first copies the window's current pixels into it (`WM_WINE_FLUSHSHMSURFACE` with a non-zero `wparam`), and `ReleaseDC()` flushes it to that process after dropping the USER lock.
- `0180`: a cursor handle of another process is shown as the arrow, through a local stand-in that winemac.drv maps to its system arrow.
- `0181`: the host process asks the thread that created the cursor for a static snapshot with the internal `WM_WINE_CURSORSNAPSHOT` message, validates it against the bytes received and shows a local copy; `0180` stays the fallback.

### Limits

- `0179` creates and seeds a new section for every `GetDC()` and flushes it on every `ReleaseDC()`, each round trip waiting up to 500 ms; nothing is cached, because no cost has been measured that would justify it.
- `0179` writes back 8-pixel aligned dirty bounds from the seeded copy, so pixels another process changes between the seed and the flush can be reverted.
- `0179` seeds only the top-down 32 bpp layout its sender creates; any other header from the other process is refused.
- `0181` covers static cursors up to 256x256, in 32 bpp or monochrome, and waits up to 200 ms for the owner; animated cursors, other formats and owners that do not answer get the arrow. The last four handles that gave no snapshot are not pulled again. The hotspot is not DPI-scaled.

### Verification

On a complete local i386/x86_64 WoW64 build of `.37`, `tests/foreign-dc-probe.c` exits 0 in `fill`, `preserve` and `crossframe` modes with the opt-in and 1 in all three without it, and 1 in `painteronly` mode, where only the drawing process has the opt-in; `bench` measured 3.2 ms on average per 1280x720 `GetDC()`/`BitBlt()`/`ReleaseDC()` round trip in the drawing process. With `+cursor`, `tests/foreign-cursor-probe.c` gives the trace counts its header lists in all eight phases: copies for the static cursors, the arrow for a 300x300 cursor, and no second pull of it; without the opt-in, `GetIconInfoExW failed` as unpatched. `tests/shm-surface-lock-probe.c` exits 0 either way. Earlier builds of the same changes rendered a windowed WebView2 application and showed its own cursor in local tests.

### Not verified yet

Owner drawing between a seed and its flush, window resizing or destruction while a DC is held, `0179` under DPI scaling and in Retina coordinates, a busy owner's worst-case cursor wait, and Retina hotspots. A windowed WebView2 game started from Steam on a local build of `.37` presented, took input, played sound, showed its own cursor and survived resizing and minimising; the Steam overlay did not open in it, and why was not established.


<a id="hkcu-wow6432node"></a>

## User hive Software shared between registry views

Candidate patch set `cx26.3-wine11.0-runeon.37` adds `patches/runeon/0183` after `0182`. It changes `kernelbase` and `server`. `server` is ABI-sensitive and the patch is not opt-in, so it needed the maintainer's individual approval: the maintainer approved it on 2026-11-15, for `server` and for shipping without an opt-in, for this patch alone; it is not a precedent. It is a downstream fix written for upstream submission, as a fix for WineHQ bug 57892; once an equivalent fix is merged upstream it is replaced by that backport.

### Defect in the base

Microsoft's "Registry Keys Affected by WOW64" lists `HKEY_CURRENT_USER\SOFTWARE` as shared between the 32-bit and 64-bit views; in a user hive only listed `Software\Classes` subtrees such as `CLSID` are redirected. Wine redirects the 32-bit view at any key with a `Wow6432Node` subkey, by two paths:

- `server/registry.c`: `key_link_name()` stores a linked `Wow6432Node` subkey as the parent's `wow6432node` pointer. For a 32-bit process, `key_lookup_name()` steps into that pointer on each path element, and `open_key()`, `create_key()`, `grab_wow6432node()` and `get_wow6432node()` follow it. A 32-bit `RegOpenKeyEx()` reaches this through `NtOpenKeyEx()` directly, because `is_win64` is false in the 32-bit `kernelbase`.
- `dlls/kernelbase/registry.c`: for `KEY_WOW64_32KEY` in a 64-bit process, and for a 32-bit create whose parents are missing, `open_key()` walks the path itself, and `open_subkey()` calls `open_wow6432node()`, which opens a subkey named `Wow6432Node` by name.

Once any program creates `HKCU\Software\Wow6432Node`, `HKCU\Software\Valve\Steam` is looked up inside it in the 32-bit view. Locally, a 32-bit `steam_api` then logged that `SteamAPI_IsSteamRunning()` did not find Steam and the game restarted itself through Steam in a loop; deleting `HKCU\Software\Wow6432Node` with Steam stopped ended it. WineHQ master as of 2026-11-13 has the same logic; the patch attached to bug 57892 disables the mapping instead and is not used.

### Change

- `server`: `key_link_name()` no longer stores the `wow6432node` pointer when the parent lies in a user hive (`\Registry\User\...`) outside its `Software\Classes` subtree. A small `is_wow64_shared_user_key()` walks the parents up to the root key; it runs only when a key named `Wow6432Node` is linked.
- `kernelbase`: `open_wow6432node()` keeps the original key when the opened `Wow6432Node` lies in the same part of a user hive, judged by its `NtQueryKey()` name. A failed name query keeps the base behaviour.
- `dlls/advapi32/tests/registry.c`: `test_redirection_hkcu()`.

Both paths stop at the same boundary. HKLM, the `Software\Classes` handling (not rewritten), paths that name `Wow6432Node` and the 64-bit view are unchanged. No protocol, request, structure or export changes. Existing data under `HKCU\Software\Wow6432Node` stays where it is, so settings a program wrote there while the defect was active are no longer seen through the shared path. With no pointer stored, renaming `HKCU\Software\Wow6432Node` is no longer refused.

### Verification

A complete local i386/x86_64 WoW64 build of `.37` with `0183` passed. Both runs below use a fresh prefix each, on that build and on `.37` without `0183`:

- `tests/hkcu-wow6432node-probe.c` (32 checks per process): 15 failures in the i686 process and 9 in the x86_64 process without `0183`, all in the HKCU checks and `HKCU\Software\Classes\CLSID`, which the redirected `Software` key also hid; 0 failures in both with it. The HKLM and HKLM `Software\Classes` checks pass on both.
- `dlls/advapi32/tests/registry.c` with the new test: without `0183`, `test_redirection_hkcu()` fails 21 times in the i386 process and 15 times in the x86_64 process; with it, it passes. In both builds the only other failures are the same two in `test_RegLoadMUIString()`, so `test_redirection()`, the classes root tests and the rest of the file show no new failures.
- `dlls/ntdll/tests/reg.c`: no failures on either build, in either architecture.
- In a copy of a Steam prefix with `HKCU\Software\Wow6432Node` present, 32-bit `reg.exe` and 64-bit `reg.exe /reg:32` could not find `HKCU\Software\Valve\Steam` without `0183` and found it with `0183`; the `Wow6432Node` key and its value stayed in place.

The patch applies to WineHQ master of 2026-11-13 (`9f8c4ae4d3`); it was not built there.

### Not verified yet

No native Windows machine was available, so the expected values come from Microsoft's documentation and the existing tests, not from a Windows run of the new test. A 32-bit Steamworks game started through Steam with `HKCU\Software\Wow6432Node` present has not been run on this build yet. `HKEY_USERS` hives other than the current user's, registry notifications and key renaming were not exercised.

<a id="macos-lasterror-gs"></a>

## Last error mirrored into the macOS %gs slot

Candidate patch set `cx26.3-wine11.0-runeon.38` adds `patches/runeon/0184` after `0183`. It changes `ntdll` and the inline `SetLastError()` in `include/winbase.h`, which every Wine PE module compiles in. `ntdll` is ABI-sensitive and the patch is not opt-in, so it needed the maintainer's individual approval: the maintainer approved it on 2026-11-16, for `ntdll`, for the `include/winbase.h` change and for shipping without an opt-in, for this patch alone; it is not a precedent. Its shape differs from upstream, which instead swaps GSBASE between the TEB and the host thread data (WineHQ `3a16aabbf5`, Wine 10.5), so it is not written for upstream submission.

### Defect in the base

On Windows x64 %gs points to the TEB, and `gs:[0x68]` is `TEB.LastErrorValue`. Mono's x64 JIT reads the last error there inline after every P/Invoke declared with `SetLastError=true`, instead of calling `GetLastError()`. In the pinned base on macOS x86_64, GSBASE is always the host pthread's TSD array; `init_syscall_frame()` mirrors only `TEB.Self` (0x30), the TLS pointer (0x58) and the PEB (0x60) into it. `gs:[0x68]` is TSD slot 13, where libc `gmtime()` keeps a pointer to its buffer and registers a destructor that frees it.

`tests/lasterror-gs-probe.c` on `.37`: after explicit `SetLastError()` calls and after failing `GetFileAttributesExW()`, `FindFirstFileExW()` and `FindNextFileW()`, `gs:[0x68]` never matched `GetLastError()`, while the TEB always did. The main thread read a constant host pointer; a Wine-created thread read 0, and a fresh `gmtime()` buffer after `GetTimeZoneInformation()`. A Unity game's corlib takes a failed attribute query whose captured error is 0 for success, so `FileInfo.Exists` reported a missing temporary file as present and the game disabled saving.

### Change

On macOS x86_64 only:

- Every write of `TEB.LastErrorValue` also stores the value, zero-extended, over the whole 64-bit slot: `RtlSetLastWin32Error()` and its aliases (`RtlRestoreLastWin32Error`, kernel32/kernelbase `SetLastError` and `RestoreLastError`), `RtlSetLastWin32ErrorAndNtStatusFromNtStatus()`, the Unix-side `RtlSetLastWin32Error()` used by `win32u` and other Unix libraries, and the inline `SetLastError()` in `include/winbase.h`. PE code finds the slot through `TEB.Instrumentation[0]`, which only the macOS branch of `init_syscall_frame()` sets, as the loader's TLS pointer mirror already does; elsewhere it is NULL and nothing is written.
- `gmtime()` is replaced by `gmtime_r()` on a thread-local buffer, installed next to the existing `localtime()` replacement for the PEB at 0x60. A scan and a run of the x86_64 libsystem_c time functions found no other user of slot 13.
- `pthread_exit_wrapper()` clears the slot next to the existing 0x60 clear, so a `gmtime()` destructor registered before the replacement is never handed an error code to free. It is the only `pthread_exit()` call in the base; return, `ExitThread()` and `TerminateThread()` all reach it, and process exit runs no TSD destructors.

`wow64win` writes the 32-bit TEB, which i386 code reads through fs, and is unchanged; Linux and i386 builds are unchanged. No protocol, export or shared structure changes. Code that writes the TEB or `gs:[0x68]` directly is not mirrored.

### Verification

A complete local i386/x86_64 WoW64 build of `.38` passed. `tests/lasterror-gs-probe.c`, each run in a fresh prefix:

- On `.37` it exits 1 with 46 failing cases; on the `.38` build it exits 0 with none. Cases: explicit values including 0, 0xffffffff and 0x80000000, `RestoreLastError()`, `RtlSetLastWin32ErrorAndNtStatusFromNtStatus()`, three failing file APIs, a `user32` call failing on the Unix side and `GetTimeZoneInformation()` (host `gmtime()` with an error code in the slot), 1000 times each on the main thread and a second thread; 16 threads setting distinct values at once, counted only when all 16 started and finished; a window procedure reached by `SendMessage()`; 50 threads each ending by return, `ExitThread()` and `TerminateThread()`; a process exit with an error code in the main thread's slot.
- A host-only model without Wine: a thread that leaves an error code in slot 13 after `gmtime()` registered its destructor aborts in `free()`; clearing the slot first avoids it.
- In Runeon, the Unity game that could not save saved normally on the `.38` build; one DXMT game and one D3DMetal game started and exited normally.

### Not verified yet

`dlls/ntdll/tests` and `dlls/kernel32/tests`, x86_64 and i386, were built from the `.37` and `.38` trees and run on both runtimes in fresh prefixes: none of the 128 files failed on `.38` in a way that `.37` does not also show on a rerun. `error`, `thread`, `time`, `sync` and `env` have no failures on either. The differing counts in kernel32 `debugger`, ntdll `file` and i386 ntdll `info` varied between reruns on both runtimes. ntdll `exception` stops on both runtimes with a Rosetta `unsupported privilege level` abort, so its later subtests did not run; the same holds for i386 ntdll `wow64` and x86_64 kernel32 `virtual`.

i386 code reading `fs:[0x34]` was not examined. Whether threads created by host libraries ever run Wine PE code was checked only by reading the code.

<a id="dwm-sheet-of-glass"></a>

## DWM sheet of glass and click-through on macOS

Candidate patch set `cx26.3-wine11.0-runeon.38` adds `patches/runeon/0185` and `0186` after `0184`. Both change `winemac.drv`, which is ABI-sensitive, and neither is opt-in, so each needed the maintainer's individual approval: the maintainer approved both on 2026-11-17, for `winemac.drv` and for default use without an opt-in, for these two patches alone; it is not a precedent.

### Defects in the base

- `DwmExtendFrameIntoClientArea()` in `dwmapi` is a stub. On Windows a negative margin (the sheet of glass, as in Microsoft's `MARGINS margins = {-1}`) makes DWM composite the window's alpha channel onto the desktop. In the base the Cocoa window and its Metal layers stay opaque, so a desktop overlay shows black where it draws nothing.
- On Windows a window with both `WS_EX_LAYERED` and `WS_EX_TRANSPARENT` is skipped by hit-testing, and the base's wineserver skips it too, but `winemac.drv` never maps the style to Cocoa. A local overlay that toggles `WS_EX_TRANSPARENT` every frame took no clicks on the D3D11 path through DXMT: macOS delivered them to the Cocoa window, and Wine dropped them. `macdrv_SetWindowStyle()` also re-synced every Cocoa window property on each call, about 3,200 times in 100 seconds for that program, although the style rarely changed.

### Change

- `0185`: `dwmapi` sets the window property `__wine_dwm_sheet_of_glass` when any margin is negative, clears it otherwise, and requests `SWP_FRAMECHANGED` when it changes. `winemac.drv` reads it when it creates the Cocoa window and on `SWP_FRAMECHANGED`, makes the window non-opaque through the existing transparency check, composites the window surface with its alpha channel and makes the window's Metal layers non-opaque. A Metal view takes its window's state whenever it joins a window, including when `SetParent()` moves it to another top-level window.
- `0186`: `macdrv_window_state` carries `ignores_mouse_events`, set when both styles are present; the Cocoa window calls `setIgnoresMouseEvents:` only when the value changes, so it is set back only when Wine set it. An explicit NO would also turn off AppKit's default pass-through on fully transparent pixels. `macdrv_SetWindowStyle()` returns early when the style did not change.

No unix-call, wineserver protocol or structure shared with other modules changes. WineHQ has no `winemac.drv` equivalent; `winewayland` carries the same two-style rule for click-through.

### Verification

`tests/dwm-sheet-of-glass-probe.c` on a local complete WoW64 build with both patches, through DXMT with its presenter keeping the layer opacity `winemac.drv` sets:

- The glass window showed the window behind in its alpha-0 quadrant, kept opaque green and black, and turned opaque again after margins of 0; the control window never changed; glass set before and after the swap chain was created gave the same result.
- The alpha-0.5 quadrant read (181,150,143) over a cyan window; a native program compositing the same premultiplied colour through a CALayer read (180,150,143), and through a CAMetalLayer in sRGB (181,150,143). The output therefore matches native macOS compositing; a linear blend of sRGB values would give about (130,128,128), but macOS composites colour-managed.
- In the click-through mode a native `windowNumberAtPoint` hit test at the window centre found the window behind while both styles were set and the Wine window otherwise, in two runs. Repeated identical styles caused no Cocoa property sync.
- In the reparent mode a child Metal view followed the transparency of the window it moved into and out of.
- A desktop overlay game became transparent and click-through on both D3DMetal and DXMT.

### Limits

- A Metal presenter that forces its layer opaque, such as DXMT without that presenter change, still shows black.
- Partial margins (blur-behind frames) are not implemented; only the sheet of glass is.
- Global keyboard and mouse state while another macOS application is active is a separate limitation and is not changed.

### Not verified yet

Partial margins, GDI-drawn content, i386, ordinary windows over a longer session and Wine's own `winemac.drv`-related tests were not run.

<a id="backports-11.22-sweep"></a>

## Wine 11.22 user-mode fix sweep

Candidate patch set `cx26.3-wine11.0-runeon.38` adds `patches/upstream/0187-wine-11.22-targeted-fixes.patch` and `0188-wine-11.22-wined3d-whole-buffer-write-upload.patch` after `0186`. `0187` squashes 47 WineHQ commits: 46 first released in `wine-11.22`, and `673e09be71`, which is on master after `wine-11.22` and not yet released. `0188` carries `b6810e410f` alone, with its tests. Each patch lists every commit with its author, and the manifest records the full commit IDs. Test-only hunks are omitted from `0187`. Two commits in `0187` are adapted to fix memory errors of their own (below).

### Selection

All 324 commits of `wine-11.21..wine-11.22` were reviewed with the rules of the [previous sweep](#backports-11.21-sweep), against a tree made of the pinned CrossOver archive plus patches `0001`–`0186`:

- 35 commits change only tests.
- 126 commits change only `ntdll`, `server`, `win32u`, `winemac.drv`, `wow64`, the loader and preloader, `include`, the build tools, bundled libraries, fonts, locale data, translations or the X11 and Wayland drivers. Most are feature work: address-space randomisation and reservation growth, ARM64EC extended state, moving `NtSetInformationFile()` classes to the server, CD-ROM pass-through, the OpenGL context rework and IOSurface window surfaces in `winemac.drv`. Each was read; three small fixes are taken under the exceptions in `CLAUDE.md` §3 and four with the maintainer's individual approval (below).
- Of the 163 user-mode commits, the sweep takes crash, memory-safety, leak, hang and error-path fixes in DLLs that Steam, launchers, installers or games load, one ddraw regression fix and, in its own patch, one wined3d buffer upload fix under the graphics rule.

For each taken commit the defect was confirmed in the base source, not inferred from a clean apply: every one applies without fuzz or offset, in upstream order; only the two adapted commits differ from upstream. Commit messages of the whole range and of master up to `433ffc868d` (2026-11-16) were searched for commits carried earlier: `c209164daf` names `6b486ec783` and `ede017e2b2` names `d56f36c1aa`, both carried, and both are taken. No later master commit names a taken one.

### Taken

| Area | Commits | Defect fixed |
|---|---|---|
| gdiplus | `f6a397054c`, `228557e936` | `GdipGetPathData` copied as many points as the caller's count, reading past the path's own arrays; several entry points dereferenced NULL rectangles, fonts or output pointers. |
| dwrite | `bcd4a666e4` | One unsupported file made creating a whole custom font collection fail, and the file stayed on the scanned list. |
| dbghelp | `53a0cdd776`, `53bac7c5f3`, `f4eea401e4` | Leaks in `SymCleanup`, source names, DWARF ranges and PDB type lists; `SymEnumSourceFilesW` dereferenced a NULL mask. |
| winhttp | `9695343d71`, `a5ea938598`, `11b324201d`, `2c56a54221`, `d52e2c87ae`, `087af9aff4`, `99b764e094` | `str_to_wire` converted the source to its terminator instead of `src_len`; `IStream::Read` wrote through a NULL count; allocation failures in redirects and headers were ignored; a failed cookie header was not reported; the IWinHttpRequest header getters overwrote the caller's BSTR on error; a redirect path was terminated at the wrong index. |
| crypt32 | `5df2ef62da`, `1170e76ca5` | A failed root certificate add went on to use the certificate; the Unix-side certificate import closed its file twice, which can close another descriptor of the process. |
| mshtml | `7009c1ed82` | Navigation callbacks used the document after it was released. |
| cabinet | `2d2dc3db07` | A failed write while extracting was ignored, so an installer reported success with a truncated file. |
| wbemprox | `845b5ddcba`, `b250fc643d`, `2d863f2774`, `dcd891037d`, `ec7a423eff`, `55e672f517`, `783a8f8fe3` | A short SMBIOS table was read past its end; printers without a device mode or location, a NULL resource and a NULL output pointer were dereferenced; filtered printer rows were matched and freed at the wrong index; an uninitialised process ID was returned; registry values added while enumerating overflowed the arrays. |
| d3dx9 | `9ead75872d` | A failed name lookup left a freed name pointer to the caller. |
| mfplat | `f436af4047`, `bfb465e290`, `26b318f826` | A failed D3D9 or DXGI surface lock left an uninitialised buffer marked as locked; `InsertElementAt` could reserve fewer elements than it then moved. |
| d2d1 | `662ebcb03b`, `7fccdf7e1c`, `ee607cf42b` | Leaks on failure paths of shared bitmaps, gradients and vertex buffers. |
| ddraw | `c209164daf` | Graphics rule: since `6b486ec783` (carried since `.27`), a version 1 primary surface was always created as a render target, even for display formats that cannot be one, so creating it failed; it is now a render target only when the format supports it. |
| quartz | `8f4826ad8c` | `IObjectWithSite::SetSite(NULL)` called AddRef on NULL. |
| mscoree | `4bd7fd80be` | `DllGetClassObject` used an unchecked allocation. |
| msvcrt | `62621e50ba` | `fscanf` on a NULL file crashed instead of failing with `EINVAL` (found with a game started outside its directory). |
| amstream | `c7e26cf11d`, `8b4cf949c8` | `Render` on a stream without a pin, or before initialisation, dereferenced NULL. |
| kernelbase | `7384c8babc` | `StrCmpN*` and `StrIsIntlEqual*` passed the full count to `CompareString`, reading past a shorter string's terminator (Wine-Bug 60241). |
| windows.gaming.input | `f9022cf74b` | Input rule: provider initialisation enumerated HID interfaces starting at the index where its XInput enumeration stopped, so it skipped that many HID controllers. |
| wininet | `412a292464` | A failed directory creation left a full-size `index.dat` with empty directory names, which broke the URL cache for good even after disk space was freed (Wine-Bug 60560). |
| winemac.drv | `ede017e2b2` | Since `d56f36c1aa` (carried in `0145`), a window restored from the Dock was activated before it became the foreground window, so it got `WM_NCACTIVATE` with wParam 0; upstream reports a game that depends on it. The base matches the code before the fix. |
| ntdll | `673e09be71` | `get_unix_full_path()` wrote a backslash through the name buffer of an object without a name, a page fault at address 2 (Wine-Bug 60781). |
| server | `3d15b75cfa` | When the descriptor a request passes was still in the socket buffer, an error left by the earlier receive attempt could make the request fail although the descriptor arrived; the error is now cleared once the descriptor is received. |
| server | `6fd2ef6b19` | A failed allocation of a window class's shared memory leaked the reference to its process. |
| win32u | `83fddb06f5` | `NtGdiCreateDIBSection()` with a section handle mapped the view without the caller's zero bits, so a 32-bit process under WoW64 could get bits above 4 GB, which WoW64 truncates to 32 bits. The existing `zero_bits` is now passed; it is 0 for native 64-bit callers, which are unchanged. |
| wow64win | `d7f7d8031a` | `NtUserGetRawInputDeviceList()` for 32-bit callers copied as many entries as the buffer holds instead of the returned count, filling the rest with uninitialised data. |
| win32u | `df33af7a82` | `NtUserToUnicodeEx()` left the output buffer untouched when the driver produced no character; Windows writes a terminator. `winemac.drv` implements the driver call, so the path is reached on macOS. |

`ede017e2b2` is carried under the macOS fix exception: it fixes a defect present in the base, touches only unchanged Wine code (the hunk is about 300 lines below the closest CrossOver change in `window.c`), changes no protocol or shared structure, and takes effect on x86_64. `673e09be71` and `df33af7a82` are carried under the Windows-behaviour exception for `ntdll`, `win32u` and `server`: each applies without fuzz, no Runeon patch touches its file, and it changes no protocol, unix-call table, export or shared structure. The added lines use only fields and functions present in the base.

`3d15b75cfa`, `6fd2ef6b19` (server), `83fddb06f5` (win32u) and `d7f7d8031a` (wow64win) need the maintainer's individual approval; the maintainer approved them on 2026-11-17, for these four commits only; it is not a precedent. None changes the server protocol, a request or reply structure, msync, a unix-call table or a structure shared between modules.

### Taken in `0188`, on condition

`b6810e410f` (wined3d, graphics rule): a non-managed buffer mapped for writing becomes dirty as a whole, as on Windows, instead of only the mapped range; managed buffers still upload only the mapped range. Upstream reports a game that maps four bytes of a vertex buffer and writes all of it. Its upstream tests, `test_map_partial_range()` in the d3d8 and d3d9 `visual` tests, are carried. The maintainer accepted it on 2026-11-17 on condition: it is a separate patch so it can be dropped alone, and stable adoption requires the regression run to check the upload and flush cost of frequent partial writes on wined3d against `.37`.

### Adaptations

| Commit | Adaptation |
|---|---|
| `55e672f517` wbemprox | Upstream doubles the array count when `enum_values()` runs out of room; for a key without values the count stays 0, `realloc()` frees the array and returns NULL, and the cleanup frees it again. The count now grows to at least 1; the error handling is unchanged. |
| `53bac7c5f3` dbghelp | Upstream frees the type record and then reads its id for the report in `pdb_reader_TPI_fillin_enumlist()` and `pdb_reader_TPI_fillin_UDTlist()`. The report result is now computed first. |
| `b6810e410f` wined3d tests | `test_map_partial_range()` is called at the end of `START_TEST(visual)`, whose last lines differ from upstream. |

Upstream master up to `433ffc868d` has no later commit touching either function or naming either commit, so there was no upstream fix to take instead.

### Left out after review

| Commit | Reason |
|---|---|
| `1b7689efe4` msi, `40a5fd6496` kerberos, `8c8759fb20` msv1_0, `effa85a115` bcrypt, `94958d450a`, `0a374631bf` winhttp | The base does not have the code: msi's combo box update allocates no text there, `map_auth_data` does not exist, bcrypt and CrossOver's winhttp request code differ. |
| `42a9195be1` wbemprox, `778f28f131` dbghelp | Do not apply: the leak happens only when `SetupDiGetClassDevsW` fails, and the PDB checks harden against malformed files that debuggers and crash reporters read. |
| `af38a86cfa` wined3d/dxgi | A Unity game hung after its window was minimised because occluded presents never released the frame latency semaphore (Wine-Bug 60111). It reaches only D3D10/11 through wined3d, adds a swapchain flag to the wined3d interface and does not apply to the base. |
| `eea4380ae7` wined3d, `d622c94d96` d3d8 | The d3d8 half does not apply, and the wined3d validation alone changes nothing. |
| `16931061c1` vbscript, the mfreadwrite sink writer series, the odbc32 `SQLDescribeCol` series, `3374e6b458` cfgmgr32, `45b9f838e5`, `c00edca7b5` mfplat D3D12 buffers, msxml3 DTD validation, the XPath engine and the namespace helpers, `cmd` `DIR` fixes | Do not apply: the base code differs or the commit builds on post-11.0 code. |
| `415a275e38` kernel32, `55ac249ed7` avifil32, `06f5b057bc` windowscodecs, `14af32b5b2`, `deacf31e09` gdiplus, `30a8ee48ce`, `ccd9f75e68` d2d1, `1f47e34e99` quartz, `e199427123` urlmon, `16a2532a84`, `13d2b5d8c6` msi, `3029211329` inetcomm, `699bd7b84e` msxml3, `9f5162bba0` services, `aa5d80283c` shell32, `37279f3648`, `9152de07c2` kernelbase, `a3199d7615`, `36c8900128`, `5af0addac2` wbemprox, `be8fe5d69d` uxtheme, `d1d484fe5e` sechost, `29aef98781`, `400454efc0` odbccp32, `b60db1e42b` odbc32, `ca48ffcd28` http | Correctness or behaviour changes with no crash, leak, hang or error path. |
| `9cf12c83b1`, `0615d2fb40`, `2feb9b273f` quartz | Video renderers now show their window on `Run()`: a behaviour change for every DirectShow player. |
| `50ec25e22f`, `5fd4d1c667` setupapi | Device identity strings that controllers and games enumerate. |
| `0ad45668e4`, `12c4f65c2a`, `9f6cea6d7a`, `b375ac79a1`, `9fc19d626c`, `09534727c1` wined3d | Vulkan renderer only. |
| Stubs, features and clean-ups | `capabilityaccessmanagerclient`, `VariantCompare`, `ITextStoreACP2`, `IMarkupContainer2`, `Win32_Processor` properties, `SystemTimeToTzSpecificLocalTimeEx` and its inverse, `mfwmaaec` capture, shell32 clipboard shortcuts, the dbghelp `.debug` search and linetab2 rewrite, the remaining amstream `Render` series, winhttp, msxml3, windowscodecs, quartz and mpr clean-ups, the gdiplus antialiasing optimisation, the aero theme metrics, service-account profile variables, the msi ARM64 custom action host and ARM64X builds. |

### ABI-sensitive commits

Apart from the seven taken, these were left out:

| Commit | Area | Reason |
|---|---|---|
| `48c5aa7ffa`, `90d140f6ae`, `b94b54c62b`, `b663047255`, `1a796c367c` | ntdll, win32u, server | Do not apply, or follow commits the base does not carry (`f3de8edf99`, `73d340dd17`, the server file disposition refactor). |
| `3f4393e5ff` | ntdll | Limits export forwarders to 32 (Wine-Bug 60130); needs the import resolution refactor `d667eb8e74` and `4180667a0c`. |
| `a9dd4878b1`, `f80250df92` | ntdll | Debugger support and a log improvement, not defects. |
| `7193be1145`, `b77a5f9b60`, `1a357f7ff9` | winemac.drv | IOSurface window surfaces: a feature that replaces CrossOver's surface code. |
| `c63ff8b706`, `35cab28451`, `50128d5b97`, `6a8402a68b`, `1fd19f83c8`, `1ec9649f57`, `0a391a4b47` | winemac.drv | Minimise, inactive-command, clipboard-on-activation and device-caps locking rework; no defect is stated, and most do not apply. |
| `d9590c210a` | winemac.drv | The upstream form of `0134`; it relies on DPI scaling in `win32u` that the base does not have, so `0134` stays. |

The 28 master commits after `wine-11.22` up to `433ffc868d` were checked for macOS fixes; they will be swept with `wine-11.23`. Of them only `673e09be71`, an ntdll crash fix, is taken (above). No macOS commit is taken: `9df43df01b` and `77bf09aaa1` move key repeat to the server, `433ffc868d` changes the Mach message that sends the task port to wineserver (a protocol change), `0a6a0dcb02` and `0e367b9393` correct return types without a stated defect, and `7e7ebb398f` silences warnings in a build tool.

### Validation

`scripts/static-check.sh` and `scripts/integration-check.sh` pass for `.38`. Not done yet: a complete i386/x86_64 build, Wine's tests for the affected modules against `.37` (including the carried `test_map_partial_range()`), the `0188` upload cost check, and product acceptance.

Unchanged in the base and not addressed here: `pdb_reader_TPI_fillin_enumlist()` also reads `cv_field->generic.id` after freeing the record that holds it, on its unexpected-field path; upstream master has the same code.

`upstreamAuditThrough` stays at `wine-11.15`. The base manifest records `upstreamSweepThrough: wine-11.22`.

<a id="sar-time-source"></a>

## Streaming audio renderer time source

Patch set `cx26.3-wine11.0-runeon.39` adds `patches/upstream/0192`–`0199` after `0191`, one patch per WineHQ commit, in upstream order. In wine-11.0 the streaming audio renderer (`dlls/mf/sar.c`) does not implement `IMFPresentationTimeSource`, so the media session clocks playback from the system clock. In a reproduced case (a game playing video through `IMFMediaEngine`), audio decoding was starved at video start, audio fell 2 to 6 seconds behind the video and never recovered. The session already prefers a sink's time source, so a renderer time source lets audio drive the clock. Upstream added it in wine-11.5.

The series is a feature by §3, so it is carried as an exception; the maintainer approved it individually on 2026-11-19, together with `0189`–`0191` and `0200`. Only the time source, the prerequisites it depends on and its follow-up fixes are taken.

### Review

- Every master commit touching `dlls/mf/sar.c` after wine-11.0 was reviewed, together with `dlls/mf/clock.c`, `session.c` and the tests, up to `433ffc868d`. Commit messages were searched for each candidate's short hash; none is named by a later commit.
- CrossOver keeps `sar.c`, `clock.c` and `session.c` identical to wine-11.0. The base's `sar.c` differs only by `53b5345251` and `37fd158bfc`, carried in `0079`. `87ced958e9` and `57dbc339bd` are in wine-10.7, so the base already has them.
- After `0199`, `sar.c` is identical to upstream master.

### Taken

| Patch | Commit | Release | Why |
|---|---|---|---|
| `0192` | `d6adaa2e59` clock: don't async notify a sink that is also the time source | 11.4 | Prerequisite. Once the renderer is the time source, it is also a clock sink. It would get each state change twice and queue a second `MEStreamSinkStarted`. `601c15ae64`, the `AddClockStateSink()` half, is already carried in `0064`. |
| `0193` | `de4fad27c3` replace state variable with `clock_state` | 11.5 | Prerequisite refactor. `0194` reports this field from `GetState()`. |
| `0194` | `77132db1ce` implement presentation time source for SAR | 11.5 | Core fix. |
| `0195` | `d93ccd7221` check the provided clock has a time source | 11.5 | Follow-up. Same series; matches Windows. |
| `0196` | `324a7da836` scrubbing in SAR | 11.5 | Follow-up. As the time source, the renderer gets the clock's rate changes. `OnClockSetRate()` returned `E_NOTIMPL`, which made every clock rate change fail. |
| `0197` | `3863fd3711` discard samples before the seek time | 11.5 | Follow-up. Without it, a start with a position derives the time from earlier samples. |
| `0198` | `512f5ddaab` report the seek time until rendering passes it | 11.5 | Follow-up. It also reads the audio clock under the renderer lock, which closes a race in `0194` with the audio client being released. |
| `0199` | `72c0c8f1a7` handle the end-of-segment marker in SAR | 11.5 | Follow-up. Silence keeps the audio clock, and so the presentation clock, running after the audio stream ends. |

### Adaptations

- `0194`: the position and timestamp reset in `audio_renderer_stream_Flush()` is placed inside the block that `0079` moved it into, as in upstream master.
- Test hunks: only the removals of the todo markers on the interface query and on `SetPresentationClock()` in the base's `test_sar()` are kept. The base's older `test_sar()` also has a todo on the same interface query, which is removed. The other test hunks change SAR tests added after wine-11.0 (`6ef0acb4b6`, `19ec23aa96`, `d2e018b832`, `38172b1262`, `7bff59d57c` and their helpers), which do not apply without about 20 earlier test-only commits. So no carried test fails before and passes after.
- The `.39` manifest now anchors `0079`'s `sar.c` change on the moved `queued_frames` reset, because `0194` rewrites the line it used to anchor on. The patch file is unchanged.
- The `.39` manifest anchors `0195` on its `GetTimeSource()` call alone, because `0200` rewrites the `else if` line around it. The patch file is unchanged.

### Left out

| Commit | Reason |
|---|---|
| `53b5345251`, `37fd158bfc` | Already carried in `0079`. |
| `739ca45017` clock: `IMFTimer` using a periodic callback | Timer rework, not needed for the time source. The base timer still converts each request to a system-time delay when it is scheduled. |
| `c71658f9c0`, `c771f1735c` | Fix code added by `739ca45017`, which the base does not have. |
| `21794b4f2e` | Moves clock callbacks to the multithreaded queue; performance change. |
| `7458d10132` session: no preroll while scrubbing | Session scrubbing behaviour, independent of the time source. |
| `5451a96017`, `5dc2b18d2a`, `c183a48528` | Clock timer fixes (`SetTimer()` without a time source, timer order) unrelated to the time source; left to a regular sweep. |
| SAR test-only commits | See Adaptations. |

### Lock order, fixed by `0200`

The clock calls `GetCorrelatedTime()`, and the time source's state sink, while holding its own lock, and `0198` makes the renderer take its lock there. The renderer's `Shutdown()` and `SetPresentationClock()` held the renderer lock while they called `GetTimeSource()`, `AddClockStateSink()` and `RemoveClockStateSink()`, so reading the clock time while the renderer shut down or changed its clock could deadlock. Upstream master has the same order; `c771f1735c` fixed it only in the periodic timer callback. It was found in review, not observed.

`patches/runeon/0200` (order 230, downstream, approved by the maintainer on 2026-11-19) makes both methods swap the clock pointer under the renderer lock and call the clock outside it. A bind lock taken only by these two methods keeps binding and unbinding serialized. Lock order: bind lock, clock lock, renderer lock. Its test checks from another thread that the time source answers while the renderer is inside each clock call.

### Validation

- `scripts/static-check.sh` and `scripts/integration-check.sh` pass for `.39`.
- The complete i386/x86_64 WoW64 build succeeds.
- Wine's `mf` (`mf`, `topology`, `transform`), `mfplat` and `mfmediaengine` tests were run on both architectures against the `.39` build without `0192`–`0199`, in fresh prefixes. There are no new failures. The `transform` failure sets are identical (89 on x86_64, 83 on i386). The `mf` test runs 23 more checks because the `test_sar()` time-source block now runs, and it passes.
- `0200`: on the complete build, `mf`, `mfplat` and `mfmediaengine` show no new failures, and both new tests pass 20 of 20 runs per architecture without a skip. Against `mf.dll` built without `0200`, `test_sar_clock_calls_unlocked()` fails four checks (each call blocks for the timeout); against `mf.dll` built without `0199`, `test_sar_end_of_segment_clock()` also fails (the clock stops at 20 ms).
- In the affected title (DXMT), video plays in sync and without distortion after `0192`–`0200` (maintainer's local test).
- Not verified: a run on Windows.

<a id="mf-converter-upstream-attributes"></a>

## Topology loader: converter output types inherit upstream attributes

Patch set `cx26.3-wine11.0-runeon.40` adds `patches/upstream/0201` (WineHQ `8db38ea190`, wine-11.7, adapted) and its test `patches/runeon/0202` after `0200`, and the downstream audio renderer fix `patches/runeon/0203` for the same title (see the last subsection). Status: the maintainer approved each individually on 2026-11-20. `mf` is not in the module lists that allow behaviour fixes without one, and these are playback failures, not crashes, leaks or hangs.

### Defect

- A WMV file (WMV3 video, WMA Lossless audio) has no native ASF demuxer in the base, so the GStreamer fallback source exposes already decoded I420 video.
- The EVR mixer does not accept I420, so the topology loader inserts the video processor. With no type offered by the EVR, it enumerates the processor's output types, which carry only major type, subtype and frame size.
- Without `MF_MT_ALL_SAMPLES_INDEPENDENT` the EVR mixer treats each type as compressed (as documented for Windows) and rejects it. No decoder takes I420, so the session fails with `MF_E_TOPO_CODEC_NOT_FOUND`: black screen, no audio.
- `update_media_type_from_upstream()` already fills these attributes for the type the loader sets on a transform, but `topology_branch_foreach_up_types()` did not call it for enumerated types. The EVR check and `IsCompressedFormat()` are left as they are.
- Evidence: a `+mfplat,+evr` trace of the failing title on this base. With `0201` the cutscene plays to its end (maintainer's local test).

### Review

- Every master commit touching `dlls/mf/topology_loader.c` after wine-11.0 was reviewed, up to `e3c3d34867`. Commit messages were searched for `8db38ea190`, `6817fb42a2` and `f0223546ac`; none is named by a later commit.
- `8db38ea190` sits on about 20 wine-11.6/11.7 refactors of the resolver (cached handlers on branches, dynamic indirect branches, the `connect_method` enum). They do not apply to the base and are not needed for this fix.
- `9ab1b050c5` and `281e7a82ef` (wine-11.19) are already carried in `0079`.

### Taken

| Patch | Commit | Release | Why |
|---|---|---|---|
| `0201` | `8db38ea190` update media types while enumerating up types | 11.7 | Core fix: each enumerated output type is copied and receives the upstream attributes it lacks. |
| `0201` | `f0223546ac` (helper only) | 11.7 | `clone_media_type()`, verbatim. |
| `0201` | `6817fb42a2` (helper only) | 11.7 | `media_type_handler_get_type()`, verbatim. The copy keeps the transform's own types unchanged; many MFTs return internal references. |
| `0202` | downstream test | — | The upstream commit has no test. |

### Adaptations

- The `upstream` type is added as a parameter next to the base's `enumerate_source_types` flag. `topology_branch_connect_indirect()` passes its `up_type`, and `topology_loader_resolve_branches()` passes `NULL`, as upstream does.
- The upstream hunk for the non-enumerating path, with `media_type_handler_get_current()`, is left out. That path is only reached when the downstream node offered a type, which the base already sets on the transform with the upstream attributes. The hunk only matters when the transform rejects that type.
- TRACE changes are left out; the base has no `debugstr_media_type()`.

### Left out

| Commit | Reason |
|---|---|
| Rest of `6817fb42a2` (`topology_branch_find_best_type()`) and of `f0223546ac` | Best-type selection for converters and decoders; needs the cached branch handlers. Not needed here. |
| `774bbd4153` | Switches video conversion to the color converter by default; behaviour change, not a fix for this defect. |
| `622651f7ee` | Asynchronous transforms in the session; feature. |
| `b2047ce19f` and the wine-11.8/11.9 walk rewrite (`82284afdc7`, `addbf54365`, `d94e691eca`, `c883ea1fca`, `6e30b14f5e`, `66db696e4f`, `ac01bc4f7d`, `1f160bc170`, `98b76f8831`) | Resolver rewrite and optional-node handling; refactors that depend on the 11.6/11.7 restructuring. |
| `6bd21dffdd`, `bb6deef75b`, `68bfdd0889`, `fcd684a325`, `1d17bcf4e0`, `96a563a061`, `cb8e94c735`, `828d9de10f`, `6c5ab878cf`, `114f0eb9a4`, `5f6a89848f` | wine-11.7 resolver restructuring and type-selection changes around `8db38ea190`; not required by the fix. |
| `7e07ee63bd`, `010f4ab9cc`, `18e19a7903`, `a3f64a3d89`, `8e9b6204e3`, `cd1207601b` | wine-11.5/11.6 clean-ups, tracing and decoder node attributes. |

### Effect on other paths

- Only branches from a converter or decoder that the loader inserts, and whose downstream node offered no type, are affected (the EVR in this base). Their enumerated types gain missing attributes; present attributes are kept.
- Upstream copies the same attribute list, including `MF_MT_DEFAULT_STRIDE`, also when the subtype changes, and copies audio block alignment and average byte rate from a compressed type into decoder output types that lack them. The video processor in this base uses only the sign of the stride.
- `IMFMediaEngine` is not affected: its video frame sink reports a current type, so the loader takes the non-enumerating path. The source reader has its own pipeline.

### Validation

- `scripts/static-check.sh` and `scripts/integration-check.sh` pass for `.40`.
- The complete i386/x86_64 WoW64 build of `.40` succeeds.
- `test_topology_loader_converter_attributes()`, from one test build, fails with `mf.dll` built without `0201` (`topology.c:3140: Test failed: got hr 0xc00d5212`, `MF_E_TOPO_CODEC_NOT_FOUND`) and passes with `0201`, on both architectures.
- Wine's `mf` (`mf`, `topology`, `transform`), `mfplat`, `mfreadwrite`, `mfmediaengine` and `evr` tests, run with the same test programs on `.39` and `.40`, both architectures, fresh prefixes: no new failure. The only difference is the `0202` check above, which fails on `.39`. The `transform` failures (89 on x86_64, 83 on i386) are identical on both, and an intermittent crash in `transform` occurs on both.
- In the affected title the cutscene plays to its end and the story continues (maintainer's local test). Not verified: a run on Windows.

### Streaming audio renderer subtype check (`0203`)

- Defect: the same file's WMA Lossless 24-bit audio is decoded by the GStreamer fallback to 32-bit integer PCM, 48 kHz, stereo. The device mix format is 32-bit float, 48 kHz, stereo.
- Wine's renderer always initializes its audio client with the mix format and copies samples into it unchanged. `check_media_type()` compared only rate, channels, bits per sample, block alignment and byte rate, so the integer type was accepted, no converter was inserted, and the samples were played as float: loud noise.
- Windows' renderer supports both PCM and float formats. Wine's must reject an input whose subtype differs from its type, or is missing, so that the loader inserts a converter. The existing test already expected a type without `MF_MT_SUBTYPE` to be rejected (marked todo).
- Evidence: a trace of the failing title on this base shows GStreamer delivering 32-bit integer PCM and a 32-bit float device format. That the renderer accepts the integer type is shown by the test below, not by the trace. WineHQ master (`76f8e94795`) has the same check.
- Change: `MF_MT_SUBTYPE` is added to the compared attributes, against the renderer's own type, not a fixed subtype. Both `IsMediaTypeSupported()` and `SetCurrentMediaType()` use it. No upstream commit exists, so this is a downstream patch.
- Test: the two todo marks are removed, and a new check copies the renderer's supported type, switches only its subtype between PCM and float, and expects both methods to return `MF_E_INVALIDMEDIATYPE`, while the original type is still accepted. It skips without an audio device.
- Effect: inputs with a missing or different subtype and otherwise matching attributes now go through a converter. Types that matched before (same subtype as the mix format) are unaffected.
- Validation: with one test build, `test_sar()` fails against `mf.dll` built without `0203` (`mf.c:4932: "MF_MT_SUBTYPE": IsMediaTypeSupported returned 0`, `mf.c:4978` and `mf.c:4980: Unexpected hr 0`, plus a leaked sink reference at `mf.c:5218`) and passes with it, on both architectures, with an audio device present (no skip).
- Wine's `mf` tests (`mf`, `topology`, `transform`) on a complete build with `0203`, both architectures: no new failure compared with `.40` without it.
- In the affected title the audio goes through the resampler into the float mix format and sounds correct, in sync with the video; a second title playing float audio through the media engine is unchanged (maintainer's local tests). Not verified: a run on Windows (in particular that the native renderer rejects the switched subtype rather than converting).

<a id="graphics-route-d3dmetal-metalfx"></a>

## Graphics route for D3DMetal with MetalFX

Patch set `cx26.3-wine11.0-runeon.41` adds `patches/runeon/0204` after `0203`. Status: the maintainer approved it individually on 2026-11-22 as an extension of the per-process graphics route (`0109`, `0113`). It is an ABI-sensitive `ntdll` change, opt-in through the route table and the launcher's variables.

### Need

- D3DMetal's NGX bridge runs DLSS super resolution on MetalFX. It needs `D3DM_ENABLE_METALFX`, `D3DM_NVNGX_PATH`, the bridge reachable as `nvngx` and `_nvngx` (PE and Unix library), and `nvngx`/`nvapi64` loaded native first.
- Setting these for a whole launcher session reaches every process, not one program, and `SetEnvironmentVariable()` cannot reach them.
- Reader: of the D3DMetal 4.0 beta 3 files, only the Unix-side `D3DMetal.framework` binary contains the three names and imports `getenv`. Its PE modules (`d3d10`, `d3d11`, `d3d12`, `dxgi`, `nvapi64`, `nvngx-on-metalfx`) contain none of them, as ASCII or UTF-16. So only the Unix environment is set.
- Builtin lookup: a builtin-marked native file is resolved again by its export name, `nvngx.dll` for the bridge, and its Unix library is taken from the same directory under that name. The alias directory therefore needs `nvngx` as well as `_nvngx`, in both forms.
- DXR: in a local run with MetalFX on, an Unreal Engine program queried NVAPI ray-tracing caps and D3DMetal aborted on opacity-micromap geometry (`Unknown geometry type 2`). With `D3DM_SUPPORT_DXR=0` added, nothing else changed, the abort was gone and DLSS could be selected. So the route turns DXR off with MetalFX.

### Design

- New backend id `d3dmetal-metalfx`. It loads D3DMetal from `RUNEON_GFX_DIR_D3DMETAL`, as the `d3dmetal` route does.
- In addition: `RUNEON_GFX_DIR_METALFX` is prepended to the builtin search path, `nvngx,nvapi64=n,b` is added with the existing `add_load_order_override()`, and the three `D3DM_` variables are set before any module loads.
- Inputs: `RUNEON_GFX_DIR_METALFX` must be absolute. `RUNEON_METALFX_NGX_PATH` must be a drive path of at most 129 printable ASCII characters, D3DMetal's limit, without a trailing separator. Otherwise, or without a D3DMetal directory, the process behaves as on the `d3dmetal` route.
- Inheritance: a child inherits the Unix environment. While `RUNEON_GFX_DIR_METALFX` is set, every process not given MetalFX drops the three variables. Without it nothing is touched, so a caller's own variables keep working.
- Older builds skip the unknown id as a malformed entry; the program gets the `d3dmetal` default.
- Capability marker: `runeon-graphics-routes-metalfx-1`, kept in a TRACE format string like `runeon-graphics-routes-protocol-1`.
- Nothing is keyed on a program; the launcher chooses the directories.

### Validation

- `tests/graphics-route-check.py` compiles the patched route code. It checks the route, load order and variables, the fallback for each invalid or missing input, overlap, malformed ids, the strip only while `RUNEON_GFX_DIR_METALFX` is set, and the marker. Against `.40` it checks that the id is skipped.
- A complete i386/x86_64 WoW64 build of `cx26.3-wine11.0-runeon.41` (tag on `45ef78a`) succeeded and ships in seed `2026.11.22`.
- In one launcher session on macOS 27 (maintainer's local tests, local build and the released Dev build): a D3D12 title on this route loaded `nvngx.dll` from the bridge directory and offered DLSS; the same title off the route, and a D3D11 title on another route, loaded neither bridge file and offered no DLSS. Every run exited normally.
- Not verified: two programs running at the same time with and without the route.

<a id="graphics-route-dxmt-metalfx"></a>

## Graphics route for DXMT with MetalFX

Patch set `cx26.3-wine11.0-runeon.41` adds `patches/runeon/0205` after `0204`. Status: the maintainer approved it individually on 2026-11-22 as an extension of `0204`. It is an ABI-sensitive `ntdll` change, opt-in through the route table and the launcher's variable.

### Need

- DXMT's nvngx converts DLSS super resolution calls to MetalFX. It takes effect only with `DXMT_ENABLE_NVEXT=1`, an NGX core directory holding a signed `nvngx` and `_nvngx`, and `nvngx`/`nvapi64` loaded native first.
- `DXMT_ENABLE_NVEXT` makes DXMT report an NVIDIA adapter. Set for a whole launcher session it reaches every process, not one program.

### Environment delivery

- Upstream DXMT reads its variables with `GetEnvironmentVariableW()`. A Wine child process takes its Windows environment from its parent's startup info, so a Unix `setenv()` as in `0204` does not reach it.
- Writing the Windows environment from `ntdll` was considered and not taken. By the time the route is known, `init_startup_info()` has built the process parameters with the environment inside the same allocation, and `init_peb()` has copied them for a WoW64 process into a second block below 2 GB. Changing the environment would mean rebuilding both blocks in the Unix part of `ntdll` before the PE loader copies them.
- Taken instead: the route sets two Unix variables, exactly as `0204` does, and DXMT from its `0.80-runeon.9` patch set reads them through its own `winemetal` Unix library. Wine's change stays in `loadorder.c`, with no change to process parameters, unix-call tables or the wineserver protocol.

### Design

- New backend id `dxmt-metalfx`. It loads DXMT from `RUNEON_GFX_DIR_DXMT`, as the `dxmt` route does.
- In addition: `nvngx,nvapi64=n,b` is added with the existing `add_load_order_override()`, and `DXMT_ENABLE_NVEXT=1` and `DXMT_NVEXT_NGX_PATH=<RUNEON_DXMT_NGX_PATH>` are set in the Unix environment before any module loads.
- Input: `RUNEON_DXMT_NGX_PATH` follows the `0204` rule for `RUNEON_METALFX_NGX_PATH`, now one helper: a drive path of at most 129 printable ASCII characters without a trailing separator. Otherwise, or without a DXMT directory, the process behaves as on the `dxmt` route.
- Inheritance: while `RUNEON_DXMT_NGX_PATH` is set, every process not given the route drops the two `DXMT_` variables. Without it nothing is touched. The `0204` strip now also runs in a process given `dxmt-metalfx`, so the two routes never share variables.
- Older builds skip the unknown id as a malformed entry; the program gets the `d3dmetal` default. A launcher publishes the id only after finding the marker.
- Capability marker: `runeon-graphics-routes-dxmt-metalfx-1`, kept in a TRACE format string.
- Nothing is keyed on a program; the launcher chooses the directories. No NVIDIA identity is set here; DXMT reports it only while its extension is on.

### Validation

- `tests/graphics-route-check.py` compiles the patched route code. It checks the route, load order and variables, the fallback for each invalid or missing input, overlap, malformed ids, the strip only while `RUNEON_DXMT_NGX_PATH` is set, that each MetalFX route drops the other's variables, and the marker. Against `.40` it checks that the id is skipped. Removing the strip or the fallback makes it fail.
- A complete i386/x86_64 WoW64 build of `cx26.3-wine11.0-runeon.41` (tag on `45ef78a`) succeeded and ships in seed `2026.11.22`.
- With DXMT `0.80-runeon.9`, in one launcher session on macOS 27 (maintainer's local tests, local build and the released Dev build): a D3D11 title on this route logged `runeon-dxmt-nvext-ngx-path-1` with the launcher's NGX directory, loaded `_nvngx.dll` from it and offered DLSS. Off the route, the same title enabled no vendor extension, loaded neither file and offered no DLSS. Every run exited normally.
- Not verified: two programs running at the same time with and without the route.

<a id="graphics-route-ngx-isolation"></a>

## NGX file isolation for graphics routes

Patch set `cx26.3-wine11.0-runeon.41` adds `patches/runeon/0206` after `0205`. Status: the maintainer approved it individually for `ntdll` on 2026-11-24, for this patch only; it is not a precedent. It is an ABI-sensitive `ntdll` change, opt-in through the launcher's variables.

### Need

- `0204` and `0205` isolate variables and search directories. They do not stop a full-path load.
- NGX finds its core through the machine-wide registry value `FullPath`. It can outlive the routed process that set it, so another process in the session may read it and load the launcher's `nvngx` or `_nvngx`.
- Whether a real program reaches this path has not been observed. The fix closes it by construction.

### Design

- After the route is settled, a process not on a MetalFX route gets a disabled load order for `<dir>\nvngx` and `<dir>\_nvngx`. A MetalFX route that fell back for invalid configuration counts as not on it.
- `<dir>` comes from `RUNEON_METALFX_NGX_PATH` (only while `RUNEON_GFX_DIR_METALFX` is set, as that route requires) and `RUNEON_DXMT_NGX_PATH`. The launcher sets them for the whole session, Wine never strips them, and a child inherits them through the Unix environment.
- Each directory passes the existing NGX path rule. One containing a space, comma or equals sign is ignored, because `add_load_order_set()` would split it into rules for other modules.
- `get_load_order()` tries the full path before any basename, ignoring case and the NT prefix. `load_builtin()` refuses a disabled module before mapping it, for native and builtin images.
- Unchanged: processes on a MetalFX route, basename rules for `nvngx` and `nvapi64`, a program's own copies, the System32 placeholder and launches without the variables. The registry is not read or written.
- Limit: the rule covers loads by these four full paths only. The same file reached through another name, such as a `Z:` path or an 8.3 short name, is not covered; `FullPath` holds the drive path the rule matches. A process on a plain `dxmt` route that resolves `nvngx` by basename, including through the System32 placeholder, can still get the same file from the DXMT component directory. That path predates this patch and is left alone because a basename rule would change the `nvngx` load order for programs' own copies.

### Validation

- `tests/graphics-route-check.py` checks the rules added for a plain route, an unrouted process, both MetalFX routes, each invalid-configuration fallback, directories containing separators and unmanaged launches.
- A complete i386/x86_64 WoW64 build of `cx26.3-wine11.0-runeon.41` (tag on `45ef78a`) succeeded and ships in seed `2026.11.22`.
- In one launcher session on macOS 27 (maintainer's local test with load tracing): after routed runs had pointed `FullPath` at the launcher's directories, later runs of a D3D11 and a D3D12 title not on a MetalFX route loaded neither `nvngx` nor `_nvngx` and offered no DLSS; switched back on, both loaded them again.
- Not verified: whether those unrouted runs attempted the load and were refused, or never tried it; two programs running at the same time; the `Z:` and 8.3 names named under Limit.

<a id="mmdevapi-watch-and-code-faults"></a>

## mmdevapi device watch start and builtin code fault logging

Patch set `cx26.3-wine11.0-runeon.42` adds `patches/runeon/0207` and `0208` after `0206`. Status: the maintainer approved the implementation of both individually on 2026-11-24, `0208` for `ntdll` and both without an opt-in; this is not a precedent. Neither has been built or run; admission to a release waits for a complete build and review.

### Need

- In a maintainer's local run of a game, a crash dump showed an execute access violation in `mmdevapi`, inside `MMDevEnum_GetDefaultAudioEndpoint`, on the thread `wine_mmdevapi_devices` that `0154` adds. Crash reports from other machines name the same function with an execute access violation or an illegal instruction.
- The cause is not known. The dumps keep neither the page's protection nor its bytes, so a page whose protection was changed, one whose content changed and a fault in how the code was run cannot be told apart.
- `0154`'s watch thread made every process that loads mmdevapi with the Mac driver run a full listing at start, with twelve calls of `GetDefaultAudioEndpoint` through the enumerator's own method, even with no device change and no registered client. Nothing on Windows or in upstream Wine does this.

### Design

- `0207`: the driver's first `wait_device_change`, which registers the Core Audio listeners and returns `S_FALSE`, runs on the initialising thread before the driver's devices are first listed. Every later change moves the driver's serial past the one the watch thread starts from, so the thread lists the devices again only after a change, and a change during the first listing still wakes it. `device_listed_serial` keeps its meaning.
- `0207`: `GetDefaultAudioEndpoint` and the watch thread share `get_default_device()`, which reads the registry and the device list directly, so the watch thread's default device lookups no longer go through mmdevapi's own COM methods. A refresh after a device change still creates an audio client for a new device (`set_format`) and takes device references. The rule is unchanged; the value size is now reset before each registry query, and an unterminated value is terminated.
- `0207` does not fix a race on a code page; it removes background work the watch thread did at start. Whether that work is needed for the fault to happen is not known.
- `0208`: an execute access violation or illegal instruction at an address inside a builtin image is logged in `dispatch_exception()` before any handler, with the page's protection, state, type and allocation base, the 16 bytes at the address and the thread name. The first four per process. `LdrFindEntryForAddress()` needs the loader lock, so it is taken with the try-only flag and the module's base and name are copied under it; when another thread holds it, the fault is not logged rather than made to wait.
- `0208`: `NtProtectVirtualMemory` making executable pages of a builtin image writable or not executable is logged with the range, the module base and the old and new protection. The thread in the line is the one that made the change; the caller's address is not recorded, because the system call frame only holds the return address into the syscall stub. The first sixteen per process.
- Both lines use their own `codepage` channel at `ERR` level, so a log reader that treats `seh` or `virtual` errors as fatal does not count them.

### Validation

- `scripts/static-check.sh` and `scripts/integration-check.sh` pass for `.42`: the patches apply with zero fuzz and the verification anchors match once.
- Not yet done: a complete i386/x86_64 WoW64 build, the mmdevapi Wine tests against `.41`, `tests/audio-device-change-probe.c` for notifications and stream invalidation, an event-synchronised probe for the watch thread at start and for both log lines, and a check that a real session log holds no protection-change line Wine itself caused, which would use up the limit of sixteen.
