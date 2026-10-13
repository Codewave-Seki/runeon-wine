# Wine 11.1 から 11.19 の互換性スタブのレビュー

[English](BACKPORTS-11.1-11.19-STUBS.md) | [简体中文](BACKPORTS-11.1-11.19-STUBS.zh-CN.md) | [日本語](BACKPORTS-11.1-11.19-STUBS.ja.md)

> 英語版が正本です。中国語版と日本語版は利便性のための完全な翻訳です。

候補パッチセット `cx26.3-wine11.0-runeon.18` は `.17` を起点とし、`patches/upstream/0081` から `0087` を追加します。この範囲の以前のレビューでは、クラッシュ、メモリ安全性、エラーパスの修正を選び、機能追加は対象外としていました。その結果、失敗を返すスタブを成功を返すように変える小さなコミットも対象外になっていました。その失敗を致命的と扱うアプリケーションはそこで停止するため、本レビューではこの種類を別途確認します。

## 選定

`wine-11.0..wine-11.19` の次の 2 群を、パッチ適用済みの `.17` ツリーに対して順方向と逆方向の適用で 1 コミットずつ確認しました。

- 件名がモジュール名に続いて「Stub」で始まる 18 コミット
- 必要としたアプリケーションをメッセージに記載している（「Needed by」「Needed for」）8 コミット

## 採用

| パッチ | 上流コミット | 変更 |
| --- | --- | --- |
| `0081` | `f19a7df7c0`（11.1） | `IUISettings2::get_TextScaleFactor()` が `E_NOTIMPL` ではなく 1.0 を返します。C++/WinRT は `E_NOTIMPL` を例外に変換し、捕捉されなければ起動時にプロセスが終了します。 |
| `0082` | `0b0bef31ea`（11.14） | `IUISettings2::add_TextScaleFactorChanged()` が、同じファイルの他のイベントスタブが既に返しているダミーの cookie を返します。 |
| `0083` | `caba5e68e2`（11.9） | WMI の `Win32_DiskDrive` に `Signature` と `TotalHeads` を追加します。 |
| `0084` | `2873b4d077`（11.5） | 引数付きで呼ばれた名前付き項目オブジェクトについて、VBScript がその既定プロパティを呼び出します（Wine-Bug 53889）。 |
| `0085` | `d30dcd75bc`（11.14） | `PdhGetFormattedCounterArrayA/W` を spec スタブではなく実装します。 |
| `0086` | `fbd19baa4f`、`20e2458ab3`（11.16） | JScript が Unicode の識別子文字を受け付けます。1 つ目のコミットは 2 つ目が依存するリファクタリングです。 |
| `0087` | `22aedfab35`、`3864ce7fcb`、`cefdd9ebe6`、`d95e15dfe8`、`44b0da29ad`、`88ceeada6d`（11.4 から 11.15） | WinHTTP が新しい 4 つのオプションを受け付け、2 つの問い合わせに応答し、`ERROR_WINHTTP_INVALID_OPTION` で失敗しなくなります。 |

`0081` から `0086` は上流のコードを変更せずに取り込み、上流のテスト変更は省いています。`0087` は次のように調整しています。

- セッションオプションは WebSocket バッファーのオプションの後に置きます。ベースには、上流でこれらが続く `WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL` のセッション分岐がないためです。
- `WINHTTP_OPTION_HTTP_VERSION` は `sizeof(HTTP_VERSION_INFO)` で検証し、その長さを報告します。上流（`wine-11.19` でも同じ）はポインターのサイズを使うため、32 ビットの呼び出し元が 4 バイトのバッファーを渡すと 8 バイトが書き込まれます。
- `WINHTTP_OPTION_DECOMPRESSION` と `WINHTTP_OPTION_ENABLE_HTTP2_PLUS_CLIENT_CERT` は、他のブール値オプションと同様に、読み取る前にバッファー長を確認します。上流は長さ 0 で渡された NULL バッファーを読み取ります。
- ベースには WinHTTP の展開機能がありません。このオプションを受け付けても圧縮された応答は要求しないため、応答は非圧縮のままです。

## 不採用

| コミット | 理由 |
| --- | --- |
| `3f65f20cc2` ntdll、`339be36e64`、`94e5bbeb6f`、`2be7717684` win32u、`5b5e95719d` mountmgr.sys | ABI に影響しやすいモジュールで、この種のレビューでは変更しません。 |
| `8571f65844` ucrtbase、`5c281ca2b2` powrprof | ベースに既に含まれています。 |
| `a33fbd48a0` wminet_utils | ベースに `wminet_utils` モジュールがありません。 |
| `043bc5a0d4` windows.web | 利用者の記載がない新しい `JsonArray` クラスで、解析は後の機能コミットで追加されました。 |
| `fff6090795` webservices | リモートデバッガーが必要としたもので、このランタイムが対象とするアプリケーションではありません。 |
| `ccdaa2b581`、`a164854c92` wined3d | Vulkan 検証レイヤーのメッセージとリンク依存を変えるだけです。 |

## 検証

`.18` は `scripts/static-check.sh` と `scripts/integration-check.sh` に合格し、既定の定義も引き続き `scripts/static-check.sh` に合格します。これらの確認はシリーズが適用できることを示すだけです。`.18` はまだビルドしていないため、i386 と x86_64 でのコンパイルは未確認です。
