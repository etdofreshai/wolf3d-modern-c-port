# Modern C Wolf3D port

This directory is the working area for an **exact-behavior Wolfenstein 3D port** written in **modern C**.

## Intent

- Keep the original game logic, data formats, and gameplay behavior as close as possible to the original source release.
- Preserve the original language choice: **C**.
- Replace obsolete 16-bit DOS, Borland, and handwritten assembly dependencies with modern, portable equivalents.

## Working definition of "exact port"

For this project, "exact port" means:

- same core gameplay rules
- same map/data file compatibility
- same enemy and weapon behavior
- same timing-sensitive logic where practical
- same rendering model and asset interpretation

It does **not** mean preserving:

- 16-bit memory model constraints
- DOS-only hardware interfaces
- Borland C++ 3.x project files
- self-modifying / generated assembly-era optimizations

## Planned modernization direction

- Language: **C17**
- Build system: **CMake**
- Platform layer: likely **SDL2** or **SDL3** for windowing, input, audio, and timing
- Rendering strategy: start with a software renderer that matches original behavior closely

## Source references

- Original source snapshot: `../source-original/`
- Game data: `../game-data/`
- Implementation plan: `docs/plans/2026-04-22-exact-modern-c-port.md`
