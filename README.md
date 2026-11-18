# BetterF3 for Bedrock

[日本語](README.ja.md) | [简体中文](README.zh-CN.md) | [繁體中文](README.zh-TW.md)

![BetterF3 for Bedrock](docs/images/screenshot.png)

A customizable F3 debug screen for Minecraft: Bedrock Edition. A [LeviLamina](https://github.com/LiteLDev/LeviLamina) port of the Java Edition mod [BetterF3](https://github.com/TreyRuffy/BetterF3) by cominixo / TreyRuffy.

> This is an unofficial port, not affiliated with the authors of the original BetterF3.

## Requirements

| | Version |
|---|---|
| Minecraft: Bedrock Edition | 1.26.60 (Windows) |
| LeviLamina | 26.60.x |

For Minecraft 1.26.51, see [Older versions](#older-versions).

## Installation

**lip**

```
lip install github.com/ch1-k-mc/BetterF3-Bedrock#client
```

Run it in the game directory of your LeviLamina client.

**Manual**: download `BetterF3-client-windows-x64.zip` from [Releases](../../releases) and extract the `BetterF3` folder into `mods/`.

## Older versions

| Minecraft | LeviLamina | BetterF3 |
|---|---|---|
| 1.26.60 | 26.60.x | 0.2.0 or later (this branch) |
| 1.26.51 | 26.51.x | [0.1.0](../../releases/tag/v0.1.0) (source: [`mc-1.26.51` branch](../../tree/mc-1.26.51)) |

With lip: `lip install github.com/ch1-k-mc/BetterF3-Bedrock#client@0.1.0`

## Usage

| Key | Action |
|---|---|
| `F3` | Open / close the debug screen |
| `F3` + `M` | Open the settings screen |
| `F3` + `Q` | List all key combinations |

See [Usage](docs/USAGE.md) for all keys, the settings screen, commands and the config file.

## Features

- All 15 modules of the original, on the left and right of the screen
- Reorder, recolor and hide modules and lines in the settings screen
- F3 key combinations: hitboxes, chunk borders, copy location, advanced tooltips, game mode switcher and more
- Profiler, FPS / TPS and ping / bandwidth charts
- Server information on multiplayer servers
- English, Japanese and Chinese (Simplified / Traditional)

See [Features](docs/FEATURES.md) for the full list.

## Building

Requires [xmake](https://xmake.io), Visual Studio 2022 or later, and clang-cl.

```
xmake f -m release -y
xmake
```

The mod is output to `bin/BetterF3/`.

## License

[MIT](LICENSE). Based on [BetterF3](https://github.com/TreyRuffy/BetterF3) (MIT) by cominixo / TreyRuffy. See [NOTICE](NOTICE.md).
