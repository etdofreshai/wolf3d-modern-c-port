# Autonomous progress and self-evaluation

This project is set up so progress can be evaluated automatically as the port grows.

## Current evaluation loop

For every small porting step:

1. **Write a failing shell test first** for the exact behavior being added.
2. **Run the test and watch it fail.**
3. Implement the minimum C code needed.
4. Re-run the specific test until it passes.
5. Run `tests/run_all.sh` to catch regressions.
6. Commit only after the full local check set passes.

## One-command project check

Run:

```bash
./tests/run_all.sh
```

This currently verifies:

- version smoke test
- Wolf3D data path validation
- `MAPHEAD.WL6` summary parsing
- additional asset inspections as they are added

## Progress rubric

The port should be judged by these milestones, in order:

1. **Scaffold works**
   - executable builds
   - smoke tests pass
2. **Data discovery works**
   - valid game-data folder is detected
   - missing required files fail cleanly
3. **Asset parsing works**
   - `MAPHEAD.WL6` parsed correctly
   - `GAMEMAPS.WL6` parsed correctly
   - graphics/audio headers parsed correctly
4. **Software rendering works**
   - framebuffer operations are deterministic
   - palette logic is testable
5. **Game boot works**
   - title / startup sequence reached without crash
6. **Behavioral compatibility improves**
   - reference outputs match original expectations

## Rules for autonomous continuation

When extending the port:

- keep each change small and testable
- prefer plain C reference implementations before optimization
- add a test for every new parser or behavior check
- avoid committing build artifacts or Steam-owned game data
- preserve gameplay/data compatibility over clever modernization

## Current blockers to note

The container currently lacks:

- `cmake`
- `SDL3` development files

So autonomous progress should continue first in the **portable C core**:

- file discovery
- asset parsing
- math helpers
- software framebuffer abstractions

Then SDL3 should be introduced once the environment supports it.
