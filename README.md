# Runeon Wine

[English](README.md) | [Chinese](README.zh-CN.md) | [Japanese](README.ja.md)

> This English document is authoritative. The Chinese and Japanese documents are complete translations for convenience.

Runeon Wine is the public source-maintenance repository for Runeon's Steam Baseline runtime. It preserves a pinned Wine/CrossOver Wine baseline, upstream backports, product-specific patches, and reproducible corresponding-source build entry points. This repository does not create a general compatibility promise and does not contain the Runeon App, Steam, games, or Apple private graphics components.

## Current baseline

- CodeWeavers source: `crossover-sources-26.3.0.tar.gz`
- Wine baseline: `Wine version 11.0`
<!-- release-facts:current-patch-set -->
- Default source definition (Production): `cx26.3-wine11.0-runeon.9` (stable), paired with seed `2026.09.13` and MoltenVK `1.4.2`
- Current Dev source: [`cx26.3-wine11.0-runeon.10`](FLSGETVALUE2.md) (Pre-release), paired with seed `2026.09.22` and MoltenVK `1.4.2`, App gate `>=1.8 (0)`. Select its independent `patchsets/` definition explicitly; the default series remains `.9`.
- Current Production source and latest stable Release: `cx26.3-wine11.0-runeon.9` for seed `2026.09.13`, App gate `>=1.8 (0)`
- Validation: Dev artifact, API integration and feed/range checks completed. A full download recheck remains pending after local transfer timeouts. Real Steam/game and App acceptance will be performed by the user after delivery; they are not claimed complete. Dev and Production App remain `1.8 (3)`; App `1.8 (4)` publication awaits release-note approval.
- Retained Production rollback sources: `cx26.3-wine11.0-runeon.8` for seed `2026.09.06`, `cx26.3-wine11.0-runeon.6` for seed `2026.08.11.1`, and `cx26.3-wine11.0-runeon.5` for seed `2026.08.03.2`
- Upstream audited through: `wine-11.15`

[`base/crossover-26.3-wine-11.0.json`](base/crossover-26.3-wine-11.0.json) is the single machine-readable source for the baseline URL, SHA-256, and source root. [`series`](series) defines patch order. [`patches/manifest.json`](patches/manifest.json) records provenance, risk, and full upstream commit identifiers.

## Scope and boundaries

- The repository does not commit complete Wine source trees, build directories, runtime archives, or Wine prefixes.
- It does not download, mirror, or distribute `D3DMetal.framework`, `libd3dshared.dylib`, or GPTK private PE/Unix overlays.
- Upstream commits enter an audit list first. A clean apply does not automatically place a commit in the active series.
- Changes to `ntdll`, `server`, `wow64`, `loader`, `winemac.drv`, `win32u`, Unix library/server protocols, or D3DMetal interfaces are ABI-sensitive by default and require a separate baseline upgrade or stronger validation.
- The Runeon product repository remains responsible for component packaging, Developer ID signing, Dev/Production feeds, download verification, and release readiness.
- This repository and its GitHub Release assets are public. Every distributed Production runtime must have an immutable stable Release containing the exact patch-set bundle, complete corresponding source, and SHA-256 files. Candidates awaiting Production promotion must remain Pre-releases.
- `patchsets/cx26.3-wine11.0-runeon.0` is the exact historical source definition for Production seed `2026.07.22`. `patchsets/cx26.3-wine11.0-runeon.1` freezes the first unreleased Escape-fix candidate. The default [`series`](series) describes the `.9` source used by Production seed `2026.09.13`. Rebuilding any earlier patch set means checking out the commit that carried it, not the current working tree.
- `release-manifests/` records each public bundle's commit, Release URL, file name, size, SHA-256, stable/prerelease state, and permanent-retention rule. A Production runtime may reference only a `stable` manifest. A Pre-release does not mean that its fixes are available to Production users.

## Release status

- [`cx26.3-wine11.0-runeon.0`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.0) is the historical stable corresponding-source Release for retained legacy seed `2026.07.22`.
- [`cx26.3-wine11.0-runeon.1`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.1) is the first unreleased candidate containing the Escape `cfgmgr32` backport. It did not ship in a Dev or Production runtime and remains a Pre-release.
- `cx26.3-wine11.0-runeon.2` adds 33 manually reviewed stability and correctness backports from Wine 11.1 through 11.14. Its decisions and exclusions are recorded in [`AUDIT-11.0-11.14.md`](AUDIT-11.0-11.14.md).
- `cx26.3-wine11.0-runeon.3` introduced managed Steam launch validation on top of `.2` and remains an immutable historical candidate.
- `cx26.3-wine11.0-runeon.4` remains an immutable historical candidate.
- [`cx26.3-wine11.0-runeon.5`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.5) is the stable corresponding-source Release for retained Production rollback seed `2026.08.03.2`. It inherited `.4`, streamlined managed Steam launch validation, and remains immutable together with its published assets and checksums.
- [`cx26.3-wine11.0-runeon.6`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.6) is the stable corresponding-source Release for retained Production rollback seed `2026.08.11.1`. It inherits `.5` and adds nine reviewed backports from Wine 11.15, extending the audit through `wine-11.15`; its decisions and exclusions are recorded in [`AUDIT-11.0-11.15.md`](AUDIT-11.0-11.15.md). Build, signing, readiness, authenticated download, and product-path validation are complete, and Production uses the exact artifact bytes verified in Dev.

- [`cx26.3-wine11.0-runeon.7`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.7) added the Vuplex capability but **does not compile**: `dlls/kernelbase/process.c` uses the registry API without including `winreg.h`. It was tagged before a full build had been run, remains an immutable historical candidate, and must not be built into any runtime.
- [`cx26.3-wine11.0-runeon.8`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.8) is `.7` with that include added, verified by a complete x86_64/WoW64 build before tagging. It is the stable corresponding-source Release for retained Production rollback seed `2026.09.06`, and Production uses the exact artifact bytes verified in Dev.

- Historical `.9` delivery and validation (still current in Production): [`cx26.3-wine11.0-runeon.9`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.9): Seed `2026.09.13` uses `.9` with MoltenVK `1.4.2` in Dev and Production, with a minimum App gate of `1.8 (0)`. Production reuses the exact signed archive verified in Dev. Build, API regression, signing, readiness, authenticated feed/ticket and full-download verification are complete. The user confirmed component updates, runtime preparation and Steam startup with the current Xcode Dev source build. Distribution was checked for `1.8 (0-3)`; per-build game, stop/restart interaction and clean-prefix acceptance were not repeated. Published App installers remain `1.8 (3)`. The source Release is stable; tag, commit and all four asset bytes, sizes and digests are unchanged. `.8` / seed `2026.09.06` is retained history. It preserves `.8` and adds 10 targeted backport files carrying 13 Wine 11.16/11.17 commits. Immutable source details are in [the release manifest](release-manifests/cx26.3-wine11.0-runeon.9.source-archive.json); selection and validation limits are in [the targeted review](BACKPORTS-11.16-11.17.md). The complete audit boundary remains `wine-11.15`.

## Quick verification

```bash
scripts/static-check.sh
source_root="$(scripts/fetch-source.sh)"
scripts/integration-check.sh "$source_root"
```

`integration-check.sh` copies the pinned baseline to a temporary tree, applies every patch in order, verifies final markers, and confirms that the same series cannot be silently applied twice. It does not modify the cached original source.

## Build corresponding-source bundles

```bash
source_root="$(scripts/fetch-source.sh)"
scripts/apply-series.sh "$source_root"
scripts/build-source-bundle.sh "$source_root" dist
```

The source bundle used for a Release must use the same base SHA and patch set as the runtime artifact. Each tagged GitHub Release must contain the source bundle, patch-set bundle, and both SHA-256 files. See [`BUILDING.md`](BUILDING.md) and [`MAINTENANCE.md`](MAINTENANCE.md) for the build, release, and maintenance policies.

The Runeon product repository consumes the smaller patch-set bundle:

```bash
scripts/build-patchset-bundle.sh dist
```

It contains the pinned baseline manifest, patches, series, verification scripts, and license. The Runeon build still obtains the complete Wine and related source archive from CodeWeavers at the pinned SHA.

To rebuild the exact source for retained legacy seed `2026.07.22`, select `.0` explicitly. Do not substitute any later patch set, including `.8` or `.9`:

```bash
export RUNEON_WINE_PATCHSET_DEFINITION=patchsets/cx26.3-wine11.0-runeon.0
source_root="$(scripts/fetch-source.sh)"
scripts/apply-series.sh "$source_root"
scripts/build-source-bundle.sh "$source_root" dist
scripts/build-patchset-bundle.sh dist
scripts/build-source-archive-manifest.sh \
  dist/runeon-wine-patchset-cx26.3-wine11.0-runeon.0.tar.gz \
  dist/runeon-wine-source-cx26.3-wine11.0-runeon.0.tar.gz \
  dist/cx26.3-wine11.0-runeon.0.source-archive.json
```

## License

Wine and the modifications derived from Wine/CrossOver Wine in this repository are licensed under LGPL-2.1-or-later. See [`LICENSE`](LICENSE). Upstream commits retain their original authorship and Wine project history. The Runeon App's license is not changed by this repository.
