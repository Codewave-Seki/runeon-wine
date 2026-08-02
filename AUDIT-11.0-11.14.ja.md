# Wine 11.0 から 11.14 までの安定性レビュー

[English](AUDIT-11.0-11.14.md) | [Chinese](AUDIT-11.0-11.14.zh-CN.md) | [Japanese](AUDIT-11.0-11.14.ja.md)

> 英語文書が正式版です。本書は完全な日本語訳です。

## 範囲

固定比較範囲は `wine-11.0..wine-11.14`、基準は SHA-256 `ac99c8ca4b3848f3e81784135f023df266b61c2345726ea55a50b3e030dd6872` の CrossOver 26.3 source archive です。

Inventory は upstream 3,143 commits で、automated candidate 2,373、subsystem-sensitive 315、ABI-sensitive 455 です。自動ラベルは review の振り分けにのみ使用し、安全性や採用を証明しません。

`.2` review は二段階です。最初に非 ABI の crash、use-after-free、out-of-bounds、overflow、race、deadlock、null dereference、resource leak を全件確認し、次に Runeon が利用する runtime、network、crypto、COM、device、audio、input、codec、UI、launcher module の correctness commits 134 件を確認しました。

各候補には Release script と同一の forward/reverse 判定を適用し、CrossOver 実装、前提 commit、test 変更、regression surface を人手で確認しました。

## `cx26.3-wine11.0-runeon.2` への採用

`.2` は `.1` の VC14 三 commits、Escape 用に適応した `cfgmgr32` shutdown fix、Runeon CEF process patch を継承し、次の 33 stability backports を追加します。

| 最初の Release | Upstream commit | 領域 | 結果 |
| --- | --- | --- | --- |
| 11.1 | `0706ffc06c18` | WindowsCodecs | 切り詰められた GIF palette の初期化を修正 |
| 11.1 | `e33a8cc4ba7d` | QASF | 未接続 pin を保護し、upstream test を移植 |
| 11.2 | `a0a82c471b09` | QASF | stopped reader callback の use-after-free race を回避 |
| 11.3 | `56a4347acac1` | CoreAudio | zero period を拒否して division by zero を回避 |
| 11.3 | `bb4ef1fbf7d8` | RichEdit | text copy 時の tab leader index を制限 |
| 11.3 | `45190d46646b` | WindowsCodecs | interlaced GIF output buffer bounds を修正 |
| 11.3 | `352d6c94fd2a` | Winsock | duplicated socket handle を正しく閉じ、upstream test を移植 |
| 11.12 | `89b35d3f7079` | WindowsCodecs | full GIF LZW table を保持 |
| 11.14 | `af25fb409cec` | WinINet | empty file URL の out-of-bounds read を防ぎ、upstream tests を移植 |
| 11.1 | `95fa88630ee4` | WindowsCodecs | 64bpp RGBA から 32bpp BGRA への変換を修正 |
| 11.3 | `5f241ebda259` | UCRT | scanf scanset range を修正し、upstream test を移植 |
| 11.3 | `91905171217d` | Advapi32 | ReportEventA failure 時に conversion buffer を解放 |
| 11.4 | `8ca440beead4` | MSVCRT | `_time32` overflow を拒否 |
| 11.4 | `8845229a35a9` | MSVCP140 | native `_Schedule_chore` callback behavior に合わせ、upstream test を移植 |
| 11.4 | `a955417b8f7b` | MSVCR | previous ExceptionPtr object を解放 |
| 11.6 | `9c7d674aa96c` | VBScript | ReDim null SAFEARRAY crash を回避し、upstream test を移植 |
| 11.7 | `1ed184e29eb9` | D3D11 | decoder creation failure 時に video-device interface を解放 |
| 11.7 | `8d1c07977dc1` | Script Runtime | create-folder BSTR length を修正 |
| 11.7 | `8f15e858e311` | Quartz | filter-graph completion event を閉じる |
| 11.8 | `08dbf01aaa36` | NSI proxy | `if_nameindex()` failure を処理 |
| 11.8 | `965a00c4b479` | NSI proxy | zero-sized interface allocation を回避 |
| 11.8 | `4781a7128e53` | IP Helper | interface がない場合の out-of-bounds read を回避 |
| 11.8 | `452d82e997fa` | ADO | provider が update status を返さない場合を処理 |
| 11.10 | `cb3de0f05be4` | WinINet | temporary authorization data を解放 |
| 11.12 | `c293e14307e7` | OleView | reload で tree を解放する前に selection を消去 |
| 11.12 | `fe4f614de6d5` | Regedit | REG_MULTI_SZ scan を value size で制限 |
| 11.13 | `3f4bff72fa8e` | URLMon | cache path character count と terminator space を修正 |
| 11.13 | `632963fc4fbb` | MSHTML | creation 後に Gecko attribute を解放 |
| 11.13 | `3131eba5b352` | MSHTML | detach 時に collection reference を解放 |
| 11.13 | `8e6d4b91001e` | MSHTML | DISPID lookup 後に Gecko attribute を解放 |
| 11.14 | `349d6af5b55b` | Crypt32 | OID info size を検証し、upstream test を移植 |
| 11.14 | `568aca4e2aad` | GDI+ | point-count arithmetic overflow を拒否 |
| 11.1 | `73e02c1e7f54` | CMD | MKLINK path を制限し、CrossOver legacy parser に適応 |

各 patch の正式な path、SHA-256、risk、完全な upstream commit、adaptation、順序は [`patches/manifest.json`](patches/manifest.json) に記録されています。

## CrossOver に同等実装がある、または不具合が該当しない項目

- `00f88cd59bb1`: `wineboot` の macOS null canonical name guard は CrossOver に既にあります。
- `13a82c7468dd`: `joy.cpl` close hang fix は CrossOver に既にあります。
- `12154421e345`: CrossOver の MMDevAPI validator は短い `WAVEFORMATEX` を extensible structure 全体として copy せず、extended field の前に `cbSize` を確認するため ASan defect はありません。
- `e0663e8a283c`: CrossOver SetupAPI は直接 registry query を使用し、問題の wide-buffer allocation path を通りません。
- `eef8e97dd335`: CrossOver Crypt32 は利用前に `cbSize` を検証し、この path で後続 exclusive fields を読みません。

## 明示的な延期項目

- Media Foundation `f34dda8a56d0` は CrossOver にない session control-flow と test prerequisites に依存します。
- WinHTTP `a48b1ff36b61` は CrossOver で異なる stream implementation を使い、upstream commit に in-tree regression test がありません。hunk が適用できるだけでは採用しません。
- SChannel `35b1e7eb9a7e` は source hunk を適用できますが、対応 test prerequisite が不足しています。本 batch では source-only backport を認めません。
- URI parser `8840f1dae9a7` は canonicalization series 全体として評価する必要があります。
- `cfgmgr32` `b39db986571e` は upstream で SetupAPI から移動しており、legacy layout 専用の semantic test が必要です。
- WinMM leak fixes と XInput controller series は順序付き behavioral chain で、実 device reconnect と audio-session probe が必要です。
- 追加の VBScript/MSXML crash fixes は parser/interpreter refactor に依存します。`.2` には独立検証できる ReDim fix のみを含めます。
- OpenGL synchronization と広範な graphics/media 変更は、独立した D3DMetal/DXMT/DXVK regression batch で扱います。
- loader、server、WoW64、winemac、win32u、Unix protocol、D3DMetal interface の ABI-sensitive 変更は policy により除外します。
- WineAndroid、Wayland、X11、PulseAudio、ALSA、OSS、BlueZ 専用変更は Runeon macOS runtime では実行されません。

## 検証と Release 境界

固定 CrossOver source に対する manifest/SHA static check、全 series の順次適用、source marker、二回目適用の fail-closed gate は合格しています。Rosetta x86_64/WoW64 の完全な source build も合格し、backport に含まれる upstream test binaries の compile も完了しました。bare build directory から affected Wine tests を直接実行したところ、seed runtime に含まれる MoltenVK と RPC service 環境がないため test host は Wine initialization で停止し、test case には到達しませんでした。この結果は環境要因であり、test pass にも patch failure にも数えません。

source と compile gate の合格だけでは配布完了を意味しません。Runeon 製品 repository で組み立てた seed 内の affected runtime tests、clean/existing prefix、Steam lifecycle、VC14、Escape shutdown、audio/network、graphics smoke を完了する必要があります。

製品 gate が完了するまで `.2` は source candidate のみです。Production `.0` source entry を置き換えたり、利用者へ提供済みと説明したりしてはいけません。
