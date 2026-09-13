# Wine 11.16/11.17 定向回移植

[英文](BACKPORTS-11.16-11.17.md) | [简体中文](BACKPORTS-11.16-11.17.zh-CN.md) | [日本語](BACKPORTS-11.16-11.17.ja.md)

> 英文文档是权威版本；简体中文与日语文档均为完整译本。

审查日期：2026-09-13。`cx26.3-wine11.0-runeon.9` 是 Dev 与 Production seed `2026.09.13` 使用的公开正式源码版（2026-09-14 转为正式版），沿用 `.8` 固定的 CrossOver 26.3 / Wine 11.0 archive，保留已有 47 个补丁文件，新增承载 13 个上游提交的 10 个文件：合计 57 个文件，其中 54 个上游 backport、三个未修改的产品补丁。本次增量不改变 ABI 敏感代码或产品策略。

本轮只对选出的 Wine 11.16/11.17 改动做定向审查，不是对 11.15 之后全部 628 个提交的完整审计。完整审计边界仍为 `wine-11.15`；不会为了消除 upstream-watch workflow 的提醒而推进 `upstreamAuditThrough` 或改写历史审计。已交付 runtime 状态继续以 [README](README.zh-CN.md) 为准。

## 候选内容

已对照现有补丁基线检查 CrossOver 等价实现与前置依赖。下列收益描述修复的 API 契约，不代表已复现或修好某款 Runeon 游戏问题；仅在实际使用对应 Wine DLL 时生效。尤其 D3D10 改动针对开放的 Wine 桥接，不修改 D3DMetal 或 DXMT。

| 补丁 | 首个 Wine tag / 上游提交 | 行为 |
|---|---|---|
| [0045](patches/upstream/0045-wine-11.16-dinput-action-device-guid.patch) | `wine-11.16`: [`2a43c4b92910`](https://github.com/wine-mirror/wine/commit/2a43c4b9291083f8892f2100051131a87278e656), [`72ffda26fa71`](https://github.com/wine-mirror/wine/commit/72ffda26fa71d42ba01e6908f39f5972b238e1c0) | 按设备 GUID 和对象 ID 共同匹配 action app data，避免键鼠 action 使用相同 ID 时串用数据。 |
| [0053](patches/upstream/0053-wine-11.16-d3d10-stateblock-disabled-byte-fields.patch) | `wine-11.16`: [`2d98b4fb0ae4`](https://github.com/wine-mirror/wine/commit/2d98b4fb0ae49aa22c735eda6f221da91b4bc3cb), [`a5c665c30fe3`](https://github.com/wine-mirror/wine/commit/a5c665c30fe3d1ba6748680e68e77a564d58a9d4) | Capture/Apply 对所有单字节 state-block mask 字段只检查最低位，让禁用状态保持禁用；不增加 D3D 功能级别。 |
| [0054](patches/upstream/0054-wine-11.17-winhttp-connection-query-buffer-length.patch) | `wine-11.17`: [`df71145f22f1`](https://github.com/wine-mirror/wine/commit/df71145f22f15e723697675e3beaf75094247c6c) | Connection 与 Proxy-Connection 查询分别使用独立 buffer 容量，防止第一个过长字段扩大第二次查询使用的容量。 |
| [0055](patches/upstream/0055-wine-11.17-winhttp-pac-scheme-host-bounds.patch) | `wine-11.17`: [`03f4957cacee`](https://github.com/wine-mirror/wine/commit/03f4957caceeb8404a119f6b8bd8cc0e76dcb76e) | 限制 PAC 小写 hostname buffer，并保护 scheme buffer。当前基线已在更早阶段拒绝过长 scheme；实际可达的修复是 hostname 拷贝。 |
| [0056](patches/upstream/0056-wine-11.17-secur32-schannel-invalid-context.patch) | `wine-11.17`: [`081d8b1dd9ff`](https://github.com/wine-mirror/wine/commit/081d8b1dd9ff4bdd067c54bb50d396e34c334c70) | SChannel 加解密前对缺失或错误类型的上下文返回 SEC_E_INVALID_HANDLE；不解决任意 provider 指针损坏或并发删除。 |
| [0057](patches/upstream/0057-wine-11.17-windowscodecs-nonnull-stream-read-count.patch) | `wine-11.17`: [`111e5197390a`](https://github.com/wine-mirror/wine/commit/111e5197390aa008789b002222024229fa2b82cf) | 始终向 IStream 传入非 NULL 的读取长度指针，并把成功的短读统一为 S_FALSE；覆盖 JPEG 元数据路径及共享 codec helper。 |
| [0058](patches/upstream/0058-wine-11.17-dwrite-ttc-header-read-size.patch) | `wine-11.17`: [`a6fc12e4a94b`](https://github.com/wine-mirror/wine/commit/a6fc12e4a94bf4dae2d5c3a297794107627dad0a) | DirectWrite 字体流请求完整 TTC header，而非指针大小的数据；修复 fragment 边界，不改变文字排版。 |
| [0059](patches/upstream/0059-wine-11.17-d3d10-device-adapter-failure-reference.patch) | `wine-11.17`: [`5ae631904a4b`](https://github.com/wine-mirror/wine/commit/5ae631904a4b4a253e22408e5cfe107c8ebb33b9), [`0e2ee8ab3e8c`](https://github.com/wine-mirror/wine/commit/0e2ee8ab3e8c72dfcd798ba6d963b6f34625989d) | D3D10 与 D3D10.1 创建设备时，只有 GetParent 成功后才增加调用方 adapter 引用，避免失败路径泄漏。 |
| [0060](patches/upstream/0060-wine-11.17-d2d1-geometry-fill-brush-lifetime.patch) | `wine-11.17`: [`4334512575c7`](https://github.com/wine-mirror/wine/commit/4334512575c7be581ae9a2bd9d9d0d11e5038542) | 创建 opacity brush 失败时，让首个克隆 brush 继续归 command list 持有，由 list 销毁时恰好释放一次。 |
| [0061](patches/upstream/0061-wine-11.17-evr-video-window-swapchain-lifetime.patch) | `wine-11.17`: [`5d711b3bdc45`](https://github.com/wine-mirror/wine/commit/5d711b3bdc455addd7a720b553600abc869937be) | EVR 更换视频窗口前清空已释放的 swapchain 指针，使重建失败不留下悬垂所有者；相同窗口的重试行为不变。 |

## 依赖与适配

- `0045` 把 DirectInput 原测试前置和修复合成一个可反向验证的最终 diff；保留原断言与作者，未适配生产代码。
- `0053` 同样包含原 D3D10 测试。唯一额外测试适配是在设备创建失败时明确 skip，避免解引用 NULL 设备。跳过设备测试不等于图形验证通过；生产修复覆盖全部 14 个单字节字段。
- `0059` 合并独立的 D3D10 与 D3D10.1 adapter 修复。所选 backport 均未适配生产语义。其余新增上游提交均未携带原始回归测试；下列独立检查覆盖各自的具体契约，仍需受影响模块测试和产品测试。
- 完整 commit ID、补丁摘要与首个 Wine release 以 [manifest](patches/manifest.json) 为准。源码包保留补丁、原作者信息和[测试源码](tests)。

## 深入复核后暂缓

| 分组 / 上游提交 | 暂缓原因 |
|---|---|
| CoreAudio 周期：`ddefc3d4569f`、`07c7fcc86fe5`、`8e09602ca6e4`、`da0b08472b4b` | 改变真实共享设备 buffer 大小，属性设置失败会使流创建失败。真实设备周期、只读或不支持的设置、不同周期的并发流及设备切换尚需验证。原测试只新增一个可接受的默认周期值；零 fuzz 应用不能覆盖这些风险。 |
| WGI 初始化：`b45d928aa12f` | 提前发送事件允许更早注册 GamepadAdded，但不保证注册时序。Activation 可能在初始手柄列表填充前返回；缺少预连接手柄与热插拔检查，现有事件注册测试不能证明此时序。 |
| XAudio2 错误解锁：`b68ea3b878e2` | 固定的内置 FAudio_CreateSourceVoice 只有成功返回，因此本构建通常无法进入所修复的 HRESULT 失败分支。上游先解锁再释放的顺序还允许 voice 复用时对 effect_chain 发生竞态；待 FAudio 错误行为改变后，以真实失败探针重新评估。 |
| MF 排空：`862453b70420` | 末级逐帧输出仅在 source_reader_get_read_result 继续排空，但 SOURCE_READER_ASYNC_SAMPLE_READY 直接取出响应。EOS 时已等待的异步读取可能消费唯一排队样本，将剩余解码帧留在 transform 内，下一次读取直接报告 EOS。这是源码路径反例，并非运行复现，需要可控的多帧排空测试；不依赖下方暂缓的 async Release 修改。 |
| 未发布的 MF 生命周期/seek：`2f73d9efc2a0`、`5fc7c7958be1`、`7edd71a8d76c` | async command 初始 refcount 为零时，直接把 Release 改为递减会使 queue AddRef 后的调用方 Release 释放仍在队列中的对象。初始所有权、队列提交失败、read/seek/flush 生命周期需要一起修正；提前设置 SEEKING 也需复核失败清理。本候选不纳入这些改动。 |

这些决定不意味着上游修复普遍无效，而是针对固定实现和本候选可用的验证证据。更广的 macOS 窗口驱动/ABI 改动及整体 Wine 升级不属于本次增量。

## 验证状态

| 检查 | 当前结果 |
|---|---|
| 固定源码重放与保留 | 全部 57 个补丁文件从固定 archive 零 fuzz 重放通过，11,104 个源码文件比对一致；此前 47 个补丁文件及其 manifest 条目未改变。 |
| 完整 x86_64/WoW64 构建 | `configure`、`make`、`make install` 均通过。 |
| 原上游测试编译 | DirectInput 与 D3D10 测试可执行文件均通过 x64、x86 编译，共四个；完整上游 suite 的执行仍待完成。 |
| 独立 Windows API 执行 | 在隔离 prefix 中，x64 与 x86 均通过：SChannel 11、adapter 6、stateblock 32、DirectWrite 6、WIC 11、DirectInput 4 条断言，每架构合计 70 条。WIC 实际查询 APP1/Exif 值，并触达两处四字节元数据读取。 |
| 旧 DLL API 对照 | 同一组 70 条检查使用 `.8` DLL 时报告 19 条失败：SChannel 6、adapter 2、stateblock 4、DirectWrite 2、WIC 3、DirectInput 2。这是在单独 runtime 中替换八个 DLL、保持与候选相同 core 和依赖的对照，并非完整旧版产品 smoke。 |
| WinHTTP 执行 | 候选 x64、x86 的 `headers` 与 `pac` 模式均通过。Header 含 100 个 A 和 40 个 B，后续 body 与 EOF 符合预期；正常 PAC hostname 返回 DIRECT，1,093 字符 hostname 被拒绝。Fixture 请求在 loopback 中确认，无外部流量。 |
| 旧 DLL WinHTTP 对照 | `.8` DLL 的过长 header 和 PAC 路径达到目标输入后复现栈损坏 page fault。Header 对照先确认长字段，PAC 对照先通过正常 hostname；两者进入 fault/debugger 处理，均需 25 秒监督超时并清理隔离 server/debugger。没有将其记录为普通断言失败或正常的崩溃退出码。候选在这些探针中消除了故障，不代表游戏级修复证明。 |
| Direct2D/EVR 所有权故障注入 | [lifetime-check.py](tests/lifetime-check.py) 提取两个真实函数，配依赖桩编译执行：候选 51 条检查通过；`.8` 有六条预期失败与两次安全捕获的重复释放。覆盖创建/替换失败、恢复、成功及销毁。这是函数级验证，不是 Wine 或 GPU 执行。 |
| WIC 读取结果故障注入 | [stream-read-check.py](tests/stream-read-check.py) 执行九个场景、38 条断言：候选通过，`.8` 有 12 条预期失败。覆盖 NULL/非 NULL count、完整/短读、S_FALSE 规范化，以及保持 E_FAIL 且不读取未写 count。这是函数级验证，不是真实 codec 执行。 |
| 新依赖构建复测 | Open Wine `.9` 使用 MoltenVK `1.4.2` 完整重编，同时保持 GnuTLS/GStreamer 依赖版本不变；通过同一组每架构 70 条 API 断言及全部四项 x64/x86 headers/PAC 检查。追加 PAC trace 确认一次下载后复用脚本缓存，并进入长 hostname 路径。这些检查不证明游戏兼容性或 MoltenVK 图形能力。 |
| 完整受影响模块 suite 与产品回归 | 完整 suite 执行、产品已有 prefix 检查、Steam CEF/停止重启及 D3DMetal/DXMT/DXVK 游戏 smoke 仍待完成，隔离 API prefix 的结果不能替代这些检查。 |
| 源码资产与 runtime 分发 | Dev 与 Production seed `2026.09.13` 均使用 `.9` 与 MoltenVK `1.4.2`，App 门槛为 `1.8 (0)`。Production 复用在 Dev 验证过的同一份签名归档。构建、API 回归、签名、readiness、鉴权 feed/ticket 与完整下载验证已完成；用户已确认当前 Xcode Dev 源码版的组件更新、runtime 准备和 Steam 启动。分发已覆盖 `1.8 (0～3)`；没有逐 build 重跑游戏、停止重启交互与全新 prefix 验收。已发布 App 安装包保持 `1.8 (3)`。源码 Release 已正式发布，tag、commit 及四个附件的字节、大小、摘要均未变；`.8` / seed `2026.09.06` 保留为历史。 |

可重复检查命令及受管理 runtime 的启动边界见 [BUILDING](BUILDING.zh-CN.md)。编译上游测试不等于执行测试；API 与函数级通过不能替代 [MAINTENANCE](MAINTENANCE.zh-CN.md) 中尚未完成的产品发布门槛。只有取得对应证据后才能更新状态。
