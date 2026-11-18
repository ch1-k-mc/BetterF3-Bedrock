# 使用方法

[English](USAGE.md) | [日本語](USAGE.ja.md) | [繁體中文](USAGE.zh-TW.md)

## 按键

按 `F3` 打开或关闭调试屏幕。按住 `F3` 再按第二个键可执行以下操作。

| 按键 | 操作 |
|---|---|
| `F3` + `Q` | 列出所有组合键 |
| `F3` + `M` | 打开设置界面（按键可更改） |
| `F3` + `A` | 重新加载区块 |
| `F3` + `B` | 显示碰撞箱 |
| `F3` + `C` | 将位置复制为 `/tp` 命令 |
| `F3` + `D` | 清空聊天记录 |
| `F3` + `F` | 渲染距离 +1（`Shift`：−1） |
| `F3` + `G` | 显示区块边界 |
| `F3` + `H` | 高级提示框（耐久度和物品 ID） |
| `F3` + `I` | 将指向的方块或实体复制为命令 |
| `F3` + `L` | 记录 10 秒的性能分析到 `mods/BetterF3/debug/` |
| `F3` + `N` | 切换旁观模式 |
| `F3` + `P` | 切换失去焦点时暂停 |
| `F3` + `V` | 显示版本信息 |
| `F3` + `1` / `2` / `3` | 性能分析 / FPS 与 TPS / 延迟与带宽图表 |
| `F3` + `F4` | 游戏模式切换器 |
| `F3` + `Esc` | 暂停但不显示暂停菜单 |

## 设置界面

使用 `F3` + `M` 打开。

- **General Settings**：以下所有选项
- **Left / Right Modules**：重新排序（点击行左端的箭头）、添加、删除和编辑模块
- **Edit Module**：启用、颜色以及每行的显示/隐藏

## 命令

| 命令 | 说明 |
|---|---|
| `/betterf3 toggle` | 打开 / 关闭调试屏幕 |
| `/betterf3 get [key]` | 显示选项 |
| `/betterf3 set <key> <value>` | 更改选项 |
| `/betterf3 reset` | 全部恢复为默认值 |
| `/betterf3 save` / `reload` | 保存 / 重新加载配置文件 |
| `/betterf3 module list` | 列出模块及其编号 |
| `/betterf3 module <left\|right> <n> enable\|disable` | 启用 / 禁用模块 |
| `/betterf3 module <side> <n> line <line> on\|off` | 显示 / 隐藏某一行 |
| `/betterf3 module <side> <n> set <key> <value>` | 更改模块设置 |
| `/betterf3 module <side> add <id> [n]` / `remove <n>` / `move <from> <to>` | 添加 / 移除 / 移动模块 |
| `/betterf3 lang [en_us\|ja_jp\|zh_cn\|zh_tw]` | 强制指定语言（无参数：跟随游戏） |

模块 ID：`minecraft` `fps` `graphics` `server` `coords` `chunks` `location` `entity` `sound` `help` `empty` `miscleft` `system` `miscright` `target`

## 配置

`mods/BetterF3/config/betterf3.json` — 与原版 Mod 的 Fabric 配置兼容。

| 键 | 默认值 | 说明 |
|---|---|---|
| `disable_mod` | `false` | 禁用调试屏幕 |
| `auto_start` | `false` | 进入世界时打开调试屏幕 |
| `space_modules` | `false` | 模块之间留空行 |
| `shadow_text` | `true` | 文字阴影 |
| `animations` | `true` | 打开 / 关闭动画 |
| `animationSpeed` | `1.0` | 动画速度（1–3） |
| `fontScale` | `1.0` | 字体缩放（0.1–2） |
| `background_color` | `#6F505050` | 行背景色（ARGB） |
| `hide_debug_crosshair` | `false` | 保留普通准星 |
| `hide_sidebar` | `true` | 打开时隐藏计分板 |
| `hide_bossbar` | `true` | 打开时隐藏 Boss 血条 |
| `always_show_profiler` | `false` | 始终显示性能分析图表 |
| `always_show_tps` | `false` | 始终显示 FPS / TPS 图表 |
| `always_show_ping` | `false` | 始终显示延迟图表 |
| `performance_optimizations` | `true` | 每 250 ms 更新一次数值 |
| `config_screen_key` | `"M"` | 与 `F3` 组合打开设置界面的按键 |

## 供其他 Mod 使用的 API

其他 Mod 可以向 *Misc Left* / *Misc Right* 模块添加行：

```cpp
using SetLines = void (*)(char const* owner, int right, char const* const* lines, int count);
auto setLines = reinterpret_cast<SetLines>(
    GetProcAddress(GetModuleHandleW(L"BetterF3.dll"), "better_f3_set_lines"));
char const* lines[] = {"My Mod: on"};
if (setLines) setLines("MyMod", 0, lines, 1); // 0 = left, 1 = right
```

参见 [`src/api/PublicApi.h`](../src/api/PublicApi.h)。
