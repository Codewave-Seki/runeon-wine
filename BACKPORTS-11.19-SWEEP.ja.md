# Wine 11.19 ユーザーモード修正の一括バックポート

[English](BACKPORTS-11.19-SWEEP.md) | [简体中文](BACKPORTS-11.19-SWEEP.zh-CN.md) | [日本語](BACKPORTS-11.19-SWEEP.ja.md)

> 英語版が正本です。中国語版と日本語版は利便性のための完全な翻訳です。

候補パッチセット `cx26.3-wine11.0-runeon.15` は、`0110` の後に `patches/upstream/0079-wine-11.19-targeted-fixes.patch` を追加します。`wine-11.19` で初めてリリースされた WineHQ の 87 コミットをまとめたもので、各コミットと作者を列挙し、manifest に完全なコミット ID を記録しています。テストのみの変更は含みません。

## 選定

[前回のバックポート](BACKPORTS-11.16-11.18-SWEEP.ja.md)と同じ基準で、`wine-11.18..wine-11.19` の 353 コミットすべてを確認しました。

- 100 コミットは `ntdll`、`server`、`win32u`、`winemac.drv`、`wow64`、`loader`、`configure`、`tools`、`libs` またはその他のホストドライバーを変更します。大半は OpenGL/EGL コンテキストのリファクタリングと、`winemac.drv` のヘッダーおよびログの整理で、いずれも採用しません。その中の小さな修正は個別に確認しました。IPv6 `IPV6_MTU_DISCOVER` のレベル修正（`fc5dd34cc3`）は、macOS には `IPV6_DONTFRAG` があるためビルドされない分岐にあります。EGL pbuffer の戻り値（`802d3d49fb`）は macOS の経路にありません。ゼロのグループアフィニティ（`0998bfa77a`）は、ほとんど使われない呼び出しが失敗するだけです。エイリアス HKL の IME 判定（`a07e981c59`）は `winemac.drv` のキーボード処理を変えるため、別途検証が必要です。`NtReleaseSemaphore` の符号付き化（`f89a7ca541`、`3e182735ea`）はシステムコールのシグネチャを変えます。
- 29 コミットはテストのみの変更です。
- 224 のユーザーモード候補からは、Steam、ランチャー、インストーラー、ゲームが読み込む DLL のクラッシュ、メモリ安全性、リーク、エラー経路の修正を選びました。採用しなかったもの：コントローラーの識別情報（`winebus.sys`、`hidclass.sys`、`xinput1_3`）、`mountmgr.sys` の CD/DVD と SCSI の IOCTL、および機能追加。たとえば D3D12 ベースの Media Foundation バッファー、GDI+ の縦書き、DNS キャッシュと hosts ファイル、VBScript パーサーの変更、`RoResolveNamespace`、D2D1 カラーマネジメントのスタブ、`GetNextAsyncId` です。

きれいに適用できた選定コミットのうち、3 つはレビュー後に除外しました。

| 除外 | 理由 |
|---|---|
| `c9723c90f1` setupapi | `delete_multi_sz_value()` のバッファーを半分にしても `WCHAR` のポインター演算が変わっていないため、コピーが割り当て範囲を超えて書き込みます。このバグは `wine-11.19` 自体にあります。変更はメモリを節約するだけです。 |
| `6880117619` oleaut32 | 10 進数の丸めの意味を変える変更で、クラッシュ修正ではありません。 |
| `5ff02cf188` mfplat | 高さゼロで `MFCalculateImageSize()` が失敗すると、ベースでは成功していたデコーダーの型ネゴシエーションが失敗し、動画再生にリスクがあります。 |

13 コミットはベースが異なるため適用できず、移植していません。`rsaenh` の HMAC 系列はベースにないハッシュ実装を必要とします（単独で適用できた 1 つは、系列の一部だけを取り込まないために取り消しました）。ほかに `secur32` の `770f0b9c7b`、hosts ファイルの後続修正 `8f6cb19433`、ベースとコードが異なる Media Foundation のクロックとセッションの修正、`msado15` の `57cebf5711`、D3DX10 スプライトの修正です。

## 適応

| コミット | 適応内容 |
|---|---|
| `60551e1d7b` msvcrt | 2 つのアラインメント付き割り当ての前にオーバーフロー検査を追加。ベースには、上流のコンテキストに含まれる後年の縮小パディング処理がありません。 |
| `105ced5bc1` msvcrt | 同じ `ARRAY_SIZE` の変更で、周囲の行が異なります。 |
| `53b5345251`、`37fd158bfc` mf | 統合：`Flush` はオーディオクライアントを保持している稼働中のレンダラーだけをリセットします。ベースには position/pts フィールドがありません。 |
| `ee38ff1725` mf | ベースに `intsafe.h` がないため、`DWORD_MAX` の代わりに `~(DWORD)0` と比較します。 |

## 検証

`scripts/static-check.sh` と `scripts/integration-check.sh` は成功し、固定されたソースアーカイブにシリーズを適用すると、レビュー済みのツリーとファイル単位で一致します。変更したすべてのモジュールと、そのソースからビルドされるすべてのモジュール（`ucrtbase`、`msvcr*` と `msvcp*` 系、`d3dcompiler_*`、`d3dx9_*`、`d3dx10_43`、`d3dx11_43`、`comctl32_v6`）は i386 と x86_64 でビルドできます。新たに追加された低レベル呼び出しは `cxx_frame_handler` の `RtlLookupFunctionEntry` だけで、ベースに実装済みです。タグを付ける前に、完全な製品ビルドとランタイムのスモークテストが必要です。

`upstreamAuditThrough` は `wine-11.15` のままです。ベースの manifest には `upstreamSweepThrough: wine-11.19` を記録しています。
