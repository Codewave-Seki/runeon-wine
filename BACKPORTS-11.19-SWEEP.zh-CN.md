# Wine 11.19 用户态修复批量回移

[English](BACKPORTS-11.19-SWEEP.md) | [简体中文](BACKPORTS-11.19-SWEEP.zh-CN.md) | [日本語](BACKPORTS-11.19-SWEEP.ja.md)

> 以英文文档为准，中文与日文为完整译文。

候选补丁集 `cx26.3-wine11.0-runeon.15` 在 `0110` 之后加入 `patches/upstream/0079-wine-11.19-targeted-fixes.patch`。它合并了 87 个首次发布于 `wine-11.19` 的 WineHQ 提交，逐一列出每个提交及作者，manifest 记录完整提交 ID。只改测试的部分不包含在内。

## 选择

按[上一次回移](BACKPORTS-11.16-11.18-SWEEP.zh-CN.md)的规则审查了 `wine-11.18..wine-11.19` 的全部 353 个提交：

- 100 个提交改动了 `ntdll`、`server`、`win32u`、`winemac.drv`、`wow64`、`loader`、`configure`、`tools`、`libs` 或其他宿主驱动，主要是 OpenGL/EGL 上下文重构，以及 `winemac.drv` 的头文件与日志整理，一个都不纳入。其中的小修复逐个核对过：IPv6 `IPV6_MTU_DISCOVER` 层级修复（`fc5dd34cc3`）所在分支在 macOS 上不会编译，因为 macOS 有 `IPV6_DONTFRAG`；EGL pbuffer 返回值（`802d3d49fb`）不在 macOS 路径上；零线程组亲和性（`0998bfa77a`）只会让一个很少用的调用失败；别名 HKL 的 IME 判断（`a07e981c59`）会改动 `winemac.drv` 的键盘处理，需要单独验证；`NtReleaseSemaphore` 的有符号改动（`f89a7ca541`、`3e182735ea`）改变了系统调用签名。
- 29 个提交只改测试。
- 在 224 个用户态候选中，选取 Steam、启动器、安装程序或游戏会加载的 DLL 里的崩溃、内存安全、泄漏与错误路径修复。未纳入：手柄设备身份（`winebus.sys`、`hidclass.sys`、`xinput1_3`）、`mountmgr.sys` 的光驱与 SCSI IOCTL，以及功能性改动，例如 D3D12 支持的 Media Foundation 缓冲、GDI+ 竖排文字、DNS 缓存与 hosts 文件、VBScript 解析器改动、`RoResolveNamespace`、D2D1 色彩管理桩和 `GetNextAsyncId`。

已选中且能干净应用的提交中，有三个经审查后移除：

| 移除 | 原因 |
|---|---|
| `c9723c90f1` setupapi | 把 `delete_multi_sz_value()` 的缓冲区减半，而函数里的 `WCHAR` 指针运算没改，复制会写出分配范围。这个 bug 在 `wine-11.19` 本身就存在。该改动只是节省内存。 |
| `6880117619` oleaut32 | 改变了十进制舍入语义，不是崩溃修复。 |
| `5ff02cf188` mfplat | `MFCalculateImageSize()` 对零高度返回失败，会让基线里原本能成功的解码器类型协商失败，对视频播放有风险。 |

有 13 个提交因基线不同而无法应用，未做移植：`rsaenh` 的 HMAC 系列需要基线没有的哈希实现（其中单独应用成功的一个已撤回，不接受部分应用一个系列）、`secur32` 的 `770f0b9c7b`、hosts 文件的后续修复 `8f6cb19433`、基线代码不同的 Media Foundation 时钟与 session 修复、`msado15` 的 `57cebf5711`，以及 D3DX10 sprite 修复。

## 适配

| 提交 | 适配 |
|---|---|
| `60551e1d7b` msvcrt | 在两处对齐分配之前加溢出检查；基线缺少上游上下文中后来加入的收缩填充代码块。 |
| `105ced5bc1` msvcrt | 同样的 `ARRAY_SIZE` 改动，周围几行不同。 |
| `53b5345251`、`37fd158bfc` mf | 合并：`Flush` 只对仍有音频客户端的活动渲染器执行重置；基线没有 position/pts 字段。 |
| `ee38ff1725` mf | 基线缺少 `intsafe.h`；用 `~(DWORD)0` 代替 `DWORD_MAX` 比较。 |
| `258ee951ab` windowscodecs | 补全：新加的溢出检查改为经由公共出口返回，从而释放解码器锁；上游版本在持锁状态下直接返回。 |
| `ae807355d6` ieframe | 补全：检查 task 的分配结果，回调创建失败时释放 task，只对已创建的回调调用 Release；上游版本仍会对空回调调用 Release，并泄漏 task。 |

## 验证

`scripts/static-check.sh` 与 `scripts/integration-check.sh` 通过；把补丁序列应用到固定的源码归档，结果与审查过的源码树逐文件一致。每个改动过的模块，以及所有用到其源码的模块（`ucrtbase`、`msvcr*` 与 `msvcp*` 系列、`d3dcompiler_*`、`d3dx9_*`、`d3dx10_43`、`d3dx11_43`、`comctl32_v6`），都能为 i386 和 x86_64 编译通过。唯一新增的底层调用是 `cxx_frame_handler` 里的 `RtlLookupFunctionEntry`，基线已实现。打 tag 之前仍需完整的产品构建与运行时冒烟测试。

`upstreamAuditThrough` 保持 `wine-11.15`。基线 manifest 记录 `upstreamSweepThrough: wine-11.19`。
