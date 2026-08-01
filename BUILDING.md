# 构建与源码包

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

patch 必须按 `series` 顺序应用。已应用、部分应用、SHA 漂移和基线不匹配都必须失败，不能静默跳过。

## 3. Runeon macOS WoW64 配置边界

当前产品构建使用完整 CrossOver 26.3/Wine 11.0 源码构建，不允许替换已构建 `lib/wine` 中的单个 DLL。核心配置为：

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

依赖准备、GStreamer、MoltenVK、GnuTLS、Rockstar scoped D2D wrapper、Apple 用户本机 overlay、签名与 runtime component packaging 仍由 Runeon 产品仓库维护。

## 4. 对应源码包

```bash
scripts/build-source-bundle.sh "$source_root" dist
```

输出包含 patched Wine source、base/patch manifest、patch files、series、维护脚本、许可证和构建说明。发布时记录生成文件的 SHA-256，并把不可变下载地址写入 Runeon runtime component metadata 与公开 source offer。

## 5. 产品构建使用的 patch-set bundle

```bash
scripts/build-patchset-bundle.sh dist
```

Runeon 产品仓库下载并验证这个小型 bundle，再对固定 SHA 的 CrossOver archive 应用 `series`。patch-set bundle 和完整 corresponding-source bundle 必须来自同一个 tag，不能分别重建后混用。

