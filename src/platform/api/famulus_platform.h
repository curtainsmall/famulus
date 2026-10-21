/* ============================================================================
 *  famulus_platform.h - the platform contract
 *
 *  Pure C99 on purpose: POD structures, function pointers, no ownership
 *  transfer, no C++ types. Any language can implement this, and the daemon can
 *  be recompiled for a platform that has none of the others.
 *
 *  "Platform" is the environment the daemon runs in, not an operating system:
 *  a desktop OS, a server, a cloud backend.
 *
 *  C style, not C++ style: structures and enums are tagged, never typedef'd.
 *  `_t` is reserved for real typedefs of scalar types, so a reader can tell
 *  `struct fam_limits` (a type you spell out) from `fam_status_t` (a name for
 *  int32_t) at a glance.
 *
 *  Three rules that the rest of the design depends on:
 *
 *  1. Calls in this direction are synchronous and made only from the daemon's
 *     single platform strand, so an implementation needs no locks.
 *  2. Events travel the other way through exactly one exit: host->post_event.
 *     It must only enqueue, must never wait on the daemon, and must return in
 *     well under a millisecond.
 *  3. Nothing crosses by ownership. Outputs are written into caller-provided
 *     buffers as `buf` plus `inout_len` (capacity in, required length out, and
 *     FAM_E_NOMEM when the capacity was too small). Inputs are borrowed for the
 *     duration of the call and must not be retained.
 * ========================================================================== */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Bump only for a breaking change; additive changes keep the number and grow
 * the structure's `size` field instead. */
#define FAM_PLATFORM_ABI_VERSION 1u

typedef int32_t fam_status_t;

enum fam_status
{
    FAM_OK = 0,
    FAM_E_UNSUPPORTED = -1, /* this platform has no such capability */
    FAM_E_DENIED = -2,      /* the platform refused: permission not granted */
    FAM_E_INVALID = -3,     /* bad argument */
    FAM_E_NOMEM = -4,       /* caller buffer too small; inout_len holds the need */
    FAM_E_IO = -5,
    FAM_E_BUSY = -6
};

typedef uint32_t fam_capabilities_t;

enum fam_capability
{
    FAM_CAP_PATHS = 1u << 0, /* fam_platform_paths is meaningful */
    FAM_CAP_HOTKEY = 1u << 1,
    FAM_CAP_NOTIFY = 1u << 2,
    FAM_CAP_SCHEDULE = 1u << 3,
    FAM_CAP_SECRET = 1u << 4,
    FAM_CAP_CLIPBOARD = 1u << 5,
    FAM_CAP_CAPTURE = 1u << 6
};

/* What the daemon must clip before it sends anything out. A shell never hard
 * codes a platform limit; the daemon never sends a shape these forbid. */
struct fam_limits
{
    uint32_t notification_max_actions;
    uint32_t artifact_max_blocks;
    uint32_t frame_max_bytes;
    uint32_t schedule_min_granularity_ms;
};

struct fam_event
{
    const char* type;    /* literal, dotted, lowercase: "hotkey.pressed" */
    const char* payload; /* UTF-8 JSON, may be NULL */
    size_t payload_len;
    int64_t wall_ms;
};

/* What the daemon lends to the platform. Append only; never reorder. An
 * implementation checks `size` before touching an optional entry. */
struct fam_host
{
    uint32_t size; /* sizeof(struct fam_host) */

    void (*log)(int level, const char* category, const char* message);
    void (*post_event)(const struct fam_event* event); /* thread safe, never blocks */
    int64_t (*monotonic_ms)(void);
};

enum fam_path_kind
{
    FAM_PATH_DATA = 0, /* durable state: sessions, artifacts, schedule table */
    FAM_PATH_CONFIG = 1,
    FAM_PATH_CACHE = 2,
    FAM_PATH_STATE = 3
};

uint32_t fam_platform_abi_version(void);

/* Bit set from enum fam_capability. A missing bit is a value, not an error: the
 * daemon degrades instead of assuming success. */
fam_capabilities_t fam_platform_capabilities(void);

fam_status_t fam_platform_limits(struct fam_limits* out_limits);

/* One call at startup, one at shutdown. After shutdown returns, no event may be
 * delivered. */
fam_status_t fam_platform_init(const struct fam_host* host);
void fam_platform_shutdown(void);

/* A directory for the given kind, created if missing, UTF-8, no trailing
 * separator. Storage locations are platform conventions - hard coding them in
 * the daemon is how a sandboxed build breaks later. */
fam_status_t fam_platform_paths(enum fam_path_kind kind, char* buf, size_t* inout_len);

/* A global hotkey. `accelerator` is "Ctrl+Alt+Shift+Win" prefixes followed by one
 * key: A-Z, 0-9, F1-F24, Space, Enter, Tab, Esc. The platform registers it and
 * posts one "hotkey.pressed" event per press, with the accelerator in the
 * payload; it never decides what a press means.
 *
 * `out_id` identifies the binding and must be passed back to unbind. Zero is
 * never a valid id. */
fam_status_t fam_platform_hotkey_bind(const char* accelerator, uint32_t* out_id);
fam_status_t fam_platform_hotkey_unbind(uint32_t id);

/* Secret storage. `name` is 1..64 characters of [A-Za-z0-9._-]; anything else is
 * FAM_E_INVALID, so a name can never escape its directory. Values are opaque
 * bytes, encrypted at rest by the platform.
 *
 * A missing secret is FAM_E_DENIED, not an empty value. */
fam_status_t fam_platform_secret_set(const char* name, const void* data, size_t size);
fam_status_t fam_platform_secret_get(const char* name, unsigned char* buf, size_t* inout_len);
fam_status_t fam_platform_secret_delete(const char* name);

#ifdef __cplusplus
}
#endif
