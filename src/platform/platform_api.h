/* ============================================================================
 *  platform_api.h - everything the platform offers
 *
 *  The one header in src/platform. Pure C99: typedef'd handles and function
 *  declarations, so any language can implement an adapter or bind to one, and no
 *  compiler's ABI becomes part of the contract. Implementations may use whatever
 *  language they like behind these symbols.
 *
 *  Naming follows GLFW: one short prefix in PascalCase for types and functions.
 *  An enum value is its enum's name plus the item - FamStatus_NoMem, not
 *  FAM_NOMEM - so a value read on its own still says which enum it came from.
 *  Types are typedef'd, so a caller never writes `struct` or `enum`.
 *
 *  The life of the platform follows GLFW as well: FamInit creates the one context
 *  the process has, FamTerminate frees it along with every handle still
 *  registered with it - including ones the caller forgot to release.
 *
 *  Rules the rest of the design depends on:
 *
 *  1. The platform is initialised once, at startup, and terminated once, at exit.
 *  2. Capability calls are synchronous and made only from the daemon's single
 *     platform strand, so an implementation needs no locks.
 *  3. Events travel the other way through exactly one exit: host->log and
 *     host->post_event. They must only enqueue, must never wait on the daemon,
 *     and must return in well under a millisecond.
 *  4. Outputs are written into caller-provided buffers: capacity in, required
 *     length out, FamStatus_NoMem when the capacity was short. Inputs are borrowed for
 *     the duration of the call.
 * ========================================================================== */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Bump only for a breaking change; additive changes keep the number and grow the
 * structure's `size` field instead. */
#define FAM_PLATFORM_ABI_VERSION 1u

typedef enum FamStatus
{
    FamStatus_Ok = 0,
    FamStatus_Unsupported = -1, /* this platform has no such capability */
    FamStatus_Denied = -2,      /* the platform refused: permission not granted */
    FamStatus_Invalid = -3,     /* bad argument */
    FamStatus_NoMem = -4,       /* caller buffer too small; inout_len holds the need */
    FamStatus_Io = -5,
    FamStatus_Busy = -6         /* already taken: another daemon, another hotkey */
} FamStatus;

typedef enum FamCapability
{
    FamCapability_Paths = 1u << 0, /* FamGetPath is meaningful */
    FamCapability_Hotkey = 1u << 1,
    FamCapability_Notify = 1u << 2,
    FamCapability_Schedule = 1u << 3,
    FamCapability_Secret = 1u << 4,
    FamCapability_Clipboard = 1u << 5,
    FamCapability_Capture = 1u << 6
} FamCapability;

typedef uint32_t FamCapabilities;

/* What the daemon must clip before it sends anything out. A shell never hard
 * codes a platform limit; the daemon never sends a shape these forbid. */
typedef struct FamLimits
{
    uint32_t notification_max_actions;
    uint32_t artifact_max_blocks;
    uint32_t frame_max_bytes;
    uint32_t schedule_min_granularity_ms;
} FamLimits;

typedef struct FamEvent
{
    const char* type;    /* literal, dotted, lowercase: "hotkey.pressed" */
    const char* payload; /* UTF-8 JSON, may be NULL */
    size_t payload_len;
    int64_t wall_ms;
} FamEvent;

/* How loud a line is. A named enum rather than a bare int: an int invites
 * numbers that mean nothing, and the first version of this header already
 * collected one - the Windows adapter logged level 3, which no reader could
 * interpret. */
typedef enum FamLogLevel
{
    FamLogLevel_Info = 0,
    FamLogLevel_Warning = 1,
    FamLogLevel_Error = 2
} FamLogLevel;

/* What the daemon lends to the platform. Append only; never reorder. An
 * implementation checks `size` before touching an optional entry. */
typedef struct FamHost
{
    uint32_t size; /* sizeof(FamHost) */

    void (*log)(FamLogLevel level, const char* category, const char* message);
    void (*post_event)(const FamEvent* event); /* thread safe, never blocks */
    int64_t (*monotonic_ms)(void);
} FamHost;

typedef enum FamPathKind
{
    FamPathKind_Data = 0, /* durable state: sessions, artifacts, schedule table */
    FamPathKind_Config = 1,
    FamPathKind_Cache = 2,
    FamPathKind_State = 3
} FamPathKind;

/* One line in, one line out, UTF-8, terminated by '\n'. The framing is part of
 * the contract because a client in another language has to reproduce it.
 *
 * Both are opaque: only the implementation knows what is inside, and every
 * caller passes the handle. */
typedef struct FamListener FamListener;
typedef struct FamStream FamStream;

/* --- lifetime ------------------------------------------------------------ */

/* Creates the one context the process has: storage roots, the hotkey worker, the
 * secret store. `host` is borrowed, not copied, so it must outlive the platform.
 * Calling it twice is FamStatus_Busy; the first call owns the process. */
FamStatus famInit(const FamHost* host);

/* Frees everything, including handles the caller did not release. No event may
 * be delivered after this returns. Safe to call when not initialised. */
void famTerminate(void);

uint32_t famGetAbiVersion(void);

/* --- capabilities -------------------------------------------------------- */

/* A missing capability is a value, not an error: the daemon degrades instead of
 * assuming success. */
FamCapabilities famGetCapabilities(void);

FamStatus famGetLimits(FamLimits* out_limits);

/* A directory for the given kind, created if missing, UTF-8, no trailing
 * separator. Storage locations are platform conventions - hard coding them in
 * the daemon is how a sandboxed build breaks later. */
FamStatus famGetPath(FamPathKind kind, const char** out_path);

/* A global hotkey. `accelerator` is "Ctrl+Alt+Shift+Win" prefixes followed by one
 * key: A-Z, 0-9, F1-F24, Space, Enter, Tab, Esc. The platform registers it and
 * posts one "hotkey.pressed" event per press, with the accelerator in the
 * payload; it never decides what a press means. `out_id` must be passed back to
 * FamUnbindHotkey, and zero is never a valid id. */
FamStatus famBindHotkey(const char* accelerator, uint32_t* out_id);
FamStatus famUnbindHotkey(uint32_t id);

/* Secret storage. `name` is 1..64 characters of [A-Za-z0-9._-]; anything else is
 * FamStatus_Invalid, so a name can never escape its directory. Values are opaque bytes,
 * encrypted at rest by the platform. A missing secret is FamStatus_Denied. */
FamStatus famSetSecret(const char* name, const void* data, size_t size);
FamStatus famGetSecret(const char* name, const unsigned char** out_data, size_t* out_len);
FamStatus famDeleteSecret(const char* name);

/* --- transport ----------------------------------------------------------- */

/* Whether this platform has a local endpoint at all: a server deployment has
 * none, and the daemon should say that rather than report a second instance. */
int famCanListen(void);

/* Takes the per-user endpoint. FamStatus_Busy when another daemon holds it, which is
 * also how single instance is enforced. The listener belongs to the context. */
FamStatus famListen(FamListener** out_listener);

/* Waits for one client. The stream belongs to the context; release it early or
 * let FamTerminate sweep it. */
FamStatus famAccept(FamListener* listener, FamStream** out_stream);

/* Client side. FamStatus_Denied when nothing is listening within the timeout. */
FamStatus famConnect(uint32_t timeout_ms, FamStream** out_stream);

FamStatus famReadLine(FamStream* stream, const char** out_text, size_t* out_len);
FamStatus famWriteLine(FamStream* stream, const char* text, size_t length);

/* Unregisters and frees. Passing NULL is a no-op. */
void famReleaseStream(FamStream* stream);
void famReleaseListener(FamListener* listener);

#ifdef __cplusplus
}
#endif
