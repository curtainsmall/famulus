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

fam_context::fam_context(const struct fam_host* host)
    : Hotkeys(Environment),
      Secrets(Environment)
{
    Environment.SetHost(host);
}

fam_context::~fam_context()
{
    // The worker owns a thread; it must stop before anything else goes away.
    Hotkeys.Stop();

    DeleteAll(Streams);
    DeleteAll(Listeners);
}

void fam_context::Track(struct fam_listener* listener)
{
    ::Track(Listeners, listener);
}

void fam_context::Track(struct fam_stream* stream)
{
    ::Track(Streams, stream);
}

void fam_context::Forget(struct fam_listener* listener)
{
    ::Forget(Listeners, listener);
}

void fam_context::Forget(struct fam_stream* stream)
{
    ::Forget(Streams, stream);
}
