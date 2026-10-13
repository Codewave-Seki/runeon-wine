# Steam launch preparation

[English](LAUNCH-PREPARE.md) | [Simplified Chinese](LAUNCH-PREPARE.zh-CN.md) | [Japanese](LAUNCH-PREPARE.ja.md)

> This English document is authoritative. The Chinese and Japanese documents are complete translations for convenience.

Candidate `cx26.3-wine11.0-runeon.18` adds `patches/runeon/0118-steam-launch-route-preparation.patch` after `0117`.

## Behaviour

The patch adds the opt-in `RUNEON_LAUNCH_PREPARE_V1` to the Steam middleware selector that `0104` and `0111` use. With it set to `1`, every x64 or i386 executable created by `steam.exe` with a valid session grant and a nonzero `SteamAppId` runs the fixed helper `C:\runeon-input-source\runeon-input-source-launch.exe` in `launch-prepare` mode first. The helper only prepares; Wine waits for it to exit and then creates the original child with the same command line, creation flags and handles.

The caller uses the step to settle state that a child reads when it starts, such as its graphics route. The first process of a game then already sees the new state, instead of the state the session started with.

- The D3D12on7 preparation from `0111` is split into a directory lock and a helper run. One lock covers both preparations and the original creation, and route preparation runs first.
- The NW.js hand-off from `0104` replaces the child and keeps priority, so it gets no route preparation.
- The NW.js and D3D12on7 gates stay x64-only.
- Without the opt-in, or when the parent is not `steam.exe`, the grant or `SteamAppId` is missing, a token or extended startup information is passed, the image is neither x64 nor i386, or the helper is missing, creation is unchanged. A helper that cannot be started is skipped.
- Wine reads no game names, AppIDs or executable names.

## Validation

`tests/launch-prepare-check.py` compiles the patched selector and preparation functions with stubs. It covers the opt-in, x64 and i386 images, the NW.js and D3D12on7 interplay, command preservation, failure fallback, and that a helper runs only while the lock is held. `scripts/integration-check.sh` runs it for any tree with the opt-in; the earlier `tests/d3d12on7-launch-check.py` still covers patch sets without it. The check does not run Wine, the helper or a real Steam launch. Compilation is shown only by a complete i386/x86_64 WoW64 build.
