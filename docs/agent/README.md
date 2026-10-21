# docs/agent

Documents written for an agent working on this repository. They record decisions
that are already settled, so they are not re-litigated by accident, and the order
in which the remaining work should land.

Not end-user documentation. Nothing here ships.

## Files

| File | Read it when |
|---|---|
| `design.md` | before touching structure, boundaries, naming or the build |
| `protocol.md` | before changing anything that goes over the pipe |
| `plan.md` | before starting work, to find the current milestone |

## Working rules

1. **Ask before changing a decision in `design.md`.** If something there looks
   wrong, report it and wait. Several of those decisions replaced earlier ones,
   so churn is expensive.
2. **Do not configure, build or commit unless asked.** `cmake --preset win-msvc`
   and `cmake --build --preset win-debug` are cheap, but a green build in the
   wrong direction is not.
3. **Do not create a directory that holds no file.** An empty directory promises
   work that does not exist. Everything is globbed, so a target appears on its
   own once a source lands.
4. **Script in Python, not PowerShell.** Quoting and BOM handling in PowerShell
   has already cost two broken commands in this repository's history.
5. **The scope today is Windows and the CLI only.** No macOS, no Linux, no cloud
   adapter, no GUI shell. See `plan.md` for what "done" means right now.

## Verifying the current state

```
cmake --preset win-msvc
cmake --build --preset win-debug
build/win-msvc/src/core/Debug/famulusd.exe --help
cmake --preset win-null        # portability gate: must always configure
```
