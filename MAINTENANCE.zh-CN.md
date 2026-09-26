# 上游维护规则

[英文](MAINTENANCE.md) | [简体中文](MAINTENANCE.zh-CN.md) | [日本語](MAINTENANCE.ja.md)

> 英文文档是权威版本；简体中文与日语文档均为完整译本。

Runeon Wine 同时接受主动上游审计和 diagnostics 驱动调查两类输入；任何路径都不能绕过同一套验证门禁。

## 主动审计

1. 每周 workflow 检查 Wine `11.x` 最新 tag。
2. `scripts/audit-upstream.sh --through latest` 生成从 `wine-11.0` 开始的 commit 清单。
3. 自动风险标签只用于缩小 review 范围，不能证明补丁安全。
4. 逐条判断 CrossOver 26.3 是否已有等价实现、是否依赖前置重构，以及是否对 Runeon 有明确价值。
5. 只有来源、依赖、测试、用户收益和回归面都明确的修复才能加入 active series。

## 风险层级

- `candidate`：局部 DLL 修复，仍需人工 review 和测试。
- `subsystem-sensitive`：图形、媒体、窗口、输入、构建系统或多模块修改。
- `abi-sensitive`：`ntdll`、`server`、`wow64`、`loader`、`winemac.drv`、`win32u`、Unix library/server protocol 或 D3DMetal 接口改动。默认不作为普通 backport 候选。

## 当前 Wine 11.x 审查基线

当前 Dev 与 Production 源码：[`cx26.3-wine11.0-runeon.11`](BACKPORTS-11.16-11.18-SWEEP.zh-CN.md)（正式版/latest），对应 seed `2026.09.26`，App 门槛 `>=1.8 (0)`。在 `.10` 基础上新增 Wine 11.16–11.18 筛选回移、winegstreamer 视频缓冲池回退与两个可选入口。构建、签名、readiness 与 Dev/Production 分发检查已通过；用户于 2026-09-26 反馈 Dev 未发现问题并授权 Production。Production promote 同一份签名 Dev 归档，不重新构建。

`.10` 历史交付（2026-09-23，保留回滚）：[`cx26.3-wine11.0-runeon.10`](FLSGETVALUE2.zh-CN.md)（正式版/latest），搭配 seed `2026.09.22` 与 MoltenVK `1.4.2`，App 门槛 `>=1.8 (0)`。Production promote 同一份签名 Dev 归档，不重新构建。须显式选择独立 `patchsets/` 定义；默认 series 保持历史 `.9`。 验证：API、隔离 prefix、签名与 release readiness 已通过。用户于 2026-09-23 反馈 Dev 未发现问题并授权 Production；该概括验收不代表每项 Steam/CEF、游戏或 App 交互均已独立实测。App 安装包与分发验证由产品仓库记录。

`.9` 历史交付与验证（2026-09-14，保留回滚）：当前 `.9` patch set 保留 `.8` 基线及其产品补丁。[Wine 11.16/11.17 定向审查](BACKPORTS-11.16-11.17.zh-CN.md) 新增承载 13 个上游提交的 10 个补丁文件，但不把完整审计边界推进到 `wine-11.15` 之后。Dev 与 Production seed `2026.09.13` 均使用 `.9` 与 MoltenVK `1.4.2`，App 门槛为 `1.8 (0)`。Production 复用在 Dev 验证过的同一份签名归档。构建、API 回归、签名、readiness、鉴权 feed/ticket 与完整下载验证已完成；用户已确认当前 Xcode Dev 源码版的组件更新、runtime 准备和 Steam 启动。分发已覆盖 `1.8 (0～3)`；没有逐 build 重跑游戏、停止重启交互与全新 prefix 验收。已发布 App 安装包保持 `1.8 (3)`。源码 Release 已正式发布，tag、commit 及四个附件的字节、大小、摘要均未变；`.8` / seed `2026.09.06` 保留为历史。

`cx26.3-wine11.0-runeon.8` 携带 `patches/runeon/0102-vuplex-accelerated-paint-policy.patch`。（`.7` 携带的是同一补丁的早期形态，**无法编译**——它在完整构建之前就被打了 tag。`static-check.sh` 与 `integration-check.sh` 只验证补丁能否应用，不验证产物能否构建，因此产品补丁必须先过完整构建再打 tag。）它只给 Wine 一种能力：让 Vuplex 3D WebView 的 CEF 宿主从 `OnAcceleratedPaint()` 回退到 CPU `OnPaint()`。该参数仅在调用方应用通过 `HKCU\Software\Wine\AppDefaults\<映像名>\Runeon` 的 `DisableVuplexAcceleratedPaint` 显式开启时才追加。Wine 中不出现任何游戏名或 Steam AppID——策略由调用方写入，哪些游戏需要回退不属于本仓库。宿主的识别条件是：命令行实际要运行的**映像名**以 `.vuplex` 结尾，且命令行含 `--vx-graphics-api=d3d11`、不含 `--type=`。因此 Chromium 子进程不受影响，某个参数里恰好出现 `.vuplex` 路径也不会误触发。回退的代价是每帧一次 CPU 拷贝，所以它是按应用可选而非默认开启。

已交付的 `cx26.3-wine11.0-runeon.8` patch set 基于截至 `wine-11.15` 的逐提交审查；完整记录见[审计报告](AUDIT-11.0-11.15.zh-CN.md)，以及它所延续的[11.0 至 11.14 审计](AUDIT-11.0-11.14.zh-CN.md)。在此前两个已接受的上游 backport 之外，累计选入 42 个局部正确性与稳定性修复，并不是把所有上游提交直接复制进来。功能新增、大范围重构、ABI 敏感改动、子系统迁移、CrossOver 26.3 已有等价实现，以及依赖或回归面尚未收敛的修复均继续延期。

审计是一个时间点上的决策记录，不是永久白名单。延期提交在其依赖链、相关测试、独立探针和 Runeon 实际价值都得到确认后，可以进入后续新的不可变 patch set。反过来，进入审计清单也不代表可以直接发布 runtime；仍必须通过下文的完整构建与产品 smoke 门槛。

## Diagnostics 驱动调查

真实用户日志可以确认影响和优先级。必须先固定调用链或用独立 probe 复现，再关联 upstream commit。没有本机证据时只能记录候选，不能把网上 issue 直接写成 Runeon 根因。

## Patch 进入 active series 的门禁

- 记录完整 upstream commit SHA、首次包含它的 Wine release 和原作者来源。
- 对固定 CrossOver source SHA clean apply，或完整记录所有适配差异。
- 回移植原始 upstream tests，或说明无法移植的原因。
- 受影响模块测试与独立 probe 通过。
- 完整 Wine runtime build 通过。
- Steam CEF、stop/relaunch、D3DMetal overlay 和 DXMT/DXVK/D3DMetal smoke 通过。
- clean prefix 与已有 prefix 都完成验证。
- source bundle、notices、component metadata 和 Runeon 文档同步。

## 发布规则

Patch set 使用不可变 ID，例如 `cx26.3-wine11.0-runeon.1`。Git tag、source bundle、runtime component metadata 和 Runeon release 文档必须引用同一个 ID。新 artifact 必须先发布 Dev，并完成 readiness、download 和 product smoke；Production 只能 promote 已验证的精确字节，不能为 Production 直接重建。

仓库与 bundle 保持公开。生成候选 tag 后，使用 `build-source-archive-manifest.sh` 记录公开 Release URL、asset SHA/size 和 `prerelease` 状态，再上传 patch-set bundle、完整对应源码和两份 `.sha256`。只有 matching runtime 真正进入 Production 后，才能把同一个不可变 Release 与 manifest 改为 `stable`。历史 tag 和 assets 永久保留，不得移动、替换或删除。

公开仓库只包含 LGPL Wine/CrossOver Wine lineage、补丁与构建脚本。Runeon App、用户诊断、Steam/游戏文件、Developer ID 或服务密钥，以及 D3DMetal/GPTK/Apple 私有组件绝不能进入 Git 历史、Actions artifact 或 Release asset。每次公开更新前都要检查完整历史、Release asset 列表和 Actions 日志。
