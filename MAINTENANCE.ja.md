# 上流保守ポリシー

[English](MAINTENANCE.md) | [简体中文](MAINTENANCE.zh-CN.md) | [日本語](MAINTENANCE.ja.md)

> 英語版が正式な文書です。簡体字中国語版と日本語版は、内容を省略しない翻訳です。

Runeon Wine は、能動的な上流監査と diagnostics 主導の調査という二つの経路から入力を受け付けます。どちらの経路も、同じ検証ゲートを省略できません。

## 能動的な監査

1. 毎週の workflow で最新の Wine `11.x` tag を確認します。
2. `scripts/audit-upstream.sh --through latest` で `wine-11.0` 以降の commit 一覧を生成します。
3. 自動リスクラベルは review 対象を絞るためだけに使用し、パッチの安全性を保証しません。
4. 各 commit について、CrossOver 26.3 に同等実装があるか、前提となる refactor が必要か、Runeon に具体的な価値があるかを判断します。
5. 出所、依存関係、テスト、ユーザー上の利点、および回帰範囲が明確な修正だけを active series に追加します。

## リスクレベル

- `candidate`: 局所的な DLL 修正であり、引き続き人による review とテストが必要です。
- `subsystem-sensitive`: グラフィックス、メディア、ウィンドウ、入力、ビルドシステム、または複数モジュールにまたがる変更です。
- `abi-sensitive`: `ntdll`、`server`、`wow64`、`loader`、`winemac.drv`、`win32u`、Unix library/server protocol、または D3DMetal interface の変更です。既定では通常の backport 候補にしません。

## 現在の Wine 11.x レビューベースライン

現在の Dev ソース：[`cx26.3-wine11.0-runeon.10`](FLSGETVALUE2.ja.md)（Pre-release）。seed `2026.09.22` と MoltenVK `1.4.2` を使用し、App 配布条件は `>=1.8 (0)` です。独立した `patchsets/` 定義を明示的に選択します。既定の series は `.9` のままです。 検証：Dev 成果物、API 統合、認証付き feed/ticket、および全体ダウンロードの SHA-256 確認は完了しました。Dev App `1.8 (4)` は配布済みで、Production App は `1.8 (3)` のままです。実際の Steam/ゲームと App の操作は、ユーザーによる Dev 実機確認を待っています。

`.9` の過去の配布と検証（現在の Production）：現在の `.9` patch set は `.8` ベースラインと製品パッチを保持します。[Wine 11.16/11.17 の対象限定レビュー](BACKPORTS-11.16-11.17.ja.md)で 13 の上流コミットを含む 10 パッチファイルを追加しますが、完全監査の境界は `wine-11.15` のままです。Dev と Production の seed `2026.09.13` は `.9` と MoltenVK `1.4.2` を使用し、App の配布条件は `1.8 (0)` 以上です。Production は Dev で検証した同一の署名済み archive を使用します。ビルド、API 回帰、署名、readiness、認証付き feed/ticket と全体ダウンロードの検証は完了しています。ユーザーは現在の Xcode Dev ソース版でコンポーネント更新、runtime 準備、Steam 起動を確認しました。配布は `1.8 (0-3)` で検証しましたが、各 build のゲーム、停止と再起動の操作、新規 prefix の受け入れ確認は再実施していません。公開 App インストーラーは `1.8 (3)` のままです。ソース Release は正式版となり、tag、commit、四つの添付 assets の bytes、size、digest は不変です。`.8` / seed `2026.09.06` は履歴として保持します。

`cx26.3-wine11.0-runeon.8` は `patches/runeon/0102-vuplex-accelerated-paint-policy.patch` を含みます。（`.7` は同じパッチの初期形態を含んでおり、**コンパイルできません**。完全なビルドを実行する前に tag が付けられたためです。`static-check.sh` と `integration-check.sh` はパッチが適用できることを検証しますが、成果物がビルドできることは検証しません。したがって製品パッチは tag を付ける前に完全なビルドを通す必要があります。）Wine に与えるのは、Vuplex 3D WebView の CEF ホストを `OnAcceleratedPaint()` から CPU の `OnPaint()` にフォールバックさせる機能だけです。このスイッチは、呼び出し元アプリケーションが `HKCU\Software\Wine\AppDefaults\<イメージ名>\Runeon` の `DisableVuplexAcceleratedPaint` で明示的に有効化した場合にのみ付加されます。Wine 内にゲーム名や Steam AppID は一切現れません。ポリシーは呼び出し元が書き込むため、どのタイトルにフォールバックが必要かは本リポジトリの外に留まります。ホストの判定は、コマンドラインが実際に実行するイメージ名が `.vuplex` で終わり、かつ `--vx-graphics-api=d3d11` を含み `--type=` を含まないことです。したがって Chromium の子プロセスには影響せず、引数のどこかに `.vuplex` を含むパスがあっても誤って発動しません。フォールバックは描画フレームごとに CPU コピーのコストがかかるため、常時有効ではなくアプリケーション単位のオプトインです。

配布済みの `cx26.3-wine11.0-runeon.8` patch set は、`wine-11.15` までのコミット単位レビューに基づきます。完全な記録は[監査レポート](AUDIT-11.0-11.15.ja.md)と、その前提となる [11.0 から 11.14 の監査](AUDIT-11.0-11.14.ja.md)を参照してください。以前に採用した 2 件の upstream backport に加え、局所的な正確性・安定性修正を累計 42 件選定しましたが、upstream の全コミットをそのまま取り込んだものではありません。機能追加、大規模リファクタリング、ABI-sensitive な変更、サブシステム移行、CrossOver 26.3 に同等実装が存在する修正、依存関係または回帰範囲を限定できていない修正は保留しています。

この監査は特定時点の判断記録であり、恒久的な allowlist ではありません。保留したコミットも、依存チェーン、関連テスト、独立 probe、Runeon での影響が確認できれば、将来の新しい immutable patch set に追加できます。一方、監査で採用しただけでは runtime の公開は許可されず、以下の完全 build と製品 smoke gate を引き続き満たす必要があります。

## Diagnostics 主導の調査

実ユーザーのログは、影響と優先度を確認するために使用できます。最初に呼び出しチェーンを特定するか、独立 probe で挙動を再現し、その後で upstream commit と対応付けます。ローカル証拠がない場合は候補としてのみ記録し、オンライン issue を Runeon の根本原因として扱ってはいけません。

## Patch を active series に追加するためのゲート

- 完全な upstream commit SHA、その修正を最初に含む Wine release、および元の作者情報を記録します。
- 固定された CrossOver source SHA に clean apply するか、すべての適応差分を記録します。
- 元の upstream tests を backport するか、移植できない理由を説明します。
- 影響対象モジュールのテストと独立 probe を通過させます。
- 完全な Wine runtime build を完了します。
- Steam CEF、stop/relaunch、D3DMetal overlay、および DXMT/DXVK/D3DMetal smoke を通過させます。
- clean prefix と既存 prefix の両方を検証します。
- source bundle、notices、component metadata、および Runeon 文書を同期します。

## リリースポリシー

Patch set には `cx26.3-wine11.0-runeon.1` のような不変 ID を使用します。Git tag、source bundle、runtime component metadata、および Runeon release 文書は、同じ ID を参照する必要があります。新しい artifact はまず Dev に公開し、readiness、download、product smoke を完了します。Production へ昇格できるのは検証済みの正確な byte だけであり、Production 向けに直接再構築してはいけません。

リポジトリと bundle は公開されています。候補 tag を作成した後、`build-source-archive-manifest.sh` で公開 Release URL、asset SHA/size、および `prerelease` 状態を記録し、patch-set bundle、完全な対応ソース、および二つの `.sha256` ファイルをアップロードします。対応する runtime が実際に Production へ入った後にのみ、同じ不変 Release と manifest を `stable` に変更できます。履歴 tag と assets は永久に保持し、移動、置換、削除してはいけません。

公開リポジトリに含められるのは、LGPL の Wine/CrossOver Wine lineage、パッチ、およびビルドスクリプトだけです。Runeon App、ユーザー diagnostics、Steam/ゲームファイル、Developer ID またはサービス secret、D3DMetal/GPTK/Apple の非公開コンポーネントは、Git 履歴、Actions artifact、Release asset のいずれにも入れてはいけません。公開更新の前には毎回、完全な履歴、Release asset 一覧、および Actions ログを確認します。
