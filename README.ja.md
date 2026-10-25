# Runeon Wine

[English](README.md) | [Chinese](README.zh-CN.md) | [Japanese](README.ja.md)

> 英語版が正です。中国語版と日本語版は参考用の完全な翻訳です。

Runeon Wine は、Runeon の Steam Baseline runtime のソースを公開で管理するリポジトリです。固定した CrossOver Wine のベースライン、上流からのバックポート、Runeon のパッチ、そして出荷したすべての runtime の対応ソースを再構築するスクリプトを含みます。一般的な互換性を約束するものではなく、Runeon App、Steam、ゲーム、Apple の非公開グラフィックス コンポーネントは含みません。

## ベースラインと現行版

- CodeWeavers ソース：`crossover-sources-26.3.0.tar.gz`、Wine `11.0`。[`base/crossover-26.3-wine-11.0.json`](base/crossover-26.3-wine-11.0.json) で固定
<!-- release-facts:current-patch-set -->
- 現在の正式ソース：[`cx26.3-wine11.0-runeon.26`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.26)。seed `2026.10.25.2`（App `>=1.14 (16)`）に対応。定義は [`patchsets/cx26.3-wine11.0-runeon.26`](patchsets/cx26.3-wine11.0-runeon.26/manifest.json)
- 上流のレビュー範囲：完全な監査は `wine-11.15` まで、絞り込みスイープは `wine-11.20` まで。[MAINTENANCE.md](MAINTENANCE.md#review-records) を参照

## リポジトリ構成

| パス | 内容 |
|---|---|
| `base/` | 固定したソースアーカイブの URL、SHA-256、ソースルート |
| `patchsets/<id>/` | 1 つのパッチセットの `series` と `manifest.json`。スクリプトは最新のものを使い、`RUNEON_WINE_PATCHSET_DEFINITION=patchsets/<id>` で別のものを選べます。`.2`–`.8` は各タグにのみ存在 |
| `patches/upstream/`、`patches/runeon/` | WineHQ からのバックポートと Runeon のパッチ |
| `release-manifests/` | 公開した各 Release のコミット、Release URL、アセット名、サイズ、SHA-256、状態 |
| `tests/`、`scripts/` | プローブとチェック。取得、適用、検証、バンドル用スクリプト |
| `docs/` | 上流レビューの記録とパッチの設計メモ（英語） |

## 範囲と境界

- Wine ソースツリー、ビルド出力、runtime アーカイブ、prefix、ユーザーデータ、診断情報はコミットしません。
- `D3DMetal.framework`、`libd3dshared.dylib`、GPTK の非公開オーバーレイはダウンロード、ミラー、配布しません。
- きれいに適用できても、上流コミットはレビューを経るまでパッチセットに入りません。`ntdll`、`server`、`wow64`、ローダー、`winemac.drv`、`win32u`、Unix/server プロトコルの変更は ABI に影響しうるため、個別に承認されない限り取り込みません。例外は macOS 固有の動作を修正する上流の修正で、CrossOver が変更していない Wine のコードだけを変え、プロトコルや共有構造体を変えないものは、レビューを経てまとめて取り込みます（`CLAUDE.md` §3 参照）。
- Runeon のパッチは呼び出し側が明示的に有効にするもので、ゲーム名や AppID では判定しません。例外は [MAINTENANCE.md](MAINTENANCE.md#product-patches-without-an-opt-in) に記載しています。
- Production に出荷する runtime には、同一のパッチセットバンドル、完全な対応ソース、SHA-256 ファイルを含む不変の正式版 Release があります。候補はプレリリースのままです。タグと Release アセットは移動、差し替え、削除しません。
- runtime のパッケージ化、署名、配布は Runeon の製品リポジトリが行います。

## 検証とビルド

```bash
scripts/static-check.sh
source_root="$(scripts/fetch-source.sh)"
scripts/integration-check.sh "$source_root"
```

`integration-check.sh` は固定ベースラインの一時コピーにすべてのパッチを順に適用し、検証マーカーを確認し、同じ series を二重に適用できないことを確かめます。Release 用のバンドルをビルドするには：

```bash
export RUNEON_WINE_PATCHSET_DEFINITION=patchsets/cx26.3-wine11.0-runeon.26
source_root="$(scripts/fetch-source.sh)"
scripts/apply-series.sh "$source_root"
scripts/build-source-bundle.sh "$source_root" dist
scripts/build-patchset-bundle.sh dist
```

以前のパッチセットを再構築するには、同じようにその定義を選ぶか、タグをチェックアウトします（`.2`–`.8` はタグのチェックアウトが必要）。ビルド手順の全体は [BUILDING.md](BUILDING.md) にあります。

## リリース履歴

各 Release にはパッチセットバンドル、完全な対応ソース、2 つの SHA-256 ファイルが含まれ、そのパッチセットで追加された内容は Release ノートに記載しています。「配布」は、製品が表の App ビルドにその seed を今も提供していることを示します。

| パッチセット | 状態 | Seed | 説明 |
|---|---|---|---|
| [`.26`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.26) | 正式版、latest | `2026.10.25.2` | 現行版。App `>=1.14 (16)` に配布 |
| [`.25`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.25) | プレリリース | `2026.10.25.1` | Dev 候補。`.26` で置き換え済み、Production では未使用 |
| [`.24`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.24) | 正式版 | `2026.10.25` | 置き換え済み、保持 |
| [`.23`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.23) | 正式版 | `2026.10.24` | 置き換え済み、保持 |
| [`.22`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.22) | 正式版 | `2026.10.20` | App `1.13 (10)`–`1.13 (15)` に配布 |
| [`.21`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.21) | 正式版 | `2026.10.18` | 置き換え済み、保持 |
| [`.20`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.20) | 正式版 | `2026.10.16.1` | 置き換え済み、保持 |
| [`.19`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.19) | プレリリース | `2026.10.16` | Dev 候補。Production では未使用 |
| [`.18`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.18) | 正式版 | `2026.10.14` | App `1.12 (6)`–`1.13 (9)` に配布 |
| [`.17`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.17) | 正式版 | `2026.10.10` | App `1.10 (3)`–`1.11 (4)` に配布 |
| [`.16`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.16) | 正式版 | `2026.10.06` | App `1.10 (0)`–`1.10 (2)` に配布 |
| [`.15`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.15) | 正式版 | `2026.10.04` | 置き換え済み、保持 |
| [`.14`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.14) | 正式版 | `2026.10.02` | 置き換え済み、保持 |
| [`.13`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.13) | 正式版 | `2026.09.28.1`, `2026.09.28` | `2026.09.28.1` を App `1.8 (0)` 以降、`1.10 (0)` より前に配布 |
| [`.12`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.12) | プレリリース | `2026.09.27` | Dev 候補。Production では未使用 |
| [`.11`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.11) | 正式版 | `2026.09.26` | 置き換え済み、保持 |
| [`.10`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.10) | 正式版 | `2026.09.22` | 置き換え済み、保持 |
| [`.9`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.9) | 正式版 | `2026.09.13` | 置き換え済み、保持 |
| [`.8`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.8) | 正式版 | `2026.09.06` | 置き換え済み、保持 |
| [`.7`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.7) | プレリリース | — | **コンパイルできません**。ビルドしないでください（`.8` で修正） |
| [`.6`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.6) | 正式版 | `2026.08.11.1` | 置き換え済み、保持 |
| [`.5`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.5) | 正式版 | `2026.08.03.2` | 置き換え済み、保持 |
| [`.2`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.2)–[`.4`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.4) | プレリリース | — | 過去の候補。未出荷 |
| [`.1`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.1) | プレリリース | — | 最初の候補。未出荷 |
| [`.0`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.0) | 正式版 | `2026.07.22` | 置き換え済み、保持 |

## ドキュメント

- [BUILDING.md](BUILDING.md)：runtime とソースバンドルのビルド
- [MAINTENANCE.md](MAINTENANCE.md)：上流レビュー、パッチの受け入れ条件、opt-in の例外、リリース方針
- [SECURITY.md](SECURITY.md)：セキュリティ問題の報告
- [docs/upstream-reviews.md](docs/upstream-reviews.md) と [docs/patch-notes.md](docs/patch-notes.md)：レビュー記録とパッチの設計メモ

## ライセンス

このリポジトリの Wine と、Wine/CrossOver Wine に由来する変更は LGPL-2.1-or-later でライセンスされています。[`LICENSE`](LICENSE) を参照してください。上流のコミットは元の作者情報を保持します。このリポジトリは Runeon App のライセンスを変更しません。
