# Runeon Wine

[英文](README.md) | [简体中文](README.zh-CN.md) | [日本語](README.ja.md)

> 英文文档是权威版本；简体中文与日语文档均为完整译本。

Runeon Wine 是 Runeon Steam Baseline runtime 的公开源码维护仓库，保存固定的 Wine/CrossOver Wine 基线、上游回移植、产品级补丁和可复现的对应源码构建入口。本仓库不构成通用兼容性承诺，也不包含 Runeon App、Steam、游戏或 Apple 私有图形组件。

## 当前基线

- CodeWeavers 源码：`crossover-sources-26.3.0.tar.gz`
- Wine 基线：`Wine version 11.0`
<!-- release-facts:current-patch-set -->
- 默认源码定义（历史）：`cx26.3-wine11.0-runeon.9`，搭配保留的 seed `2026.09.13` 与 MoltenVK `1.4.2`
- 当前 Dev 与 Production 源码：[`cx26.3-wine11.0-runeon.11`](BACKPORTS-11.16-11.18-SWEEP.zh-CN.md)（正式版/latest），搭配 seed `2026.09.26` 与 MoltenVK `1.4.2`，App 门槛 `>=1.8 (0)`。保留 `.10` 全部补丁，新增 Wine 11.16–11.18 用户态修复批量回移、winegstreamer 视频缓冲池回退，以及两个调用方未启用时不生效的可选入口（NW.js 启动、DXGI 驱动版本）。Production promote 同一份签名 Dev 归档，不重新构建。须显式选择其 `patchsets/` 定义；默认 series 仍为历史 `.9`。
- 当前 Production 源码及 latest 正式 Release：seed `2026.09.26` 对应 `cx26.3-wine11.0-runeon.11`，App 门槛 `>=1.8 (0)`
- 验证：`.11` 已通过完整 x86_64/WoW64 构建、签名、release readiness，以及受支持 build 的鉴权 Dev 与 Production feed/ticket/完整下载检查。用户于 2026-09-26 反馈 Dev 未发现问题并授权 Production；该概括验收不代表每项 Steam/CEF、游戏或 App 交互均已独立实测。
- Production 保留回滚源码：seed `2026.09.22` 对应 `cx26.3-wine11.0-runeon.10`，seed `2026.09.13` 对应 `cx26.3-wine11.0-runeon.9`，seed `2026.09.06` 对应 `cx26.3-wine11.0-runeon.8`，seed `2026.08.11.1` 对应 `cx26.3-wine11.0-runeon.6`，seed `2026.08.03.2` 对应 `cx26.3-wine11.0-runeon.5`
- 上游已审计至：`wine-11.15`（完整审计）；筛选回移覆盖至 `wine-11.18`

[`base/crossover-26.3-wine-11.0.json`](base/crossover-26.3-wine-11.0.json) 是基线 URL、SHA-256 和源码根目录的唯一机器可读来源。[`series`](series) 定义补丁顺序；[`patches/manifest.json`](patches/manifest.json) 记录来源、风险和完整 upstream commit。

## 范围与边界

- 本仓库不提交完整 Wine 源码、编译目录、runtime archive 或 Wine prefix。
- 本仓库不下载、镜像或分发 `D3DMetal.framework`、`libd3dshared.dylib` 或 GPTK 私有 PE/Unix overlay。
- 上游提交必须先进入审计清单；能够 clean apply 不代表可以自动进入 active series。
- `ntdll`、`server`、`wow64`、`loader`、`winemac.drv`、`win32u`、Unix library/server protocol 和 D3DMetal 接口改动默认属于 ABI-sensitive，必须走独立基线升级或更强验证。
- Runeon 产品仓库继续负责 component packaging、Developer ID 签名、Dev/Production feed、下载校验和 release readiness。
- 本仓库及其 GitHub Release assets 公开。每个实际分发的 Production runtime 必须对应一个不可变正式 Release，并同时包含精确 patch-set bundle、完整对应源码和 SHA-256 文件；尚未提升至 Production 的候选必须保持 Pre-release。
- `patchsets/cx26.3-wine11.0-runeon.0` 是 Production seed `2026.07.22` 的精确历史源码定义；`patchsets/cx26.3-wine11.0-runeon.1` 冻结了第一版未发布的 Escape 修复候选。默认 [`series`](series) 描述历史 Production seed `2026.09.13` 使用的 `.9` 源码。要重建任何更早的 patch set，必须检出承载它的那个提交，而不是当前工作树。
- `release-manifests/` 记录每个公开 bundle 的 commit、Release URL、文件名、size、SHA-256、stable/prerelease 状态和永久保留规则。Production runtime 只能引用 `stable` manifest；Pre-release 不代表其中修复已提供给 Production 用户。

## 发布状态

- [`cx26.3-wine11.0-runeon.0`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.0) 是保留的旧 seed `2026.07.22` 对应的历史正式源码 Release。
- [`cx26.3-wine11.0-runeon.1`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.1) 是第一版包含 Escape `cfgmgr32` backport 的未上线候选，未随 Dev 或 Production runtime 发布，保持 Pre-release。
- `cx26.3-wine11.0-runeon.2` 新增了 33 个从 Wine 11.1 至 11.14 人工评审的稳定性与正确性 backport。纳入、等价和延期决定见 [`AUDIT-11.0-11.14.zh-CN.md`](AUDIT-11.0-11.14.zh-CN.md)。
- `cx26.3-wine11.0-runeon.3` 在 `.2` 基础上首次加入受管理的 Steam 启动验证，并作为不可变历史候选保留。
- `cx26.3-wine11.0-runeon.4` 作为不可变历史候选保留。
- [`cx26.3-wine11.0-runeon.5`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.5) 是 Production 保留 rollback seed `2026.08.03.2` 对应的正式源码 Release：继承 `.4`，精简受管理的 Steam 启动验证，并与已发布 assets、checksum 一起保持不可变。
- [`cx26.3-wine11.0-runeon.6`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.6) 是 Production 保留 rollback seed `2026.08.11.1` 对应的正式源码 Release：继承 `.5`，新增 9 个来自 Wine 11.15 的评审 backport，并把审计范围推进到 `wine-11.15`；纳入、等价和延期决定见 [`AUDIT-11.0-11.15.zh-CN.md`](AUDIT-11.0-11.15.zh-CN.md)。构建、签名、readiness、鉴权下载和产品路径验证均已完成，Production 使用在 Dev 验证过的完全相同 artifact bytes。

- [`cx26.3-wine11.0-runeon.7`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.7) 加入了 Vuplex 能力但**无法编译**：`dlls/kernelbase/process.c` 使用了注册表 API 却未包含 `winreg.h`。它在完整构建之前就被打了 tag，作为不可变历史候选保留，不得构建进任何 runtime。
- [`cx26.3-wine11.0-runeon.8`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.8) 是 `.7` 补上该 include 的版本，**打 tag 前已通过完整 x86_64/WoW64 构建验证**。它是保留回滚 Production seed `2026.09.06` 对应的正式源码 Release，Production 使用在 Dev 验证过的完全相同 artifact bytes。

- `.9` 历史交付与验证（2026-09-14，保留回滚）：[`cx26.3-wine11.0-runeon.9`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.9): Dev 与 Production seed `2026.09.13` 均使用 `.9` 与 MoltenVK `1.4.2`，App 门槛为 `1.8 (0)`。Production 复用在 Dev 验证过的同一份签名归档。构建、API 回归、签名、readiness、鉴权 feed/ticket 与完整下载验证已完成；用户已确认当前 Xcode Dev 源码版的组件更新、runtime 准备和 Steam 启动。分发已覆盖 `1.8 (0～3)`；没有逐 build 重跑游戏、停止重启交互与全新 prefix 验收。已发布 App 安装包保持 `1.8 (3)`。源码 Release 已正式发布，tag、commit 及四个附件的字节、大小、摘要均未变；`.8` / seed `2026.09.06` 保留为历史。 它保留 `.8` 并新增承载 13 个 Wine 11.16/11.17 提交的 10 个定向 backport 文件。不可变源码信息见[发布 manifest](release-manifests/cx26.3-wine11.0-runeon.9.source-archive.json)，选择与验证限制见[定向审查](BACKPORTS-11.16-11.17.zh-CN.md)；完整审计边界仍为 `wine-11.15`。
- [`cx26.3-wine11.0-runeon.10`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.10) 是保留回滚 Production seed `2026.09.22` 对应的正式源码 Release。
- [`cx26.3-wine11.0-runeon.11`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.11) 是当前 Dev 与 Production seed `2026.09.26` 对应的正式源码 Release；Production 使用 Dev 已验证的同一份产物字节。不可变源码详情见 [release manifest](release-manifests/cx26.3-wine11.0-runeon.11.source-archive.json)。

## 快速验证

```bash
scripts/static-check.sh
source_root="$(scripts/fetch-source.sh)"
scripts/integration-check.sh "$source_root"
```

`integration-check.sh` 会把固定基线复制到临时目录，按顺序应用全部补丁，验证最终 marker，并确认同一 series 不能被静默重复应用。它不会修改缓存中的原始源码。

## 生成对应源码包

```bash
source_root="$(scripts/fetch-source.sh)"
scripts/apply-series.sh "$source_root"
scripts/build-source-bundle.sh "$source_root" dist
```

Release 使用的 source bundle 必须与 runtime artifact 使用相同 base SHA 和 patch set。对应 tag 的 GitHub Release 必须同时包含 source bundle、patch-set bundle 和两份 SHA-256 文件。构建、发布与维护规则见 [`BUILDING.zh-CN.md`](BUILDING.zh-CN.md) 和 [`MAINTENANCE.zh-CN.md`](MAINTENANCE.zh-CN.md)。

Runeon 产品仓库消费较小的 patch-set bundle：

```bash
scripts/build-patchset-bundle.sh dist
```

其中包含固定基线 manifest、补丁、series、验证脚本和许可证。Runeon 构建仍从 CodeWeavers 获取固定 SHA 的完整 Wine 与相关源码 archive。

重建保留的旧 seed `2026.07.22` 的精确源码时必须显式选择 `.0`，不能用任何后续 patch set（包括 `.8` 或 `.9`）代替：

```bash
export RUNEON_WINE_PATCHSET_DEFINITION=patchsets/cx26.3-wine11.0-runeon.0
source_root="$(scripts/fetch-source.sh)"
scripts/apply-series.sh "$source_root"
scripts/build-source-bundle.sh "$source_root" dist
scripts/build-patchset-bundle.sh dist
scripts/build-source-archive-manifest.sh \
  dist/runeon-wine-patchset-cx26.3-wine11.0-runeon.0.tar.gz \
  dist/runeon-wine-source-cx26.3-wine11.0-runeon.0.tar.gz \
  dist/cx26.3-wine11.0-runeon.0.source-archive.json
```

## 许可证

Wine 以及本仓库基于 Wine/CrossOver Wine 形成的修改遵循 LGPL-2.1-or-later，许可证文本见 [`LICENSE`](LICENSE)。各上游提交保留原作者和 Wine 项目历史；Runeon App 本身的许可方式不因本仓库而改变。
