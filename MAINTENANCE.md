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

The active `cx26.3-wine11.0-runeon.2` candidate is based on a commit-by-commit review through `wine-11.14`; see [the complete audit](AUDIT-11.0-11.14.md). The review selected 33 localized correctness and stability fixes in addition to the two previously accepted upstream backports. It did not attempt to copy every upstream commit. Feature work, broad refactors, ABI-sensitive changes, subsystem migrations, fixes already present in CrossOver 26.3, and fixes whose dependency or regression surface was not yet bounded remain deferred.

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
