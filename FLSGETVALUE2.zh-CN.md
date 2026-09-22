# FlsGetValue2 候选

[English](FLSGETVALUE2.md) | [简体中文](FLSGETVALUE2.zh-CN.md) | [日本語](FLSGETVALUE2.ja.md)

`cx26.3-wine11.0-runeon.10` 保留 `.9` 并新增一个下游补丁。不可变 Pre-release 与对应源码用于 Dev seed `2026.09.22`，App 门槛 `>=1.8 (0)`。Production 与默认源码定义保持 `.9`；真实 Steam/游戏仍待用户 Dev 真机验收。

## 原因与实现

已分发的 x86 和 x64 kernelbase 均缺少 `FlsGetValue2`，必须使用该导出的调用方无法解析它。这只能确认 API 缺口，不能确认某款 Runeon 游戏崩溃的根因。

[修正后的下游实现](https://github.com/dappermint/winecx/commit/e0aa380780b73e20fabcfe78fd42713b94929a53) 使用 `RtlFlsGetValue` 并保留 last error。此前直接别名到 `FlsGetValue` 的做法不充分：旧 API 成功时清空、失败时设置 last error。补丁 `0103` 增加 kernelbase 实现、kernel32 导入与公开声明，复用已有 FLS 分配、存储和回调；不修改 ntdll、server、WoW64 或图形 ABI。这是下游适配代码，不是 WineHQ backport。

## 验证

`tests/flsgetvalue2-probe.c` 动态解析两处公开导出，检查非空/空值、非法索引、错误码保留、旧 API 行为不变、线程/fiber 隔离和清理回调。分别使用 `x86_64-w64-mingw32-gcc` 与 `i686-w64-mingw32-gcc`、参数 `-O2 -Wall -Wextra` 编译，在候选 runtime 的隔离干净 prefix 中运行。旧 `.9` 应因缺少导出失败；`--legacy-getter` 应因错误码断言失败，证明别名不能完成修复。

构建前运行 `scripts/static-check.sh` 与 `scripts/integration-check.sh <未打补丁源码>`。完整 runtime 构建和 managed launch grant harness 由产品仓库负责。本地证据记在产品任务 T784 下；本文不代表发布验收通过。

2026-09-22 本地验证：使用修正后的产品 pipe2 配置完成完整 Wine 构建。x86/x64 均在干净 prefix 和由旧测试 prefix 副本升级的环境通过 API 探针；旧 runtime 复现两处导出缺失，全部旧 getter 负对照按预期失败。

尚未独立验证原生 Windows 等价行为或受影响游戏。本次 Dev 交付由用户明确要求先完成代码兼容与产物分发验证，再由用户真机验收 Steam/CEF 和游戏；Production 仍须完整发布链。不增加游戏特判或新的兼容性承诺。产品构建必须保留 macOS pipe2 能力覆盖配置：否则 SDK 27 可在较旧受支持系统上弱链接不存在的符号，并在加载 Windows DLL 前失败。

```sh
export RUNEON_WINE_PATCHSET_DEFINITION=patchsets/cx26.3-wine11.0-runeon.10
scripts/static-check.sh
scripts/integration-check.sh /absolute/path/to/unpatched/wine
```
