# Wine 11.0 至 11.14 稳定性评审

[English](AUDIT-11.0-11.14.md) | [Chinese](AUDIT-11.0-11.14.zh-CN.md) | [Japanese](AUDIT-11.0-11.14.ja.md)

> 英文文档是权威版本；本文件是完整简体中文译本。

## 范围

固定比较范围为 `wine-11.0..wine-11.14`，基线是 SHA-256 为 `ac99c8ca4b3848f3e81784135f023df266b61c2345726ea55a50b3e030dd6872` 的 CrossOver 26.3 源码包。

清单共有 3,143 个上游提交：2,373 个自动 candidate、315 个 subsystem-sensitive、455 个 ABI-sensitive。自动标签只用于分流人工评审，不能证明安全，也不能直接作为纳入依据。

`.2` 采用两轮评审：首先覆盖所有非 ABI 的崩溃、释放后使用、越界、溢出、竞态、死锁、空指针和资源泄漏信号；然后评审 Runeon 实际触达的 runtime、网络、加密、COM、设备、音频、输入、codec、UI 和 launcher 模块中的 134 个正确性提交。

每个入围提交都使用发布脚本同一套 forward/reverse 算法检查，并人工核对 CrossOver 实现、依赖、测试变化和回归面。

## 纳入 `cx26.3-wine11.0-runeon.2`

`.2` 继承 `.1` 的三个 VC14 提交、适配后的 Escape `cfgmgr32` 退出修复和 Runeon CEF 进程补丁，并新增以下 33 个稳定性 backport：

| 首次发布 | 上游提交 | 模块 | 结果 |
| --- | --- | --- | --- |
| 11.1 | `0706ffc06c18` | WindowsCodecs | 正确初始化截断 GIF 调色板 |
| 11.1 | `e33a8cc4ba7d` | QASF | 防护未连接 pin，并带入上游测试 |
| 11.2 | `a0a82c471b09` | QASF | 避免 stopped reader callback 的释放后使用竞态 |
| 11.3 | `56a4347acac1` | CoreAudio | 拒绝零 period，避免除零 |
| 11.3 | `bb4ef1fbf7d8` | RichEdit | 复制文本时限制 tab leader 索引 |
| 11.3 | `45190d46646b` | WindowsCodecs | 修正 interlaced GIF 输出 buffer 边界 |
| 11.3 | `352d6c94fd2a` | Winsock | 正确关闭重复 socket handle，并带入上游测试 |
| 11.12 | `89b35d3f7079` | WindowsCodecs | 保留已填满的 GIF LZW table |
| 11.14 | `af25fb409cec` | WinINet | 防止空 file URL 越界读取，并带入上游测试 |
| 11.1 | `95fa88630ee4` | WindowsCodecs | 修正 64bpp RGBA 至 32bpp BGRA 转换 |
| 11.3 | `5f241ebda259` | UCRT | 修正 scanf scanset range，并带入上游测试 |
| 11.3 | `91905171217d` | Advapi32 | ReportEventA 失败时释放转换 buffer |
| 11.4 | `8ca440beead4` | MSVCRT | 拒绝 `_time32` 溢出 |
| 11.4 | `8845229a35a9` | MSVCP140 | 对齐原生 `_Schedule_chore` callback 行为，并带入上游测试 |
| 11.4 | `a955417b8f7b` | MSVCR | 释放旧 ExceptionPtr 对象 |
| 11.6 | `9c7d674aa96c` | VBScript | 避免 ReDim 空 SAFEARRAY 崩溃，并带入上游测试 |
| 11.7 | `1ed184e29eb9` | D3D11 | decoder 创建失败时释放 video-device interface |
| 11.7 | `8d1c07977dc1` | Script Runtime | 修正 create-folder BSTR 长度 |
| 11.7 | `8f15e858e311` | Quartz | 关闭 filter-graph completion event |
| 11.8 | `08dbf01aaa36` | NSI proxy | 处理 `if_nameindex()` 失败 |
| 11.8 | `965a00c4b479` | NSI proxy | 避免分配零大小 interface 数组 |
| 11.8 | `4781a7128e53` | IP Helper | 无 interface 时避免越界读取 |
| 11.8 | `452d82e997fa` | ADO | 处理 provider 未返回 update status 的情况 |
| 11.10 | `cb3de0f05be4` | WinINet | 释放临时 authorization 数据 |
| 11.12 | `c293e14307e7` | OleView | reload 释放树之前清空 selection |
| 11.12 | `fe4f614de6d5` | Regedit | 按 value size 限制 REG_MULTI_SZ 扫描 |
| 11.13 | `3f4bff72fa8e` | URLMon | 修正 cache path 字符数与 terminator 空间 |
| 11.13 | `632963fc4fbb` | MSHTML | 创建后释放 Gecko attribute |
| 11.13 | `3131eba5b352` | MSHTML | detach 时释放 collection 引用 |
| 11.13 | `8e6d4b91001e` | MSHTML | DISPID 查询后释放 Gecko attribute |
| 11.14 | `349d6af5b55b` | Crypt32 | 校验 OID info size，并带入上游测试 |
| 11.14 | `568aca4e2aad` | GDI+ | 拒绝 point-count 算术溢出 |
| 11.1 | `73e02c1e7f54` | CMD | 限制 MKLINK path；已适配 CrossOver 旧 parser |

每个补丁的权威路径、SHA-256、风险、完整上游 commit、适配说明和顺序都记录在 [`patches/manifest.json`](patches/manifest.json)。

## CrossOver 已等价包含或缺陷不适用

- `00f88cd59bb1`：CrossOver 已包含 `wineboot` 的 macOS 空 canonical name 防护。
- `13a82c7468dd`：CrossOver 已包含 `joy.cpl` 关闭卡死修复。
- `12154421e345`：CrossOver 的 MMDevAPI validator 不会把短 `WAVEFORMATEX` 整体复制为 extensible 结构，并在读取扩展字段前检查 `cbSize`，所以上游 ASan 缺陷不存在。
- `e0663e8a283c`：CrossOver 的 SetupAPI 使用直接 registry query，不走有问题的宽字符 buffer 分配路径。
- `eef8e97dd335`：CrossOver 的 Crypt32 在使用前检查 `cbSize`，且不会在该路径读取后置 exclusive 字段。

## 明确延期，并非静默遗漏

- Media Foundation `f34dda8a56d0` 依赖 CrossOver 尚未具备的 session 控制流和测试前置改动。
- WinHTTP `a48b1ff36b61` 在 CrossOver 中使用不同的 stream 实现，且上游提交没有 in-tree 回归测试；不能只凭 hunk 可应用就纳入。
- SChannel `35b1e7eb9a7e` 的源码 hunk 可应用，但配套测试依赖缺失；本轮不接受只移植源码。
- URI parser `8840f1dae9a7` 需要连同 canonicalization 系列整体评估。
- `cfgmgr32` `b39db986571e` 的上游实现已从 SetupAPI 迁移，旧布局必须补专门语义测试。
- WinMM 泄漏修复和 XInput controller 系列均为有顺序的行为链，需要真实设备重连与音频 session probe。
- 其余 VBScript/MSXML 崩溃修复依赖 parser/interpreter 重构；`.2` 只纳入可独立验证的 ReDim 修复。
- OpenGL 同步及大范围 graphics/media 改动必须放到独立 D3DMetal/DXMT/DXVK 回归批次。
- loader、server、WoW64、winemac、win32u、Unix protocol 和 D3DMetal 接口等 ABI-sensitive 改动继续按策略排除。
- WineAndroid、Wayland、X11、PulseAudio、ALSA、OSS 和仅 BlueZ 的改动不会在 Runeon macOS runtime 执行。

## 验证与发布边界

固定 CrossOver 源码上的 manifest/SHA 静态检查、完整顺序应用、源码标记和二次应用 fail-closed 门禁已通过。完整 Rosetta x86_64/WoW64 源码构建也已通过，其中包括随 backport 带入的上游测试二进制编译。直接从裸构建目录运行受影响 Wine 测试时，测试宿主因缺少 seed runtime 中的 MoltenVK 与 RPC service 环境而停在 Wine 初始化阶段，尚未进入测试用例；这是环境结果，既不能记为测试通过，也不能记为补丁失败。

源码与编译门禁通过仍不等于已经交付。还必须在 Runeon 产品仓库组装后的 seed 内执行受影响运行期测试，并完成干净/既有 prefix、Steam 生命周期、VC14、Escape 退出、音频/网络和图形 smoke。

这些产品门禁完成前，`.2` 只是源码候选，不能替换 Production `.0` 入口，也不能描述成用户已经获得。
