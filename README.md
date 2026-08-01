# Runeon Wine

Runeon Wine 维护 Runeon Steam Baseline runtime 使用的 Wine 源码基线、上游 backport 和产品级补丁。它不是新的公开兼容性承诺，也不包含 Runeon App、Steam、游戏或 Apple 私有图形组件。

## 当前基线

- CodeWeavers source：`crossover-sources-26.3.0.tar.gz`
- Wine baseline：`Wine version 11.0`
- Patch set：`cx26.3-wine11.0-runeon.1`
- 已审计 upstream：`wine-11.14`

基线 URL、SHA-256 和源码根目录以 [`base/crossover-26.3-wine-11.0.json`](base/crossover-26.3-wine-11.0.json) 为唯一机器可读来源。补丁顺序以 [`series`](series) 为准，来源、风险和完整 upstream commit 记录在 [`patches/manifest.json`](patches/manifest.json)。

## 边界

- 本仓库不提交完整 Wine 源码、编译目录、runtime archive 或 Wine prefix。
- 本仓库不下载、镜像或分发 `D3DMetal.framework`、`libd3dshared.dylib` 或 GPTK 私有 PE/unix overlay。
- 上游提交只进入审计清单，不会因为能够 clean apply 就自动进入 active series。
- `ntdll`、`server`、`wow64`、`loader`、`winemac.drv`、`win32u`、unixlib/server protocol 和 D3DMetal 接口改动默认视为 ABI-sensitive，必须走独立基线升级或强化验证。
- Runeon 产品仓库继续负责 component packaging、Developer ID 签名、Dev/Production feed、下载校验和 release readiness。

## 快速验证

```bash
scripts/static-check.sh
source_root="$(scripts/fetch-source.sh)"
scripts/integration-check.sh "$source_root"
```

`integration-check.sh` 会复制一份临时基线、顺序应用全部 patch、验证最终 marker，并确认同一 series 不能被静默重复应用。它不会修改缓存中的原始源码。

## 生成对应源码包

```bash
source_root="$(scripts/fetch-source.sh)"
scripts/apply-series.sh "$source_root"
scripts/build-source-bundle.sh "$source_root" dist
```

发布用 source bundle 必须与实际 runtime artifact 使用同一个 base SHA 和 patch set。构建、发布与维护流程见 [`BUILDING.md`](BUILDING.md) 和 [`MAINTENANCE.md`](MAINTENANCE.md)。

产品仓库消费的是较小的 patch-set bundle：

```bash
scripts/build-patchset-bundle.sh dist
```

它包含固定基线 manifest、patch、series、校验脚本和许可证；Runeon 构建仍从 CodeWeavers 固定 SHA 的完整 archive 获取 Wine 与同源依赖。

## 许可证

Wine 与这里基于 Wine/CrossOver Wine 源码形成的修改遵循 LGPL-2.1-or-later。许可证文本见 [`LICENSES/LGPL-2.1.txt`](LICENSES/LGPL-2.1.txt)。各上游提交保留原作者和 Wine 项目历史；Runeon App 本身不因本仓库而改变许可方式。
