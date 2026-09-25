# 可选 DXGI 驱动版本修正

[English](DXGI-DRIVER-VERSION.md) | [简体中文](DXGI-DRIVER-VERSION.zh-CN.md) | [日本語](DXGI-DRIVER-VERSION.ja.md)

候选 `.11` 的 `0105-dxgi-driver-version.patch` 独立实现 LGPL-2.1-or-later 的 `runeondxgi.dll`，使用公开 COM/SetupAPI 与 Wine GPU LUID 属性，未引入无授权的外部 shim。

仅修正 `CheckInterfaceSupport` 成功且版本为 -1 的结果，要求唯一精确匹配 LUID，并按 DXGI `VendorId` 选择固定 Wine `win32u::driver_vendor_to_version` 表项。其它版本、HRESULT、空输出、歧义或未知设备保留原行为。按 LUID/厂商的有界进程级缓存避免重复枚举设备。工厂返回原 COM 对象；虚表登记有锁，模块先固定生命周期，修改方法指针时保留页面执行权限。

**产品默认关闭此实验性修正。** 本机持久偏好可以显式启用；关闭会恢复 prefix 原始 DXGI，后续准备不会重新启用。本机 key 为 `DXGIDriverVersionCorrectionEnabled`（缺失或 false 表示关闭），不再读取未发布的负向 key。Intel `8086`、AMD `1002`、NVIDIA `10de` 分别使用 `35.0.101.6314`、`35.0.21025.1024`、`35.0.15.6094`，未知厂商保留 -1。这些仍是 Wine 合成的兼容版本，不是真实驱动或软件包版本，目标游戏效果尚未验证。

App 不替换共享 runtime 的 `dxgi.dll`，仅在指定 prefix 安装 wrapper/provider 对。wrapper 保留 `runeondxgi.dll` 导出库身份，让 Wine 解析到可选 builtin。准备前检查归属、目标与必需源文件；既有 prefix 事务先发布依赖、再发布 wrapper。wineboot 初始化或修复可能覆盖 wrapper，因此 App 只在显式启用时于游戏启动策略准备前重应用。关闭时首装不预写 DLL、启动不重新同步；同步未提供新指纹时保留同一 runtime 的已有指纹，避免误清 Steam 网页缓存。旧的未发布全局方案从保存的原件恢复。

用户本机的 Apple provider 改名为 `rxdg.dll`，使用对应的本地 unix 别名。模块附加时就加载原 provider，保留在调用方创建 D3D 设备之前完成的原初始化时序。加载失败时尝试裸模块名并输出明确调试消息；本机 runtime 的裸名回退未解决 prefix provider 缺失，因此安全边界不依赖该回退。在 loader lock 内加载是 Wine 兼容约束，并非一般安全的 DllMain 用法。T798 在 Apple 4.0b2 / M3 Max 上验证了顺序工厂和设备优先初始化；provider 若启动线程并等待加载器，仍可能死锁。启用前目标游戏验收必须包含多线程创建设备。seed 排除私有 provider 与备份；本仓库不包含 Apple 私有二进制或游戏特判。

显式选择 `.11` 通过正常 Wine 构建。策略检查：`x86_64-w64-mingw32-gcc -std=c11 -I /path/to/patched/wine/dlls/runeondxgi tests/dxgi-driver-version.c -o /tmp/dxgi-policy.exe -lsetupapi -ldxguid -luuid`；另以 i686 编译，通过产品授权的隔离 Wine 探针执行。覆盖 Intel/AMD/NVIDIA 值、未知厂商、LUID/厂商缓存隔离、重复/缺失设备、原返回保留和可执行页面保护。App 检查覆盖关闭时准备/启动/准备指纹、htmlcache 保留、首装无操作和真实指纹变更的缓存失效。

本地证据见 Runeon T798：x64/i386 模块构建、策略检查、App 编译、prefix 限定、持久关闭、回滚及修复/还原 API 对照通过。工厂入口与 EnumAdapterByLuid 可修正；D3D11 设备优先创建成功并保留原 -1，不属于已覆盖的修正入口；EnumWarpAdapter 保留原不支持结果。已复现并修复 provider 初始化被延后的回归。尚未发布 seed，不宣称游戏兼容。
