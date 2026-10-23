# Runeon Wine

[English](README.md) | [Chinese](README.zh-CN.md) | [Japanese](README.ja.md)

> 英文版为准，中文与日文为完整译本，仅供参考。

Runeon Wine 是 Runeon Steam Baseline runtime 的公开源码维护仓库，包含固定的 CrossOver Wine 基线、上游回移、Runeon 补丁，以及能为每个已发布 runtime 重建完全对应源码的脚本。本仓库不作通用兼容承诺，也不包含 Runeon App、Steam、游戏或 Apple 私有图形组件。

## 基线与当前版本

- CodeWeavers 源码：`crossover-sources-26.3.0.tar.gz`，Wine `11.0`，固定于 [`base/crossover-26.3-wine-11.0.json`](base/crossover-26.3-wine-11.0.json)
- 当前正式源码：[`cx26.3-wine11.0-runeon.23`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.23)，对应 seed `2026.10.24`（App `>=1.14 (16)`）。定义见 [`patchsets/cx26.3-wine11.0-runeon.23`](patchsets/cx26.3-wine11.0-runeon.23/manifest.json)
<!-- release-facts:current-patch-set -->
- 默认源码定义（历史）：`cx26.3-wine11.0-runeon.9`，未选择补丁集时使用
- 上游审查范围：完整审计至 `wine-11.15`，筛选回移至 `wine-11.20`；见 [MAINTENANCE.md](MAINTENANCE.md#review-records)

## 仓库结构

| 路径 | 内容 |
|---|---|
| `base/` | 固定源码包的 URL、SHA-256 与源码根目录 |
| `patchsets/<id>/` | 单个补丁集的 `series` 与 `manifest.json`；用 `RUNEON_WINE_PATCHSET_DEFINITION=patchsets/<id>` 选择 |
| `series`、`patches/manifest.json` | 历史默认定义（`.9`） |
| `patches/upstream/`、`patches/runeon/` | WineHQ 回移与 Runeon 补丁 |
| `release-manifests/` | 每个已发布 Release 的提交、Release URL、附件名、大小、SHA-256 与状态 |
| `tests/`、`scripts/` | 探针与检查；获取、应用、校验与打包脚本 |
| `docs/` | 上游审查记录与补丁设计说明（英文） |

## 范围与边界

- 不提交 Wine 源码树、构建产物、runtime 归档、prefix、用户数据或诊断信息。
- 从不下载、镜像或分发 `D3DMetal.framework`、`libd3dshared.dylib` 与 GPTK 私有 overlay。
- 能干净应用不等于进入补丁集，每个上游提交都先经审查。改动 `ntdll`、`server`、`wow64`、loader、`winemac.drv`、`win32u` 或 Unix/server 协议属于 ABI 敏感，除非单独批准，否则不纳入。
- Runeon 补丁须由调用方显式开启，从不按游戏名或 AppID 匹配；例外列在 [MAINTENANCE.md](MAINTENANCE.md#product-patches-without-an-opt-in)。
- 每个进入 Production 的 runtime 都有不可变的正式版 Release，含完全一致的补丁集包、完整对应源码与 SHA-256 文件。候选保持预发布。tag 与 Release 附件从不移动、替换或删除。
- runtime 的打包、签名与分发由 Runeon 产品仓库负责。

## 校验与构建

```bash
scripts/static-check.sh
source_root="$(scripts/fetch-source.sh)"
scripts/integration-check.sh "$source_root"
```

`integration-check.sh` 把全部补丁按顺序应用到固定基线的临时副本，检查校验标记，并确认同一 series 不能重复应用。为 Release 构建源码包：

```bash
export RUNEON_WINE_PATCHSET_DEFINITION=patchsets/cx26.3-wine11.0-runeon.23
source_root="$(scripts/fetch-source.sh)"
scripts/apply-series.sh "$source_root"
scripts/build-source-bundle.sh "$source_root" dist
scripts/build-patchset-bundle.sh dist
```

重建更早的补丁集时，同样选择它的定义，或检出它的 tag。完整构建步骤见 [BUILDING.md](BUILDING.md)。

## 版本历史

每个 Release 都附有补丁集包、完整对应源码与两个 SHA-256 文件，说明中写有该补丁集新增的内容。「分发给」表示产品仍向所列 App 版本提供该 seed。

| 补丁集 | 状态 | Seed | 说明 |
|---|---|---|---|
| [`.23`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.23) | 正式版，latest | `2026.10.24` | 当前版本；分发给 App `>=1.14 (16)` |
| [`.22`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.22) | 正式版 | `2026.10.20` | 分发给 App `1.13 (10)`–`1.13 (15)` |
| [`.21`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.21) | 正式版 | `2026.10.18` | 已被接替，保留 |
| [`.20`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.20) | 正式版 | `2026.10.16.1` | 已被接替，保留 |
| [`.19`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.19) | 预发布 | `2026.10.16` | Dev 候选，未用于 Production |
| [`.18`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.18) | 正式版 | `2026.10.14` | 分发给 App `1.12 (6)`–`1.13 (9)` |
| [`.17`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.17) | 正式版 | `2026.10.10` | 分发给 App `1.10 (3)`–`1.11 (4)` |
| [`.16`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.16) | 正式版 | `2026.10.06` | 分发给 App `1.10 (0)`–`1.10 (2)` |
| [`.15`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.15) | 正式版 | `2026.10.04` | 已被接替，保留 |
| [`.14`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.14) | 正式版 | `2026.10.02` | 已被接替，保留 |
| [`.13`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.13) | 正式版 | `2026.09.28.1`, `2026.09.28` | `2026.09.28.1` 分发给 App `1.8 (0)` 及以上、`1.10 (0)` 之前 |
| [`.12`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.12) | 预发布 | `2026.09.27` | Dev 候选，未用于 Production |
| [`.11`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.11) | 正式版 | `2026.09.26` | 已被接替，保留 |
| [`.10`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.10) | 正式版 | `2026.09.22` | 已被接替，保留 |
| [`.9`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.9) | 正式版 | `2026.09.13` | 已被接替，保留；默认定义 |
| [`.8`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.8) | 正式版 | `2026.09.06` | 已被接替，保留 |
| [`.7`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.7) | 预发布 | — | **无法编译**，切勿构建（`.8` 已修正） |
| [`.6`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.6) | 正式版 | `2026.08.11.1` | 已被接替，保留 |
| [`.5`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.5) | 正式版 | `2026.08.03.2` | 已被接替，保留 |
| [`.2`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.2)–[`.4`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.4) | 预发布 | — | 历史候选，未发布 |
| [`.1`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.1) | 预发布 | — | 首个候选，未发布 |
| [`.0`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.0) | 正式版 | `2026.07.22` | 已被接替，保留 |

## 文档

- [BUILDING.md](BUILDING.md)：构建 runtime 与源码包
- [MAINTENANCE.md](MAINTENANCE.md)：上游审查、补丁准入、免开关例外与发布政策
- [SECURITY.md](SECURITY.md)：安全问题报告
- [docs/upstream-reviews.md](docs/upstream-reviews.md) 与 [docs/patch-notes.md](docs/patch-notes.md)：审查记录与补丁设计说明

## 许可

本仓库中的 Wine 及源自 Wine/CrossOver Wine 的修改以 LGPL-2.1-or-later 授权，见 [`LICENSE`](LICENSE)。上游提交保留原作者信息。本仓库不改变 Runeon App 的许可。
