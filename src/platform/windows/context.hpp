// ============================================================================
//  context.hpp - what a FamContext owns on Windows
//
//  The C API hands out a context and opaque handles; this header says what they
//  are. Every allocation a capability makes hangs off the context, so
//  famTerminate can free whatever the caller forgot, including handles that were
//  released twice or never released at all.
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
    struct FamContext* Owner;
    Famulus::Windows::NamedPipe Pipe;
};

struct FamListener
{
    struct FamContext* Owner;

    // One instance alive at all times; replaced the moment a client takes it.
    Famulus::Windows::NamedPipe Current;
};

struct FamContext
{
    explicit FamContext(const FamHost* host);

    ~FamContext();

    FamContext(const FamContext&) = delete;
    auto operator=(const FamContext&) -> FamContext& = delete;

    void Track(FamListener* listener);
    void Track(FamStream* stream);
    void Forget(FamListener* listener);
    void Forget(FamStream* stream);

    // The error of the last ABI call. Cleared at the entry of every one of them,
    // so a success never reports the failure before it, and the description is a
    // copy the caller may hold until the next call.
    void ClearError();
    void SetError(FamError error, std::string_view description);
    auto GetError(const char** description) -> FamError;

    // Declaration order is construction order: the capabilities need the
    // environment, so it must come first.
    Famulus::Windows::PlatformEnvironment Environment;
    Famulus::Windows::HotkeyWorker Hotkeys;
    Famulus::Windows::SecretStore Secrets;

    // Scratch for the borrowed line the ABI hands out on the last read.
    std::string LineScratch;

    // Why the last call failed, and the sentence that goes with it. Both are
    // reset by the next call that starts.
    FamError LastError = FamError_None;
    std::string ErrorText;

    // Every handle handed out, so destroy can free the leftovers.
    std::vector<FamListener*> Listeners;
    std::vector<FamStream*> Streams;
};
