# 可选 DXGI 驱动版本修正

[English](DXGI-DRIVER-VERSION.md) | [简体中文](DXGI-DRIVER-VERSION.zh-CN.md) | [日本語](DXGI-DRIVER-VERSION.ja.md)

候选 `.11` 包含 `0105-dxgi-driver-version.patch`，独立实现 LGPL-2.1-or-later 的 `runeondxgi.dll`。使用公开 COM/SetupAPI 接口和 Wine 的 GPU LUID 设备属性，未引入无授权的外部 shim。

仅修正 `CheckInterfaceSupport` 成功但版本为 -1 的结果。必须唯一精确匹配 LUID，且已有 `DriverVersion` 是有效四段版本；正常版本、失败、空输出和未知设备保留原行为。工厂返回原 COM 对象，按 provider 虚表分别加锁登记，并在修改方法指针前固定模块生命周期；超过有界表容量的对象保持原状。

模块默认不启用。App 将它与用户本地 x64 Apple provider 装配：保留原件，仅将原件导出库身份改为 `rxdg.dll`，提供匹配的 unix 别名，最后原子替换 wrapper。provider 新增导出时必须重新审查。seed 必须排除私有 provider 及备份；本仓库不包含按游戏名特判或 Apple 私有文件。

显式选择 `.11` 并通过正常 Wine 构建。独立策略检查：`x86_64-w64-mingw32-gcc -std=c11 -I /path/to/patched/wine/dlls/runeondxgi tests/dxgi-driver-version.c -o /tmp/dxgi-policy.exe -lsetupapi -ladvapi32 -ldxguid -luuid`；另以 i686 编译，通过产品授权的隔离 Wine 探针执行。

本地状态：Wine 构建系统已生成 x64/i386 模块。x64 模块通过原始/修复/还原 API 对照、三个工厂入口，以及产品同步器预置的全新 prefix；x64/x86 策略检查通过。改名 provider 必须在首次初始化前写入，因为 Wine 不会自动创建它。产品安装器、私有组件边界与 App 编译通过。尚未发布 seed，不代表某款游戏可以运行；完整 runtime 和发布门禁另行执行。
