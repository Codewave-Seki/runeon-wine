# Wine 11.1 至 11.19 兼容性 stub 审查

[English](BACKPORTS-11.1-11.19-STUBS.md) | [简体中文](BACKPORTS-11.1-11.19-STUBS.zh-CN.md) | [日本語](BACKPORTS-11.1-11.19-STUBS.ja.md)

> 以英文文档为准，中文与日文为完整译文。

候选补丁集 `cx26.3-wine11.0-runeon.18` 以 `.17` 为起点，加入 `patches/upstream/0081` 至 `0087`。此前对这一范围的审查只挑选崩溃、内存安全和错误路径修复，功能性工作不纳入。这也漏掉了一类小提交：把返回失败的 stub 改为返回成功。把这种失败当作致命错误的应用会就此停下，因此本次单独审查这一类。

## 选择

对 `wine-11.0..wine-11.19` 中的两组提交，在已打补丁的 `.17` 源码树上用正向与反向应用逐个审查：

- 标题为模块名加「Stub」的 18 个提交；
- 提交说明写明哪个应用需要它（「Needed by」「Needed for」）的 8 个提交。

## 已采用

| 补丁 | 上游提交 | 变化 |
| --- | --- | --- |
| `0081` | `f19a7df7c0`（11.1） | `IUISettings2::get_TextScaleFactor()` 返回 1.0，而不是 `E_NOTIMPL`。C++/WinRT 会把 `E_NOTIMPL` 转为异常，未捕获时进程在启动阶段结束。 |
| `0082` | `0b0bef31ea`（11.14） | `IUISettings2::add_TextScaleFactorChanged()` 返回同一文件中其他事件 stub 已在使用的占位 cookie。 |
| `0083` | `caba5e68e2`（11.9） | WMI `Win32_DiskDrive` 增加 `Signature` 与 `TotalHeads`。 |
| `0084` | `2873b4d077`（11.5） | 带参数调用命名项对象时，VBScript 调用其默认属性（Wine-Bug 53889）。 |
| `0085` | `d30dcd75bc`（11.14） | 实现 `PdhGetFormattedCounterArrayA/W`，不再是 spec stub。 |
| `0086` | `fbd19baa4f`、`20e2458ab3`（11.16） | JScript 接受 Unicode 标识符字符。第一个提交是第二个所依赖的重构。 |
| `0087` | `22aedfab35`、`3864ce7fcb`、`cefdd9ebe6`、`d95e15dfe8`、`44b0da29ad`、`88ceeada6d`（11.4 至 11.15） | WinHTTP 接受四个较新的选项并回答两个查询，不再以 `ERROR_WINHTTP_INVALID_OPTION` 失败。 |

`0081` 至 `0086` 原样采用上游代码，省略上游测试改动。`0087` 有改编：

- 会话选项放在 WebSocket 缓冲区选项之后，因为底版本没有上游中它们所跟随的 `WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL` 会话分支。
- `WINHTTP_OPTION_HTTP_VERSION` 按 `sizeof(HTTP_VERSION_INFO)` 校验并报告长度。上游（`wine-11.19` 中仍是如此）用的是指针大小，32 位调用方可传入 4 字节缓冲区，随后被写入 8 字节。
- `WINHTTP_OPTION_DECOMPRESSION` 与 `WINHTTP_OPTION_ENABLE_HTTP2_PLUS_CLIENT_CERT` 在读取缓冲区前先检查长度，与其他布尔选项一致。上游会读取以零长度传入的 NULL 缓冲区。
- 底版本没有 WinHTTP 解压。接受该选项不会请求压缩响应，因此响应保持未压缩。

## 未采用

| 提交 | 原因 |
| --- | --- |
| `3f65f20cc2` ntdll，`339be36e64`、`94e5bbeb6f`、`2be7717684` win32u，`5b5e95719d` mountmgr.sys | ABI 敏感模块，这类审查不改动。 |
| `8571f65844` ucrtbase，`5c281ca2b2` powrprof | 本系列较早的补丁（`0063`）已包含。 |
| `a33fbd48a0` wminet_utils | 底版本没有 `wminet_utils` 模块。 |
| `043bc5a0d4` windows.web | 新的 `JsonArray` 类，没有写明使用者；解析在之后的功能提交中才加入。 |
| `fff6090795` webservices | 由远程调试器需要，不是本运行时面向的应用。 |
| `ccdaa2b581`、`a164854c92` wined3d | 只改变 Vulkan 验证层消息和一项链接依赖。 |

## 验证

`.18` 通过 `scripts/static-check.sh` 与 `scripts/integration-check.sh`，默认定义仍通过 `scripts/static-check.sh`。这些检查证明补丁序列可以应用；`.18` 尚未构建，i386 与 x86_64 的编译仍待证明。
