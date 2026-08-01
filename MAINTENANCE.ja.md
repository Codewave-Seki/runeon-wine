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
