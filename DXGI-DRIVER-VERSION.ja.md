# オプションの DXGI ドライバーバージョン修正

[English](DXGI-DRIVER-VERSION.md) | [简体中文](DXGI-DRIVER-VERSION.zh-CN.md) | [日本語](DXGI-DRIVER-VERSION.ja.md)

候補 `.11` は `0105-dxgi-driver-version.patch` を含み、LGPL-2.1-or-later の `runeondxgi.dll` を独自実装します。公開 COM/SetupAPI と Wine の GPU LUID デバイスプロパティを使い、ライセンス未提示の外部 shim は取り込みません。

`CheckInterfaceSupport` が成功し、バージョンが -1 の場合だけ修正します。LUID の一意な完全一致と有効な既存の四部形式 `DriverVersion` が必要です。正常値、失敗、NULL 出力、不明なデバイスは元の動作を維持します。ファクトリは元の COM オブジェクトを返します。provider の仮想関数表を別々にロック付きで登録し、ポインタ変更前にモジュールを固定します。固定容量を超える表には介入しません。

App がユーザーのローカル x64 Apple provider と組み合わせるまでは無効です。原本を保存し、コピーのエクスポートライブラリ名だけを `rxdg.dll` に変更、unix 別名を作成してから wrapper をアトミックに有効化します。provider の新規エクスポートには再審査が必要です。seed から私有 provider とバックアップを除外します。本リポジトリにゲーム名別の処理や Apple 私有ファイルは含めません。

`.11` を明示して通常の Wine ビルドを実行します。単独ポリシー検査は `x86_64-w64-mingw32-gcc -std=c11 -I /path/to/patched/wine/dlls/runeondxgi tests/dxgi-driver-version.c -o /tmp/dxgi-policy.exe -lsetupapi -ladvapi32 -ldxguid -luuid` です。i686 でもビルドし、製品が認可する隔離 Wine プローブで実行します。

ローカル状況：Wine のビルドシステムで x64/i386 モジュールを生成しました。x64 モジュールは修正前・修正後・復元後の API 比較、三つのファクトリエントリ、製品同期処理で準備した新規 prefix の検査に成功し、x64/x86 ポリシー検査も成功しました。Wine は改名 provider を自動作成しないため、初回起動前に配置する必要があります。製品インストーラー、私有コンポーネント境界および App ビルドも成功しました。seed は未公開で、特定ゲームの動作を保証しません。完全な runtime と公開ゲートは別途必要です。
