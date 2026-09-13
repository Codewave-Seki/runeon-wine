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

## 6. 対象限定チェックの再現

[tests](tests) ディレクトリは patch-set bundle と完全な対応ソース bundle の両方に含まれます。`source_root` には対象 patch set 適用済みのソースツリーを指定してください。上記の製品構成では Wine の上流テストを無効化しているため、対象の上流 suite には別のテスト有効ビルドが必要です。`configure` と `make` でコンパイラーと依存関係の `PATH` を一致させてください。

`lifetime-check.py` は macOS の Python 標準ライブラリと `clang` を使い、実際の Direct2D/EVR 関数を抽出し、小さな障害注入用依存 stub とともにコンパイル・実行して 51 の所有権 assertion を検証します。生成した一時 C ソースと実行ファイルは自動で削除し、Wine や GPU は起動しません。

`stream-read-check.py` も実際の WIC `stream_read` 関数を抽出し、九つのシナリオで 38 の assertion を実行します。NULL/非 NULL の count ポインター、完全な読み取り、成功した短い読み取りの `S_FALSE` 化、既存の `S_FALSE`、未書き込み count を読まずに `E_FAIL` を維持する動作を検証します。読み取り不可の出力スロットで失敗経路を調べます。依存 stub を使用するもので、Wine や実 codec を起動せず、一時ビルドファイルを自動削除します。

残りのコマンドは MinGW で三つの独立 Windows API probe をコンパイルするもので、実行はしません。

```bash
python3 tests/lifetime-check.py "$source_root"
python3 tests/stream-read-check.py "$source_root"

probe_dir="$(mktemp -d)"
x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Werror \
  -Wno-unused-parameter -O0 tests/graphics-probe.c \
  -o "$probe_dir/graphics-probe.exe" \
  -lsecur32 -ld3d10 -ldwrite -lwindowscodecs -lole32 -luuid
x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Werror \
  -O2 tests/dinput-probe.c -o "$probe_dir/dinput-probe.exe" \
  -ldinput8 -ldxguid -lole32
x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Werror \
  -O0 -municode tests/winhttp-probe.c \
  -o "$probe_dir/winhttp-probe.exe" -lwinhttp
```

グラフィックス probe は state-block mask、adapter の参照所有権、SChannel context、WIC stream、TTC fragment を対象にします。DirectInput probe は異なるデバイス GUID が同じ control ID を共有する二通りの action 順序を調べます。WinHTTP probe は [winhttp-fixture.py](tests/winhttp-fixture.py) を使用します。長い header の応答と `DIRECT` PAC を `127.0.0.1` だけで提供し、外部へリクエストしません。

probe のコンパイル後、`probe_dir` 変数が残っている同じ shell で fixture を起動します。

```bash
fixture_port=8765
python3 tests/winhttp-fixture.py --port "$fixture_port" \
  >"$probe_dir/winhttp-fixture.log" 2>&1 &
fixture_pid=$!
cat "$probe_dir/winhttp-fixture.log"
```

ログに `Wine regression fixture on 127.0.0.1:8765` が表示されてから probe を実行してください。起動中ならログを再確認します。ポートが使用中の場合は別のポートを選び、一貫して指定します。下記の managed launch 経路で、引数 `headers 8765`、続いて `pac 8765` を指定して `winhttp-probe.exe` を実行します（選択したポートに置き換えてください）。前者は長い header の応答を最後まで読み取り、後者は通常と長すぎる PAC hostname を調べます。この手順は API 実行の合格を示すものではありません。

両方の probe が終了したら、上で起動した fixture プロセスだけを停止します。`wait` は要求したシグナルによる終了を報告する場合があります。

```bash
kill -TERM "$fixture_pid"
wait "$fixture_pid" 2>/dev/null || true
```

PE probe は、実際の gatekeeper grant と隔離したテスト prefix を使い、Runeon 製品がサポートする managed launch 経路で実行してください。通常の直接 Wine 起動は managed launch 検証に拒否される場合がありますが、それは DLL テストの失敗を意味しません。この公開リポジトリは非公開 App テストランチャーを含まず、gate を迂回しません。対応する builtin Wine DLL を使い、正確な runtime と prefix を記録して旧版と候補を比較し、軽量な結果を保存した後に一時 probe バイナリーを削除してください。

probe のコンパイル、関数単位の障害注入、Windows API 実行、対象モジュールテスト、製品のグラフィックス/Steam smoke は別々のチェックです。現在の状態は[対象限定レビュー](BACKPORTS-11.16-11.17.ja.md)に記録し、単独のチェックで runtime リリースを許可しません。
