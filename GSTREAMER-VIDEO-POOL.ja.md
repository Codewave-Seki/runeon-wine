# GStreamer ビデオバッファプールのフォールバック

[English](GSTREAMER-VIDEO-POOL.md) | [简体中文](GSTREAMER-VIDEO-POOL.zh-CN.md) | [日本語](GSTREAMER-VIDEO-POOL.ja.md)

候補 `cx26.3-wine11.0-runeon.11` にダウンストリームパッチ `0106-winegstreamer-unfixed-caps-pool.patch` を追加します。

## 原因と実装

`wg_video_buffer_pool_create` は `gst_video_info_from_caps` の戻り値を無視していました。アロケーションクエリがまだ記述できない caps を持つ場合、ゼロのビデオ情報のままサイズ 0 のプールを返し、デコーダーが確保できないバッファを待ち続ける可能性があります。パッチは戻り値を確認し、失敗時はプールを解放して NULL を返します。既存の呼び出し側は NULL を「プールを提供しない」として扱い、GStreamer の既定の割り当てに戻ります。

出典は [dappermint/winecx `9fcbfbe`](https://github.com/dappermint/winecx/commit/9fcbfbe12bd9e3d03787909d9488009dcdba0a90) で、Wine ソースファイルに対する LGPL-2.1-or-later の変更です。11.0 ベースには後の `output_plane_stride` 引数がないため、文脈のみを調整しました。WineHQ の master もこの戻り値を確認していないため、リリースからのバックポートではありません。ゲーム固有の処理は含みません。

## 検証

パッチは固定ベースに適用でき、`scripts/static-check.sh` と `scripts/integration-check.sh` の対象です。winegstreamer の unix ライブラリは完全な Wine ビルドでコンパイルされます。記述できない caps の再現手段はないため、既存経路が変わらないことのみを主張します。製品側の検証では既存の動画経路に回帰がないことを確認してください。特定ゲームの動画が修正されたとは主張しません。
