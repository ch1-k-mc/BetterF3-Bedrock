# 功能

[English](FEATURES.md) | [日本語](FEATURES.ja.md) | [简体中文](FEATURES.zh-CN.md)

## 模組

**左側**

| 模組 | 顯示內容 |
|---|---|
| Minecraft | 遊戲與 LeviLamina 版本 |
| FPS | 目前 FPS 與 FPS 上限 |
| Graphics | 顯示距離、畫質模式、雲 |
| Server | 伺服器 tick 時間或伺服器版本、傳送 / 接收的封包 |
| Coordinates | 位置、方塊、區塊、速度 |
| Chunks | 已繪製的子區塊、已載入區塊、強制載入區塊 |
| Location | 維度、朝向、旋轉、光照、高度圖、生態域、當日 tick、史萊姆區塊 |
| Entities | 粒子、已繪製 / 全部實體、各類別數量 |
| Sounds | 正在播放的聲音與環境音 |
| Help | 圖表狀態與說明提示 |
| Separator | 空行 |
| Misc Left | 來自其他 Mod 的行（[API](USAGE.zh-TW.md#供其他-mod-使用的-api)） |

**右側**

| 模組 | 顯示內容 |
|---|---|
| System | 時間、作業系統、記憶體、CPU、顯示器、GPU、圖形 API、驅動程式 |
| Misc Right | 來自其他 Mod 的行 |
| Separator | 空行 |
| Targeted | 指向的方塊、流體與實體 |

## F3 操作

碰撞箱、區塊邊界、複製位置 / 方塊 / 實體、進階提示框、顯示距離、重新載入區塊、清除聊天、切換旁觀模式、失去焦點時暫停、版本資訊、效能分析、遊戲模式切換器、不顯示選單的暫停。請參閱[按鍵](USAGE.zh-TW.md#按鍵)。

## 圖表

- `F3` + `1`：效能分析圓餅圖
- `F3` + `2`：FPS 與 TPS
- `F3` + `3`：延遲與頻寬

## 與 Java 版的差異

- *Java Version* 行改為顯示作業系統。
- 在多人伺服器上，不顯示伺服器端的高度圖與區塊數量。
- 史萊姆區塊判定使用基岩版的公式，在伺服器上同樣有效。
- `F3` + `Esc` 再按一次即可恢復。

## 無法使用

- 行：Biome Blend Radius, Shader, Tick Manager Status, Chunk Culling, Pending Chunks, Pending Uploads, Available Buffers, Spawn Chunks, Region File, Local Difficulty, Mood, Fluid Tags
- 按鍵：`F3` + `S` / `T` / `X` / `4` / `F6`、`Ctrl` + `F3` + `F`

## 已知問題

- 僅限用戶端。
- 在 Minecraft 1.26.60 上，LeviLamina 尚未提供部分遊戲函式，因此以下內容無法使用：Chunk Sections 的總數與 Forceloaded Chunks（顯示為 0）、Packets Received 與接收流量、`F3` + `D`、`F3` + `V` 中的協定版本號。設定畫面的輸入欄只能輸入字母、數字以及 `.`、`-`、`#`。這些在 1.26.51 上可以使用（[舊版本](../README.zh-TW.md#舊版本)）。
- 基岩版的座標顯示與聊天會與偵錯畫面左側重疊。
