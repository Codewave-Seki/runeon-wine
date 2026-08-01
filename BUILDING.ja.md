# ビルドとソース bundle

[English](BUILDING.md) | [简体中文](BUILDING.zh-CN.md) | [日本語](BUILDING.ja.md)

> 英語版が正式な文書です。簡体字中国語版と日本語版は、内容を省略しない翻訳です。

## 1. 固定ベースラインの取得

```bash
source_root="$(scripts/fetch-source.sh)"
```

スクリプトは manifest に固定された archive SHA-256 のみを受け入れ、`VERSION` を検証します。既定のキャッシュはリポジトリ内の `.work/` に置かれ、Git にはコミットされません。

## 2. Patch series の適用

```bash
scripts/apply-series.sh "$source_root"
scripts/verify-source.sh "$source_root"
```

パッチは [`series`](series) の順序どおりに適用する必要があります。適用済み、部分適用、SHA のずれ、ベースライン不一致は、暗黙にスキップせず失敗させます。

## 3. Runeon macOS WoW64 の構成境界

製品ビルドは、完全な CrossOver 26.3/Wine 11.0 ソースツリーを使用します。ビルド済みの `lib/wine` 内にある個別 DLL の置き換えはサポートされません。主要な構成は次のとおりです。

```bash
"$source_root/configure" \
  --build=x86_64-apple-darwin \
  --host=x86_64-apple-darwin \
  --prefix="$install_root" \
  --enable-archs=i386,x86_64 \
  --disable-tests \
  --with-gstreamer \
  --without-usb \
  --without-pcap \
  --without-cups \
  --without-krb5 \
  --without-gssapi \
  --without-sdl \
  --without-opencl \
  --without-x
```

依存関係の準備、GStreamer、MoltenVK、GnuTLS、Rockstar-scoped D2D wrapper、Apple のユーザーローカル overlay、署名、および runtime component packaging は、引き続き Runeon 製品リポジトリで管理します。

## 4. 対応ソース bundle の生成

```bash
scripts/build-source-bundle.sh "$source_root" dist
```

出力には、パッチ適用済み Wine ソース、base/patch manifest、patch files、[`series`](series)、保守スクリプト、ライセンス、およびビルド手順が含まれます。Release では生成物の SHA-256 を記録し、両方の bundle と各 `.sha256` ファイルを同じ tag の公開 GitHub Release へアップロードし、不変の asset URL を Runeon runtime component metadata と公開 source offer に記録します。

## 5. 製品ビルド用 patch-set bundle の生成

```bash
scripts/build-patchset-bundle.sh dist
```

Runeon 製品リポジトリは、この小さい bundle をダウンロードして検証し、固定 SHA の CrossOver archive に [`series`](series) を適用します。patch-set bundle と完全な corresponding-source bundle は同じ tag から取得する必要があり、別々に再構築したものを混在させてはいけません。

候補 tag は GitHub Pre-release として公開します。同じ不変 tag を正式 Release に変更できるのは、対応する runtime が Dev 検証を完了し、実際に Production へ昇格した後だけです。Production ビルドで branch archive や `latest` URL を使用してはいけません。

patch-set ID と同名の tag がすでに存在する場合、bundle スクリプトはその tag の正確な commit を checkout した状態でのみ再構築を許可します。これにより、その後の `main` 上の文書やスクリプト変更が履歴 asset を暗黙に変更することを防ぎます。継続開発では新しい patch-set ID を作成し、古い tag の移動や古い Release asset の置き換えを行ってはいけません。
