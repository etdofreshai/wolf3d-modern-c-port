# Wolf3D Exact Modern C Port Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task.

**Goal:** Build an exact-behavior Wolfenstein 3D port in modern C that preserves original game logic and data compatibility while replacing obsolete 16-bit DOS/Borland/assembly-era platform dependencies with portable systems.

**Architecture:** Keep the port in C and preserve the original module boundaries where practical (`WL_*`, `ID_*`, data-loading, gameplay, renderer). Split the codebase into two layers: a portable core that mirrors original behavior and a modern platform layer for video, audio, input, files, and timing. Replace assembly and memory-model assumptions with clear C implementations, emphasizing correctness before optimization.

**Tech Stack:** C17, CMake, SDL2/SDL3 for platform services, original Steam-sourced game data in `../game-data/`, tests run with CTest plus targeted golden/reference checks.

---

## Ground truth gathered from the original source

- Original codebase language is primarily **C**, with **26 `.C` files**, **44 headers**, and **10 `.ASM` files** in `source-original/WOLFSRC/`.
- Upstream `README.rst` states the original project was built in **Borland C++ 3.0/3.1**.
- The Steam data currently available includes:
  - base Wolf3D registered `.WL6` data in `../game-data/base/`
  - Spear of Destiny `.SOD` data in `../game-data/base/m1/`
  - Steam DOSBox packaging in `../game-data/DOSBox/`

## Non-goals for the first implementation pass

- New content, gameplay changes, or quality-of-life redesigns
- Hardware-accurate DOS emulation inside the port
- Immediate rewrite to GPU rendering
- Multiplayer, mod framework, or editor work

## Directory target

All new implementation work should live under:

- `source-modern-c/`

Recommended structure:

- `source-modern-c/CMakeLists.txt`
- `source-modern-c/README.md`
- `source-modern-c/docs/plans/2026-04-22-exact-modern-c-port.md`
- `source-modern-c/include/wolf3d/...`
- `source-modern-c/src/core/...`
- `source-modern-c/src/platform/...`
- `source-modern-c/src/game/...`
- `source-modern-c/tests/...`
- `source-modern-c/tools/...`

---

### Task 1: Create the modern port scaffold

**Objective:** Establish the new project layout and build entry point for a clean modern-C implementation.

**Files:**
- Create: `source-modern-c/CMakeLists.txt`
- Create: `source-modern-c/include/wolf3d/port.h`
- Create: `source-modern-c/src/main.c`
- Create: `source-modern-c/src/platform/platform_stub.c`
- Create: `source-modern-c/tests/smoke_test.c`

**Step 1: Write failing smoke test registration**

Create a trivial test target that expects the executable to run with `--version` and exit successfully.

**Step 2: Run test to verify failure**

Run: `cmake -S source-modern-c -B source-modern-c/build && cmake --build source-modern-c/build && ctest --test-dir source-modern-c/build --output-on-failure`

Expected: FAIL because the executable or test target does not yet exist.

**Step 3: Write minimal implementation**

- Add a CMake project targeting C17.
- Build an executable named `wolf3d_port`.
- Add `main.c` that prints a placeholder version string for `--version`.
- Add a placeholder platform translation unit to establish the layering.

**Step 4: Run test to verify pass**

Run the same commands and confirm the smoke test passes.

**Step 5: Commit**

```bash
git add source-modern-c
git commit -m "build: scaffold modern c wolf3d port"
```

---

### Task 2: Inventory and map the original modules

**Objective:** Create a migration map from Borland-era source files to portable modern modules.

**Files:**
- Create: `source-modern-c/docs/original-module-map.md`
- Read: `source-original/WOLFSRC/*.C`
- Read: `source-original/WOLFSRC/*.H`
- Read: `source-original/WOLFSRC/*.ASM`

**Step 1: Document each original subsystem**

Group modules into:
- engine/core loop
- renderer
- audio
- input
- memory/cache/page manager
- menus/UI
- game logic/actors
- asset/data loading
- platform/compiler glue

**Step 2: Identify modern replacements**

For each Borland/DOS concern, specify the replacement:
- `conio.h`/DOS UI → SDL event/input + stdio logging where needed
- timer/interrupt assumptions → SDL timing or portable monotonic clock
- VGA direct access → software framebuffer in system memory
- AdLib/SoundBlaster paths → SDL audio mixer backend
- assembly routines → plain C reference implementations first

**Step 3: Verify completeness**

Confirm every original `.C` and `.ASM` file is accounted for in the mapping document.

**Step 4: Commit**

```bash
git add source-modern-c/docs/original-module-map.md
git commit -m "docs: map original wolf3d modules to modern port"
```

---

### Task 3: Freeze the behavioral compatibility target

**Objective:** Write down what “exact port” means in testable terms so future implementation does not drift.

**Files:**
- Create: `source-modern-c/docs/behavioral-compatibility.md`
- Test: `source-modern-c/tests/behavioral_targets.md` (documentation checklist or future golden-test manifest)

**Step 1: Define exactness categories**

Document expected equivalence for:
- map loading
- wall rendering order
- actor AI and movement
- hit detection
- weapon behavior
- damage rules
- intermission/stat screens
- save/load compatibility goals (if in scope for v1)

**Step 2: Record acceptable modernization changes**

Explicitly allow:
- 32/64-bit clean pointer usage
- portable file I/O
- SDL-managed window/input/audio
- replacing assembly with C where behavior remains equivalent

**Step 3: Record deferred areas**

Mark any behavior that may temporarily differ during bring-up:
- palette presentation details
- audio emulation nuances
- exact frame pacing until timer layer is stabilized

**Step 4: Commit**

```bash
git add source-modern-c/docs/behavioral-compatibility.md
git commit -m "docs: define exact-port compatibility target"
```

---

### Task 4: Create the portable platform abstraction layer

**Objective:** Isolate modern OS/library dependencies behind narrow C interfaces.

**Files:**
- Create: `source-modern-c/include/wolf3d/platform.h`
- Create: `source-modern-c/src/platform/platform.c`
- Create: `source-modern-c/src/platform/sdl_app.c`
- Create: `source-modern-c/src/platform/sdl_audio.c`
- Create: `source-modern-c/src/platform/sdl_input.c`
- Create: `source-modern-c/src/platform/sdl_video.c`
- Test: `source-modern-c/tests/test_platform_init.c`

**Step 1: Write failing test**

Add a test for platform init/shutdown returning success in a headless or dummy-driver configuration.

**Step 2: Run test to verify failure**

Run the specific test target and confirm missing symbols or stub failures.

**Step 3: Write minimal implementation**

Expose functions like:

```c
bool wolf_platform_init(void);
void wolf_platform_shutdown(void);
uint64_t wolf_platform_ticks_us(void);
```

Back them with SDL and keep the API free of SDL types where possible.

**Step 4: Run test to verify pass**

Use SDL dummy drivers if needed for CI-like environments.

**Step 5: Commit**

```bash
git add source-modern-c/include/wolf3d/platform.h source-modern-c/src/platform source-modern-c/tests/test_platform_init.c
git commit -m "feat: add portable platform abstraction layer"
```

---

### Task 5: Implement data-path discovery for Steam-sourced assets

**Objective:** Load the copied game assets cleanly from `../game-data/` without baking in DOS assumptions.

**Files:**
- Create: `source-modern-c/include/wolf3d/filesystem.h`
- Create: `source-modern-c/src/platform/filesystem.c`
- Create: `source-modern-c/tests/test_filesystem_paths.c`

**Step 1: Write failing test**

Test that the port can resolve and validate a Wolf3D data directory containing required `.WL6` files.

**Step 2: Run test to verify failure**

Expected: the resolver does not exist yet.

**Step 3: Write minimal implementation**

Support:
- explicit command-line data path
- default relative path search for `../game-data/base`
- validation for required files like `VGAGRAPH.WL6`, `VSWAP.WL6`, `MAPHEAD.WL6`, `GAMEMAPS.WL6`

**Step 4: Run test to verify pass**

Confirm both success and missing-file error behavior.

**Step 5: Commit**

```bash
git add source-modern-c/include/wolf3d/filesystem.h source-modern-c/src/platform/filesystem.c source-modern-c/tests/test_filesystem_paths.c
git commit -m "feat: detect and validate wolf3d data paths"
```

---

### Task 6: Build a reference software framebuffer path

**Objective:** Replace VGA-memory assumptions with a portable software framebuffer while preserving original rendering semantics.

**Files:**
- Create: `source-modern-c/include/wolf3d/video.h`
- Create: `source-modern-c/src/platform/video_framebuffer.c`
- Create: `source-modern-c/tests/test_framebuffer.c`
- Read/port from: original `ID_VL.C`, `ID_VH.C`, `WL_DRAW.C`, related headers

**Step 1: Write failing tests**

Test framebuffer allocation, pixel writes, column draws, and palette-backed blit behavior at the chosen logical resolution.

**Step 2: Run test to verify failure**

Expected: no framebuffer implementation yet.

**Step 3: Write minimal implementation**

- store an 8-bit indexed framebuffer in memory
- support palette lookup
- expose upload-to-window through platform code
- keep APIs small and deterministic

**Step 4: Run test to verify pass**

Verify framebuffer operations without requiring full game startup.

**Step 5: Commit**

```bash
git add source-modern-c/include/wolf3d/video.h source-modern-c/src/platform/video_framebuffer.c source-modern-c/tests/test_framebuffer.c
git commit -m "feat: add software framebuffer path"
```

---

### Task 7: Port the low-level asset readers first

**Objective:** Get original data file parsing working before attempting full game execution.

**Files:**
- Create: `source-modern-c/include/wolf3d/assets.h`
- Create: `source-modern-c/src/core/assets.c`
- Create: `source-modern-c/tests/test_assets_wl6.c`
- Read/port from: `ID_CA.C`, `ID_PM.C`, `ID_MM.C`, `MAPS*.H`, `GFX*.H`, `AUDIO*.H`

**Step 1: Write failing tests**

Add tests that:
- open the `.WL6` set from `../game-data/base/`
- parse headers
- verify expected chunk counts and non-empty data blocks

**Step 2: Run test to verify failure**

Expected: parsing functions absent or incomplete.

**Step 3: Write minimal implementation**

Implement clean C readers for:
- map headers
- graphics/audio dictionaries and headers
- chunk-offset validation

Avoid porting page-manager complexity until correctness is established.

**Step 4: Run test to verify pass**

Confirm the readers can parse real Steam-sourced files.

**Step 5: Commit**

```bash
git add source-modern-c/include/wolf3d/assets.h source-modern-c/src/core/assets.c source-modern-c/tests/test_assets_wl6.c
git commit -m "feat: parse original wl6 asset files"
```

---

### Task 8: Port the core fixed-point and math helpers from assembly/C to clean C

**Objective:** Preserve behavior-critical numeric routines without carrying forward 16-bit compiler assumptions.

**Files:**
- Create: `source-modern-c/include/wolf3d/math_fixed.h`
- Create: `source-modern-c/src/core/math_fixed.c`
- Create: `source-modern-c/tests/test_math_fixed.c`
- Read/port from: `C0.ASM`, `H_LDIV.ASM`, `WL_ASM.ASM`, `WL_DR_A.ASM`, `ID_VL_A.ASM`, `ID_VH_A.ASM`, `ID_SD_A.ASM`, `WHACK_A.ASM`

**Step 1: Write failing tests**

Capture edge cases for fixed-point multiply/divide, angle math, scaling math, and clipping-sensitive routines.

**Step 2: Run test to verify failure**

Expected: routines unimplemented.

**Step 3: Write minimal implementation**

Implement readable C equivalents first, using fixed-width integer types and explicit overflow expectations.

**Step 4: Run test to verify pass**

Compare outputs against original formulas or extracted reference data where available.

**Step 5: Commit**

```bash
git add source-modern-c/include/wolf3d/math_fixed.h source-modern-c/src/core/math_fixed.c source-modern-c/tests/test_math_fixed.c
git commit -m "feat: port fixed-point math helpers to modern c"
```

---

### Task 9: Bring up the game loop and renderer using original module structure

**Objective:** Reach first playable rendering with preserved raycasting behavior.

**Files:**
- Create or modify: `source-modern-c/src/game/wl_main.c`
- Create or modify: `source-modern-c/src/game/wl_draw.c`
- Create or modify: `source-modern-c/src/game/wl_game.c`
- Create or modify: `source-modern-c/src/game/wl_play.c`
- Test: `source-modern-c/tests/test_boot_to_title.c`
- Read/port from: `WL_MAIN.C`, `WL_DRAW.C`, `WL_GAME.C`, `WL_PLAY.C`, `WL_SCALE.C`, `WL_STATE.C`

**Step 1: Write failing test**

Define a boot smoke test that initializes data, starts the engine, and reaches the title or attract sequence without crashing.

**Step 2: Run test to verify failure**

Expected: unported subsystems block startup.

**Step 3: Write minimal implementation**

Port the original flow in stages:
- initialization
- resource setup
- title/startup loop
- world rendering entry points

Prefer preserving original function names where it helps traceability.

**Step 4: Run test to verify pass**

Confirm the engine boots and produces frames.

**Step 5: Commit**

```bash
git add source-modern-c/src/game source-modern-c/tests/test_boot_to_title.c
git commit -m "feat: boot modern port to title loop"
```

---

### Task 10: Port input, menus, and gameplay interaction

**Objective:** Make the port controllable and usable with keyboard input while retaining original menu/game behavior.

**Files:**
- Create or modify: `source-modern-c/src/game/id_in.c`
- Create or modify: `source-modern-c/src/game/wl_menu.c`
- Create or modify: `source-modern-c/src/game/wl_inter.c`
- Test: `source-modern-c/tests/test_menu_navigation.c`
- Read/port from: `ID_IN.C`, `WL_MENU.C`, `WL_INTER.C`

**Step 1: Write failing tests**

Add deterministic tests for menu navigation logic and action dispatch where practical.

**Step 2: Run test to verify failure**

Expected: no menu/input compatibility layer yet.

**Step 3: Write minimal implementation**

Translate SDL events into the original command model and preserve menu state logic.

**Step 4: Run test to verify pass**

Verify keyboard-driven navigation and starting a game.

**Step 5: Commit**

```bash
git add source-modern-c/src/game/id_in.c source-modern-c/src/game/wl_menu.c source-modern-c/src/game/wl_inter.c source-modern-c/tests/test_menu_navigation.c
git commit -m "feat: port input and menu flow"
```

---

### Task 11: Port audio last, after core correctness exists

**Objective:** Add modern playback for original sound and music assets without blocking the core port.

**Files:**
- Create or modify: `source-modern-c/src/game/id_sd.c`
- Create or modify: `source-modern-c/tests/test_audio_asset_loading.c`
- Read/port from: `ID_SD.C`, audio headers, sound-mode definitions

**Step 1: Write failing tests**

Test that sound assets load and basic playback queues without crashing.

**Step 2: Run test to verify failure**

Expected: audio backend absent or stubbed.

**Step 3: Write minimal implementation**

Start with functional digital playback and stub music if needed, then iterate toward closer fidelity.

**Step 4: Run test to verify pass**

Confirm initialization and asset loading succeed.

**Step 5: Commit**

```bash
git add source-modern-c/src/game/id_sd.c source-modern-c/tests/test_audio_asset_loading.c
git commit -m "feat: add basic audio playback backend"
```

---

### Task 12: Add reference-based compatibility verification

**Objective:** Prevent behavioral drift while the port grows.

**Files:**
- Create: `source-modern-c/tests/reference/`
- Create: `source-modern-c/tools/extract_reference_data.py`
- Create: `source-modern-c/tests/test_reference_behavior.c`

**Step 1: Build golden references**

Capture reproducible reference outputs such as:
- map metadata
- actor state transitions
- selected raycast column heights
- deterministic gameplay state snapshots

**Step 2: Write failing tests**

Assert the modern port matches stored references.

**Step 3: Implement harness**

Add tooling to produce and compare machine-readable outputs.

**Step 4: Run test to verify pass**

Run the compatibility suite and ensure it stays green as subsystems are ported.

**Step 5: Commit**

```bash
git add source-modern-c/tests/reference source-modern-c/tools/extract_reference_data.py source-modern-c/tests/test_reference_behavior.c
git commit -m "test: add compatibility reference harness"
```

---

## Key technical decisions

1. **Language stays C.** We are not translating the game to Rust, C++, C#, or another language.
2. **Modernization happens at the platform boundary.** SDL replaces DOS-era I/O, not the game design.
3. **Behavior over cleverness.** Prefer a straightforward C implementation that matches original outputs before optimizing.
4. **Assembly is translated, not preserved.** Any `.ASM` routine should become readable, tested C first.
5. **Data compatibility is mandatory.** The port must use the copied Steam assets in `../game-data/`.

## Highest-risk areas

- 16-bit assumptions and pointer-size dependencies
- self-modifying/generated scaling and handwritten assembly paths
- timer/input behavior tied to original DOS expectations
- renderer exactness versus modern frame timing
- audio fidelity differences after replacing legacy hardware paths

## Immediate next milestone

Reach: **"Boot the modern C executable, find `../game-data/base`, parse `.WL6` headers, and open a window with a software framebuffer."**

## Verification commands

Once scaffolded, use commands like:

```bash
cmake -S source-modern-c -B source-modern-c/build
cmake --build source-modern-c/build
ctest --test-dir source-modern-c/build --output-on-failure
```

For data validation during bring-up:

```bash
./source-modern-c/build/wolf3d_port --data ../game-data/base --version
```

---

Plan complete and saved. Ready to execute as a real implementation sequence in `source-modern-c/`.
