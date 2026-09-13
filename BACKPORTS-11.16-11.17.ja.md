# Wine 11.16/11.17 の対象限定バックポート

[English](BACKPORTS-11.16-11.17.md) | [简体中文](BACKPORTS-11.16-11.17.zh-CN.md) | [日本語](BACKPORTS-11.16-11.17.ja.md)

> 英語版が正式な文書です。簡体字中国語版と日本語版は、内容を省略しない翻訳です。

レビュー日: 2026-09-13。`cx26.3-wine11.0-runeon.9` は Dev seed `2026.09.13` が使用する公開 Pre-release で、`.8` と同じ固定 CrossOver 26.3 / Wine 11.0 archive に基づきます。既存の 47 パッチファイルを保持し、13 の上流コミットを含む 10 ファイルを追加します。合計は 57 ファイルで、54 の upstream backport と変更していない三つの製品パッチからなります。この増分は ABI-sensitive なコードや製品ポリシーを変更しません。

今回は選定した Wine 11.16/11.17 変更の対象限定レビューであり、11.15 以降の 628 コミットすべての完全監査ではありません。完全監査の境界は `wine-11.15` のままです。upstream-watch workflow の通知を消すために `upstreamAuditThrough` を進めたり、過去の監査を書き換えたりしません。配布済み runtime の状態は引き続き [README](README.ja.md) を参照してください。

## 候補の内容

既存のパッチ適用済みベースラインで CrossOver の同等実装と前提依存を確認しました。以下の利点は修正対象の API 契約を示すもので、特定の Runeon ゲームの問題を再現・解決したという主張ではありません。対応する Wine DLL が使われる場合だけ適用されます。特に D3D10 の変更は公開 Wine bridge が対象で、D3DMetal や DXMT は変更しません。

| パッチ | 最初の Wine tag / 上流コミット | 動作 |
|---|---|---|
| [0045](patches/upstream/0045-wine-11.16-dinput-action-device-guid.patch) | `wine-11.16`: [`2a43c4b92910`](https://github.com/wine-mirror/wine/commit/2a43c4b9291083f8892f2100051131a87278e656), [`72ffda26fa71`](https://github.com/wine-mirror/wine/commit/72ffda26fa71d42ba01e6908f39f5972b238e1c0) | デバイス GUID とオブジェクト ID の両方で action app data を照合し、同じ ID を持つキーボードとマウスの action が誤った値を共有することを防ぎます。 |
| [0053](patches/upstream/0053-wine-11.16-d3d10-stateblock-disabled-byte-fields.patch) | `wine-11.16`: [`2d98b4fb0ae4`](https://github.com/wine-mirror/wine/commit/2d98b4fb0ae49aa22c735eda6f221da91b4bc3cb), [`a5c665c30fe3`](https://github.com/wine-mirror/wine/commit/a5c665c30fe3d1ba6748680e68e77a564d58a9d4) | Capture/Apply が単一バイトの state-block mask 各フィールドの最下位ビットを調べ、無効化した状態を維持します。D3D feature level の追加ではありません。 |
| [0054](patches/upstream/0054-wine-11.17-winhttp-connection-query-buffer-length.patch) | `wine-11.17`: [`df71145f22f1`](https://github.com/wine-mirror/wine/commit/df71145f22f15e723697675e3beaf75094247c6c) | Connection と Proxy-Connection の照会に個別の buffer 容量を使い、最初の長すぎるヘッダーが次の照会の容量を拡大することを防ぎます。 |
| [0055](patches/upstream/0055-wine-11.17-winhttp-pac-scheme-host-bounds.patch) | `wine-11.17`: [`03f4957cacee`](https://github.com/wine-mirror/wine/commit/03f4957caceeb8404a119f6b8bd8cc0e76dcb76e) | PAC の小文字 hostname buffer を制限し、scheme buffer も保護します。このベースラインは長すぎる scheme を前段で拒否するため、実際に到達可能な修正対象は hostname のコピーです。 |
| [0056](patches/upstream/0056-wine-11.17-secur32-schannel-invalid-context.patch) | `wine-11.17`: [`081d8b1dd9ff`](https://github.com/wine-mirror/wine/commit/081d8b1dd9ff4bdd067c54bb50d396e34c334c70) | SChannel の暗号化・復号前に、存在しない、または型が異なる context に SEC_E_INVALID_HANDLE を返します。任意の provider ポインター破損や並行削除は解決しません。 |
| [0057](patches/upstream/0057-wine-11.17-windowscodecs-nonnull-stream-read-count.patch) | `wine-11.17`: [`111e5197390a`](https://github.com/wine-mirror/wine/commit/111e5197390aa008789b002222024229fa2b82cf) | IStream に常に非 NULL の読み取りバイト数ポインターを渡し、成功した短い読み取りを S_FALSE に統一します。JPEG メタデータ経路と共有 codec helper が対象です。 |
| [0058](patches/upstream/0058-wine-11.17-dwrite-ttc-header-read-size.patch) | `wine-11.17`: [`a6fc12e4a94b`](https://github.com/wine-mirror/wine/commit/a6fc12e4a94bf4dae2d5c3a297794107627dad0a) | DirectWrite font stream にポインターサイズではなく完全な TTC header を要求します。fragment 境界の修正であり、文字組版は変えません。 |
| [0059](patches/upstream/0059-wine-11.17-d3d10-device-adapter-failure-reference.patch) | `wine-11.17`: [`5ae631904a4b`](https://github.com/wine-mirror/wine/commit/5ae631904a4b4a253e22408e5cfe107c8ebb33b9), [`0e2ee8ab3e8c`](https://github.com/wine-mirror/wine/commit/0e2ee8ab3e8c72dfcd798ba6d963b6f34625989d) | D3D10 と D3D10.1 のデバイス作成で、GetParent が成功した後にだけ呼び出し元 adapter の参照を取得し、失敗時のリークを防ぎます。 |
| [0060](patches/upstream/0060-wine-11.17-d2d1-geometry-fill-brush-lifetime.patch) | `wine-11.17`: [`4334512575c7`](https://github.com/wine-mirror/wine/commit/4334512575c7be581ae9a2bd9d9d0d11e5038542) | opacity brush の作成に失敗しても、最初の複製 brush の所有権を command list に残し、list の破棄時に一度だけ解放します。 |
| [0061](patches/upstream/0061-wine-11.17-evr-video-window-swapchain-lifetime.patch) | `wine-11.17`: [`5d711b3bdc45`](https://github.com/wine-mirror/wine/commit/5d711b3bdc455addd7a720b553600abc869937be) | EVR のビデオウィンドウ変更時に解放済み swapchain ポインターをクリアし、再作成失敗後にダングリング所有者を残しません。同じウィンドウへの再試行動作は変更しません。 |

## 依存関係と適応

- `0045` は DirectInput の元のテスト前提と修正を、逆適用も検証できる一つの最終 diff にまとめます。元の assertion と作者情報を維持し、製品コードの適応はありません。
- `0053` も元の D3D10 テストを含みます。追加のテスト適応は、デバイス作成に失敗した場合の明示的な skip のみで、NULL デバイスの参照を防ぎます。デバイステストの skip はグラフィックス検証の合格ではありません。製品修正は全 14 個の単一バイトフィールドを対象にします。
- `0059` は独立した D3D10 と D3D10.1 の adapter 修正をまとめます。選定した backport に製品側の意味変更を伴う適応はありません。それ以外の追加上流コミットには元の回帰テストがありません。以下の独立チェックで個別の契約を検証し、対象モジュールと製品のテストも引き続き必要です。
- 完全な commit ID、パッチ digest、最初の Wine release は [manifest](patches/manifest.json) を正とします。ソース bundle はパッチ、原作者情報、[テストソース](tests) を保持します。

## 詳細レビュー後の保留

| グループ / 上流コミット | 保留理由 |
|---|---|
| CoreAudio 周期: `ddefc3d4569f`、`07c7fcc86fe5`、`8e09602ca6e4`、`da0b08472b4b` | 実際の共有デバイスの buffer サイズを変更し、プロパティ設定が失敗すると stream 作成も失敗します。実デバイス周期、読み取り専用・非対応設定、異なる周期の並行 stream、デバイス切替の検証が必要です。元のテストは許容する既定周期値を一つ追加するだけで、zero-fuzz 適用ではこれらのリスクを検証できません。 |
| WGI 初期化: `b45d928aa12f` | provider 列挙前の通知は GamepadAdded の早期登録を可能にしますが、順序を保証しません。初期 gamepad 一覧の完成前に activation が戻り得ます。接続済みコントローラーと hotplug のチェックがなく、既存のイベント登録テストではこの時序を証明できません。 |
| XAudio2 失敗時 unlock: `b68ea3b878e2` | 固定された内蔵 FAudio_CreateSourceVoice は成功値しか返さず、このビルドでは対象の HRESULT 失敗分岐に通常到達しません。上流の unlock 後に解放する順序には、voice 再利用時に effect_chain の競合もあり得ます。FAudio のエラー動作が変わった時点で、実際の失敗 probe とともに再評価します。 |
| MF drain: `862453b70420` | 末端出力を一つずつ drain する継続処理は source_reader_get_read_result にありますが、SOURCE_READER_ASYNC_SAMPLE_READY は応答を直接取り出します。EOS 時に待機済みの非同期 read が唯一の queued sample を消費すると、残りのデコーダーフレームが取り残され、次の read が EOS を返し得ます。これはソース経路の反例で、runtime 再現ではありません。制御された複数フレームの drain テストが必要です。下記の async Release 変更への依存はありません。 |
| 未リリースの MF lifecycle/seek: `2f73d9efc2a0`、`5fc7c7958be1`、`7edd71a8d76c` | async command の初期 refcount がゼロのまま Release を減算に変えると、queue AddRef 後の呼び出し元 Release が queued object を解放し得ます。初期所有権、queue 投入失敗、read/seek/flush の寿命を一体で修正する必要があります。SEEKING の早期設定も失敗時 cleanup のレビューが必要で、本候補には追加しません。 |

これらは上流修正が一般的に無効だという判断ではなく、固定実装と本候補の検証証拠に基づく判断です。より広範な macOS window driver/ABI の変更や Wine 全体の更新は、この増分の対象外です。

## 検証状態

| チェック | 現在の結果 |
|---|---|
| 固定ソースへの適用と保持 | 固定 archive から全 57 パッチファイルの zero-fuzz 適用に合格し、11,104 ソースファイルの比較が一致しました。従来の 47 パッチファイルと manifest 項目は変更していません。 |
| 完全な x86_64/WoW64 ビルド | `configure`、`make`、`make install` に合格しました。 |
| 元の上流テストのコンパイル | DirectInput と D3D10 のテスト実行ファイルが x64、x86 ともにコンパイルでき、合計四つです。完全な上流 suite の実行は未完了です。 |
| 独立 Windows API 実行 | 隔離 prefix で x64 と x86 の両方が合格しました。SChannel 11、adapter 6、stateblock 32、DirectWrite 6、WIC 11、DirectInput 4 の assertion で、各アーキテクチャ合計 70 です。WIC は実際の APP1/Exif 値を問い合わせ、二つの四バイト metadata 読み取りに到達します。 |
| 旧 DLL の API 対照 | 同じ 70 チェックを `.8` DLL に適用すると 19 の失敗を報告します。内訳は SChannel 6、adapter 2、stateblock 4、DirectWrite 2、WIC 3、DirectInput 2 です。候補と同じ core・依存関係を持つ別 runtime で八つの DLL を置き換えた対照であり、旧製品全体の smoke テストではありません。 |
| WinHTTP 実行 | 候補の x64 と x86 で `headers`、`pac` 両モードが合格しました。Header は A が 100 文字、B が 40 文字で、その後の body と EOF も期待どおりです。通常の PAC hostname は DIRECT を返し、1,093 文字の hostname は拒否されます。Fixture リクエストは loopback で確認し、外部通信はありません。 |
| 旧 DLL の WinHTTP 対照 | `.8` DLL は過長 header と PAC の対象入力に到達した後、stack corruption の page fault を再現します。Header 対照は先に長い header を確認し、PAC 対照は先に通常 hostname を通過します。両方とも fault/debugger 処理へ入り、25 秒の監視 timeout と隔離 server/debugger の cleanup が必要でした。通常の assertion 失敗や通常の crash 終了コードとしては記録しません。候補ではこれらの probe の障害が解消しましたが、ゲーム単位の修正証明ではありません。 |
| Direct2D/EVR 所有権の障害注入 | [lifetime-check.py](tests/lifetime-check.py) が二つの実関数を依存 stub とともにコンパイル・実行し、候補は 51 チェックに合格しました。`.8` は六つの想定失敗と、安全に検出した二つの重複解放を報告します。作成/置換失敗、回復、成功、破棄を含む関数単位の検証であり、Wine や GPU の実行ではありません。 |
| WIC 読み取り結果の障害注入 | [stream-read-check.py](tests/stream-read-check.py) は九つのシナリオで 38 assertion を実行し、候補は合格、`.8` は 12 の想定失敗です。NULL/非 NULL count、完全/短い読み取り、S_FALSE 正規化、未書き込み count を読まずに E_FAIL を保つ動作を検証します。関数単位の検証で、実 codec の実行ではありません。 |
| 新しい依存関係での再検証 | Open Wine `.9` は既存の GnuTLS/GStreamer 依存バージョンを維持し、MoltenVK `1.4.2` で完全に再ビルドしました。同じ各アーキテクチャ 70 の API assertion と、x64/x86 の headers/PAC 全四チェックに合格しました。追加の PAC trace では、一度のダウンロード後に script cache を再利用し、長い hostname の経路へ到達することを確認しました。ゲーム互換性や MoltenVK のグラフィックス機能を証明するチェックではありません。 |
| 完全な対象モジュール suite と製品回帰 | 完全な suite 実行、製品の既存 prefix チェック、Steam CEF/停止・再起動、D3DMetal/DXMT/DXVK のゲーム smoke は未完了です。隔離した API prefix の結果では代替できません。 |
| ソース assets と runtime 配布 | `.9` の公開 Pre-release と四つの不変の assets は公開済みです。[ソース manifest](release-manifests/cx26.3-wine11.0-runeon.9.source-archive.json) に commit、archive の digest とサイズを記録します。Dev seed `2026.09.13` は `.9` と MoltenVK `1.4.2` を組み合わせ、seed のビルド、署名、readiness、feed/ticket/download 検証が完了しています。ユーザーによる Steam/ゲームの実機受け入れ確認は未実施で、現在の Runeon App ソースからビルドする Dev `1.8 (3)` を使用する予定です。既存の公開 App インストーラーは変更しません。`.9` は Production へ昇格しておらず、正式版にも変更していません。Production は引き続き `.8` / seed `2026.09.06` です。 |

再現可能なチェックコマンドと managed runtime の起動境界は [BUILDING](BUILDING.ja.md) を参照してください。上流テストのコンパイルは実行を意味せず、API と関数単位の合格は [MAINTENANCE](MAINTENANCE.ja.md) の未完了の製品リリースゲートを代替しません。対応する証拠を得た後にのみ状態を更新します。
