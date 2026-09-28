# SwarmRogue

基于 Unreal Engine 5.4 和 C++ 开发的群体生存类游戏项目。

## 技术栈

### 引擎与语言

![Unreal Engine 5.4](https://img.shields.io/badge/Unreal%20Engine-5.4-0E1128?logo=unrealengine&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![Visual Studio](https://img.shields.io/badge/IDE-Visual%20Studio-5C2D91?logo=visualstudio&logoColor=white)

### Unreal Engine 模块

![Gameplay Ability System](https://img.shields.io/badge/Gameplay%20Ability%20System-GAS-7B42BC?logo=unrealengine&logoColor=white)
![Enhanced Input](https://img.shields.io/badge/Enhanced%20Input-Input-2E7D32?logo=unrealengine&logoColor=white)
![UMG](https://img.shields.io/badge/UMG-UI-1565C0?logo=unrealengine&logoColor=white)
![Gameplay Tags](https://img.shields.io/badge/Gameplay%20Tags-Data-00897B?logo=unrealengine&logoColor=white)
![Data Registry](https://img.shields.io/badge/Data%20Registry-Data-6D4C41?logo=unrealengine&logoColor=white)
![NetCore](https://img.shields.io/badge/NetCore-Networking-D84315?logo=unrealengine&logoColor=white)

### 项目插件

![SPCR Joint Dynamics](https://img.shields.io/badge/SPCR%20Joint%20Dynamics-Animation-8E24AA?logo=unrealengine&logoColor=white)
![SwitchLanguage](https://img.shields.io/badge/SwitchLanguage-Editor-546E7A?logo=unrealengine&logoColor=white)

## 目录

- `Source/`：游戏 C++ 模块和运行时逻辑
- `Config/`：项目、输入、Gameplay Tags 和编辑器配置
- `Plugins/`：项目使用的插件源码及插件描述文件
- `SwarmRogue.uproject`：Unreal Engine 项目文件

## 开发环境

1. 安装 Unreal Engine 5.4。
2. 克隆仓库并右键 `SwarmRogue.uproject`，选择生成 Visual Studio 项目文件。
3. 使用 Visual Studio 打开生成的解决方案并编译 `Development Editor`。
4. 在 Unreal Editor 中打开项目。

## 资产说明

为保持仓库体积小于 10 MB，项目的 `Content/` 目录、构建产物和编辑器缓存不会提交到 Git。完整运行项目需要另外获取对应的美术资产。

## 许可证

当前仓库未声明开源许可证。
