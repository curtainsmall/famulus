// ============================================================================
//  hotkey_worker.hpp - the thread and message-only window behind global hotkeys
//
//  RegisterHotKey reports only to the thread that called it, so registration
//  lives on a worker thread with a message-only window. This class owns that
//  thread, the window and the registration table.
// ============================================================================

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include "platform_api.h"

namespace Famulus::Windows
{
    class PlatformEnvironment;

    class HotkeyWorker
    {
    public:
        explicit HotkeyWorker(PlatformEnvironment& environment);
        ~HotkeyWorker();

        HotkeyWorker(const HotkeyWorker&) = delete;
        auto operator=(const HotkeyWorker&) -> HotkeyWorker& = delete;

        // "Ctrl+Alt+Shift+K" and friends. Parsing is this class's business
        // because the accelerator is what it registers and reports back.
        auto Bind(std::string_view accelerator, uint32_t& out_id) -> fam_status_t;
        auto Unbind(uint32_t id) -> fam_status_t;

        // Stops the thread and drops every registration.
        void Stop();

    private:
        struct Binding
        {
            std::string accelerator;
        };

        struct Job
        {
            const std::function<void()>* body;
            std::atomic<bool>* completed;
        };

        auto Start() -> bool;
        auto RunOnThread(const std::function<void()>& body) -> bool;
        auto Run() -> void;
        auto HandleMessage(HWND window, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT;
        void PostPress(uint32_t id);

        static auto CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT;

        PlatformEnvironment& Environment;

        // Window and registration table: touched only on the worker thread.
        HWND WindowHandle = nullptr;
        std::map<uint32_t, Binding> Bindings;
        uint32_t NextId = 1;

        // Thread bookkeeping: guarded by Mutex.
        std::thread Thread;
        std::mutex Mutex;
        std::condition_variable StartedSignal;
        DWORD ThreadId = 0;
        bool Started = false;
        bool Ready = false;
    };
}
