# Plan

Scope for now: **Windows only, CLI surface only.** No GUI shell, no macOS or Linux,
no cloud adapter, no tool host. The seams for those stay open in the design; the
work does not.

A milestone is done when its acceptance command runs clean, not when the code
compiles.

## M0 — skeleton (done)

Build tree, targets, presets, a `famulusd` that prints `hello world`.

Acceptance: `cmake --preset win-msvc` && `cmake --build --preset win-debug` &&
`famulusd.exe` prints `hello world` && `cmake --preset win-null`.

## M1 — the transport and the handshake

Why first: it fixes the product boundary before anything interesting exists, and
it gives every later milestone a way to be observed from the terminal.

- `src/proto/`: message types for `hello`/`welcome`/`error`, the `caps` and
  `limits` structures. Content only — no code.
- `src/base/`: framing (newline delimited, one frame in, one frame out, a hard cap
  on frame size) and JSON encode/decode.
- `src/platform/api/famulus_transport.h`: `IListener` and `IByteStream`, the
  interface only.
- `src/platform/windows/`: the named pipe — `CreateNamedPipeW` with a DACL for the
  current user, `FILE_FLAG_FIRST_PIPE_INSTANCE` as the single-instance lock,
  `ConnectNamedPipe`, and a `CreateFileW` client with a bounded connect timeout.
- `src/core/`: accept loop, one connection per client, a serial write queue per
  connection, `hello` handling, backpressure (a capped outbound queue closes the
  connection instead of growing).
- `src/core/main.cpp`: create the pipe, serve, `--foreground` keeps logs on
  stderr, `--version`.
- `src/shell/cli/src`: argument parsing, connect, `hello`, print `welcome`.
  Commands: `status`, `shutdown`.

Acceptance:
```
famulusd.exe                      # in one terminal
famulusctl.exe status      # protocol 1, platform windows, daemon version, capabilities
famulusctl.exe status      # exits 2 with "daemon not running" when it is not
famulusctl.exe shutdown    # daemon exits 0 and its pipe disappears
famulusd.exe                      # second instance exits quietly, does not steal the pipe
famulusd.exe --version            # works with no daemon running
```

Needs `FAMULUS_FETCH_DEPS=ON` for nlohmann/json. If the machine has no network,
vendor the header under `third_party/` instead — decide before starting.

## M2 — the platform capabilities

Why second: the daemon cannot do anything platform-ish until the ABI exists, and
the ABI is cheaper to change while nothing depends on it.

- `src/platform/api/famulus_platform.h`: ABI version, capability bits, limits,
  `init`/`shutdown`, `paths`, and the `post_event` contract written down as
  comments.
- `src/platform/windows/`: `init`/`shutdown`, capability bits, `fam_platform_paths`
  (data, config, cache). No notification or hotkey yet.
- `src/platform/null/`: the same ABI, every capability missing.
- `src/core/bridge/`: the only C++ translation of `fam_platform_*`, plus the
  platform strand that calls into the adapter.
- `tests/support/platform_mock/`: records calls, injects events.

Acceptance:
```
famulusctl.exe status   # reports the platform capabilities, not a hard-coded list
famulus_core_tests             # the mock receives init/shutdown, events reach core
famulusctl.exe status   # under FAMULUS_PLATFORM=null: zero capabilities, no crash
```

## M3 — artifacts, sequencing and replay

Why third: every later feature is delivered as an artifact, and replay is the
protocol property that is expensive to retrofit.

- Artifact store with `id`/`rev`; blocks rendered as data, not as code.
- Monotonic `seq`, a bounded replay log, `since_seq` handling, `resync`.
- CLI: `artifacts`, `watch`, rendering `text`, `fields`, `list`, `table`, `code`,
  `buttons`; unknown blocks become a placeholder line.
- A `form` artifact stays `pending` for the CLI and says so.

Acceptance:
```
famulusctl.exe watch &      # kill and restart it mid-stream: no gap, no duplicate
famulusctl.exe artifacts    # a form artifact shows as pending, not an error
famulus_protocol_tests             # daemon + cli, replay verified over the pipe
```

## M4 — sessions and a streaming provider

- Session state in core, one provider adapter (OpenAI-compatible is the cheapest
  first target), asio plus incremental SSE parsing.
- Cancellation that actually aborts the socket: Ctrl+C in the CLI, and daemon-side
  cleanup on client disconnect.
- `ask "..."` prints deltas as they arrive; `session.done` reports usage;
  `session.error` is explicitly retryable or not.

Acceptance:
```
famulusctl.exe ask "say hello in five tokens"   # tokens appear as they stream
# ctrl+c aborts within 100ms, daemon stays healthy
# with the network unplugged: retryable error, no hang
```

## M5 — scheduler, notifications, actions

Why last: it needs artifacts (M3) and the platform adapter (M2), and it is the
first feature whose correctness depends on the daemon *not* running.

- Durable schedule table in SQLite; registration with Windows Task Scheduler;
  `take_fired` on startup; idempotent identity so reconciliation cannot double
  fire.
- `fam_platform_notify_show`/`dismiss` in the Windows adapter, with the action
  limit surfaced in `welcome.limits`.
- `action.invoke` executed in core, artifact updated, never in the shell.

Acceptance:
```
famulusctl.exe schedule add --in 60s "standup"
# quit the daemon, wait, alarm still fires
# restart the daemon: the fired alarm is reconciled exactly once
famulusctl.exe action a_... snooze   # same effect as clicking it in a future GUI
```

## M6 — settings and secrets

- Settings as a schema owned by core, rendered generically; the CLI reads and
  writes, core validates.
- Secrets through the platform (DPAPI now, Keychain later); never in the settings
  file, never in logs.

Acceptance:
```
famulusctl.exe settings set provider.model <name>   # bad values rejected by core
famulusctl.exe settings get                          # no secret values printed
```

## Out of scope, deliberately

GUI shell, hotkey-driven overlay, local models, tool host, multi-user, macOS,
Linux, cloud adapter. The design keeps room for them; the plan does not schedule
them.

## Open questions for the human

1. **CLI binary name.** The target is `famulus_shell_cli`, so the executable is
   `famulusctl.exe`. A shorter command (`famulus.exe`) is one
   `OUTPUT_NAME` away but breaks "target name = path".
2. **Which provider first?** An OpenAI-compatible endpoint is the least code and
   covers most; a vendor SDK is more work and less portable.
3. **Network on the build machine?** FetchContent needs it once per machine. If
   not available, dependencies get vendored under `third_party/`.
4. **Autostart now or later?** A Run-key entry or a logon task is a few lines but
   changes the developer experience.
5. **Log location and retention** — `%LOCALAPPDATA%\famulus\logs`, rotating, has
   to be decided before M4 makes debugging painful.
6. **Interactive REPL in the CLI?** `chat` is cheap and makes M4 usable by a human.
