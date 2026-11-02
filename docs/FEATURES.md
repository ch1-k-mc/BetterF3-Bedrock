# Features

[日本語](FEATURES.ja.md)

## Modules

**Left**

| Module | Shows |
|---|---|
| Minecraft | Game and LeviLamina version |
| FPS | Current FPS and FPS limit |
| Graphics | Render distance, graphics mode, clouds |
| Server | Server tick time or server version, packets sent / received |
| Coordinates | Position, block, chunk, velocity |
| Chunks | Rendered sections, loaded chunks, force-loaded chunks |
| Location | Dimension, facing, rotation, light, heightmaps, biome, day ticks, slime chunk |
| Entities | Particles, rendered / total entities, counts per category |
| Sounds | Playing sounds and ambient sounds |
| Help | Chart status and help hint |
| Separator | Empty lines |
| Misc Left | Lines from other mods ([API](USAGE.md#api-for-other-mods)) |

**Right**

| Module | Shows |
|---|---|
| System | Time, OS, memory, CPU, display, GPU, graphics API, driver |
| Misc Right | Lines from other mods |
| Separator | Empty lines |
| Targeted | Targeted block, fluid and entity |

## F3 actions

Hitboxes, chunk borders, copy location / block / entity, advanced tooltips, render distance, reload chunks, clear chat, spectator toggle, pause on lost focus, version info, profiling, game mode switcher, pause without menu. See [Keys](USAGE.md#keys).

## Charts

- `F3` + `1`: profiler pie chart
- `F3` + `2`: FPS and TPS
- `F3` + `3`: ping and bandwidth

## Differences from Java Edition

- The *Java Version* line shows the OS instead.
- On multiplayer servers, server-side heightmaps and chunk counts are not shown.
- The slime chunk check uses the Bedrock formula and also works on servers.
- `F3` + `Esc` resumes when pressed again.

## Not available

- Lines: Biome Blend Radius, Shader, Tick Manager Status, Chunk Culling, Pending Chunks, Pending Uploads, Available Buffers, Spawn Chunks, Region File, Local Difficulty, Mood, Fluid Tags
- Keys: `F3` + `S` / `T` / `X` / `4` / `F6`, `Ctrl` + `F3` + `F`

## Known issues

- Client only.
- Bedrock's coordinate display and chat overlap the left side of the debug screen.
