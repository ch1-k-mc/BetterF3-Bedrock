# 功能

[English](FEATURES.md) | [日本語](FEATURES.ja.md) | [繁體中文](FEATURES.zh-TW.md)

## 模块

**左侧**

| 模块 | 显示内容 |
|---|---|
| Minecraft | 游戏和 LeviLamina 版本 |
| FPS | 当前 FPS 和 FPS 上限 |
| Graphics | 渲染距离、图像模式、云 |
| Server | 服务器 tick 时间或服务器版本、发送 / 接收的数据包 |
| Coordinates | 位置、方块、区块、速度 |
| Chunks | 已渲染的子区块、已加载区块、强制加载区块 |
| Location | 维度、朝向、旋转、光照、高度图、生物群系、当日 tick、史莱姆区块 |
| Entities | 粒子、已渲染 / 全部实体、各类别数量 |
| Sounds | 正在播放的声音和环境音 |
| Help | 图表状态和帮助提示 |
| Separator | 空行 |
| Misc Left | 来自其他 Mod 的行（[API](USAGE.zh-CN.md#供其他-mod-使用的-api)） |

**右侧**

| 模块 | 显示内容 |
|---|---|
| System | 时间、操作系统、内存、CPU、显示器、GPU、图形 API、驱动 |
| Misc Right | 来自其他 Mod 的行 |
| Separator | 空行 |
| Targeted | 指向的方块、流体和实体 |

## F3 操作

碰撞箱、区块边界、复制位置 / 方块 / 实体、高级提示框、渲染距离、重新加载区块、清空聊天、切换旁观模式、失去焦点时暂停、版本信息、性能分析、游戏模式切换器、不显示菜单的暂停。参见[按键](USAGE.zh-CN.md#按键)。

## 图表

- `F3` + `1`：性能分析饼图
- `F3` + `2`：FPS 与 TPS
- `F3` + `3`：延迟与带宽

## 与 Java 版的区别

- *Java Version* 行改为显示操作系统。
- 在多人服务器上，不显示服务端的高度图和区块数量。
- 史莱姆区块判定使用基岩版的公式，在服务器上同样有效。
- `F3` + `Esc` 再按一次即可恢复。

## 不可用

- 行：Biome Blend Radius, Shader, Tick Manager Status, Chunk Culling, Pending Chunks, Pending Uploads, Available Buffers, Spawn Chunks, Region File, Local Difficulty, Mood, Fluid Tags
- 按键：`F3` + `S` / `T` / `X` / `4` / `F6`、`Ctrl` + `F3` + `F`

## 已知问题

- 仅限客户端。
- 在 Minecraft 1.26.60 上，LeviLamina 尚未提供部分游戏函数，因此以下内容无法使用：Chunk Sections 的总数和 Forceloaded Chunks（显示为 0）、Packets Received 和接收流量、`F3` + `D`、`F3` + `V` 中的协议版本号。设置界面的输入框只能输入字母、数字以及 `.`、`-`、`#`。这些在 1.26.51 上可以使用（[旧版本](../README.zh-CN.md#旧版本)）。
- 基岩版的坐标显示和聊天会与调试屏幕左侧重叠。
