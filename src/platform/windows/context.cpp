// ============================================================================
//  context.cpp - what a context owns on Windows
// ============================================================================

#include "context.hpp"

#include <algorithm>

namespace
{
    // Freeing a handle twice would be a double free, so the handle lists are the
    // single source of truth: allocate tracks, release forgets, destroy sweeps.
    template <class Handle>
    void Track(std::vector<Handle*>& handles, Handle* handle)
    {
        handles.push_back(handle);
    }

    template <class Handle>
    void Forget(std::vector<Handle*>& handles, Handle* handle)
    {
        handles.erase(std::remove(handles.begin(), handles.end(), handle), handles.end());
    }

    template <class Handle>
    void DeleteAll(std::vector<Handle*>& handles)
    {
        for (Handle* handle : handles)
        {
            delete handle;
        }

        handles.clear();
    }
} // namespace

FamContext::FamContext(const FamHost* host)
    : Hotkeys(Environment),
      Secrets(Environment)
{
    Environment.SetHost(host);
}

FamContext::~FamContext()
{
    // The worker owns a thread; it must stop before anything else goes away.
    Hotkeys.Stop();

    DeleteAll(Streams);
    DeleteAll(Listeners);
}

void FamContext::Track(FamListener* listener)
{
    ::Track(Listeners, listener);
}

void FamContext::Track(FamStream* stream)
{
    ::Track(Streams, stream);
}

void FamContext::Forget(FamListener* listener)
{
    ::Forget(Listeners, listener);
}

void FamContext::Forget(FamStream* stream)
{
    ::Forget(Streams, stream);
}

void FamContext::ClearError()
{
    LastError = FamError_None;
    ErrorText.clear();
}

void FamContext::SetError(FamError error, std::string_view description)
{
    LastError = error;
    ErrorText.assign(description);
}

auto FamContext::GetError(const char** description) -> FamError
{
    if (description != nullptr)
    {
        *description = ErrorText.empty() ? nullptr : ErrorText.c_str();
    }

    return LastError;
}
