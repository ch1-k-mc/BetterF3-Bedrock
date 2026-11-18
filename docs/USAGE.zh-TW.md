# 使用方法

[English](USAGE.md) | [日本語](USAGE.ja.md) | [简体中文](USAGE.zh-CN.md)

## 按鍵

按 `F3` 開啟或關閉偵錯畫面。按住 `F3` 再按第二個鍵可執行以下操作。

| 按鍵 | 操作 |
|---|---|
| `F3` + `Q` | 列出所有組合鍵 |
| `F3` + `M` | 開啟設定畫面（按鍵可變更） |
| `F3` + `A` | 重新載入區塊 |
| `F3` + `B` | 顯示碰撞箱 |
| `F3` + `C` | 將位置複製為 `/tp` 指令 |
| `F3` + `D` | 清除聊天紀錄 |
| `F3` + `F` | 顯示距離 +1（`Shift`：−1） |
| `F3` + `G` | 顯示區塊邊界 |
| `F3` + `H` | 進階提示框（耐久度與物品 ID） |
| `F3` + `I` | 將指向的方塊或實體複製為指令 |
| `F3` + `L` | 記錄 10 秒的效能分析至 `mods/BetterF3/debug/` |
| `F3` + `N` | 切換旁觀模式 |
| `F3` + `P` | 切換失去焦點時暫停 |
| `F3` + `V` | 顯示版本資訊 |
| `F3` + `1` / `2` / `3` | 效能分析 / FPS 與 TPS / 延遲與頻寬圖表 |
| `F3` + `F4` | 遊戲模式切換器 |
| `F3` + `Esc` | 暫停但不顯示暫停選單 |

## 設定畫面

使用 `F3` + `M` 開啟。

- **General Settings**：以下所有選項
- **Left / Right Modules**：重新排序（點擊列左端的箭頭）、新增、刪除與編輯模組
- **Edit Module**：啟用、顏色以及每一行的顯示/隱藏

## 指令

| 指令 | 說明 |
|---|---|
| `/betterf3 toggle` | 開啟 / 關閉偵錯畫面 |
| `/betterf3 get [key]` | 顯示選項 |
| `/betterf3 set <key> <value>` | 變更選項 |
| `/betterf3 reset` | 全部重設為預設值 |
| `/betterf3 save` / `reload` | 儲存 / 重新載入設定檔 |
| `/betterf3 module list` | 列出模組及其編號 |
| `/betterf3 module <left\|right> <n> enable\|disable` | 啟用 / 停用模組 |
| `/betterf3 module <side> <n> line <line> on\|off` | 顯示 / 隱藏某一行 |
| `/betterf3 module <side> <n> set <key> <value>` | 變更模組設定 |
| `/betterf3 module <side> add <id> [n]` / `remove <n>` / `move <from> <to>` | 新增 / 移除 / 移動模組 |
| `/betterf3 lang [en_us\|ja_jp\|zh_cn\|zh_tw]` | 強制指定語言（無參數：跟隨遊戲） |

模組 ID：`minecraft` `fps` `graphics` `server` `coords` `chunks` `location` `entity` `sound` `help` `empty` `miscleft` `system` `miscright` `target`

## 設定檔

`mods/BetterF3/config/betterf3.json` — 與原版 Mod 的 Fabric 設定檔相容。

| 鍵 | 預設值 | 說明 |
|---|---|---|
| `disable_mod` | `false` | 停用偵錯畫面 |
| `auto_start` | `false` | 進入世界時開啟偵錯畫面 |
| `space_modules` | `false` | 模組之間留空行 |
| `shadow_text` | `true` | 文字陰影 |
| `animations` | `true` | 開啟 / 關閉動畫 |
| `animationSpeed` | `1.0` | 動畫速度（1–3） |
| `fontScale` | `1.0` | 字型縮放（0.1–2） |
| `background_color` | `#6F505050` | 行背景色（ARGB） |
| `hide_debug_crosshair` | `false` | 保留一般準心 |
| `hide_sidebar` | `true` | 開啟時隱藏計分板 |
| `hide_bossbar` | `true` | 開啟時隱藏 Boss 血條 |
| `always_show_profiler` | `false` | 一律顯示效能分析圖表 |
| `always_show_tps` | `false` | 一律顯示 FPS / TPS 圖表 |
| `always_show_ping` | `false` | 一律顯示延遲圖表 |
| `performance_optimizations` | `true` | 每 250 ms 更新一次數值 |
| `config_screen_key` | `"M"` | 與 `F3` 組合開啟設定畫面的按鍵 |

## 供其他 Mod 使用的 API

其他 Mod 可以在 *Misc Left* / *Misc Right* 模組中新增行：

```cpp
using SetLines = void (*)(char const* owner, int right, char const* const* lines, int count);
auto setLines = reinterpret_cast<SetLines>(
    GetProcAddress(GetModuleHandleW(L"BetterF3.dll"), "better_f3_set_lines"));
char const* lines[] = {"My Mod: on"};
if (setLines) setLines("MyMod", 0, lines, 1); // 0 = left, 1 = right
```

請參閱 [`src/api/PublicApi.h`](../src/api/PublicApi.h)。
