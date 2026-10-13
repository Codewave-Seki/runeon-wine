# Steam 启动准备

[English](LAUNCH-PREPARE.md) | [简体中文](LAUNCH-PREPARE.zh-CN.md) | [日本語](LAUNCH-PREPARE.ja.md)

> 以英文文档为准，中文与日文为完整译文。

候选补丁集 `cx26.3-wine11.0-runeon.18` 在 `0117` 之后加入 `patches/runeon/0118-steam-launch-route-preparation.patch`。

## 行为

该补丁在 `0104` 与 `0111` 共用的 Steam 中间件选择器上加入显式开关 `RUNEON_LAUNCH_PREPARE_V1`。设为 `1` 时，由 `steam.exe` 创建、带有效会话 grant 与非零 `SteamAppId` 的每个 x64 或 i386 可执行文件，都会先以 `launch-prepare` 模式运行固定 helper `C:\runeon-input-source\runeon-input-source-launch.exe`。helper 只做准备；Wine 等它退出后，再以相同的命令行、创建标志和句柄创建原子进程。

调用方用这一步确定子进程启动时会读取的状态，例如它的图形路由。这样游戏的第一个进程看到的已是新状态，而不是会话开始时的状态。

- `0111` 的 D3D12on7 准备拆分为目录锁与 helper 运行。同一把锁覆盖两项准备和原子进程的创建，路由准备先执行。
- `0104` 的 NW.js 接管会替换子进程并保持优先，因此不做路由准备。
- NW.js 与 D3D12on7 两项仍只针对 x64。
- 未开启、父进程不是 `steam.exe`、缺少 grant 或 `SteamAppId`、传入了 token 或扩展启动信息、映像既不是 x64 也不是 i386，或 helper 不存在时，创建过程不变。无法启动的 helper 会被跳过。
- Wine 不读取任何游戏名、AppID 或可执行文件名。

## 验证

`tests/launch-prepare-check.py` 用桩函数编译补丁后的选择器与准备函数，覆盖开关、x64 与 i386 映像、与 NW.js 和 D3D12on7 的配合、命令保留、失败回退，以及 helper 只在持锁期间运行。`scripts/integration-check.sh` 对含该开关的源码树运行它；不含开关的补丁集仍由原有的 `tests/d3d12on7-launch-check.py` 覆盖。该检查不运行 Wine、helper 或真实的 Steam 启动。能否编译只能由完整的 i386/x86_64 WoW64 构建证明。
