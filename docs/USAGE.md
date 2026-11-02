# Usage

[日本語](USAGE.ja.md)

## Keys

Press `F3` to open or close the debug screen. Hold `F3` and press a second key for the actions below.

| Keys | Action |
|---|---|
| `F3` + `Q` | List all key combinations |
| `F3` + `M` | Open the settings screen (key can be changed) |
| `F3` + `A` | Reload chunks |
| `F3` + `B` | Show hitboxes |
| `F3` + `C` | Copy location as a `/tp` command |
| `F3` + `D` | Clear chat history |
| `F3` + `F` | Render distance +1 (`Shift`: −1) |
| `F3` + `G` | Show chunk borders |
| `F3` + `H` | Advanced tooltips (durability and item ID) |
| `F3` + `I` | Copy the targeted block or entity as a command |
| `F3` + `L` | Record a 10-second profile to `mods/BetterF3/debug/` |
| `F3` + `N` | Toggle spectator mode |
| `F3` + `P` | Toggle pause on lost focus |
| `F3` + `V` | Show version information |
| `F3` + `1` / `2` / `3` | Profiler / FPS and TPS / ping and bandwidth charts |
| `F3` + `F4` | Game mode switcher |
| `F3` + `Esc` | Pause without the pause menu |

## Settings screen

Open it with `F3` + `M`.

- **General Settings**: all options below
- **Left / Right Modules**: reorder (click the arrows at the left edge of a row), add, delete and edit modules
- **Edit Module**: enable, colors and per-line visibility

## Commands

| Command | Description |
|---|---|
| `/betterf3 toggle` | Open / close the debug screen |
| `/betterf3 get [key]` | Show options |
| `/betterf3 set <key> <value>` | Change an option |
| `/betterf3 reset` | Reset everything to the defaults |
| `/betterf3 save` / `reload` | Save / reload the config file |
| `/betterf3 module list` | List modules with their numbers |
| `/betterf3 module <left\|right> <n> enable\|disable` | Enable / disable a module |
| `/betterf3 module <side> <n> line <line> on\|off` | Show / hide a line |
| `/betterf3 module <side> <n> set <key> <value>` | Change a module setting |
| `/betterf3 module <side> add <id> [n]` / `remove <n>` / `move <from> <to>` | Add / remove / move modules |
| `/betterf3 lang [en_us\|ja_jp]` | Force the language (no argument: follow the game) |

Module ids: `minecraft` `fps` `graphics` `server` `coords` `chunks` `location` `entity` `sound` `help` `empty` `miscleft` `system` `miscright` `target`

## Config

`mods/BetterF3/config/betterf3.json` — compatible with the Fabric config of the original mod.

| Key | Default | Description |
|---|---|---|
| `disable_mod` | `false` | Disable the debug screen |
| `auto_start` | `false` | Open the debug screen when joining a world |
| `space_modules` | `false` | Empty line between modules |
| `shadow_text` | `true` | Text shadow |
| `animations` | `true` | Open / close animation |
| `animationSpeed` | `1.0` | Animation speed (1–3) |
| `fontScale` | `1.0` | Font scale (0.1–2) |
| `background_color` | `#6F505050` | Line background (ARGB) |
| `hide_debug_crosshair` | `false` | Keep the normal crosshair |
| `hide_sidebar` | `true` | Hide the scoreboard while open |
| `hide_bossbar` | `true` | Hide boss bars while open |
| `always_show_profiler` | `false` | Always show the profiler chart |
| `always_show_tps` | `false` | Always show the FPS / TPS charts |
| `always_show_ping` | `false` | Always show the ping charts |
| `performance_optimizations` | `true` | Update values every 250 ms |
| `config_screen_key` | `"M"` | Key for `F3` + key to open the settings screen |

## API for other mods

Other mods can add lines to the *Misc Left* / *Misc Right* modules:

```cpp
using SetLines = void (*)(char const* owner, int right, char const* const* lines, int count);
auto setLines = reinterpret_cast<SetLines>(
    GetProcAddress(GetModuleHandleW(L"BetterF3.dll"), "better_f3_set_lines"));
char const* lines[] = {"My Mod: on"};
if (setLines) setLines("MyMod", 0, lines, 1); // 0 = left, 1 = right
```

See [`src/api/PublicApi.h`](../src/api/PublicApi.h).
