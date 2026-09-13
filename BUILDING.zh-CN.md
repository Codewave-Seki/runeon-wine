# 构建与源码包

[英文](BUILDING.md) | [简体中文](BUILDING.zh-CN.md) | [日本語](BUILDING.ja.md)

> 英文文档是权威版本；简体中文与日语文档均为完整译本。

## 1. 获取固定基线

```bash
source_root="$(scripts/fetch-source.sh)"
```

脚本只接受 manifest 中固定的 archive SHA-256，并检查 `VERSION`。缓存默认位于仓库的 `.work/`，不会进入 Git。

## 2. 应用 patch series

```bash
scripts/apply-series.sh "$source_root"
scripts/verify-source.sh "$source_root"
```

补丁必须按 [`series`](series) 顺序应用。已应用、部分应用、SHA 漂移和基线不匹配都必须失败，不能静默跳过。

## 3. Runeon macOS WoW64 配置边界

当前产品构建使用完整 CrossOver 26.3/Wine 11.0 源码树，不允许替换已构建 `lib/wine` 中的单个 DLL。核心配置为：

```bash
"$source_root/configure" \
  --build=x86_64-apple-darwin \
  --host=x86_64-apple-darwin \
  --prefix="$install_root" \
  --enable-archs=i386,x86_64 \
  --disable-tests \
  --with-gstreamer \
  --without-usb \
  --without-pcap \
  --without-cups \
  --without-krb5 \
  --without-gssapi \
  --without-sdl \
  --without-opencl \
  --without-x
```

依赖准备、GStreamer、MoltenVK、GnuTLS、Rockstar scoped D2D wrapper、Apple 用户本机 overlay、签名和 runtime component packaging 仍由 Runeon 产品仓库维护。

## 4. 生成对应源码包

```bash
scripts/build-source-bundle.sh "$source_root" dist
```

输出包含 patched Wine source、base/patch manifest、patch files、[`series`](series)、维护脚本、许可证和构建说明。发布时必须记录生成文件的 SHA-256，把两个 bundle 与各自 `.sha256` 上传到同一 tag 的公开 GitHub Release，并将不可变 asset URL 写入 Runeon runtime component metadata 与公开 source offer。

## 5. 生成产品构建使用的 patch-set bundle

```bash
scripts/build-patchset-bundle.sh dist
```

Runeon 产品仓库下载并验证这个较小的 bundle，再对固定 SHA 的 CrossOver archive 应用 [`series`](series)。patch-set bundle 和完整 corresponding-source bundle 必须来自同一个 tag，不能分别重建后混用。

候选 tag 必须发布为 GitHub Pre-release。只有 matching runtime 完成 Dev 验证并真正进入 Production 后，才能把同一个不可变 tag 的 Release 标记为正式版。Production 构建不得使用 branch archive 或 `latest` URL。

如果 patch-set ID 已存在同名 tag，bundle 脚本只允许在该 tag 的精确 commit checkout 上重建，防止 `main` 后续文档或脚本变化悄悄改变历史 asset。继续开发时必须创建新的 patch-set ID；不能移动旧 tag 或替换旧 Release asset。

## 6. 重现定向检查

[tests](tests) 目录同时包含在 patch-set 与完整对应源码 bundle 中。`source_root` 应指向已应用目标 patch set 的源码树。上方产品配置禁用了 Wine 上游测试；受影响的上游 suite 需使用单独启用测试的构建。`configure` 与 `make` 应使用一致的编译器和依赖 `PATH`。

`lifetime-check.py` 在 macOS 使用 Python 标准库与 `clang`，提取真实 Direct2D/EVR 函数，配小型故障注入依赖编译执行，检查 51 条所有权断言，并自动清理临时 C 源码与可执行文件；不启动 Wine，也不使用 GPU。

`stream-read-check.py` 同样提取真实 WIC `stream_read` 函数，执行九个场景、38 条断言，覆盖 NULL/非 NULL count 指针、完整读取、成功短读转为 `S_FALSE`、原有 `S_FALSE`，以及保持 `E_FAIL` 且不读取未写入 count。不可读的输出槽用于检查失败路径。它使用依赖桩，不启动 Wine 或真实 codec，并自动清理临时构建文件。

其余命令用 MinGW 编译三个独立 Windows API 探针，并不执行它们。

```bash
python3 tests/lifetime-check.py "$source_root"
python3 tests/stream-read-check.py "$source_root"

probe_dir="$(mktemp -d)"
x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Werror \
  -Wno-unused-parameter -O0 tests/graphics-probe.c \
  -o "$probe_dir/graphics-probe.exe" \
  -lsecur32 -ld3d10 -ldwrite -lwindowscodecs -lole32 -luuid
x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Werror \
  -O2 tests/dinput-probe.c -o "$probe_dir/dinput-probe.exe" \
  -ldinput8 -ldxguid -lole32
x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Werror \
  -O0 -municode tests/winhttp-probe.c \
  -o "$probe_dir/winhttp-probe.exe" -lwinhttp
```

图形探针覆盖 state-block mask、adapter 引用所有权、SChannel context、WIC stream 和 TTC fragment。DirectInput 探针检查不同设备 GUID 共用 control ID 时的两种 action 顺序。WinHTTP 探针使用 [winhttp-fixture.py](tests/winhttp-fixture.py)，只在 `127.0.0.1` 提供过长 header 响应与 `DIRECT` PAC，不发起外部请求。

编译探针后，在同一 shell 中启动 fixture，以保留 `probe_dir` 变量：

```bash
fixture_port=8765
python3 tests/winhttp-fixture.py --port "$fixture_port" \
  >"$probe_dir/winhttp-fixture.log" 2>&1 &
fixture_pid=$!
cat "$probe_dir/winhttp-fixture.log"
```

日志出现 `Wine regression fixture on 127.0.0.1:8765` 后再运行探针；若仍在启动，重新读取日志。端口被占用时选择其它端口并统一使用。通过下方受管理启动路径，依次用参数 `headers 8765`、`pac 8765` 运行 `winhttp-probe.exe`（替换为所选端口）。前者完整读取过长 header 响应，后者检查正常及过长 PAC hostname。这些说明不代表 API 执行已通过。

两个探针结束后，只停止上方启动的 fixture 进程；`wait` 可能报告由所请求信号终止：

```bash
kill -TERM "$fixture_pid"
wait "$fixture_pid" 2>/dev/null || true
```

PE 探针必须通过 Runeon 产品支持的受管理启动路径执行，使用真实 gatekeeper grant 和隔离测试 prefix。普通直接调用 Wine 可能被受管理启动验证拒绝，这不代表 DLL 测试失败。本公开仓库不包含私有 App 测试启动器，也不绕过 gate。使用匹配的 Wine builtin DLL，记录精确 runtime 与 prefix，对照旧版和候选版本，保留轻量结果后清理临时探针二进制。

探针编译、函数级故障注入、Windows API 执行、受影响模块测试以及产品图形/Steam smoke 是不同检查。[定向审查](BACKPORTS-11.16-11.17.zh-CN.md) 记录当前状态，任何单项都不能独立授权 runtime 发布。
