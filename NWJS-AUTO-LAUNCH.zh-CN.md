# NW.js 自动启动候选

[English](NWJS-AUTO-LAUNCH.md) | [Chinese](NWJS-AUTO-LAUNCH.zh-CN.md) | [Japanese](NWJS-AUTO-LAUNCH.ja.md)

英文文档为准，其余语言为完整翻译。

候选 `cx26.3-wine11.0-runeon.11` 继承 `.10`，新增 `0104-steam-nwjs-automatic-launch.patch`。使用 `RUNEON_WINE_PATCHSET_DEFINITION=patchsets/cx26.3-wine11.0-runeon.11` 选择。历史定义和默认补丁集不变。

只处理 `steam.exe` 创建进程，要求 App 会话标记 `RUNEON_NWJS_AUTO_V1=1`、grant、非零 SteamAppId、x64 PE，以及相邻的 `nw.dll`、`nw_elf.dll`、`package.json`。原命令交给固定的 App 受管包装器。调试启动保持原样。命令首参数与实际 executable 不同、包装后命令超过 Windows 长度上限或分配失败时，不拦截原进程创建。不匹配游戏 ID 或游戏 exe 名。

配套 App/包装器验证本地请求，先只读检查实际运行时资格，通过后才检查账号、安装、命令、保护、组件、存档和用户配置。结果为受管 NW.js 启动、明确不适用时执行原命令，或报告准备失败。入口不修改游戏文件或运行中的 Steam 配置。旧 App 不设置能力标记；新 App 必须确认本次实际 runtime 的两种 kernelbase 架构均含标记，且包装器安装及接收服务已就绪，才启用标记并移除旧引导。

本地开发检查：修改后的 process 编译单元在 i686、x86_64 均编译通过；App/包装器协议检查覆盖受管启动、原命令放行、拒绝、超时和会话取消。这不代表完整 Wine 构建或真实 Steam/游戏启动。配套 runtime 和 App 需按现有发布流程一起构建分发；此候选尚未发布为 runtime。
