# Runeon Wine

[English](README.md) | [Chinese](README.zh-CN.md) | [Japanese](README.ja.md)

> This English document is authoritative. The Chinese and Japanese documents are complete translations for convenience.

Runeon Wine is the public source-maintenance repository for Runeon's Steam Baseline runtime. It preserves a pinned Wine/CrossOver Wine baseline, upstream backports, product-specific patches, and reproducible corresponding-source build entry points. This repository does not create a general compatibility promise and does not contain the Runeon App, Steam, games, or Apple private graphics components.

## Current baseline

- CodeWeavers source: `crossover-sources-26.3.0.tar.gz`
- Wine baseline: `Wine version 11.0`
- Active source candidate: `cx26.3-wine11.0-runeon.4`
- Upstream audited through: `wine-11.14`

[`base/crossover-26.3-wine-11.0.json`](base/crossover-26.3-wine-11.0.json) is the single machine-readable source for the baseline URL, SHA-256, and source root. [`series`](series) defines patch order. [`patches/manifest.json`](patches/manifest.json) records provenance, risk, and full upstream commit identifiers.

## Scope and boundaries

- The repository does not commit complete Wine source trees, build directories, runtime archives, or Wine prefixes.
- It does not download, mirror, or distribute `D3DMetal.framework`, `libd3dshared.dylib`, or GPTK private PE/Unix overlays.
- Upstream commits enter an audit list first. A clean apply does not automatically place a commit in the active series.
- Changes to `ntdll`, `server`, `wow64`, `loader`, `winemac.drv`, `win32u`, Unix library/server protocols, or D3DMetal interfaces are ABI-sensitive by default and require a separate baseline upgrade or stronger validation.
- The Runeon product repository remains responsible for component packaging, Developer ID signing, Dev/Production feeds, download verification, and release readiness.
- This repository and its GitHub Release assets are public. Every distributed Production runtime must have an immutable stable Release containing the exact patch-set bundle, complete corresponding source, and SHA-256 files. Unreleased candidates must remain Pre-releases.
- `patchsets/cx26.3-wine11.0-runeon.0` is the exact historical source definition for Production seed `2026.07.22`. `patchsets/cx26.3-wine11.0-runeon.1` freezes the first unreleased Escape-fix candidate. The default [`series`](series) describes the active `.4` source candidate; an active source candidate is not a shipped runtime.
- `release-manifests/` records each public bundle's commit, Release URL, file name, size, SHA-256, stable/prerelease state, and permanent-retention rule. A Production runtime may reference only a `stable` manifest. A Pre-release does not mean that its fixes are available to Production users.

## Release status

- [`cx26.3-wine11.0-runeon.0`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.0) is the stable corresponding-source Release for current Production seed `2026.07.22`.
- [`cx26.3-wine11.0-runeon.1`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.1) is the next-runtime candidate containing the Escape `cfgmgr32` backport. It has not shipped in a Dev or Production runtime and remains a Pre-release.
- `cx26.3-wine11.0-runeon.2` adds 33 manually reviewed stability and correctness backports from Wine 11.1 through 11.14. Its decisions and exclusions are recorded in [`AUDIT-11.0-11.14.md`](AUDIT-11.0-11.14.md).
- `cx26.3-wine11.0-runeon.3` introduced managed Steam launch validation on top of `.2` and remains an immutable historical candidate.
- `cx26.3-wine11.0-runeon.4` is the active source candidate. It inherits `.3` and improves managed Steam launch validation. It is not available to users until the source Release and separate Runeon runtime gates are complete.

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

To rebuild the exact source for distributed seed `2026.07.22`, select `.0` explicitly. Do not substitute `.1` or the active `.2` candidate:

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
