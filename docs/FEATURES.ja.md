# 機能

[English](FEATURES.md)

## モジュール

**左**

| モジュール | 表示 |
|---|---|
| Minecraft | ゲームと LeviLamina の版 |
| FPS | 今の FPS と上限 |
| Graphics | 描画距離・グラフィックの設定・雲 |
| Server | サーバーの tick の時間かサーバーの版、送受信のパケット数 |
| Coordinates | 位置・ブロック・チャンク・速度 |
| Chunks | 描画しているセクション・読み込みチャンク・常時読み込みのチャンク |
| Location | 次元・向き・回転・明るさ・ハイトマップ・バイオーム・日の tick・スライムチャンク |
| Entities | パーティクル・描画しているエンティティ / 全体・種類ごとの数 |
| Sounds | 鳴っている音と環境音 |
| Help | グラフの状態とヘルプの案内 |
| Separator | 空行 |
| Misc Left | ほかの Mod の行（[API](USAGE.ja.md#ほかの-mod-向けの-api)） |

**右**

| モジュール | 表示 |
|---|---|
| System | 時刻・OS・メモリ・CPU・画面・GPU・描画 API・ドライバ |
| Misc Right | ほかの Mod の行 |
| Separator | 空行 |
| Targeted | 見ているブロック・液体・エンティティ |

## F3 の操作

当たり判定・チャンクの境界・位置 / ブロック / エンティティのコピー・詳細なツールチップ・描画距離・チャンクの読み込み直し・チャットの消去・スペクテイターの切り替え・フォーカスを失ったときの一時停止・版の情報・プロファイル・ゲームモードの切り替え・メニューなしの一時停止。[キー](USAGE.ja.md#キー) を見てください。

## グラフ

- `F3` + `1`: 処理時間の円グラフ
- `F3` + `2`: FPS と TPS
- `F3` + `3`: 遅延と通信量

## Java 版との違い

- 「Java Version」の行は OS を表示します。
- マルチプレイのサーバーでは、サーバー側のハイトマップとチャンク数は表示しません。
- スライムチャンクは統合版の式で判定し、サーバーでも表示します。
- `F3` + `Esc` はもう一度押すと再開します。

## 使えないもの

- 行: Biome Blend Radius・Shader・Tick Manager Status・Chunk Culling・Pending Chunks・Pending Uploads・Available Buffers・Spawn Chunks・Region File・Local Difficulty・Mood・Fluid Tags
- キー: `F3` + `S` / `T` / `X` / `4` / `F6`、`Ctrl` + `F3` + `F`

## 既知の問題

- クライアント専用です。
- 統合版の座標表示とチャットが、デバッグ画面の左側と重なります。
