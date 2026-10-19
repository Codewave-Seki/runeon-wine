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

当前正式源码：[`cx26.3-wine11.0-runeon.21`](patchsets/cx26.3-wine11.0-runeon.21/manifest.json)（正式版/latest），搭配 seed `2026.10.18`（App `>=1.13 (10)`）、MoltenVK `1.4.2` 与 SDL2 `2.32.10`。保留 `.20`，新增 `0121`（把 `0112` 的栈上界检查扩展到 x64 处理函数查找的两个 collided unwind 分支）、`0122`（mmdevapi Unix 调用失败时在断言前记录驱动名、call code 与 status）与 `0123`（异步 I/O 状态块所在页已不可写时跳过写回）。`0121` 与 `0123` 无需开关即生效，作为逐项批准的例外列在 MAINTENANCE 中；`0123` 是有限防护，检查之后才释放的内存仍可能触发故障。已通过 x86_64/WoW64 完整构建、静态与集成检查，以及本地与 Dev 验收；Production 使用 Dev 已验证的同一份签名归档。这些结果不代表普遍的游戏兼容性。不可变附件记录在[发布清单](release-manifests/cx26.3-wine11.0-runeon.21.source-archive.json)。

上一正式源码，已由 `.21` 接替，保留用于回滚：[`cx26.3-wine11.0-runeon.20`](patchsets/cx26.3-wine11.0-runeon.20/manifest.json)（正式版），对应 seed `2026.10.16.1`，该 seed 已不再分发。保留 `.19` 并新增 `0089`、`0090` 与 `0120`。不可变附件记录在[发布清单](release-manifests/cx26.3-wine11.0-runeon.20.source-archive.json)。

`cx26.3-wine11.0-runeon.19`（Pre-release）曾是 Dev seed `2026.10.16` 的 Dev 候选，已由 `.20` 接替，未用于 Production。不可变附件记录在[发布清单](release-manifests/cx26.3-wine11.0-runeon.19.source-archive.json)。

保留的正式源码，仍分发给 App `>=1.12 (6)`、`1.13 (10)` 之前：[`cx26.3-wine11.0-runeon.18`](patchsets/cx26.3-wine11.0-runeon.18/manifest.json)（正式版），对应 seed `2026.10.14`。保留 `.17` 并新增 `0081`–`0087` 与 `0118`。不可变附件记录在[发布清单](release-manifests/cx26.3-wine11.0-runeon.18.source-archive.json)。

`cx26.3-wine11.0-runeon.17`（正式版）对应 seed `2026.10.10`（App `>=1.10 (3)`，`1.12 (6)` 之前），保留用于回滚；它在 `.16` 上新增 `0112`–`0117`。

`cx26.3-wine11.0-runeon.16`（正式版）对应 seed `2026.10.06`（App `>=1.10 (0)`），保留用于回滚；它在 `.15` 上新增可选的 Steam downlevel D3D12 准备入口（`0111`）。

`cx26.3-wine11.0-runeon.15`（正式版）对应 seed `2026.10.04`（App `>=1.10 (0)`），保留用于回滚；它在 `.14` 之上新增 dwrite emoji 系统回退（`0110`）与 `wine-11.19` 定向修复（`0079`）。

`cx26.3-wine11.0-runeon.14`（正式版）对应 seed `2026.10.02`（App `>=1.10 (0)`），保留用于回滚；它在 `.13` 之上新增按进程选择图形后端（`0109`）。

较早的 App（`>=1.8 (0)`）继续使用 [`cx26.3-wine11.0-runeon.13`](patchsets/cx26.3-wine11.0-runeon.13/manifest.json)（正式版）对应的 seed `2026.09.28.1`。`.13` 在 `.11` 基础上新增上游 `RtlVirtualUnwind2` 空输出修复、另外 12 个 ntdll 修复与可选的 Rosetta 自修改代码开关（`0107`、`0108`），用户于 2026-09-28 授权 Production。seed `2026.09.28.1` 以同一份 `.13` 源码启用 SDL 重新构建，并随附 SDL2 `2.32.10`；真实手柄验收仍待进行。`.12`（seed `2026.09.27`）是已被取代的 Dev 候选，保持不变保留。

`.11` 历史交付（2026-09-26，保留回滚）：[`cx26.3-wine11.0-runeon.11`](BACKPORTS-11.16-11.18-SWEEP.zh-CN.md)（正式版/latest），对应 seed `2026.09.26`，App 门槛 `>=1.8 (0)`。在 `.10` 基础上新增 Wine 11.16–11.18 筛选回移、winegstreamer 视频缓冲池回退与两个可选入口。构建、签名、readiness 与 Dev/Production 分发检查已通过；用户于 2026-09-26 反馈 Dev 未发现问题并授权 Production。Production promote 同一份签名 Dev 归档，不重新构建。

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

## 不需要开关的产品补丁

Runeon 产品补丁默认必须开启后才生效，见 `CLAUDE.md`。只有维护者逐项批准、并在本表或其 manifest 条目中写明原因与边界的具体修复，才可以不经开关生效。批准只覆盖该补丁本身，不构成类似改动的先例。目前不经开关生效的补丁如下：

| 补丁 | 不设开关的原因 | 边界 |
|---|---|---|
| `0100` | 中间件修复：所列 Steam 与 Rockstar 的 CEF 进程需要这些参数。 | 按中间件进程名匹配，从不匹配游戏。Steam webhelper 与 SocialClubHelper 只在主进程追加 `--in-process-gpu`；SocialClubHelper 的 SwiftShader 开关追加到每个 SocialClubHelper 进程。 |
| `0101` | 产品受管 runtime 的新会话必须通过启动校验；缺少凭证或校验失败时，服务器拒绝启动，不回退为未校验启动。 | 只在定义了 `RUNEON_MANAGED_LAUNCH_VALIDATION` 的 macOS 构建中生效（任何值都算，包括 0）；在新 wineserver 启动时校验，已运行会话中的每个子进程不再重复校验。未定义该宏或非 macOS 构建时，检查直接放行。 |
| `0103` | 补上 Windows 提供而 Wine 缺少的 `FlsGetValue2` 导出。 | 只新增导出；last error 与 FLS 存储行为不变。 |
| `0106` | winegstreamer 无法描述分配 caps 时拒绝提供视频缓冲池，而不是提供大小为零的池。 | 只限这种情况；分配交给 GStreamer。 |
| `0110` | 在 dwrite 系统回退中，把 Windows 用 Segoe UI Emoji 绘制的字符映射到该字体族。 | 回退表条目；另外，某个映射没有可用字体时，只上报共享该映射的那一段。没有 Segoe UI Emoji 字体时，每个字符的解析与以前相同。 |
| `0112`、`0121` | x64 处理函数查找在展开读到 StackBase 之外前结束，与现有建立帧检查对栈外帧的处理一致。 | 查找以 `EXCEPTION_STACK_INVALID` 结束；分发的其他行为不变。 |
| `0115`、`0116` | 代码页 54936（GB18030）：Windows 以代码实现而 Wine 缺少；msvcrt 的 `setlocale()` 与 `_setmbcp()` 仍像以前一样拒绝它。 | 只涉及代码页 54936，包括 `IsValidCodePage()` 与 `GetCPInfo()` 现在接受它；其他代码页不变。 |
| `0120` | 构建选项：dsound 以 SSE 浮点数学编译。 | 只涉及这一个模块：i386 构建像 x64 构建一样以单精度混音，并使用 SSE 重采样循环。 |
| `0122` | 在现有断言之前记录失败的 mmdevapi 驱动调用。 | 只做诊断；断言不变。 |
| `0123` | 异步 I/O 状态块不再可写时，防止嵌套故障的有限防护；它只替换会终止线程或让等待失败的故障。 | 只跳过检查时观察到不可写的状态块；写入之前或写入期间被释放、取消提交或修改保护仍会故障。guard 页被跳过且保持有效。等待中的完成现在会完成，而不是让等待失败。Windows 的行为未实测。 |
| `0124` | 把 macOS 任务的内存占用（footprint）报告为提交量（`PagefileUsage`，以及经 `ProcessVmCounters` 返回的 `PrivateUsage`），与 Windows 对该计数的定义一致，而不是报告地址空间。 | 只改这个计数及其峰值；内核提供时（rev3）峰值取内核记录的占用最高值，否则取当前值。`VirtualSize` 与工作集不变；没有 `TASK_VM_INFO`、查询失败或回答早于 rev1 时保留原值。footprint 是 Windows 私有提交量的近似值，不完全相等。 |
| `0125` | macOS 钥匙串中没有匹配项时，带过滤条件的 `CredEnumerateW()` 仍返回注册表中匹配的凭据，与 Windows 返回已存凭据的行为一致。 | 只把宿主凭据库返回的 `ERROR_NOT_FOUND` 与原有的 `ERROR_NOT_SUPPORTED` 同样处理；任何地方都没有匹配时仍返回 `ERROR_NOT_FOUND`。 |
| `0126` | 让签名的 app bundle 可以只包含 loader（macOS 要据此才会给 Wine 程序开启游戏模式）：loader 先在启动它的软链接旁加载 `ntdll.so`。 | 只在 loader 解析后的目录是 app bundle 的 `Contents/MacOS` 时生效，打包布局就是开关；其他布局以及查找失败时走原有路径。设置了 `WINEDLLPATH` 且没有 preloader 时，CrossOver Hack 22144 会经 bundle 外的硬链接启动子进程，它们能运行，但没有游戏模式，也没有 `0127` 的程序名。 |
| `0127` | 在 `0126` 布局下保留按程序显示的应用菜单名（CrossOver Hack 13438）：AppKit 启动前，把 loader 内嵌 `Info.plist` 中的名称复制到主 bundle 的 info 字典。 | 只在主 bundle 是 app bundle 且能读到内嵌副本时生效；其余不变。依赖该 info 字典实际可写，这一点 macOS 没有文档保证；若以后变化，调用会直接 abort 而不是失败返回。 |
| `0128` | 按 WAVEFORMATEXTENSIBLE 定义的声道顺序分配空间音频静态对象，7.1 或 7.1.4 声道不再互换侧置与后置声道，也不再丢掉后中置。 | 只改静态对象的声道映射。 |
| `0129` | 把渲染流的 HAL I/O 缓冲限制在半个周期以内，即共享模式客户端被唤醒的 10 ms 周期之内，预备约一个周期数据的程序不再有部分被播成静音。 | 只影响缓冲大于半个周期的渲染流。缓冲按进程、按设备生效，同一进程在兼具输出的设备上的采集流也会随之变小；多个渲染流时以最后一个为准。半个周期是经验值，回调频率加倍。读写该属性失败时保持不变。WineHQ 11.16 改设为整个周期；以后回移那组提交时以它取代本补丁。 |
| `0130` | 在不支持 GL_ARB_ES2_compatibility 的上下文里也提供它的五个函数，与 Windows 驱动在任何上下文都提供一致。 | 只影响原本返回空的函数；支持该扩展的上下文会用到同样的等价实现，而 macOS 不列出任何二进制着色器格式。 |
| `0131` | 在 macOS 上报告每个运行中线程自己的 CPU 时间，与 Windows 一致，不再是 0 或整个进程的时间。 | 只限调用进程内尚未退出的线程；已退出的线程仍报 0（Windows 报其最终值），查询其他进程仍与以前一样失败。 |
| `0132` | 从注册表回答应用包查询，并在 `wine.inf` 中登记 VCLibs 框架包，与 Windows 已安装这些包一致。 | 只涉及三个查询函数与 VCLibs 条目；不覆盖已有值。 |
| `0133` | 在引擎锁之外调用应用的媒体引擎回调，仍在产生事件的线程上、在该调用返回前送达，与 Windows 一致，回调不会再与引擎互相死锁。 | 只改事件派发；每个线程内的顺序与时机不变。`Shutdown()` 之后的事件被丢弃。同一线程在回调里产生的事件会先于外层调用剩余的事件送达。 |

## 发布规则

Patch set 使用不可变 ID，例如 `cx26.3-wine11.0-runeon.1`。Git tag、source bundle、runtime component metadata 和 Runeon release 文档必须引用同一个 ID。新 artifact 必须先发布 Dev，并完成 readiness、download 和 product smoke；Production 只能 promote 已验证的精确字节，不能为 Production 直接重建。

仓库与 bundle 保持公开。生成候选 tag 后，使用 `build-source-archive-manifest.sh` 记录公开 Release URL、asset SHA/size 和 `prerelease` 状态，再上传 patch-set bundle、完整对应源码和两份 `.sha256`。只有 matching runtime 真正进入 Production 后，才能把同一个不可变 Release 与 manifest 改为 `stable`。历史 tag 和 assets 永久保留，不得移动、替换或删除。

公开仓库只包含 LGPL Wine/CrossOver Wine lineage、补丁与构建脚本。Runeon App、用户诊断、Steam/游戏文件、Developer ID 或服务密钥，以及 D3DMetal/GPTK/Apple 私有组件绝不能进入 Git 历史、Actions artifact 或 Release asset。每次公开更新前都要检查完整历史、Release asset 列表和 Actions 日志。
