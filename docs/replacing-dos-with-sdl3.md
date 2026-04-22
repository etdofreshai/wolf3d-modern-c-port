# Replacing DOS/Borland-era APIs with SDL3 and modern C

This review is based on the original Wolf3D source in `../source-original/WOLFSRC/` and is intended to guide the modern exact-behavior port.

## Big picture

The original code is mostly portable game logic wrapped around a **very non-portable DOS-era platform layer**.

The main things that need replacement are:

1. **Video hardware access**
2. **Audio hardware and timer interrupts**
3. **Keyboard/mouse/joystick interrupt handling**
4. **16-bit memory model / far-pointer assumptions**
5. **Borland/Turbo DOS file and runtime APIs**
6. **Handwritten assembly or generated code used for performance**

The good news: the gameplay, map logic, AI, state machines, and most data formats can remain conceptually the same.

---

## 1. Video / drawing / palette management

### Original DOS-specific pieces

Primary files:
- `ID_VL.C`
- `ID_VH.C`
- `WL_DRAW.C`
- `WL_SCALE.C`
- `CONTIGSC.C`
- `WL_DEBUG.C`

Examples found in the original source:
- `#include <dos.h>`, `#include <alloc.h>`, `#include <mem.h>` in `ID_VL.C`
- VGA register I/O via `outportb()` / `inportb()`
- direct video memory access via `MK_FP(SCREENSEG, ...)`
- planar VGA logic using `VGAMAPMASK`, `VGAREADMAP`, `EGAMAPMASK`
- page flipping using `bufferofs`, `displayofs`, and VGA memory pages
- palette DAC writes using `PEL_WRITE_ADR` / `PEL_DATA`

### What should replace it

Use **SDL3** only for:
- window creation
- presenting a completed software frame
- receiving resize / quit events
- optionally texture upload to the window

Keep the renderer itself in **plain C** with an internal software framebuffer.

### Recommended replacement design

- Maintain an internal **8-bit indexed framebuffer** that mirrors the original rendering model.
- Keep palette logic as explicit C data (`uint8_t palette[256][3]`).
- Replace VGA page flipping with either:
  - one backbuffer plus one presented buffer, or
  - an SDL texture updated each frame from the software framebuffer.
- Replace planar-memory assumptions with linear packed memory while preserving draw order and logic.
- Re-implement fizzle fades, palette fades, pic draws, and wall scaling as deterministic C routines.

### SDL3 APIs likely to use

- `SDL_Init(SDL_INIT_VIDEO)`
- `SDL_CreateWindow()`
- `SDL_CreateRenderer()` or direct surface/texture path
- `SDL_CreateTexture()`
- `SDL_UpdateTexture()`
- `SDL_RenderTexture()` / `SDL_RenderPresent()`
- `SDL_PollEvent()`

### Important caution

**Do not hand the actual raycaster over to SDL rendering primitives.**
For exactness, SDL3 should present pixels, not replace the rendering model.

---

## 2. Sound, music, timer, and hardware detection

### Original DOS-specific pieces

Primary file:
- `ID_SD.C`

Related files:
- `DETECT.C`
- `ID_SD_A.ASM`

Examples found in the original source:
- `#include <dos.h>` in `ID_SD.C`
- SoundBlaster detection and programming
- AdLib register writes with `outportb()` / `inportb()`
- Sound Source and PC speaker paths
- DMA setup for SoundBlaster playback
- timer interrupt replacement with `setvect(8, ...)`
- ISR-style services: `SDL_t0Service`, `SDL_SBService`, etc. (note: this is old in-source naming, not modern SDL)
- direct PIC interrupt acknowledgements via `outportb(0x20, 0x20)`

### What should replace it

Use **SDL3 audio and timing**, not direct hardware emulation.

### Recommended replacement design

Split audio into two concerns:

#### A. Game-facing audio API
Keep a small C API such as:

```c
bool wolf_audio_init(void);
void wolf_audio_shutdown(void);
void wolf_audio_play_sfx(int sound_id, int left, int right);
void wolf_audio_start_music(int music_id);
void wolf_audio_stop_music(void);
```

#### B. SDL3 backend
Use SDL3 for:
- audio device initialization
- queuing/mixing PCM output
- timing support where the original code depended on timer interrupts

### What to do about original sound device modes

Original modes include:
- PC speaker
- AdLib
- Sound Source
- SoundBlaster digitized playback

For a modern exact port, the practical strategy is:

- preserve the **game’s selection and triggering behavior**
- translate final playback to **SDL3 audio output**
- optionally keep separate emulation paths later if exact device-character fidelity matters

### Music and sound fidelity strategy

Recommended order:

1. get **digitized sound effects** loading and playing through SDL3
2. implement or stub music selection logic
3. later decide how far to go on **AdLib/OPL** fidelity:
   - minimal first pass: functional placeholder or converted playback path
   - higher fidelity later: integrate an OPL emulator library behind the C audio layer

### Timing replacement

The original audio layer is tied to timer interrupts.
Replace that with:
- a fixed-timestep game tick driven from SDL3 high-resolution time, or
- an explicit update call from the main loop

Likely SDL3 timing APIs:
- `SDL_GetTicks()` or high-resolution timer facilities
- `SDL_Delay()` only for coarse fallback, not core simulation correctness

### Important caution

Do **not** try to port DMA, PIC, or IRQ code literally.
That logic should become queueing, scheduling, and state updates in plain C.

---

## 3. Keyboard, mouse, and joystick input

### Original DOS-specific pieces

Primary file:
- `ID_IN.C`

Examples found in the original source:
- keyboard ISR via `getvect()` / `setvect()`
- raw keyboard controller I/O via `inportb(0x60)` and `outportb(0x61, ...)`
- BIOS memory pokes with `peek()` / `poke()`
- DOS mouse interrupt calls via `geninterrupt(MouseInt)`
- joystick polling through I/O port `0x201`

### What should replace it

Use **SDL3 events and state polling**.

### Recommended replacement design

- Map SDL keyboard events into the original internal key/button abstractions.
- Replace custom keyboard interrupt buffering with a small software event/state layer.
- Replace DOS mouse calls with SDL mouse motion/button APIs.
- Replace joystick port reads with SDL gamepad/joystick APIs only if needed.

Likely SDL3 APIs:
- `SDL_PollEvent()`
- keyboard key events / scancodes
- mouse motion and button events
- optional joystick/gamepad APIs

### Important caution

Preserve **game-level semantics**, not DOS scancode plumbing.
The goal is behavior equivalence, not port-level equivalence.

---

## 4. 16-bit memory model, far pointers, and segmented memory

### Original DOS-specific pieces

Primary files:
- `ID_MM.C`
- `ID_PM.C`
- `ID_CA.C`
- `WL_SCALE.C`
- `CONTIGSC.C`
- multiple headers such as `ID_MM.H`, `ID_CA.H`, `ID_VL.H`, `WL_DEF.H`

Examples found in the original source:
- `far`, `huge`, `_seg`, `interrupt`
- `memptr` defined as segmented pointer type
- `farmalloc()` / `farfree()`
- `FP_SEG`, `FP_OFF`, `MK_FP`
- `_fmemcpy`, `_fmemset`, `_fmemcmp`
- `movedata()`
- EMS/XMS-era logic and `geninterrupt()` in `ID_PM.C`

### What should replace it

Use normal 32/64-bit memory and fixed-width integer types.

### Recommended replacement design

- Replace segmented pointers with ordinary pointers.
- Replace far-memory copies with `memcpy`, `memset`, `memcmp`.
- Replace memory managers with simpler ownership-based allocators first.
- Convert page-manager logic into explicit heap buffers and file-backed chunk caches.
- Use `uint8_t`, `uint16_t`, `uint32_t`, `int32_t`, etc. for exact data handling.

### Important caution

Do not preserve complexity that only existed to survive 16-bit DOS limits.
Some of the original memory/page logic should be behaviorally preserved only where it changes data interpretation or loading order.

---

## 5. Borland/Turbo DOS file and runtime APIs

### Original DOS-specific pieces

Examples found across the source:
- `open`, `read`, `write`, `close`, `lseek`, `filelength`
- `_dos_write`
- binary config/save/data access through DOS-style descriptors

### What should replace it

Use portable C or POSIX-like wrappers behind a narrow filesystem API.

### Recommended replacement design

- Create a small file abstraction for opening game assets and configs.
- Use standard C file APIs or small POSIX wrappers internally.
- Normalize paths for:
  - `../game-data/base`
  - `../game-data/base/m1`
- Keep save/config binary layout compatibility only where needed.

### Important caution

File I/O itself is not hard to replace. The real issue is to avoid carrying DOS path assumptions or segmented-memory reading code into the port.

---

## 6. Assembly and self-modifying/generated code

### Original DOS-specific pieces

Files include:
- `WL_ASM.ASM`
- `WL_DR_A.ASM`
- `ID_VL_A.ASM`
- `ID_VH_A.ASM`
- `ID_SD_A.ASM`
- `WHACK_A.ASM`
- `H_LDIV.ASM`
- `C0.ASM`
- generated scaler logic in `WL_SCALE.C`, `CONTIGSC.C`, `OLDSCALE.C`

### What should replace it

Use clear, testable **C implementations first**.

### Recommended replacement design

- Treat assembly routines as reference behavior, not source to preserve literally.
- Write tests for fixed-point math, scaling, clipping, and audio sequencing behavior.
- Port to readable C using fixed-width integers.
- Optimize later only after reference tests exist.

### Important caution

The original source itself warns that some dynamically generated scaling paths are a poor fit for modern CPUs.
That is a strong signal to replace them with straightforward C.

---

## 7. What can mostly stay the same

These parts are likely to port with much less conceptual change:

- map and asset formats
- game state machines
- actor logic
- weapon logic
- raycasting structure
- menu flow logic
- score/config/game rules
- save/load concepts, if binary compatibility is pursued carefully

These are the places where we should preserve naming and module structure where practical.

---

## Recommended modern layering

## Layer A: core game logic
Plain C, no SDL headers exposed.

Owns:
- gameplay
- map loading
- actor logic
- fixed-point math
- software renderer
- game timing rules

## Layer B: SDL3 platform backend
SDL3 only.

Owns:
- window
- presenting the framebuffer
- input collection
- audio device output
- wall-clock timing
- filesystem/environment helpers if desired

This keeps the port exact where it matters and modern where it must be.

---

## Replacement summary table

| Old system | Original examples | Modern replacement |
|---|---|---|
| VGA registers / planar VRAM | `outportb`, `MK_FP`, `VGAMAPMASK` | internal software framebuffer + SDL3 presentation |
| Palette DAC writes | `PEL_WRITE_ADR`, `PEL_DATA` | C palette arrays + SDL3 texture/surface upload |
| SoundBlaster DMA / IRQ | `setvect`, DMA ports, PIC ack | SDL3 audio callback/queue + C mixer/scheduler |
| AdLib register writes | `selreg`, `writereg` | C music/sfx backend, optionally OPL emulation later |
| Keyboard ISR / BIOS state | `getvect`, `setvect`, `peek`, `poke` | SDL3 keyboard events/state |
| Mouse interrupt API | `geninterrupt(MouseInt)` | SDL3 mouse events/state |
| Joystick port reads | `inportb(0x201)` | SDL3 joystick/gamepad APIs |
| Far memory model | `far`, `huge`, `farmalloc`, `FP_SEG` | normal pointers + `malloc`/`memcpy` |
| EMS/XMS/page tricks | `geninterrupt`, segmented page manager | heap-backed caches and direct file readers |
| Borland file/runtime details | `filelength`, `_dos_write` | portable file wrappers |
| ASM/scaler codegen | `.ASM`, generated scalers | readable tested C implementations |

---

## Recommendation for this port

Use **SDL3** as the platform boundary for:
- video presentation
- input
- timing
- audio output

But keep these in **plain C**:
- the renderer
- game logic
- data loading
- math
- deterministic simulation behavior

That combination is the best fit for an exact modern port.

## Immediate next implementation steps

1. scaffold a C project that can build with a stub backend
2. define the SDL3-facing platform API
3. implement data path validation for `../game-data/base`
4. build the software framebuffer path
5. begin asset parsing before full gameplay bring-up
