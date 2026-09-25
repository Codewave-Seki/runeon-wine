# GStreamer 视频缓冲池回退

[English](GSTREAMER-VIDEO-POOL.md) | [简体中文](GSTREAMER-VIDEO-POOL.zh-CN.md) | [日本語](GSTREAMER-VIDEO-POOL.ja.md)

候选 `cx26.3-wine11.0-runeon.11` 增加下游补丁 `0106-winegstreamer-unfixed-caps-pool.patch`。

## 原因与实现

`wg_video_buffer_pool_create` 忽略了 `gst_video_info_from_caps` 的返回值。分配查询可能携带尚无法描述的 caps，此时仍使用全零视频信息，并回复一个大小为零的缓冲池，解码器可能一直等待永远无法分配的缓冲。补丁检查返回值，失败时释放缓冲池并返回 NULL；现有调用方已把 NULL 视为「不提供缓冲池」，回落到 GStreamer 默认分配。

来源为 [dappermint/winecx `9fcbfbe`](https://github.com/dappermint/winecx/commit/9fcbfbe12bd9e3d03787909d9488009dcdba0a90)，属于 Wine 源文件的 LGPL-2.1-or-later 改动，仅适配上下文：11.0 基线没有后来的 `output_plane_stride` 参数。WineHQ 主线仍未检查该返回值，因此不是发布版回移。不含任何按游戏特判。

## 验证

补丁可应用于固定基线，由 `scripts/static-check.sh` 与 `scripts/integration-check.sh` 覆盖；winegstreamer unix 库由完整 Wine 构建编译。本地没有无法描述 caps 的复现，因此只主张原有路径不受影响；产品验证应确认现有视频路径无回归。不声称任何游戏的视频已修复。
