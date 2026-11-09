# ExampleExtension

一个简易的 JackalClient C++ 原生模块示例，你可以参考其写法创建你自己的扩展。JackalClient 扩展不一定会注册模块，也不一定必须使用 C++ 编写扩展。[想了解更多有关 「Jackal 扩展与SDK」 的相关信息，请参考此处。](https://doc.wormwake.com/extension-sdk.html)

对于本示例扩展模块，启用后观察 `时钟 (Clock)` 模块的开关和配置，布尔等配置发生变化时通知变化项名称；可选请求开启目标模块、停用本扩展时恢复原状态。每次启用还会更新本模块的 启用次数 (Activation Count)，演示配置读取和异步写入。
目标名称使用原始英文名；改动目标和联动选项后重新启用本模块。

`Run Command On Enable` / `Enable Command` 在观察目标由关变开时执行；`Run Command On Disable` / `Disable Command` 在观察目标由开变关时执行，不再因启动本扩展而执行。

新增词典词条位于本扩展的 `Dictionary.txt`，通过清单 `contributes.Dictionary` 引用并随扩展一起打包；安装后补充到内存词典。公共词条优先，扩展之间同名词条按扩展 ID 排序取第一项。

在编写扩展时，请额外注意 `extension.json` 的书写，包括各个文件的类型与哈希等，注意格式。

## 构建

需要 Windows、Visual Studio C++、CMake 和 nlohmann-json。SDK 位于同级 `../sdk`。独立上传 GitHub 时将 `sdk` 一并放在同级目录，或调整 CMake 的 include 路径。

在仓库根目录执行，使用现有 vcpkg 安装前缀（按本机实际路径修改）：

```powershell
cmake -S extensions/example-module -B build/example-module-x64 -A x64 -DCMAKE_PREFIX_PATH="$env:VCPKG_ROOT/installed/x64-windows-static"
cmake --build build/example-module-x64 --config Release
cmake -S extensions/example-module -B build/example-module-x86 -A Win32 -DCMAKE_PREFIX_PATH="$env:VCPKG_ROOT/installed/x86-windows-static"
cmake --build build/example-module-x86 --config Release
```

直接在本目录生成 `example-module-x86.dll` 与 `example-module-x64.dll`；中间构建文件仍位于仓库根目录的 `build/`。清单同时声明两种位数，因此两者必须都存在才能导入、打包或发布。

## 使用与发布

在 扩展 (Extensions) 点击 导入本地 (Import local...) → 选择扩展文件夹，选择本目录；首次导入完成后即可在 `Misc` 找到 “示例扩展”模块，手动开启即可，无需重启。已加载扩展的更新、禁用和删除仍在重启后生效。也可先打包生成带校验的发布目录，再压缩导入。
