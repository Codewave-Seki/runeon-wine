# Building and Source Bundles

[English](BUILDING.md) | [Chinese](BUILDING.zh-CN.md) | [Japanese](BUILDING.ja.md)

> This English document is authoritative. The Chinese and Japanese documents are complete translations for convenience.

## 1. Fetch the pinned baseline

```bash
source_root="$(scripts/fetch-source.sh)"
```

The script accepts only the archive SHA-256 pinned in the manifest and verifies `VERSION`. The default cache is under `.work/` and is not committed to Git.

## 2. Apply the patch series

```bash
scripts/apply-series.sh "$source_root"
scripts/verify-source.sh "$source_root"
```

Patches must be applied in [`series`](series) order. Already-applied patches, partial application, SHA drift, and baseline mismatches must fail rather than being skipped silently.

## 3. Runeon macOS WoW64 configuration boundary

The product build uses the complete CrossOver 26.3/Wine 11.0 source tree. Replacing individual DLLs inside an already-built `lib/wine` is not supported. The core configuration is:

```bash
"$source_root/configure" \
  --build=x86_64-apple-darwin \
  --host=x86_64-apple-darwin \
  --prefix="$install_root" \
  --enable-archs=i386,x86_64 \
  --disable-tests \
  --with-gstreamer \
  --without-usb \
  --without-pcap \
  --without-cups \
  --without-krb5 \
  --without-gssapi \
  --without-sdl \
  --without-opencl \
  --without-x
```

Dependency preparation, GStreamer, MoltenVK, GnuTLS, the Rockstar-scoped D2D wrapper, Apple user-local overlays, signing, and runtime component packaging remain in the Runeon product repository.

## 4. Build the corresponding-source bundle

```bash
scripts/build-source-bundle.sh "$source_root" dist
```

The output contains the patched Wine source, base and patch manifests, patch files, [`series`](series), maintenance scripts, the license, and build instructions. For a Release, record the generated SHA-256 values, upload both bundles and their `.sha256` files to the public GitHub Release for the same tag, and place the immutable asset URLs in Runeon runtime component metadata and the public source offer.

## 5. Build the patch-set bundle used by the product

```bash
scripts/build-patchset-bundle.sh dist
```

The Runeon product repository downloads and verifies this smaller bundle, then applies [`series`](series) to the CrossOver archive at the pinned SHA. The patch-set bundle and complete corresponding-source bundle must come from the same tag and must not be rebuilt independently and mixed.

Candidate tags must be published as GitHub Pre-releases. The same immutable tag may become a stable Release only after the matching runtime passes Dev validation and is actually promoted to Production. Production builds must not use branch archives or `latest` URLs.

If a patch-set ID already has a tag with the same name, bundle scripts permit rebuilding only from the exact commit checked out at that tag. This prevents later documentation or script changes on `main` from silently changing historical assets. Create a new patch-set ID for continued development; do not move an old tag or replace an old Release asset.
