# Optional DXGI driver version correction

[English](DXGI-DRIVER-VERSION.md) | [Chinese](DXGI-DRIVER-VERSION.zh-CN.md) | [Japanese](DXGI-DRIVER-VERSION.ja.md)

Candidate `.11` includes `0105-dxgi-driver-version.patch`, an independent LGPL-2.1-or-later `runeondxgi.dll`. It uses public COM/SetupAPI interfaces and Wine's GPU LUID device property; it does not incorporate the unlicensed external shim.

The module repairs only successful `CheckInterfaceSupport` results equal to -1. It requires one exact LUID match and a valid four-part existing `DriverVersion`; normal versions, failures, null outputs and unknown devices retain provider behavior. Factories return original COM objects. Provider vtables are registered separately under a lock and the module is pinned before changing a method pointer. The bounded table registry leaves additional tables untouched.

The module is inert until the App stages it with the user's local x64 Apple provider. The App preserves the original, renames only its export-library identity to `rxdg.dll`, supplies the matching unix alias and installs the wrapper atomically. New provider exports require review. The seed must exclude this private provider and its backup. No game-name matching or private Apple files belong in this repository.

Build with the explicit `.11` definition and the normal Wine build. A standalone policy check is `x86_64-w64-mingw32-gcc -std=c11 -I /path/to/patched/wine/dlls/runeondxgi tests/dxgi-driver-version.c -o /tmp/dxgi-policy.exe -lsetupapi -ladvapi32 -ldxguid -luuid`; also build with i686. Run through the product's authorized isolated Wine probe.

Local status: the Wine build system produces both x64 and i386 modules. The x64 module passes original/fixed/restored API comparison through all three factory entry points, including a fresh prefix staged by the production synchronizer; x64/x86 policy checks pass. The renamed provider must be staged before first boot, as Wine does not create it automatically. Product installer/private-overlay checks and App compilation pass. This is not a published seed or proof that a specific game runs; the complete runtime and release gates remain separate.
