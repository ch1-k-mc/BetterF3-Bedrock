# 使い方

[English](USAGE.md)

## キー

`F3` でデバッグ画面を開く / 閉じる。`F3` を押したまま別のキーを押すと、次の操作になります。

| キー | 動作 |
|---|---|
| `F3` + `Q` | キー操作の一覧 |
| `F3` + `M` | 設定画面を開く（キーは変更可） |
| `F3` + `A` | チャンクを読み込み直す |
| `F3` + `B` | 当たり判定を表示 |
| `F3` + `C` | 位置を `/tp` コマンドとしてコピー |
| `F3` + `D` | チャットの履歴を消す |
| `F3` + `F` | 描画距離 +1（`Shift` で −1） |
| `F3` + `G` | チャンクの境界を表示 |
| `F3` + `H` | 詳細なツールチップ（耐久とアイテム ID） |
| `F3` + `I` | 見ているブロック・エンティティをコマンドとしてコピー |
| `F3` + `L` | 10 秒のプロファイルを `mods/BetterF3/debug/` に記録 |
| `F3` + `N` | スペクテイターの切り替え |
| `F3` + `P` | フォーカスを失ったときの一時停止の切り替え |
| `F3` + `V` | 版の情報を表示 |
| `F3` + `1` / `2` / `3` | 処理時間 / FPS と TPS / 遅延と通信量のグラフ |
| `F3` + `F4` | ゲームモードの切り替え画面 |
| `F3` + `Esc` | 一時停止の画面を出さずに一時停止 |

## 設定画面

`F3` + `M` で開きます。

- **全体設定**: 下の設定項目すべて
- **左 / 右のモジュール**: 並び替え（行の左端の矢印をクリック）・追加・削除・編集
- **モジュールの編集**: 有効 / 無効・色・行ごとの表示

## コマンド

| コマンド | 内容 |
|---|---|
| `/betterf3 toggle` | デバッグ画面を開く / 閉じる |
| `/betterf3 get [キー]` | 設定を表示 |
| `/betterf3 set <キー> <値>` | 設定を変更 |
| `/betterf3 reset` | すべて既定に戻す |
| `/betterf3 save` / `reload` | 設定ファイルを保存 / 読み直す |
| `/betterf3 module list` | モジュールを番号つきで表示 |
| `/betterf3 module <left\|right> <番号> enable\|disable` | モジュールを有効 / 無効に |
| `/betterf3 module <側> <番号> line <行> on\|off` | 行を表示 / 非表示に |
| `/betterf3 module <側> <番号> set <キー> <値>` | モジュールの設定を変更 |
| `/betterf3 module <側> add <id> [番号]` / `remove <番号>` / `move <元> <先>` | モジュールを追加 / 削除 / 移動 |
| `/betterf3 lang [en_us\|ja_jp]` | 言語を固定（引数なしでゲームの言語に戻す） |

モジュールの id: `minecraft` `fps` `graphics` `server` `coords` `chunks` `location` `entity` `sound` `help` `empty` `miscleft` `system` `miscright` `target`

## 設定ファイル

`mods/BetterF3/config/betterf3.json`（本家の Fabric 版の設定と互換）。

| キー | 既定 | 内容 |
|---|---|---|
| `disable_mod` | `false` | デバッグ画面を無効にする |
| `auto_start` | `false` | ワールドに入ったときに開く |
| `space_modules` | `false` | モジュールの間に空行 |
| `shadow_text` | `true` | 文字の影 |
| `animations` | `true` | 開閉のアニメーション |
| `animationSpeed` | `1.0` | アニメーションの速さ（1〜3） |
| `fontScale` | `1.0` | 文字の大きさ（0.1〜2） |
| `background_color` | `#6F505050` | 行の背景の色（ARGB） |
| `hide_debug_crosshair` | `false` | 普通の照準のままにする |
| `hide_sidebar` | `true` | 開いている間スコアボードを隠す |
| `hide_bossbar` | `true` | 開いている間ボスバーを隠す |
| `always_show_profiler` | `false` | 処理時間のグラフを常に出す |
| `always_show_tps` | `false` | FPS / TPS のグラフを常に出す |
| `always_show_ping` | `false` | 遅延のグラフを常に出す |
| `performance_optimizations` | `true` | 値の更新を 250ms ごとにする |
| `config_screen_key` | `"M"` | 設定画面を開く `F3` + キーのキー |

## ほかの Mod 向けの API

ほかの Mod から *Misc Left* / *Misc Right* のモジュールに行を足せます。

```cpp
using SetLines = void (*)(char const* owner, int right, char const* const* lines, int count);
auto setLines = reinterpret_cast<SetLines>(
    GetProcAddress(GetModuleHandleW(L"BetterF3.dll"), "better_f3_set_lines"));
char const* lines[] = {"My Mod: on"};
if (setLines) setLines("MyMod", 0, lines, 1); // 0 = 左、1 = 右
```

詳しくは [`src/api/PublicApi.h`](../src/api/PublicApi.h)。
