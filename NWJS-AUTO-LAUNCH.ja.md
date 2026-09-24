# NW.js 自動起動の候補

[English](NWJS-AUTO-LAUNCH.md) | [Chinese](NWJS-AUTO-LAUNCH.zh-CN.md) | [Japanese](NWJS-AUTO-LAUNCH.ja.md)

英語文書を正本とし、他の言語はその完全な翻訳です。

候補 `cx26.3-wine11.0-runeon.11` は `.10` を継承し、`0104-steam-nwjs-automatic-launch.patch` を追加します。`RUNEON_WINE_PATCHSET_DEFINITION=patchsets/cx26.3-wine11.0-runeon.11` で選択します。過去の定義と既定のパッチセットは変更しません。

対象は `steam.exe` によるプロセス生成のみです。App のセッションフラグ `RUNEON_NWJS_AUTO_V1=1`、grant、非ゼロの SteamAppId、x64 PE、および隣接する `nw.dll`、`nw_elf.dll`、`package.json` を要求します。元のコマンドを固定の App 管理ヘルパーに渡します。デバッグ起動は変更しません。ゲーム ID やゲームの exe 名では判定しません。

対応する App/ヘルパーはローカル要求を認証し、アカウント、インストール、コマンド、ランタイムのバージョン、保護、コンポーネント、セーブデータ、ユーザー設定を検査します。結果は管理 NW.js の起動、適用対象外と確認した元コマンドの実行、または準備失敗の報告です。この入口はゲームファイルや実行中の Steam 設定を変更しません。旧 App は機能フラグを設定せず、新 App は両アーキテクチャの kernelbase に機能マーカーがあることを確認してから旧案内を非表示にします。

ローカル開発確認では、変更した process 翻訳単位を i686 と x86_64 向けにコンパイルしました。App/ヘルパーのプロトコル確認は管理起動、元コマンドの許可、拒否、タイムアウト、セッション取消を含みます。完全な Wine ビルドや実際の Steam/ゲーム起動を意味しません。対応する runtime と App を既存のリリース手順で一緒にビルド、配布する必要があります。この候補は runtime として未公開です。
