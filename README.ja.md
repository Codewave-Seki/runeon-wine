# Runeon Wine

[English](README.md) | [简体中文](README.zh-CN.md) | [日本語](README.ja.md)

> 英語版が正式な文書です。簡体字中国語版と日本語版は、内容を省略しない翻訳です。

Runeon Wine は、Runeon の Steam Baseline runtime 用に公開されているソース保守リポジトリです。固定された Wine/CrossOver Wine ベースライン、上流からのバックポート、製品固有のパッチ、および再現可能な対応ソースのビルド入口を管理します。本リポジトリは一般的な互換性を保証するものではなく、Runeon App、Steam、ゲーム、Apple の非公開グラフィックスコンポーネントを含みません。

## 現在のベースライン

- CodeWeavers ソース: `crossover-sources-26.3.0.tar.gz`
- Wine ベースライン: `Wine version 11.0`
- 現在の source candidate: `cx26.3-wine11.0-runeon.2`
- 上流の監査済み範囲: `wine-11.14` まで

[`base/crossover-26.3-wine-11.0.json`](base/crossover-26.3-wine-11.0.json) は、ベースライン URL、SHA-256、およびソースルートに関する唯一の機械可読な情報源です。[`series`](series) がパッチの順序を定義し、[`patches/manifest.json`](patches/manifest.json) が出所、リスク、および完全な upstream commit ID を記録します。

## 対象範囲と境界

- 完全な Wine ソースツリー、ビルドディレクトリ、runtime archive、Wine prefix はコミットしません。
- `D3DMetal.framework`、`libd3dshared.dylib`、または GPTK の非公開 PE/Unix overlay をダウンロード、ミラー、配布しません。
- 上流コミットは最初に監査リストへ入ります。clean apply できることだけを理由に active series へ自動追加しません。
- `ntdll`、`server`、`wow64`、`loader`、`winemac.drv`、`win32u`、Unix library/server protocol、または D3DMetal interface の変更は、既定で ABI-sensitive と扱い、個別のベースライン更新または強化された検証を必要とします。
- component packaging、Developer ID 署名、Dev/Production feed、ダウンロード検証、release readiness は、引き続き Runeon 製品リポジトリが担当します。
- 本リポジトリと GitHub Release assets は公開されています。実際に配布される各 Production runtime には、正確な patch-set bundle、完全な対応ソース、および SHA-256 ファイルを含む不変の正式 Release が必要です。未公開候補は Pre-release のままにします。
- `patchsets/cx26.3-wine11.0-runeon.0` は Production seed `2026.07.22` の正確な履歴ソース定義です。`patchsets/cx26.3-wine11.0-runeon.1` は、最初の未公開 Escape 修正候補を固定します。既定の [`series`](series) は、より広い `.2` source candidate を示します。どちらの候補も配布済み runtime ではありません。
- `release-manifests/` は、各公開 bundle の commit、Release URL、ファイル名、size、SHA-256、stable/prerelease 状態、および永続保持ルールを記録します。Production runtime が参照できるのは `stable` manifest のみです。Pre-release であることは、その修正が Production ユーザーへ提供済みであることを意味しません。

## リリース状況

- [`cx26.3-wine11.0-runeon.0`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.0) は、現在の Production seed `2026.07.22` に対応する正式なソース Release です。
- [`cx26.3-wine11.0-runeon.1`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.1) は、Escape の `cfgmgr32` backport を含む次期 runtime 候補です。Dev または Production runtime としてはまだ配布されておらず、Pre-release のままです。
- `cx26.3-wine11.0-runeon.2` は現在の source candidate です。`.1` を継承し、Wine 11.1 から 11.14 までの安定性・正確性修正 33 件を人手でレビューして追加しています。採用、同等実装、延期の判断は [`AUDIT-11.0-11.14.ja.md`](AUDIT-11.0-11.14.ja.md) に記録します。source Release と Runeon runtime の独立 gate が完了するまでは、ユーザーに提供済みとは扱いません。

## クイック検証

```bash
scripts/static-check.sh
source_root="$(scripts/fetch-source.sh)"
scripts/integration-check.sh "$source_root"
```

`integration-check.sh` は固定ベースラインを一時ツリーへコピーし、すべてのパッチを順番に適用して最終 marker を検証し、同じ series が暗黙に二重適用されないことを確認します。キャッシュ内の元ソースは変更しません。

## 対応ソース bundle の生成

```bash
source_root="$(scripts/fetch-source.sh)"
scripts/apply-series.sh "$source_root"
scripts/build-source-bundle.sh "$source_root" dist
```

Release で使用する source bundle は、runtime artifact と同じ base SHA および patch set を使用する必要があります。各 tag の GitHub Release には、source bundle、patch-set bundle、および両方の SHA-256 ファイルを含めます。ビルド、リリース、保守の規則については、[`BUILDING.ja.md`](BUILDING.ja.md) と [`MAINTENANCE.ja.md`](MAINTENANCE.ja.md) を参照してください。

Runeon 製品リポジトリは、より小さい patch-set bundle を使用します。

```bash
scripts/build-patchset-bundle.sh dist
```

この bundle には、固定ベースライン manifest、パッチ、series、検証スクリプト、およびライセンスが含まれます。Runeon のビルドは、完全な Wine と関連ソースの archive を、固定 SHA に基づいて CodeWeavers から取得します。

配布済み seed `2026.07.22` の正確なソースを再構築する場合は、`.0` を明示的に選択してください。`.1` または現在の `.2` candidate で置き換えてはいけません。

```bash
export RUNEON_WINE_PATCHSET_DEFINITION=patchsets/cx26.3-wine11.0-runeon.0
source_root="$(scripts/fetch-source.sh)"
scripts/apply-series.sh "$source_root"
scripts/build-source-bundle.sh "$source_root" dist
scripts/build-patchset-bundle.sh dist
scripts/build-source-archive-manifest.sh \
  dist/runeon-wine-patchset-cx26.3-wine11.0-runeon.0.tar.gz \
  dist/runeon-wine-source-cx26.3-wine11.0-runeon.0.tar.gz \
  dist/cx26.3-wine11.0-runeon.0.source-archive.json
```

## ライセンス

Wine および本リポジトリ内の Wine/CrossOver Wine 由来の変更は、LGPL-2.1-or-later の下でライセンスされます。[`LICENSE`](LICENSE) を参照してください。上流コミットには、元の作者情報と Wine プロジェクトの履歴が保持されます。本リポジトリによって Runeon App のライセンスが変更されることはありません。
