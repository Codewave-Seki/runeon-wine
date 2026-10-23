# Wine 11.20 用户态修复回移

[English](BACKPORTS-11.20-SWEEP.md) | [简体中文](BACKPORTS-11.20-SWEEP.zh-CN.md) | [日本語](BACKPORTS-11.20-SWEEP.ja.md)

> 以英文文档为准，中文与日文为完整译文。

候选补丁集 `cx26.3-wine11.0-runeon.23` 在 `0134` 之后加入 `patches/upstream/0135-wine-11.20-targeted-fixes.patch`。它合并了 34 个 WineHQ 提交：32 个首次发布于 `wine-11.20`，另有 2 个 `wine-11.12` 的 msvcp60 修复，是其中一个提交的前置。补丁逐一列出每个提交及作者，manifest 记录完整提交 ID。只改测试的部分不包含在内。

## 选择

按[上一次回移](BACKPORTS-11.19-SWEEP.zh-CN.md)的规则，审查了 `wine-11.19..wine-11.20` 的全部 342 个提交。对照的树是固定的 CrossOver 归档加上补丁 `0001`–`0134`：

- 50 个提交只改测试。
- 80 个提交只改 `ntdll`、`server`、`win32u`、`winemac.drv`、loader、`include`、构建工具或翻译。大多是功能性改动：内存保留区调整、EGL 与 D3DKMT、ARM64EC relay 支持、卷卸载请求。这些提交逐个读过，其中两个小缺陷经确认基线同样存在，予以纳入（见下文），其余不纳入。
- 212 个用户态候选中，只取 Steam、启动器、安装程序或游戏会加载的 DLL 里的崩溃、内存安全、泄漏、卡死与错误路径修复。不纳入的有：功能系列（quartz 颜色转换器、msxml3 的 DTD 校验、dmime 循环重写、`wminet_utils`、dsound 混音、`winebus` 设备身份、jscript/mshtml 循环回收、vbscript 解析器改动、gdiplus 抗锯齿）、版本资源号调整，以及 `cmd` 的 word iterator 重构。

纳入的每个提交都能按上游顺序应用到这棵树上，说明基线仍带着未修复的代码。

## 纳入

| 模块 | 提交 | 修复的缺陷 |
|---|---|---|
| msvcp60 | `5e5ca9fefc`、`076438fa7c`（11.12）、`794feb9289` | 宽字符串的 `replace` 按 `sizeof(char)` 移动和填充数据，会破坏字符串；窄字符和宽字符的 `replace` 在结果为空时，还会通过 NULL 缓冲区写结尾符。 |
| msvcp60、msvcp90 | `3d27b08dde`、`c54fd1eac5`、`c9173a6c5d`、`fb997bf426`、`ad23bc1551` | 流没有缓冲区时，`operator<<`／`operator>>` 直接崩溃，而不是设置 bad 位。msvcp60 的 `narrow`／`widen` 从流缓冲区取 locale，所以 `std::endl` 在那里仍会崩溃；现在改用流自身的 locale，与 msvcp90 已有的做法一致。 |
| msvcp90 | `f011c203a0` | 区域设置名无效时，抛出异常前释放了一个从未分配过的指针。 |
| msvcp90 | `f0fef9fef7` | `_Strxfrm` 把排序键长度当作源长度传入，导致越界读源字符串。 |
| msvcp90 | `00469cf5cf` | `codecvt<char16_t>` 对 U+10000 生成了错误的代理对。 |
| msvcp60 | `0740f54989` | `mbsrtowcs` 不推进调用方的源指针。 |
| vcruntime140_1 | `86b80d02b6` | C++ 异常状态只在 `DLL_THREAD_ATTACH` 中初始化，所以 DLL 加载前就已创建的线程在展开时 try level 错误。 |
| winegstreamer | `73cc740a01`、`d7c49769d9`、`33fba6fc26`、`52f8e6b5a5`、`774cb470c5`、`90e98d3d94` | drain／flush 时解引用 NULL transform；媒体类型转换失败却报告成功，导致 transform 未被设置；忽略 WMA 输出类型的设置失败；finalize 时泄漏异步结果；使用未初始化的流数量；未检查内存分配。 |
| d3d8 | `6f14a8ea45` | 释放引用计数已为 0 的纹理时，计数会回绕，而 Windows 返回 0。推断的后果：「一直释放到 0」的循环不会结束。 |
| d3dx10、d3dx11 | `547e01858d`、`16c13088f9` | 显式指定的宽高没有向上对齐到压缩格式的块大小，导致纹理创建失败。 |
| dinput | `9eb4614ad7` | 在另一个模块的 `DllMain` 中停止输入线程时，会等待该线程退出，而 loader lock 使它无法退出，于是游戏退出时卡死。 |
| wininet | `f564234cb2`、`436ec312c3` | 查询选项时解引用 NULL 的 size 指针（Wine-Bug 58418）。 |
| crypt32 | `bf9b5e567c` | 时间无效时在 `__TRY` 块内直接返回，异常帧留在了链上。 |
| dmband | `146fbbe78b` | 没有 band 的 band item 会把未初始化的指针加入轨道。 |
| dmusic | `43b83b580f` | 转换乐器名时，把字节数当作 WCHAR 容量传入。 |
| winmm | `0e7224e852` | `midiStreamOpen` 失败时释放了 stream，却没有把它从 stream 树中移除，之后的查找会读到已释放的内存。 |
| hnetcfg | `baa7c5d335` | 创建 `NetFwPolicy2` 失败时，返回了已释放的对象。Steam 的 webhelper 会用到这个 DLL，Runeon 的 Steam 日志里有它的 `fwpolicy2` FIXME。 |
| wintypes | `07c33a0e92` | 清除 map 条目时调用了 AddRef，而不是 Release。 |
| kernelbase | `0875da63b7` | 错误路径：`UpdateProcThreadAttribute` 对 `PROC_THREAD_ATTRIBUTE_DESKTOP_APP_POLICY` 返回失败，而 Windows 接受该属性。`CreateProcess` 仍会记录 FIXME 并忽略它。目前没有已知需要它的启动器。 |
| ntdll | `81e52628b2` | `RtlWakeAddressSingle`／`RtlWakeAddressAll` 在移除队列节点之前，就以无内存序保证的方式公开了「已出队」状态（`entry->addr = NULL`）。因超时或误唤醒醒来的等待方不加锁读到清空的地址后直接返回，唤醒方随后的 `list_remove` 与读取 `entry->tid` 就落在已失效的栈帧上，可能破坏等待队列（丢失唤醒、卡死或崩溃），或唤醒错误的线程。SRW 锁、条件变量和 `WaitOnAddress` 都建立在这两个函数之上。 |
| winemac.drv | `48e2d50404` | `macdrv_GetWindowStyleMasks()` 清零的是 `ex_style` 参数而不是输出掩码，随后又往未初始化的掩码上按位或。 |

`msvcp90/ios.c` 与 `msvcp90/locale.c` 也编进 `msvcp100`–`msvcp140` 的 builtin，所以表中 msvcp90 的各行同样适用于它们。

`81e52628b2` 与 `48e2d50404` 属于本类回移通常不纳入的范围。两者都是小修复，且缺陷在基线中存在：CrossOver 的 `RtlWaitOnAddress` 与 `macdrv_GetWindowStyleMasks()` 和上游修复前的代码一致。

## 审查后不纳入

| 提交 | 原因 |
|---|---|
| `365e610147` ddraw | 修复了 `dwVertexCount` 未初始化导致的崩溃，但做法是把 execute buffer 的暂存从 `SetExecuteData` 挪到每次 `Execute`，并复制整个缓冲区。这改变了所有使用 execute buffer 的游戏的行为，不是局部修复。 |
| `8ab37b5ee3` user32 | 为了与 Windows 一致，让 `CharToOemBuffW` 最多写入两倍字节数。这改变了写入调用方缓冲区的长度，也不修复崩溃。 |
| `bce98257e7`、`264434d93d` dmusic | 让 band 解析在原本会继续的情况下按 Windows 的行为失败，属于行为变更。 |
| `d26a2d2deb`、`331cb04791` winegstreamer、`f6c606ea2c` dmusic、`70aeffabea` fusion | 只是删除无效赋值或换用等价宏，行为不变。 |
| `3d6505bfa0`、`66f0fbf9aa`、`5e0589a44f`、`30a0d6cf0d` bcrypt | 修的是基线没有的 SymCrypt RSA 路径；基线使用 GnuTLS。 |
| `4e02394aa1` comsvcs | 基线的 `holder_AllocResource` 中没有这个泄漏的分配。 |
| `adb2b0d59a` cmd | `cmd` 解析出错时的泄漏，依赖未纳入的 word iterator 重构。 |
| `eba89375a0` ntdll | 改变了 ntdll 与 server 之间的线程亲和性协议。 |
| `1fa894d7a4`、`ba392e9016`、`505f126cc3` | 新增的桩函数与导出，目前没有已知的 Steam 游戏需要；第一个是 Microsoft 远程调试器需要的。 |

## 已知风险

`9eb4614ad7` 让 dinput 改为等待输入线程退出前设置的事件，而不再等待线程句柄。如果输入线程已经从「意外终止」路径退出，停止消息送不到，这次等待就永远不会返回；改动前，这种情况会立即返回。上游行为相同。该路径很罕见，所以原样纳入，并在此记录风险。

`48e2d50404` 让 `ex_style_mask` 从未初始化值变为确定值，会改变带装饰窗口的可见区域计算。因此运行时检查包括带标题栏窗口（例如 Steam）的几何是否正常。

## 适配

| 提交 | 适配 |
|---|---|
| `146fbbe78b` dmusic | 应用到 `dlls/dmband/bandtrack.c`。上游在 `220b4e7ff6` 中把 band track 移到了 dmusic，那个提交未纳入；除此之外，基线代码与上游一致。 |
| `48e2d50404` winemac.drv | 同样的一行修改。在基线中它位于 CrossOver 的 Quicken hack 之后，所以 hunk 有偏移。 |

## 验证

`.23` 的 `scripts/static-check.sh` 与 `scripts/integration-check.sh` 通过。除两个适配的提交外，所有提交都能按顺序无 fuzz 应用。完整构建与运行时检查记录在 Runeon 任务列表的 T948 条目中，`.23` 打 tag 前必须完成。

`upstreamAuditThrough` 保持 `wine-11.15`。基线 manifest 记录 `upstreamSweepThrough: wine-11.20`。
