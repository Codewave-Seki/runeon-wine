# 上游维护规则

Runeon Wine 同时使用主动审计和 diagnostics 驱动两条输入；任何路径都不能绕过同一验证门禁。

## 主动审计

1. 每周 workflow 检查 Wine `11.x` 最新 tag。
2. `scripts/audit-upstream.sh --through latest` 生成从 `wine-11.0` 开始的 commit 清单。
3. 自动风险标签只用于缩小 review 范围，不代表补丁安全。
4. 逐条判断 CrossOver 26.3 是否已有等价实现、是否依赖前置重构、是否值得进入 Runeon。
5. 只有来源、依赖、测试、真实收益和回归面明确的修复才加入 active series。

## 风险层级

- `candidate`：局部 DLL 修复，仍需人工 review 和测试。
- `subsystem-sensitive`：图形、媒体、窗口、输入、构建系统或多模块修改。
- `abi-sensitive`：`ntdll`、`server`、`wow64`、`loader`、`winemac.drv`、`win32u`、unixlib/server protocol、D3DMetal 接口。默认不做普通 backport。

## Diagnostics 驱动

真实用户日志用于确认影响和优先级。必须先固定调用链或独立 probe，再关联 upstream commit。没有本机证据时只能记录候选，不能把网上 issue 直接写成 Runeon 根因。

## Patch 进入 active series 的门禁

- 完整 upstream commit SHA、首次进入的 Wine release 和作者来源；
- 对固定 CrossOver source SHA clean apply，或明确记录适配差异；
- 原始 upstream tests 一并回移植，或说明无法移植的原因；
- 受影响模块测试与独立 probe 通过；
- 完整 Wine runtime build 通过；
- Steam CEF、stop/relaunch、D3DMetal overlay、DXMT/DXVK/D3DMetal smoke 通过；
- clean prefix 与已有 prefix 都验证；
- source bundle、notices、component metadata 和 Runeon 文档同步。

## 发布

Patch set 使用不可变 ID，例如 `cx26.3-wine11.0-runeon.1`。Git tag、source bundle、runtime component metadata 和 Runeon release 文档必须引用同一个 ID。新 artifact 先发布 Dev 并完成 readiness/download/product smoke；Production 只能 promote 已验证的精确字节，不得重建后直接上线。

仓库与 bundle 保持公开。生成候选 tag 后，用 `build-source-archive-manifest.sh` 记录公开 Release URL、asset SHA/size 和 `prerelease` 状态，并上传 patch-set、完整 corresponding source 及两份 `.sha256`。只有 matching runtime 真正进入 Production 后，才把同一不可变 tag 的 Release 与 manifest 状态改为 `stable`；历史 tag 和 assets 永久保留，不覆盖、不删除。

公开仓库只维护 LGPL Wine/CrossOver Wine lineage、补丁与构建脚本。Runeon App、用户诊断、Steam/游戏文件、Developer ID/服务密钥，以及 D3DMetal/GPTK/Apple 私有组件不得进入 Git 历史、Actions artifact 或 Release asset。每次公开前检查完整历史、Release asset 列表和 Actions 日志。
