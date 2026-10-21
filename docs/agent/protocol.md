# Protocol v1 (draft)

The pipe protocol between `famulusd` and every shell. It is the product boundary:
anything a shell can do, it does through these messages. Nothing here is
implemented yet; this is the contract to implement against.

## Transport

- Windows: named pipe `\\.\pipe\famulus-<user-sid-or-session>`.
- Framing: newline-delimited JSON (one object per line, UTF-8, LF).
- Single-instance: the pipe name is the mutex. The first daemon creates it; a
  second daemon fails to bind and exits quietly, or forwards `surface.open` to the
  first and exits.
- Authentication: the pipe is created with a DACL that grants the current user
  only. No TCP, ever — a localhost port is reachable from any web page.
- Version check: `hello.protocol` is an integer. A mismatch is a hard error with
  a human-readable message, not a best-effort connection.

## Envelope

Every message is one object with `op`. Daemon-to-client messages carry a
monotonically increasing `seq`, which is what makes reconnect work.

```json
{ "op": "hello", "protocol": 1, "client_id": "ctl-7f3a", "kind": "cli",
  "caps": ["text", "actions"], "since_seq": 0 }
```

`caps` is how a surface declares what it can render. It is the only thing that
decides whether an artifact can be shown here:

| cap | meaning |
|---|---|
| `text` | plain text lines |
| `rich` | markdown-ish text with emphasis and code blocks |
| `fields` | key/value pairs |
| `table` | rows and columns |
| `actions` | can invoke actions on an artifact |
| `notify` | platform notification channel is available to this surface |
| `form` | interactive input inside an artifact |

The CLI declares `text`, `fields`, `table`, `actions`. It cannot render a `form`,
so such artifacts stay `pending` until a capable surface connects. That is the
degradation ladder, and it is testable from day one because the CLI is the
smallest surface.

## Client to daemon

| op | fields | notes |
|---|---|---|
| `hello` | `protocol`, `client_id`, `kind`, `caps`, `since_seq` | first message |
| `session.send` | `session_id`, `text` | append a user turn |
| `session.open` | `session_id?` | create or resume |
| `action.invoke` | `artifact_id`, `action_id`, `payload` | the only way a button does anything |
| `settings.get` | — | |
| `settings.set` | `patch` | validated by core, not by the shell |
| `ack` | `seq` | optional: lets the daemon trim its replay log |
| `shutdown` | — | ask the live daemon to exit gracefully: drain, close the pipe, exit 0 |
| `bye` | — | disconnect this client only |

## Daemon to client

| op | fields | notes |
|---|---|---|
| `welcome` | `protocol`, `daemon_version`, `platform`, `caps`, `limits` | reply to `hello` |
| `artifact` | `seq`, `artifact` | create or full replace |
| `artifact.patch` | `seq`, `id`, `rev`, `ops` | incremental update |
| `artifact.remove` | `seq`, `id`, `reason` | expired or dismissed |
| `event` | `seq`, `type`, `...` | session output, state changes, platform events |
| `error` | `seq`, `code`, `message`, `retryable`, `request_id?` | |

Reconnect: a client sends `since_seq` and receives everything still in the replay
log from that point, then live traffic. If `since_seq` is older than the log, the
daemon sends a fresh snapshot of all artifacts and a `resync` event.

## Artifact

```json
{
  "id": "a_01J...",
  "rev": 1,
  "kind": "notification | panel | document | stream | form | notice",
  "title": "standup in 15 minutes",
  "urgency": "ambient | normal | interrupt",
  "blocks": [ { "type": "fields", "rows": [["time", "09:00"]] } ],
  "actions": [ { "id": "snooze", "label": "snooze 10m" }, { "id": "open", "label": "open calendar" } ],
  "lifetime": { "until": "2026-10-20T09:00:00Z", "ttl_sec": 600 },
  "origin": { "session_id": "s_...", "source": "scheduler" }
}
```

`kind` is a hint about intent, not about window type: the surface picks where it
lands. `urgency` only orders things; a surface may ignore `interrupt` if it has no
focus-stealing right.

## Blocks

A closed set. Adding one is a protocol version bump, and every surface must either
render it or fall back.

| type | fields | CLI renders as |
|---|---|---|
| `text` | `md` | text |
| `fields` | `rows` | aligned `key: value` |
| `list` | `items` | bulleted lines |
| `table` | `columns`, `rows` | padded columns |
| `code` | `lang`, `text` | indented |
| `buttons` | `actions` | numbered list of action ids |
| `form` | `schema`, `submit` | **not renderable** |

A block the surface does not know is rendered as a placeholder line naming it;
it never aborts the artifact.

## Events

`type` is dotted and lowercase. v1 needs only these:

| type | payload |
|---|---|
| `session.delta` | `session_id`, `text` |
| `session.done` | `session_id`, `reason`, `usage?` |
| `session.error` | `session_id`, `code`, `message`, `retryable` |
| `platform.event` | `name`, `payload` (what the adapter posted, for example `hotkey.pressed`) |
| `daemon.state` | `state`, `since` |
| `resync` | `reason` |

## Limits

`welcome.limits` carries what the daemon already clipped for the platform:
`notification_max_actions`, `artifact_max_blocks`, `field_max_length`. The shell
never hard-codes a platform limit; the daemon never sends a shape the declared
limits forbid.

## Deliberately absent in v1

Binary payloads, streaming audio or screen frames, plugin-supplied HTML, and
multi-user sessions. When a data plane is genuinely needed it gets its own
channel with explicit handle release, not a JSON blob.
