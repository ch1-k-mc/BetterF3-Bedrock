# Changelog

## 0.2.0

Support Minecraft: Bedrock Edition 1.26.60 / LeviLamina 26.60 ([#1](../../issues/1)).

- Built for LeviLamina 26.60.x. For Minecraft 1.26.51, use 0.1.0.
- Added Chinese (Simplified / Traditional), from the original's language files and new translations of the Bedrock-specific text.
- Command (`/betterf3`) messages and log messages follow the game language (English / Japanese / Chinese) instead of Japanese only.
- Known limitations on 1.26.60 (LeviLamina 26.60 does not yet provide some game functions): Chunk Sections total and Forceloaded Chunks show 0, Packets Received and incoming bandwidth are not counted, `F3` + `D` and the protocol number in `F3` + `V` are not available, and settings text fields accept only letters, digits, `.`, `-` and `#`.

## 0.1.0 — first release

Initial port of BetterF3 (`architectury/26.3`, `d7c9ba6`) to Minecraft: Bedrock Edition 1.26.51 / LeviLamina 26.51 (client).

- F3 debug screen with all 15 modules of the original, default order, colors, formats, background, shadow, font scale and animations
- Settings screen (entry, general settings, left / right module lists, edit module, add module) re-created from the original, opened with F3 + M (key configurable)
- `/betterf3` command and a configuration file compatible with the original's Fabric config
- Java Edition F3 key combinations (A, B, C, D, F, G, H, I, L, N, P, Q, V, 1, 2, 3, F4, Esc)
- Profiler, FPS / TPS and ping / bandwidth charts
- Server information on remote servers
- English and Japanese
- C API for other mods (`better_f3_set_lines`)
