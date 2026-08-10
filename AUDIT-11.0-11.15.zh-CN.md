# Wine 11.0 至 11.15 稳定性评审

[English](AUDIT-11.0-11.15.md) | [Chinese](AUDIT-11.0-11.15.zh-CN.md) | [Japanese](AUDIT-11.0-11.15.ja.md)

> 英文文档是权威版本；本文件是完整简体中文译本。

## 范围

固定比较范围为 `wine-11.0..wine-11.15`，基线是 SHA-256 为 `ac99c8ca4b3848f3e81784135f023df266b61c2345726ea55a50b3e030dd6872` 的 CrossOver 26.3 源码包。

清单共有 3,350 个上游提交：2,520 个自动 candidate、340 个 subsystem-sensitive、490 个 ABI-sensitive。自动标签只用于分流人工评审，不能证明安全，也不能直接作为纳入依据。

`wine-11.0..wine-11.14` 的判定保持不变，记录在 [`AUDIT-11.0-11.14.zh-CN.md`](AUDIT-11.0-11.14.zh-CN.md)。本文件补充 `wine-11.14..wine-11.15` 这一增量轮次——它由定时上游 watch 在 `wine-11.15` 于 2026-08-08 发布时触发。

## 增量区间

`wine-11.14..wine-11.15` 共 207 个提交：147 个自动 candidate、25 个 subsystem-sensitive、35 个 ABI-sensitive，其中 39 个只改测试文件。

每个提交都使用发布脚本同一套 forward/reverse 算法检查：正向 clean 说明基线仍是未修复的代码、CrossOver 没有等价实现；反向 clean 则说明修复已经存在。凡是冲突的提交，都在做判断前先读过 CrossOver 的实现。

## 纳入 `cx26.3-wine11.0-runeon.6`

`.6` 继承 `.5` 的全部补丁，新增 9 个 backport，覆盖 12 个上游提交。

| 首个 release | 上游提交 | 模块 | 结果 |
| --- | --- | --- | --- |
| 11.15 | `702e44767a8e` | Win32u | 修正美式键盘表多报一个扫描码、越过 `vsc_to_vk` 末尾的问题 |
| 11.15 | `bb33d1d34204` | MFReadWrite | transform 协商期间不再修改调用方传入的 media type，并带上游测试 |
| 11.15 | `9a26041b38d8` | MFReadWrite | 下游还有 transform 时，允许从解码器抽出多个 sample |
| 11.15 | `0e3bbc87cf61` | Video Resizer | 注册 DMO 实际支持的 media type |
| 11.15 | `cd0d7910b23b` | SChannel | 改用 bcrypt 计算服务器证书哈希以支持 ECDSA，并带上游测试 |
| 11.15 | `1fc89cc934e9` | GDI+ | 修正 `convert_pixels()` 每行字节数向下取整，并带上游测试 |
| 11.15 | `27ee4f5db28c`、`2f66b17d51de` | ComCtl32、User32 | 两套 listbox 实现在 `LB_SETTOPINDEX` 越界时都返回 `LB_ERR` |
| 11.15 | `d8186e465ad4` | CMD | 在读取前初始化 `node_builder_parse` 的循环标志 |
| 11.15 | `d4b32af24629` | Crypt32 | 导入 PFX 时忽略外层 DER SEQUENCE 之后的字节，并带上游测试 |

每个补丁的权威路径、SHA-256、风险等级、完整 commit ID 和顺序见 [`patches/manifest.json`](patches/manifest.json)。

### 测试回移植

9 个补丁中有 7 个带上游测试变更，其中 3 个还需要上游的测试前置提交：

- listbox 补丁额外携带 `ab21fbc49a25` 和 `dd7f58d774`，即在两套测试里引入 `test_LB_SETTOPINDEX` 的提交——基线里根本没有这个测试，也就无从去掉 `todo_wine`。两个前置提交都在同一发布区间内，且 clean apply。
- Crypt32 测试按基线测试文件的结构重写：`test_PFXImportCertStore_trailing_zeros()` 插在 `test_PFXImportCertStore()` 之后，而不是基线中并不存在的 `_sha256_signing` 变体之后，并在 `START_TEST` 的对应位置注册。测试体本身未改。
- Win32u、MFReadWrite `9a26041b38d8`、Video Resizer 和 CMD 四个提交上游就没有测试。它们是一到两行的缺陷修复，两个版本的代码都没有对应的 in-tree 测试覆盖。

### 适配说明

有三个补丁偏离了上游原文：

- `bb33d1d34204`：该 helper 在基线里叫 `update_media_type_from_upstream()`，上游后来改名为 `update_media_type()`。适配内容仅此改名；控制流、media type 拷贝及其释放都来自上游。
- `d4b32af24629`：`open_cert_store()` 在 11.0 之后多了 `pgnutls_pkcs12_verify_mac()` 调用，因此 hunk 上下文不同。新增的 `pfx_der_outer_length()` helper 原样搬入 `SONAME_LIBGNUTLS` 块内，两行 guard 放在相对 `pgnutls_pkcs12_import()` 的同一位置。
- `cd0d7910b23b`：上游这条提交没有改 `dlls/secur32/Makefile.in`，因为上游 secur32 已经通过 `f4a9724c89`（mTLS 相关）加入的 `ncrypt` delay import 间接拿到 `BCryptHash`。本基线没有这个 import，PE 链接会报 `undefined reference to BCryptHash`。为了不牵入那条无关的 mTLS 提交，本补丁直接在 `DELAYIMPORTS` 里加上 `bcrypt`。这只是构建依赖图的适配，C 代码全部来自上游。

### ABI-sensitive 例外

`702e44767a8e` 改的是 `dlls/win32u`，按风险策略属 ABI-sensitive、默认排除。本次作为有记录的例外纳入：改动只是一个静态键盘表的初始化项，把 `ARRAY_SIZE(vsc_to_vk)` 改成 `ARRAY_SIZE(vsc_to_vk) - 1`，不涉及任何结构布局、接口或 Unix 边界，修的是 ASan 发现的越界读。本区间内没有其它 ABI-sensitive 提交被纳入。

## CrossOver 已有等价实现或缺陷不适用

- `0fde54a3574a`（wined3d frame latency）：CrossOver 26.3 根本不走上游的 `frame_latency_semaphore` 路径，而是在 `dlls/wined3d/cs.c` 里用自己的 `pending_presents` / `waiting_for_present` / `present_event` / `max_frame_latency` 实现，并且已经在等待前后释放和重新获取 wined3d mutex。上游要消除的多线程 D3D9 停顿在本基线上不存在。
- `852e39a89c0d`（host window state 加锁）：基线中只有 `winex11.drv` 实现 `pGetWindowStateUpdates`，`winemac.drv` 没有该回调，补丁依赖的解锁协议也写在 X11 驱动里，因此该竞态在 macOS 路径上不会发生。
- `95e5d76a1882` 与 `0aa733c17aff`（wined3d planar CPU blits）：属 Vulkan H.264 解码链，Runeon 的视频解码走 D3DMetal 与 DXVK，不经过这条路径。
- `bfe591cbef5b`（WindowsCodecs 边界检查）：它移除的检查由同一发布区间的 `2e68942fce46` 引入，从未进入本基线，两者合起来在这里没有任何效果。
- 本区间的 4 个 `winebus.sys` 提交分别走 `hidraw`、`inotify` 和 `evdev` 路径。macOS 的手柄枚举经由 IOHID，这些代码在 Runeon 中不会执行。
- `winex11.drv`、`winewayland.drv`、glibc `sa_restorer` 以及 ARM64EC / ARM WoW64 相关提交都不在 macOS 的 x86_64 与 arm64 目标上运行。

## 延期，而不是无声丢弃

- UCRT `b0150eda77d7`（`exp(NAN)`）：源码修复只有三行，但它唯一的回归测试需要 `8bed5ce602`，而该提交还重写了 `test_cexp` 与 `test_expf` 中不相关的断言——这些断言在本基线上的结果取决于并未回移植的 11.x 数学修复。按上一轮对 `35b1e7eb9a7e` 的同一判定，不接受只取源码的 backport。
- KernelBase `5e5fea8b9117`（改查 debug port 而非 PEB `BeingDebugged` 标志）：这是未处理异常路径上的行为变更。出于反篡改目的设置或清除 `BeingDebugged` 的游戏会走到与当前不同的分支。这需要产品实测证据，而不是源码评审就能定。
- Win32u `3ec143785905`（detached source 的 DPI，Wine-Bug 59970）：上游在 11.0 之后把 monitor DPI helper 改成了分数比值，两处 `DISPLAY_DEVICE_ATTACHED_TO_DESKTOP` 判断需要在 ABI-sensitive 的显示代码里手工改写，且该提交没有测试。
- MFReadWrite `4534f6cd0471`（1:1 像素宽高比）：属新增输出行为而非修复缺陷，且目前没有任何 Runeon 游戏需要变形画面矫正。
- Win32u `a1bae27f21b5`（raw mouse input，Wine-Bug 59986）：修的是 11.0 之后才引入的 raw input 批处理机制的回归。基线没有 `info->raw_mouse` 路径，不先移植那套重构就无从修起。
- WinHTTP `f27d14a21cfe` 与 `93ef2498b391`：两者都在修 `WINHTTP_OPTION_SERVER_CERT_CHAIN_CONTEXT`，而该 option 在本基线中并不存在。
- Win32u `a37867ddf841`（窗口创建请求传初始 monitor DPI）：改动 `server/protocol.def` 与 `include/wine/server_protocol.h`。
- `server` 的 24 个 `init()` object operation 提交：ABI-sensitive 重构，作为一整条链延期。
- MSXML3、WMI .NET utilities、WinRT metadata、JScript、msv1_0 / sspicli / secur32 的 NTLM 与 auth identity 系列、DNS-SD stub、D2D1 sprite batch、D3DX9 相交计算、bcrypt HKDF 与 TLS1 KDF、Shell32 CSIDL 处理、conhost beep 等功能新增。
- 头文件、构建工具、symcrypt 对齐、翻译和调试器改动对本 patch set 没有 runtime 价值。

## 验证与发布边界

静态 manifest 与 SHA-256 检查通过。完整的 46 个补丁按顺序应用到固定的 CrossOver 源码成功，源码标记已写入，二次应用的 fail-closed 门禁成立。

已在打好补丁的源码树上做过一次启用测试的原生 arm64 构建：所有被修改的源文件和所有回移植的测试文件全部编译通过，`SONAME_LIBGNUTLS` 解析为 `libgnutls.30.dylib`，说明补丁 0040 与 0044 是真正生效的代码路径而非空转。上面记录的 `bcrypt` delay import 缺失，也正是这次构建发现的。

该次构建中有两处失败属于环境问题，与本 patch set 无关：`dlls/winemac.drv/cocoa_window.m` 在缺少产品 D3DMetal 配置时看不到 `WineMetalLayer`，`dlls/win32u/vulkan.c` 需要来自 MoltenVK 安装的 `SONAME_LIBVULKAN`。series 中没有任何补丁触及这两个文件。

还有一项验证未完成，必须在 `.6` 推进前由构建门禁关闭：尚未在文档所述的产品配置下（含 D3DMetal 与 MoltenVK 依赖）做过完整的 Rosetta x86_64/WoW64 构建。

源码门禁通过并不等于可以发布。受影响模块在已组装 seed 中的 runtime 测试、干净与已有 prefix 的探针、Steam CEF、stop/relaunch、D3DMetal overlay 以及 DXMT/DXVK smoke，仍必须在 Runeon 产品仓库中通过。

在这些产品门禁通过之前，`.6` 只是源码候选，不得替换 Production 源码条目，也不得对外描述为用户可用。
