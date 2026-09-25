# Wine 11.16–11.18 用户态修复批量回移

[English](BACKPORTS-11.16-11.18-SWEEP.md) | [简体中文](BACKPORTS-11.16-11.18-SWEEP.zh-CN.md) | [日本語](BACKPORTS-11.16-11.18-SWEEP.ja.md)

> 以英文文档为准，中文与日文为完整译文。

候选 `cx26.3-wine11.0-runeon.11` 在 `0106` 之后增加四个上游补丁：

| 补丁 | 内容 |
|---|---|
| `0062-wine-11.16-targeted-fixes.patch` | 首次发布于 `wine-11.16` 的 20 个 WineHQ 提交 |
| `0063-wine-11.17-targeted-fixes.patch` | `wine-11.17` 的 107 个提交 |
| `0064-wine-11.18-targeted-fixes.patch` | `wine-11.18` 的 77 个提交 |
| `0065-wine-11.18-imm32-ime-keydown-lparam.patch` | 按 CrossOver 输入法代码适配的 `a6b3099042` |

每个合并补丁都列出全部上游提交及作者，manifest 记录完整提交号。`0045`–`0061` 已包含的 13 个提交不重复。唯一的生产代码适配在 `0064`：`d22810d47b` 引用了本基线没有的 `intsafe.h`，因此 `bmpdecode.c` 定义了一个溢出约定相同的本地 `UIntMult()`。这是完整产品构建发现的问题，仅靠补丁能否干净应用发现不了。

## 选取规则

审阅了 `wine-11.15..wine-11.18` 的全部 1,009 个提交。一个提交必须同时满足以下全部条件才会入选：

- 改动 Steam、启动器、安装器或游戏会加载的用户态 DLL，包括媒体、输入、音频、COM、网络与 TLS、加密、WMI、文字、图像与 D3DX、D3D9 路径、安装器和界面控件。
- 不改动 `ntdll`、`server`、`win32u`、`winemac.drv`、`wow64`、`loader`、`configure`、`tools`、`libs`，也不改动服务器协议头。
- 按上游顺序，在补丁 `0001`–`0106` 之上零偏移应用。完整系列重放后与审阅树逐文件一致。

能应用的 271 个提交中，以下几类虽然文本能干净应用，但在本基线上会造成错误行为，因此移除：

| 移除 | 原因 |
|---|---|
| NUMA（`073a4edad7`、`6ea56eb539` 及配套 FIXME） | 调用本基线 ntdll 未实现的 `SystemNumaProcessorMap`，原先返回成功的桩函数会变成失败 |
| UI 语言重实现（`5b4f8aa35b`、`480b8e4601`、`fa7d1b1571`、`6c91a3f4e7`、`74858bee70`、`065f1ad633`）、控制台输入重写 `8126bd7f9c` | 建立在被排除的 ntdll 改动上的全局语言/控制台行为；游戏语言检测不能被悄悄改变 |
| PnP 身份系列（`hidclass.sys`、`winebus.sys` 兼容 ID、`mmdevapi`/`coreaudio` 实例 ID、`IRP_MJ_CREATE` 处理、setupapi 改用 cfgmgr32） | 依赖被排除的 `ntoskrnl`/`mountmgr` 改动，会改变手柄和游戏枚举到的设备身份 |
| `msvcrt` 标准句柄系列、`user32` 对象安全描述符 | 进程级 stdio/对象创建行为，部分依赖服务器端改动 |
| D3D11 `Discard*`/资源共享及相关 wined3d 重构、`WINED3D_TEXTURE_GENERATE_MIPMAPS` 及依赖它的 d3d9 修复、D3DX10 精灵实现、quartz 全屏模拟、WMA 解码器与 wg_parser 标志/PTS 系列 | 功能或重构系列：改变图形、视频或窗口行为而 Runeon 无此需求，或依赖链条中已被排除的部分 |
| secur32 的 LSA/negotiate 改动 | 属于被排除的 msv1_0/kerberos/lsass 工作；secur32 中的 SChannel 与通用修复保留 |
| `mfreadwrite` `4d22860530` | 属于 `.9` 已推迟的异步命令引用计数系列 |
| 工具栏、Shell 视图、卷标等小改动 | 对 Steam 与游戏无收益 |

此前推迟的 CoreAudio 周期、WGI 初始化、XAudio2 解锁以及 MF drain/生命周期提交，按 [BACKPORTS-11.16-11.17.zh-CN.md](BACKPORTS-11.16-11.17.zh-CN.md) 的理由继续推迟。

上游 revert `e5abb7c03a` 会移除 x64 `RaiseException` 中保留非易失寄存器的实现。该实现存在于本基线，注释说明部分 DRM 依赖它。这个 revert 的依据是 Windows 11 25H2 的测试结果，而不是 Wine 缺陷，因此不携带。

## 不适用于本基线

部分修复针对的代码在 CrossOver 26.3 中不存在，例如 msxml3 XPath 引擎、WinHTTP `WINHTTP_OPTION_SERVER_CBT`、基于 SymCrypt 的 bcrypt/rsaenh 路径、较新的 d2d1 命令列表与 dsound 重采样代码，以及 cfgmgr32。wined3d 交换链互斥锁修复与 wg_parser 初始 gap 修复所针对的故障，本基线本就没有。以上都未做适配。

## 验证

`scripts/static-check.sh` 与 `scripts/integration-check.sh` 通过。从固定归档加补丁系列重建的源码树与审阅树逐文件一致。逐一检查了新增的底层调用，只引入了通用堆与临界区函数。产品完整 Wine 构建随本地 seed 候选执行。本批回移不声称修复了任何具体游戏问题。

`upstreamAuditThrough` 仍为 `wine-11.15`，因为这是筛选式回移，不是完整审计。
