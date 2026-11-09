# ExampleExtension

一个不依赖绘图接口的 Jackal 原生模块示例。启用后观察 `Clock` 的开关和配置，布尔等配置发生变化时通知变化项名称；可选请求开启目标模块、停用本扩展时恢复原状态。每次启用还会更新本模块的 Activation Count，演示配置读取和异步写入。

默认只观察状态，不控制模块、不执行命令。目标名称使用原始英文名；改动目标和联动选项后重新启用本模块。权限及风险确认仍由客户端处理；启用请求可能等待用户确认，不能将入队成功当成已经开启。

`Run Command On Enable` / `Enable Command` 在观察目标由关变开时执行；`Run Command On Disable` / `Disable Command` 在观察目标由开变关时执行，不再因启动本扩展而执行。两者不依赖 `Control Watched Module` 或 `Notify Changes`。轮询仅能观察两次采样间保留下来的变化，快速开关后立即恢复原状可能不会触发。

词条位于本扩展的 `Dictionary.txt`，通过清单 `contributes.Dictionary` 引用并随扩展一起打包；安装后补充到内存词典，不修改客户端或云控的公共 `Dictionary.txt`。公共词条优先，扩展之间同名词条按扩展 ID 排序取第一项。

## 构建

需要 Windows、Visual Studio C++、CMake 和 nlohmann-json。SDK 位于同级 `../sdk`。独立上传 GitHub 时将 `sdk` 一并放在同级目录，或调整 CMake 的 include 路径。

在仓库根目录执行，使用现有 vcpkg 安装前缀（按本机实际路径修改）：

```powershell
cmake -S extensions/example-module -B build/example-module-x64 -A x64 -DCMAKE_PREFIX_PATH="$env:VCPKG_ROOT/installed/x64-windows-static"
cmake --build build/example-module-x64 --config Release
cmake -S extensions/example-module -B build/example-module-x86 -A Win32 -DCMAKE_PREFIX_PATH="$env:VCPKG_ROOT/installed/x86-windows-static"
cmake --build build/example-module-x86 --config Release
```

直接在本目录生成 `example-module-x86.dll` 与 `example-module-x64.dll`，不创建 DLL 输出子目录；中间构建文件仍位于仓库根目录的 `build/`。清单同时声明两种位数，因此两者必须都存在才能导入、打包或发布。不要提交构建目录或 DLL；GitHub 仓库保留代码，发行附件提供构建后的扩展包。

## 使用与发布

在 Extensions 点击 Import local... → 选择扩展文件夹，选择本目录；首次导入完成后即可在 Misc 找到“示例扩展”，手动开启即可，无需重启。已加载扩展的更新、禁用和删除仍在重启后生效。也可先打包生成带校验的发布目录，再压缩导入。

```powershell
py -3 scripts/extension-package.py publish --id example-module --destination D:/Wormwaker/IMAGES/Ricochesta/extensions
```

该命令会为所有 DLL、图标和词典文件计算 `size` 与 `sha256`，不要将本目录未打包的开发清单直接复制后发布。它会写入云控仓库，运行前审查目的目录；不会提交或推送。目录已存在时需先停止使用并审查 `--replace` 的备份行为。完整接口、异步结果和信任边界见 `docs/extension-sdk.zh-CN.md`。
