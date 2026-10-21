# Design

Settled decisions. Change these only with the human's agreement.

## What famulus is

A resident assistant. One always-on daemon holds state and policy; every surface
is a separate process that speaks a protocol to it. An LLM provider is one
dependency of the daemon, not the architecture.

Today: Windows only, CLI surface only. The design keeps the platform seam and the
protocol seam open, because those are the two things that would be expensive to
retrofit.

## Directories, and what each may depend on

The rule is a dependency table, the same shape systemd and VS Code use: a
directory may use only those above it.

| Directory | Contents | May depend on |
|---|---|---|
| `src/proto/` | the contract: schemas, message types, constants. No code. | nothing |
| `src/base/` | our own foundation: framing, encoding, small utilities | nothing in the tree |
| `src/platform/` | the abstracted environment and the local transport | `base` |
| `src/core/` | the daemon: sessions, artifacts, providers, scheduling, policy | `base`, `platform` |
| `src/shell/` | the surfaces: `cli`, `win-desktop` | `base`, `platform` |
| `tests/` | all tests, outside `src/` because no test ships | anything, per suite |

Names were chosen for what a directory *is*, not for who uses it: `platform` is
shared by daemon and shells alike, so "shared" or "common" would describe
nothing. `base` is the word both precedent projects use for the bottom layer;
`core` is the daemon, which is also what systemd means by `src/core`.

Two things are therefore enforced by the link graph rather than convention:

- **A shell never links `famulus_core`.** The edge does not exist.
- **`src/base` never includes a platform, core or shell header.**

## Targets

| Target | Kind | Source | Linked by |
|---|---|---|---|
| `famulus_proto` | INTERFACE | `src/proto` | everyone |
| `famulus_base` | STATIC | `src/base/src` | core, shells |
| `famulus_platform_api` | INTERFACE | `src/platform/api` | core, shells, the adapter |
| `famulus_platform_<name>` | STATIC | `src/platform/<name>` | the facade only |
| `famulus_platform` | INTERFACE | — | core, daemon, shells |
| `famulus_core` | STATIC | `src/core/<subdir>` | `famulusd`, core tests |
| `famulusd` | EXE | `src/core/main.cpp` | — |
| `famulus_shell_cli` | EXE | `src/shell/cli/src` | — |
| `famulus_shell_win_desktop` | EXE | `src/shell/win-desktop/src` | — |

The three-way platform split earns its keep: `famulus_platform_api` is the
headers, `famulus_platform_<name>` is the one adapter the build selected, and
`famulus_platform` is the facade consumers link. **The platform question is asked
in exactly one file**, and no consumer repeats it.

Names follow the path: `src/shell/cli` → `famulus_shell_cli`. Warnings are a
function (`famulus_apply_warnings`), not a target, because they are a build
setting rather than a component.

**Executables are the one place where the artifact name differs from the target
name**, because a target name is for the build and an artifact name is for a
person:

| Target | Artifact | Seen by |
|---|---|---|
| `famulusd` | `famulusd.exe` | the system: logon task, Task Manager |
| `famulus_shell_cli` | `famulusctl.exe` | a person typing, a script |
| `famulus_shell_win_desktop` | `famulus.exe` | a person double-clicking |

Libraries keep their exact target name; only executables get `OUTPUT_NAME`. The
product name is reserved for the thing a person launches, so it does not have to
be renamed the day the GUI lands.

## Who owns what

| Binary | Owns | Needs the daemon running |
|---|---|---|
| `famulusd` | process lifecycle: serve, foreground logging, install/uninstall of autostart | it *is* that process |
| `famulusctl` | everything functional plus liveness: `status`, `ask`, `artifacts`, `settings`, `shutdown` | yes — refuses with a clear message when absent |

Consequences worth keeping:

- **`famulusd` contains no client code.** It has no subcommand that talks to a
  running daemon, because it is the daemon.
- **Shutting down is a protocol operation.** A live process shuts itself down
  cleanly: the CLI sends the request, the daemon drains state, closes its pipe and
  exits. Nothing kills a process from outside.
- **`status` has two meanings** and they stay separate: "is the daemon alive,
  what protocol, what capabilities" is a protocol request; "what version is this
  executable" is `famulusd --version` and needs no daemon.
- **A missing daemon is an error, not an invitation to start one.** Auto-spawning
  from a client makes scripts unpredictable. Convenience is an explicit flag,
  never the default.

## The platform boundary

`src/platform/api/famulus_platform.h` is pure C99: POD structs, function pointers,
no ownership transfer. `src/platform/api/famulus_transport.h` is C++ and declares
the byte stream (`IListener`, `IByteStream`); it is the only reason a shell links
the platform at all.

- **Direction is asymmetric.** `core -> platform` are synchronous calls made only
  from the daemon's single platform strand, so the adapter needs no locks.
  `platform -> core` has exactly one exit: the `post_event` pointer handed over at
  `fam_platform_init`.
- **`post_event` only enqueues.** It never waits, never re-enters core, and
  returns in well under a millisecond. That is what makes it safe to call while
  core is blocked inside a platform call, and therefore what makes deadlock
  impossible.
- **Nothing crosses by ownership.** Outputs go into caller-provided buffers;
  inputs are borrowed for the duration of the call; `post_event` copies
  synchronously. There is no allocator in the ABI and no "who frees this".
- **Cold start pulls, it does not wait.** Anything that can happen while the
  process is not running (a scheduled alarm) is drained by core at startup via a
  `take_fired`-style call.
- **The adapter never asks core anything.** Policy the adapter would need is
  configured up front (for example "suppress the key"), never negotiated mid
  callback.
- **Missing capability is a value, not a failure.** Capability bits plus limits
  are queried at init; core degrades — an artifact goes `pending`, or a different
  surface is chosen.

Only `src/core/bridge/` may include `famulus_platform.h`. Everything else in core
sees a C++ interface.

## Naming

C++ follows C# style: namespace `Famulus`, types and functions PascalCase,
parameters and locals snake_case, private data members `_PascalCase`, constants
PascalCase. Braces, indentation and wrapping come from `.clang-format`.

The C boundary uses `fam_platform_*` functions, `fam_*_t` types and `FAM_*`
constants; macros that are not part of the ABI use `FAMULUS_`.

CMake targets are snake_case and traceable to a path; cache variables are
`FAMULUS_UPPER_SNAKE`. Directories are snake_case, with a dash where two words
form one name (`win-desktop`).

## Build

- Everything generated lands under `build/`, so `git clean -xdf` is the only reset.
- `FAMULUS_PLATFORM` selects the adapter and is also its path component:
  `windows` means `src/platform/windows` and `famulus_platform_windows`. `null` is
  the portability gate.
- Presets: `win-msvc` (development), `win-null` (gate). No presets for platforms
  that are not supported: the seam exists, the presets do not.
- Dependencies are FetchContent and **off by default** (`FAMULUS_FETCH_DEPS`). A
  checkout with no network must configure and build.
- Targets appear only when their sources exist; every directory globs its own.

## Testing

All tests live in `tests/`, never next to sources.

| Suite | Links | Sees |
|---|---|---|
| `tests/core` | `famulus_core`, `famulus_platform_mock` | internals |
| `tests/protocol` | `famulus_proto`, `famulus_base`, `famulus_platform_mock` | the protocol only |

`tests/support/platform_mock` is a **test double, not a test**: it records calls
and lets a test inject events. It is deliberately absent from
`FAMULUS_PLATFORM_SUPPORTED`. Tests link `famulus_platform_api` plus the mock, and
never the facade — otherwise they would pull in the real adapter.

The split matters: if a behaviour can only be checked by including a core header,
that is a finding about the protocol, not a reason to widen the protocol suite.

## Deadlines

An alarm must fire even if the daemon is not running, so scheduling is handed to
whatever the platform provides (Windows Task Scheduler, later `launchd`,
`UNUserNotificationCenter`, `AlarmManager`). Core keeps a durable table and
reconciles at startup; it does not keep the only copy in a timer.
