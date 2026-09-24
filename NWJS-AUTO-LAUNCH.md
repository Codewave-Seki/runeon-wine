# Automatic NW.js launch candidate

[English](NWJS-AUTO-LAUNCH.md) | [Chinese](NWJS-AUTO-LAUNCH.zh-CN.md) | [Japanese](NWJS-AUTO-LAUNCH.ja.md)

The English document is authoritative. The other versions are complete translations.

Candidate `cx26.3-wine11.0-runeon.11` inherits `.10` and adds `0104-steam-nwjs-automatic-launch.patch`. Select it with `RUNEON_WINE_PATCHSET_DEFINITION=patchsets/cx26.3-wine11.0-runeon.11`. Historical definitions and the default patch set are unchanged.

Only process creation by `steam.exe` is considered, with the App session flag `RUNEON_NWJS_AUTO_V1=1`, a grant, a nonzero SteamAppId, an x64 PE and adjacent `nw.dll`, `nw_elf.dll` and `package.json`. The original command is routed to the fixed App-owned helper. Debug launches are left alone. No game ID or game executable name is matched.

The matching App/helper authenticates the local request and checks the account, installation, command, runtime version, protection, component, saves and user settings. It either launches the managed NW.js, explicitly passes through an ineligible original command, or reports preparation failure. The hook does not modify game files or live Steam configuration. Old Apps never set the capability flag; new Apps require the capability marker in both kernelbase architectures before removing the old guidance.

Local development checks: the modified process translation unit compiles for i686 and x86_64; the App/helper protocol check covers managed launch, pass-through, denial, timeout and session cancellation. These are not a full Wine build or a real Steam/game launch. Build and distribute the matching runtime and App together through the existing release process; this candidate is not a published runtime.
