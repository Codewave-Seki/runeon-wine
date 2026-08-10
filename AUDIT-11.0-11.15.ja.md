# Wine 11.0 から 11.15 までの安定性レビュー

[English](AUDIT-11.0-11.15.md) | [Chinese](AUDIT-11.0-11.15.zh-CN.md) | [Japanese](AUDIT-11.0-11.15.ja.md)

> 英語文書が正式版です。本書は完全な日本語訳です。

## 範囲

固定比較範囲は `wine-11.0..wine-11.15`、基準は SHA-256 `ac99c8ca4b3848f3e81784135f023df266b61c2345726ea55a50b3e030dd6872` の CrossOver 26.3 source archive です。

Inventory は upstream 3,350 commits で、automated candidate 2,520、subsystem-sensitive 340、ABI-sensitive 490 です。自動ラベルは review の振り分けにのみ使用し、安全性や採用を証明しません。

`wine-11.0..wine-11.14` の判断は変更なく、[`AUDIT-11.0-11.14.ja.md`](AUDIT-11.0-11.14.ja.md) に記録されています。本書は `wine-11.14..wine-11.15` の増分回を追加します。この回は `wine-11.15` が 2026-08-08 に公開された際、定期 upstream watch が検出したものです。

## 増分範囲

`wine-11.14..wine-11.15` は 207 commits で、automated candidate 147、subsystem-sensitive 25、ABI-sensitive 35 です。うち 39 件は test file のみを変更します。

すべての commit に Release script と同一の forward/reverse 判定を適用しました。forward が clean であれば基準側が未修正のコードのままで CrossOver に等価実装がないことを示し、reverse が clean であれば修正が既に存在することを示します。衝突した commit は判断前に CrossOver の実装を読み合わせました。

## `cx26.3-wine11.0-runeon.6` への採用

`.6` は `.5` の全 patch を継承し、upstream 12 commits をまとめた 9 件の backport を追加します。

| 初出 release | Upstream commit | 領域 | 結果 |
| --- | --- | --- | --- |
| 11.15 | `702e44767a8e` | Win32u | US keyboard table が `vsc_to_vk` の末尾を 1 つ超えて scan code を報告する問題を修正 |
| 11.15 | `bb33d1d34204` | MFReadWrite | transform 交渉中に呼び出し側の media type を変更しないようにし、upstream test を同梱 |
| 11.15 | `9a26041b38d8` | MFReadWrite | 下流に別の transform がある場合でも decoder から複数 sample を取り出せるようにする |
| 11.15 | `0e3bbc87cf61` | Video Resizer | DMO が実際に対応する media type を登録 |
| 11.15 | `cd0d7910b23b` | SChannel | server 証明書の hash を bcrypt で計算し ECDSA に対応、upstream test を同梱 |
| 11.15 | `1fc89cc934e9` | GDI+ | `convert_pixels()` の行あたり byte 数の切り捨てを修正、upstream test を同梱 |
| 11.15 | `27ee4f5db28c`、`2f66b17d51de` | ComCtl32、User32 | 両 listbox 実装で範囲外の `LB_SETTOPINDEX` に `LB_ERR` を返す |
| 11.15 | `d8186e465ad4` | CMD | `node_builder_parse` の loop flag を読み取り前に初期化 |
| 11.15 | `d4b32af24629` | Crypt32 | PFX import 時に外側 DER SEQUENCE 以降の byte を無視、upstream test を同梱 |

各 patch の正式な path、SHA-256、risk、完全な commit ID、順序は [`patches/manifest.json`](patches/manifest.json) にあります。

### Test の移植

9 件中 7 件が upstream の test 変更を含み、うち 3 件は upstream の前提 test commit も必要でした。

- listbox patch は `ab21fbc49a25` と `dd7f58d774`、すなわち両 test suite に `test_LB_SETTOPINDEX` を導入する commit を追加で同梱します。基準側にはこの test 自体が存在せず、`todo_wine` を外す対象がないためです。両前提 commit は同一 release 範囲内にあり clean に適用できます。
- Crypt32 test は基準 test file の構成に合わせて書き直しました。`test_PFXImportCertStore_trailing_zeros()` は、基準に存在しない `_sha256_signing` 版の後ではなく `test_PFXImportCertStore()` の後に挿入し、`START_TEST` の対応位置に登録します。test 本体は変更していません。
- Win32u、MFReadWrite `9a26041b38d8`、Video Resizer、CMD の 4 commit は upstream 側にも test がありません。いずれも 1〜2 行の不具合修正で、どちらの revision でも in-tree の test coverage がない箇所です。

### 適応

upstream の原文から逸脱するのは 3 件です。

- `bb33d1d34204`：当該 helper は基準側で `update_media_type_from_upstream()`、upstream では `update_media_type()` です。適応はこの改名のみで、制御フロー、media type の複製とその解放はすべて upstream のものです。
- `d4b32af24629`：`open_cert_store()` には 11.0 以降 `pgnutls_pkcs12_verify_mac()` が追加されたため hunk の文脈が異なります。新規 helper `pfx_der_outer_length()` は `SONAME_LIBGNUTLS` block 内へそのまま移植し、2 行の guard は `pgnutls_pkcs12_import()` に対して同じ位置に配置しました。
- `cd0d7910b23b`：upstream の commit は `dlls/secur32/Makefile.in` を変更しません。upstream の secur32 は mTLS 対応の `f4a9724c89` が追加した `ncrypt` delay import 経由で既に `BCryptHash` に到達できるためです。基準側にはその import がなく、PE link が `undefined reference to BCryptHash` で失敗します。無関係な mTLS commit を持ち込まないため、本 patch では `DELAYIMPORTS` に `bcrypt` を直接追加しました。build graph のみの適応で、C コードはすべて upstream のものです。

### ABI-sensitive の例外

`702e44767a8e` は `dlls/win32u` を変更するため、risk 方針では ABI-sensitive として既定で除外されます。今回は記録付きの例外として採用します。変更は静的 keyboard table の初期化子 1 つ、`ARRAY_SIZE(vsc_to_vk)` を `ARRAY_SIZE(vsc_to_vk) - 1` にするだけで、構造体 layout も interface も Unix 境界も変えず、ASan が検出した out-of-bounds read を修正します。本範囲の他の ABI-sensitive commit は採用していません。

## CrossOver の等価実装、または該当しない不具合

- `0fde54a3574a`（wined3d frame latency）：CrossOver 26.3 は upstream の `frame_latency_semaphore` 経路を使わず、`dlls/wined3d/cs.c` に独自の `pending_presents` / `waiting_for_present` / `present_event` / `max_frame_latency` 実装を持ち、待機の前後で既に wined3d mutex を解放・再取得しています。upstream commit が解消する multi-thread D3D9 の停滞は本基準には存在しません。
- `852e39a89c0d`（host window state の lock）：基準側で `pGetWindowStateUpdates` を実装するのは `winex11.drv` のみです。`winemac.drv` に該当 callback はなく、fix が前提とする unlock 手順も X11 driver 側にあるため、macOS 経路でこの race は発生しません。
- `95e5d76a1882` と `0aa733c17aff`（wined3d planar CPU blits）：Vulkan H.264 decoder chain の一部で、Runeon の video decode は D3DMetal と DXVK を通るため実行されません。
- `bfe591cbef5b`（WindowsCodecs の境界検査）：削除対象の検査は同一 release 範囲の `2e68942fce46` が追加したもので、本基準には届いていません。2 件を合わせると本環境では無効果です。
- 本範囲の `winebus.sys` 4 commits は `hidraw`、`inotify`、`evdev` の経路を対象とします。macOS の controller 列挙は IOHID を通るため Runeon では実行されません。
- `winex11.drv`、`winewayland.drv`、glibc `sa_restorer`、ARM64EC / ARM WoW64 の commit は macOS の x86_64 および arm64 target では動作しません。

## 延期（黙って落としていません）

- UCRT `b0150eda77d7`（`exp(NAN)`）：source 修正は 3 行ですが、唯一の regression test には `8bed5ce602` が必要で、この commit は無関係な `test_cexp` と `test_expf` の assertion も書き換えます。それらの結果は backport しない 11.x の数学修正に依存します。前回の `35b1e7eb9a7e` と同じ判断により、source のみの backport は受け入れません。
- KernelBase `5e5fea8b9117`（PEB `BeingDebugged` flag ではなく debug port を参照）：unhandled exception 経路の挙動変更です。anti-tamper 目的で `BeingDebugged` を操作する title は現在と異なる分岐に入ります。source review ではなく製品側の実証が必要です。
- Win32u `3ec143785905`（detached source の DPI、Wine-Bug 59970）：upstream は 11.0 以降 monitor DPI helper を分数比に変更したため、2 箇所の `DISPLAY_DEVICE_ATTACHED_TO_DESKTOP` 判定を ABI-sensitive な display code 内で手作業で書き直す必要があり、test も付属しません。
- MFReadWrite `4534f6cd0471`（1:1 pixel aspect ratio）：不具合修正ではなく新しい出力挙動の追加であり、anamorphic 補正を必要とする Runeon title は確認されていません。
- Win32u `a1bae27f21b5`（raw mouse input、Wine-Bug 59986）：11.0 以降に導入された raw input の一括処理機構の regression 修正です。基準に `info->raw_mouse` 経路がないため、その refactor を先に移植しない限り修正対象がありません。
- WinHTTP `f27d14a21cfe` と `93ef2498b391`：いずれも `WINHTTP_OPTION_SERVER_CERT_CHAIN_CONTEXT` の修正ですが、この option は基準に存在しません。
- Win32u `a37867ddf841`（window 生成 request への初期 monitor DPI 受け渡し）：`server/protocol.def` と `include/wine/server_protocol.h` を変更します。
- `server` の `init()` object operation 24 commits：ABI-sensitive な refactor として、一連の chain のまま延期します。
- MSXML3、WMI .NET utilities、WinRT metadata、JScript、msv1_0 / sspicli / secur32 の NTLM および auth identity 系列、DNS-SD stub、D2D1 sprite batch、D3DX9 交差判定、bcrypt HKDF と TLS1 KDF、Shell32 の CSIDL 対応、conhost beep などの機能追加。
- header、build tool、symcrypt の alignment、翻訳、debugger の変更は本 patch set にとって runtime 上の価値がありません。

## 検証と Release 境界

静的な manifest と SHA-256 の検査は通過しました。46 patch の完全な series は固定された CrossOver source へ順次適用でき、source marker が書き込まれ、二度目の適用に対する fail-closed gate も成立します。

patch 適用後の tree に対し、test を有効にした native arm64 build を実施しました。変更したすべての source file と移植したすべての test file が compile を通り、`SONAME_LIBGNUTLS` は `libgnutls.30.dylib` に解決されたため、patch 0040 と 0044 は無効ではなく実際に有効な経路です。上記の `bcrypt` delay import の欠落も、この build が検出したものです。

この build における 2 件の失敗は環境要因で、本 patch set とは無関係です。`dlls/winemac.drv/cocoa_window.m` は製品側の D3DMetal 構成なしでは `WineMetalLayer` を参照できず、`dlls/win32u/vulkan.c` は MoltenVK 導入由来の `SONAME_LIBVULKAN` を必要とします。series 内のどの patch もこの 2 file には触れていません。

未了の検証項目が 1 つ残ります。`.6` を進める前に、文書化された製品構成（D3DMetal と MoltenVK の依存を含む）での Rosetta x86_64/WoW64 完全 build を build gate で実施する必要があります。

source gate の通過だけでは出荷になりません。組み立てた seed 上での該当 runtime test、clean および既存 prefix の probe、Steam CEF、stop/relaunch、D3DMetal overlay、DXMT/DXVK の smoke は、引き続き Runeon 製品 repository 側で通過させる必要があります。

これらの製品 gate が通るまで `.6` は source candidate に過ぎず、Production の source 項目を置き換えてはならず、利用者に提供可能と説明してもなりません。
