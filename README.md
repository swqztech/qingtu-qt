# qingtu-qt（轻图 · Qt6）

图片压缩 / 横向拼接，Qt Widgets + stb（与 [qingtu-cpp](https://github.com/swqztech/qingtu-cpp) 同源图像逻辑）。

## 构建

```bash
cmake -B build -DCMAKE_PREFIX_PATH="d:/codes/soft/.qt/6.8.2/msvc2022_64"
cmake --build build --config Release
```

## 绿色版

```bash
bash packaging/build.sh          # 绿色 zip + NSIS 安装包（默认 INSTALLER=both）
INSTALLER=nsis bash packaging/build.sh   # 仅安装包
```

安装包：`dist/轻图-qt-<版本>-win64-setup.exe`（需本机 [NSIS](https://nsis.sourceforge.io/)）。
