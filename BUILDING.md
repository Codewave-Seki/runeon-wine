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

## 6. Reproduce the targeted checks

The [tests](tests) directory is included in both patch-set and corresponding-source bundles. Use a source tree with the intended patch set already applied as `source_root`. The product configuration above disables upstream Wine tests; affected upstream suites require a separate test-enabled build. Keep the compiler and dependency `PATH` consistent between `configure` and `make`.

`lifetime-check.py` uses Python's standard library and `clang` on macOS. It extracts the real Direct2D/EVR functions, compiles them with small failure-injection dependencies, and checks 51 ownership assertions. Its temporary C source and executable are cleaned automatically. It does not start Wine or use a GPU.

`stream-read-check.py` similarly extracts the real WIC `stream_read` function and runs nine scenarios with 38 assertions. It covers NULL/non-NULL count pointers, full reads, successful short reads normalized to `S_FALSE`, existing `S_FALSE`, and unchanged `E_FAIL` without reading an unwritten count. A protected output slot checks the failure path. It uses dependency stubs, not Wine or real codecs, and automatically cleans its temporary build files.

The remaining commands compile the three independent Windows API probes with MinGW; they do not execute them.

```bash
python3 tests/lifetime-check.py "$source_root"
python3 tests/stream-read-check.py "$source_root"

probe_dir="$(mktemp -d)"
x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Werror \
  -Wno-unused-parameter -O0 tests/graphics-probe.c \
  -o "$probe_dir/graphics-probe.exe" \
  -lsecur32 -ld3d10 -ldwrite -lwindowscodecs -lole32 -luuid
x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Werror \
  -O2 tests/dinput-probe.c -o "$probe_dir/dinput-probe.exe" \
  -ldinput8 -ldxguid -lole32
x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Werror \
  -O0 -municode tests/winhttp-probe.c \
  -o "$probe_dir/winhttp-probe.exe" -lwinhttp
```

The graphics probe covers state-block masks, adapter reference ownership, SChannel contexts, WIC streams and TTC fragments. The DirectInput probe checks both action orders for identical control IDs on different device GUIDs. The WinHTTP probe uses [winhttp-fixture.py](tests/winhttp-fixture.py), which serves the long-header response and a `DIRECT` PAC only on `127.0.0.1`; it makes no external requests.

After compiling the probes, start the fixture in the same shell so `probe_dir` remains available:

```bash
fixture_port=8765
python3 tests/winhttp-fixture.py --port "$fixture_port" \
  >"$probe_dir/winhttp-fixture.log" 2>&1 &
fixture_pid=$!
cat "$probe_dir/winhttp-fixture.log"
```

Wait until the log prints `Wine regression fixture on 127.0.0.1:8765` before running a probe; read the log again if startup is still in progress. If the port is occupied, select another port and use it consistently. Through the managed launch path described below, run `winhttp-probe.exe` with arguments `headers 8765`, then `pac 8765` (substitute the selected port). The first reads the complete oversized-header response; the second checks normal and excessive PAC hostnames. These instructions do not mark API execution as passed.

After both probes finish, stop only the fixture process started above. `wait` may report termination by the requested signal:

```bash
kill -TERM "$fixture_pid"
wait "$fixture_pid" 2>/dev/null || true
```

Execute the PE probes through the Runeon product's supported managed launch path, with a genuine gatekeeper grant and isolated test prefix. An ordinary direct Wine invocation can be rejected by managed launch validation; that is not evidence that a DLL test failed. This public repository does not contain the private App test launcher or bypass the gate. Use matching builtin Wine DLLs, record the exact runtime and prefix, compare old and candidate builds, and remove temporary probe binaries after retaining lightweight results.

Probe compilation, function-level fault injection, Windows API execution, affected-module tests, and product graphics/Steam smoke are separate checks. Their current status is recorded in [the targeted review](BACKPORTS-11.16-11.17.md); none alone authorizes a runtime release.
