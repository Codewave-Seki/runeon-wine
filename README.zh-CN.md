# Runeon Wine

[英文](README.md) | [简体中文](README.zh-CN.md) | [日本語](README.ja.md)

> 英文文档是权威版本；简体中文与日语文档均为完整译本。

Runeon Wine 是 Runeon Steam Baseline runtime 的公开源码维护仓库，保存固定的 Wine/CrossOver Wine 基线、上游回移植、产品级补丁和可复现的对应源码构建入口。本仓库不构成通用兼容性承诺，也不包含 Runeon App、Steam、游戏或 Apple 私有图形组件。

## 当前基线

- CodeWeavers 源码：`crossover-sources-26.3.0.tar.gz`
- Wine 基线：`Wine version 11.0`
- 当前 active patch set：`cx26.3-wine11.0-runeon.1`
- 上游已审计至：`wine-11.14`

[`base/crossover-26.3-wine-11.0.json`](base/crossover-26.3-wine-11.0.json) 是基线 URL、SHA-256 和源码根目录的唯一机器可读来源。[`series`](series) 定义补丁顺序；[`patches/manifest.json`](patches/manifest.json) 记录来源、风险和完整 upstream commit。

## 范围与边界

- 本仓库不提交完整 Wine 源码、编译目录、runtime archive 或 Wine prefix。
- 本仓库不下载、镜像或分发 `D3DMetal.framework`、`libd3dshared.dylib` 或 GPTK 私有 PE/Unix overlay。
- 上游提交必须先进入审计清单；能够 clean apply 不代表可以自动进入 active series。
- `ntdll`、`server`、`wow64`、`loader`、`winemac.drv`、`win32u`、Unix library/server protocol 和 D3DMetal 接口改动默认属于 ABI-sensitive，必须走独立基线升级或更强验证。
- Runeon 产品仓库继续负责 component packaging、Developer ID 签名、Dev/Production feed、下载校验和 release readiness。
- 本仓库及其 GitHub Release assets 公开。每个实际分发的 Production runtime 必须对应一个不可变正式 Release，并同时包含精确 patch-set bundle、完整对应源码和 SHA-256 文件；未上线候选必须保持 Pre-release。
- `patchsets/cx26.3-wine11.0-runeon.0` 是 Production seed `2026.07.22` 的精确历史源码定义，不含尚未上线的 Escape `cfgmgr32` backport。默认 [`series`](series) 仍描述下一版 `.1` 候选。
- `release-manifests/` 记录每个公开 bundle 的 commit、Release URL、文件名、size、SHA-256、stable/prerelease 状态和永久保留规则。Production runtime 只能引用 `stable` manifest；Pre-release 不代表其中修复已提供给 Production 用户。

## 发布状态

- [`cx26.3-wine11.0-runeon.0`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.0) 是当前 Production seed `2026.07.22` 的正式对应源码 Release。
- [`cx26.3-wine11.0-runeon.1`](https://github.com/Codewave-Seki/runeon-wine/releases/tag/cx26.3-wine11.0-runeon.1) 是包含 Escape `cfgmgr32` backport 的下一版候选，尚未随 Dev 或 Production runtime 发布，保持 Pre-release。

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

重建已分发 seed `2026.07.22` 的精确源码时必须显式选择 `.0`，不能用包含 Escape 修复的 `.1` candidate 代替：

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
