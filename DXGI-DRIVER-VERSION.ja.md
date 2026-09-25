# オプションの DXGI ドライバーバージョン修正

[English](DXGI-DRIVER-VERSION.md) | [简体中文](DXGI-DRIVER-VERSION.zh-CN.md) | [日本語](DXGI-DRIVER-VERSION.ja.md)

候補 `.11` の `0105-dxgi-driver-version.patch` は LGPL-2.1-or-later の `runeondxgi.dll` を独自実装します。公開 COM/SetupAPI と Wine の GPU LUID 属性を使用し、ライセンス未提示の外部 shim は取り込みません。

`CheckInterfaceSupport` が成功し、バージョンが -1 の場合だけ、一意な LUID の完全一致を確認し、DXGI `VendorId` に対応する固定 Wine `win32u::driver_vendor_to_version` の値で修正します。その他の値、HRESULT、NULL 出力、曖昧または不明なデバイスは元の動作を維持します。容量制限付きのプロセス単位 LUID/ベンダーキャッシュで列挙の繰り返しを避けます。元の COM オブジェクトを返し、仮想関数表の登録をロックで保護し、モジュールを固定した後、実行権限を維持してポインタを変更します。

**製品ではこの実験的修正をデフォルトで無効にします。** ローカルの永続設定で明示的に有効化できます。無効化すると prefix の元の DXGI に戻り、その後の準備でも再有効化しません。ローカルキーは `DXGIDriverVersionCorrectionEnabled` で、未設定または false は無効です。未公開の否定形キーは読みません。Intel `8086`、AMD `1002`、NVIDIA `10de` にはそれぞれ `35.0.101.6314`、`35.0.21025.1024`、`35.0.15.6094` を使い、不明なベンダーは -1 を維持します。これらは Wine の合成互換バージョンであり実際のドライバーやパッケージの値ではなく、対象ゲームの動作は未検証です。

App は共有 runtime の `dxgi.dll` を置き換えず、指定 prefix にだけ wrapper/provider を配置します。wrapper のエクスポートライブラリ名を `runeondxgi.dll` のまま保ち、Wine に対応する builtin を選択させます。準備前に所有範囲、宛先、必須ソースを検証し、既存の prefix トランザクションで依存 DLL を先に公開します。wineboot の初期化や修復で wrapper が置き換わる場合があるため、明示的な有効化時だけゲーム起動設定の準備前に再適用します。無効時は初回起動前に DLL を書きません。配置済みの wrapper を無効にすると、seed にオプションモジュールがなくても次の Steam 起動で元に戻し、以後の無効状態では再同期しません。新しい指紋を受け取らない同期では同じ runtime の既存指紋を保持し、Steam ウェブキャッシュの誤消去を防ぎます。未公開の runtime 全体への旧実装は保存した原本から復元します。

ユーザーのローカル Apple provider を `rxdg.dll` に改名し、対応するローカル unix 別名を使います。モジュールのアタッチ時に provider をロードし、呼び出し元が D3D デバイスを作る前の元の初期化順序を維持します。ロード失敗時にはパスなしの名前も試し、明確なデバッグメッセージを出します。ただし検証した runtime では prefix の provider 欠落をこの方法で解消できず、安全性をこのフォールバックには依存させません。loader lock 内のロードは Wine の互換制約であり、一般に安全な DllMain の使い方ではありません。T798 は Apple 4.0b2 / M3 Max で順次ファクトリ作成とデバイス優先初期化を確認しましたが、provider がスレッドを開始してローダーを待つとデッドロックの可能性があります。有効化前の対象ゲーム検証には複数スレッドからのデバイス作成を含めます。seed は私有 provider とバックアップを除外し、本リポジトリには Apple の私有バイナリやゲーム別処理を含めません。

`.11` を明示して通常の Wine ビルドを使います。ポリシー検査は `x86_64-w64-mingw32-gcc -std=c11 -I /path/to/patched/wine/dlls/runeondxgi tests/dxgi-driver-version.c -o /tmp/dxgi-policy.exe -lsetupapi -ldxguid -luuid` です。i686 でもビルドし、製品が認可する隔離 Wine プローブで実行します。Intel/AMD/NVIDIA の値、不明なベンダー、LUID/ベンダー別キャッシュ、重複・欠落デバイス、元の戻り値、実行可能ページの保護を検査します。App では無効時の準備/起動/準備の指紋と htmlcache 保持、初回起動前の無操作、実際の指紋変更時のキャッシュ消去を検査します。

ローカル証拠は Runeon T798 に記録します。x64/i386 モジュール、ポリシー検査、App ビルド、prefix 範囲、永続無効化、ロールバック、修正・復元 API 比較が成功しました。ファクトリと EnumAdapterByLuid は修正されます。D3D11 デバイス優先の作成は成功し元の -1 を維持しますが、修正対象としては未対応です。EnumWarpAdapter は元の非対応結果を維持します。provider の初期化遅延による回帰を再現し修正しました。seed は未公開で、ゲーム互換性は保証しません。
