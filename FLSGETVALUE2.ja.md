# FlsGetValue2 候補

[English](FLSGETVALUE2.md) | [简体中文](FLSGETVALUE2.zh-CN.md) | [日本語](FLSGETVALUE2.ja.md)

`cx26.3-wine11.0-runeon.10` は `.9` を保持し、下流パッチを 1 件追加します。不変の正式版/latest Release と対応ソースは Dev と Production の seed `2026.09.22` に使用され、App 配布条件は `>=1.8 (0)` です。Production は同一の署名済み Dev archive を再利用し、tag、commit、四つの assets は不変です。既定のソース定義は過去の `.9` のままで、`.10` を明示的に選択します。

## 原因と実装

配布済みの x86 と x64 の kernelbase には `FlsGetValue2` がなく、このエクスポートを必須とする呼び出し元は解決できません。これは API の不足を示すもので、特定の Runeon ゲームのクラッシュ原因を確定するものではありません。

[修正された下流実装](https://github.com/dappermint/winecx/commit/e0aa380780b73e20fabcfe78fd42713b94929a53) は `RtlFlsGetValue` を使用し、last error を保持します。以前の `FlsGetValue` へのエイリアスでは不十分です。旧 API は成功時に last error をクリアし、失敗時に設定します。パッチ `0103` は kernelbase の実装、kernel32 のインポート、公開宣言を追加し、既存の FLS 割り当て、ストレージ、コールバックを再利用します。ntdll、server、WoW64、グラフィックス ABI は変更しません。WineHQ backport ではなく、下流コードの適応です。

## 検証

`tests/flsgetvalue2-probe.c` は両方の公開エクスポートを解決し、非 null/null 値、無効なインデックス、エラー値の保持、旧 API の動作、スレッド/fiber の分離、終了コールバックを検査します。`x86_64-w64-mingw32-gcc` と `i686-w64-mingw32-gcc` で `-O2 -Wall -Wextra` を指定してコンパイルし、候補 runtime の隔離された新規 prefix で実行します。旧 `.9` はエクスポート不足で失敗し、`--legacy-getter` はエラー値のアサーションで失敗する必要があります。これによりエイリアスでは修正できないことを検証します。

ビルド前に `scripts/static-check.sh` と `scripts/integration-check.sh <未適用ソース>` を実行します。完全な runtime ビルドと managed launch grant harness は製品リポジトリが管理します。ローカルの証拠は製品タスク T784 に記録します。本文はリリース受け入れ完了を意味しません。

2026-09-22 のローカル検証：製品の pipe2 設定修正後、完全な Wine ビルドが成功しました。x86/x64 は新規 prefix と、古いテスト prefix のコピーを候補で更新した環境の両方で API 検査に合格しました。旧 runtime は両エクスポートの不足を再現し、旧 getter の負対照はすべて予期どおり失敗しました。

ネイティブ Windows との同等性および影響を受けるゲームは独立に検証していません。ユーザーは 2026-09-23 に Dev で問題が見つからなかったと報告し、Production を承認しました。この包括的な報告は、すべての Steam/CEF またはゲームの場面を個別に実測したことを意味しません。パッケージ、readiness、配布の証拠は製品リポジトリに記録します。ゲーム専用の例外や新しい互換性の約束は追加しません。製品ビルドでは macOS の pipe2 機能設定を保持する必要があります。SDK 27 は古い対応システムに存在しないシンボルを弱リンクできるため、Windows DLL の読み込み前に失敗する可能性があります。

```sh
export RUNEON_WINE_PATCHSET_DEFINITION=patchsets/cx26.3-wine11.0-runeon.10
scripts/static-check.sh
scripts/integration-check.sh /absolute/path/to/unpatched/wine
```
