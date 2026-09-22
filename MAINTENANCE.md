# Upstream Maintenance Policy

[English](MAINTENANCE.md) | [Chinese](MAINTENANCE.zh-CN.md) | [Japanese](MAINTENANCE.ja.md)

> This English document is authoritative. The Chinese and Japanese documents are complete translations for convenience.

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

## Current Wine 11.x review baseline

Current Dev source: [`cx26.3-wine11.0-runeon.10`](FLSGETVALUE2.md) (Pre-release), paired with seed `2026.09.22` and MoltenVK `1.4.2`, App gate `>=1.8 (0)`. Select its independent `patchsets/` definition explicitly; the default series remains `.9`. Validation: Dev artifact, API integration and feed/range checks completed. A full download recheck remains pending after local transfer timeouts. Real Steam/game and App acceptance will be performed by the user after delivery; they are not claimed complete. Dev and Production App remain `1.8 (3)`; App `1.8 (4)` publication awaits release-note approval.

Historical `.9` delivery and validation (still current in Production): The active `.9` patch set preserves the `.8` baseline and its product patches. Its [targeted Wine 11.16/11.17 review](BACKPORTS-11.16-11.17.md) adds 10 patch files carrying 13 upstream commits; it does not extend the complete audit beyond `wine-11.15`. Seed `2026.09.13` uses `.9` with MoltenVK `1.4.2` in Dev and Production, with a minimum App gate of `1.8 (0)`. Production reuses the exact signed archive verified in Dev. Build, API regression, signing, readiness, authenticated feed/ticket and full-download verification are complete. The user confirmed component updates, runtime preparation and Steam startup with the current Xcode Dev source build. Distribution was checked for `1.8 (0-3)`; per-build game, stop/restart interaction and clean-prefix acceptance were not repeated. Published App installers remain `1.8 (3)`. The source Release is stable; tag, commit and all four asset bytes, sizes and digests are unchanged. `.8` / seed `2026.09.06` is retained history.

`cx26.3-wine11.0-runeon.8` carries `patches/runeon/0102-vuplex-accelerated-paint-policy.patch`. (`.7` carried an earlier form of the same patch that does not compile — it was tagged before a full build had been run. `static-check.sh` and `integration-check.sh` verify that patches apply, not that the result builds, so a product patch must go through a complete build before it is tagged.) It gives Wine the ability to make a Vuplex 3D WebView CEF host fall back from `OnAcceleratedPaint()` to CPU `OnPaint()`, and nothing more: the switch is only appended when the calling application has opted in through `HKCU\Software\Wine\AppDefaults\<image>\Runeon`, value `DisableVuplexAcceleratedPaint`. No game name or Steam AppID appears anywhere in Wine — the policy is written by the caller, so which titles need the fallback stays outside this repository. The host is matched on the image the command line will actually run ending in `.vuplex`, carrying `--vx-graphics-api=d3d11` and no `--type=`, so neither Chromium child processes nor a stray `.vuplex` path inside some other argument can trigger it. Falling back costs a CPU copy per painted frame, which is why it is opt-in rather than always on.

The shipped `cx26.3-wine11.0-runeon.8` patch set is based on a commit-by-commit review through `wine-11.15`; see [the complete audit](AUDIT-11.0-11.15.md) and the earlier [11.0 to 11.14 audit](AUDIT-11.0-11.14.md) it extends. The review selected 42 localized correctness and stability fixes in addition to the two previously accepted upstream backports. It did not attempt to copy every upstream commit. Feature work, broad refactors, ABI-sensitive changes, subsystem migrations, fixes already present in CrossOver 26.3, and fixes whose dependency or regression surface was not yet bounded remain deferred.

The audit is a point-in-time decision record, not a permanent allowlist. A deferred commit may enter a later immutable patch set after its dependency chain, affected tests, independent probe, and Runeon impact are established. Conversely, inclusion in the audit does not authorize publishing a runtime: the complete build and product smoke gates below still apply.

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

## Release policy

Patch sets use immutable identifiers such as `cx26.3-wine11.0-runeon.1`. The Git tag, source bundle, runtime component metadata, and Runeon release documentation must reference the same identifier. Publish a new artifact to Dev first and complete readiness, download, and product smoke tests. Production may promote only the exact verified bytes; do not rebuild directly for Production.

The repository and bundles are public. After creating a candidate tag, use `build-source-archive-manifest.sh` to record the public Release URL, asset SHA/size, and `prerelease` state, then upload the patch-set bundle, complete corresponding source, and both `.sha256` files. Change the same immutable Release and manifest to `stable` only after the matching runtime actually enters Production. Historical tags and assets are retained permanently and must not be moved, replaced, or deleted.

The public repository contains only the LGPL Wine/CrossOver Wine lineage, patches, and build scripts. The Runeon App, user diagnostics, Steam/game files, Developer ID or service secrets, and D3DMetal/GPTK/Apple private components must never enter Git history, Actions artifacts, or Release assets. Before every public update, inspect the complete history, Release asset list, and Actions logs.
