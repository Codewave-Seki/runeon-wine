# Runeon Wine

[English](README.md) | [Chinese](README.zh-CN.md) | [Japanese](README.ja.md)

> This English document is authoritative. The Chinese and Japanese documents are complete translations for convenience.

Runeon Wine is the public source-maintenance repository for Runeon's Steam Baseline runtime. It holds a pinned CrossOver Wine baseline, upstream backports, Runeon patches, and the scripts that rebuild the exact corresponding source of every shipped runtime. It makes no general compatibility promise, and it does not contain the Runeon App, Steam, games or Apple private graphics components.

## Baseline and current release

- CodeWeavers source: `crossover-sources-26.3.0.tar.gz`, Wine `11.0`, pinned in [`base/crossover-26.3-wine-11.0.json`](base/crossover-26.3-wine-11.0.json)
<!-- release-facts:current-patch-set -->
- Current stable source: [`cx26.3-wine11.0-runeon.27`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.27), for seed `2026.10.26` (App `>=1.14 (16)`). Definition in [`patchsets/cx26.3-wine11.0-runeon.27`](patchsets/cx26.3-wine11.0-runeon.27/manifest.json)
- Upstream reviewed through: `wine-11.15` (complete audit), `wine-11.20` (filtered sweeps); see [MAINTENANCE.md](MAINTENANCE.md#review-records)

## Repository layout

| Path | Contents |
|---|---|
| `base/` | Pinned source archive URL, SHA-256 and source root |
| `patchsets/<id>/` | `series` and `manifest.json` for one patch set. Scripts use the newest one unless `RUNEON_WINE_PATCHSET_DEFINITION=patchsets/<id>` selects another; `.2`–`.8` exist only at their tags |
| `patches/upstream/`, `patches/runeon/` | WineHQ backports and Runeon patches |
| `release-manifests/` | Commit, Release URL, asset names, sizes, SHA-256 and state of every published Release |
| `tests/`, `scripts/` | Probes and checks; fetch, apply, verify and bundle scripts |
| `docs/` | Upstream review records and patch design notes (English) |

## Scope and boundaries

- No Wine source trees, build output, runtime archives, prefixes, user data or diagnostics are committed.
- `D3DMetal.framework`, `libd3dshared.dylib` and GPTK private overlays are never downloaded, mirrored or distributed.
- A clean apply does not put an upstream commit into a patch set; every commit is reviewed first. Changes to `ntdll`, `server`, `wow64`, the loader, `winemac.drv`, `win32u` or Unix/server protocols are ABI-sensitive and excluded unless individually approved. The exception is upstream fixes for macOS-specific behaviour that apply to Wine code CrossOver keeps unchanged and change no protocol or shared structure; they are carried in reviewed batches (see `CLAUDE.md` §3).
- Runeon patches are opt-in, never match game names or AppIDs, and the exceptions are listed in [MAINTENANCE.md](MAINTENANCE.md#product-patches-without-an-opt-in).
- Every runtime shipped to Production has an immutable stable Release with the exact patch-set bundle, complete corresponding source and SHA-256 files. Candidates stay Pre-releases. Tags and Release assets are never moved, replaced or deleted.
- Packaging, signing and distribution of the runtime are done by the Runeon product repository.

## Verify and build

```bash
scripts/static-check.sh
source_root="$(scripts/fetch-source.sh)"
scripts/integration-check.sh "$source_root"
```

`integration-check.sh` applies every patch in order to a temporary copy of the pinned baseline, checks the verification markers, and confirms that the series cannot be applied twice. To build the bundles for a Release:

```bash
export RUNEON_WINE_PATCHSET_DEFINITION=patchsets/cx26.3-wine11.0-runeon.27
source_root="$(scripts/fetch-source.sh)"
scripts/apply-series.sh "$source_root"
scripts/build-source-bundle.sh "$source_root" dist
scripts/build-patchset-bundle.sh dist
```

To rebuild an earlier patch set, select its definition the same way, or check out its tag (required for `.2`–`.8`). The full build procedure is in [BUILDING.md](BUILDING.md).

## Release history

Each Release carries the patch-set bundle, the complete corresponding source and both SHA-256 files; its notes describe what the patch set adds. "Delivered" means the product still serves that seed to the App builds shown.

| Patch set | State | Seed | Status |
|---|---|---|---|
| [`.27`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.27) | stable, latest | `2026.10.26` | Current; delivered to App `>=1.14 (16)` |
| [`.26`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.26) | stable | `2026.10.25.2` | Superseded, retained |
| [`.25`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.25) | pre-release | `2026.10.25.1` | Dev candidate, superseded by `.26`, never in Production |
| [`.24`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.24) | stable | `2026.10.25` | Superseded, retained |
| [`.23`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.23) | stable | `2026.10.24` | Superseded, retained |
| [`.22`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.22) | stable | `2026.10.20` | Delivered to App `1.13 (10)`–`1.13 (15)` |
| [`.21`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.21) | stable | `2026.10.18` | Superseded, retained |
| [`.20`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.20) | stable | `2026.10.16.1` | Superseded, retained |
| [`.19`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.19) | pre-release | `2026.10.16` | Dev candidate, never in Production |
| [`.18`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.18) | stable | `2026.10.14` | Delivered to App `1.12 (6)`–`1.13 (9)` |
| [`.17`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.17) | stable | `2026.10.10` | Delivered to App `1.10 (3)`–`1.11 (4)` |
| [`.16`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.16) | stable | `2026.10.06` | Delivered to App `1.10 (0)`–`1.10 (2)` |
| [`.15`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.15) | stable | `2026.10.04` | Superseded, retained |
| [`.14`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.14) | stable | `2026.10.02` | Superseded, retained |
| [`.13`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.13) | stable | `2026.09.28.1`, `2026.09.28` | `2026.09.28.1` delivered to App `1.8 (0)` and later, before `1.10 (0)` |
| [`.12`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.12) | pre-release | `2026.09.27` | Dev candidate, never in Production |
| [`.11`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.11) | stable | `2026.09.26` | Superseded, retained |
| [`.10`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.10) | stable | `2026.09.22` | Superseded, retained |
| [`.9`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.9) | stable | `2026.09.13` | Superseded, retained |
| [`.8`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.8) | stable | `2026.09.06` | Superseded, retained |
| [`.7`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.7) | pre-release | — | **Does not compile**; never build it (fixed in `.8`) |
| [`.6`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.6) | stable | `2026.08.11.1` | Superseded, retained |
| [`.5`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.5) | stable | `2026.08.03.2` | Superseded, retained |
| [`.2`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.2)–[`.4`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.4) | pre-release | — | Historical candidates, never shipped |
| [`.1`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.1) | pre-release | — | First candidate, never shipped |
| [`.0`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.0) | stable | `2026.07.22` | Superseded, retained |

## Documentation

- [BUILDING.md](BUILDING.md): building the runtime and the source bundles
- [MAINTENANCE.md](MAINTENANCE.md): upstream review, patch gates, opt-in exceptions and release policy
- [SECURITY.md](SECURITY.md): reporting security issues
- [docs/upstream-reviews.md](docs/upstream-reviews.md) and [docs/patch-notes.md](docs/patch-notes.md): review records and patch design notes

## License

Wine and the modifications derived from Wine/CrossOver Wine in this repository are licensed under LGPL-2.1-or-later. See [`LICENSE`](LICENSE). Upstream commits keep their original authorship. This repository does not change the license of the Runeon App.
