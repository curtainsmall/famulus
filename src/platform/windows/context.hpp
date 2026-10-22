// ============================================================================
//  context.hpp - what a fam_context owns on Windows
//
//  The C API hands out a context and opaque handles; this header says what they
//  are. Every allocation a capability makes hangs off the context, so
//  fam_platform_destroy can free whatever the caller forgot, the way
//  glfwTerminate destroys every remaining window and cursor.
// ============================================================================

#pragma once

#include <string>
#include <vector>

#include "hotkey_worker.hpp"
#include "named_pipe.hpp"
#include "platform_api.h"
#include "platform_environment.hpp"
#include "secret_store.hpp"

struct FamStream
{
    struct fam_context* Owner;
    Famulus::Windows::NamedPipe Pipe;
};

struct FamListener
{
    struct fam_context* Owner;

    // One instance alive at all times; replaced the moment a client takes it.
    Famulus::Windows::NamedPipe Current;
};

struct fam_context
{
    explicit fam_context(const FamHost* host);

    ~fam_context();

    fam_context(const fam_context&) = delete;
    auto operator=(const fam_context&) -> fam_context& = delete;

    void Track(FamListener* listener);
    void Track(FamStream* stream);
    void Forget(FamListener* listener);
    void Forget(FamStream* stream);

    // Declaration order is construction order: the capabilities need the
    // environment, so it must come first.
    Famulus::Windows::PlatformEnvironment Environment;
    Famulus::Windows::HotkeyWorker Hotkeys;
    Famulus::Windows::SecretStore Secrets;

    // Scratch for the borrowed line the ABI hands out on the last read.
    std::string LineScratch;

    // Every handle handed out, so destroy can free the leftovers.
    std::vector<FamListener*> Listeners;
    std::vector<FamStream*> Streams;
};
