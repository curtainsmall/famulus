// ============================================================================
//  HotkeyWorker.cpp - global hotkey registration and dispatch
//
//  Presses leave through host->post_event, the only exit the contract allows.
//  The adapter never decides what a press means: it reports the accelerator and
//  forgets.
// ============================================================================

#include "hotkey_worker.hpp"

#include <atomic>
#include <chrono>
#include <vector>

#include "platform_environment.hpp"

namespace
{
    constexpr wchar_t WindowClassName[] = L"famulus-hotkey-window";
    constexpr UINT JobMessage = WM_APP + 1;

    auto GetWallMilliseconds() -> int64_t
    {
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    }

    auto IsEqualIgnoreCase(std::string_view left, std::string_view right) -> bool
    {
        if (left.size() != right.size())
        {
            return false;
        }

        for (size_t i = 0; i < left.size(); ++i)
        {
            const char a = left[i] >= 'A' && left[i] <= 'Z' ? static_cast<char>(left[i] - 'A' + 'a') : left[i];
            const char b = right[i] >= 'A' && right[i] <= 'Z' ? static_cast<char>(right[i] - 'A' + 'a') : right[i];

            if (a != b)
            {
                return false;
            }
        }

        return true;
    }

    auto ParseKey(std::string_view token, UINT& virtual_key) -> bool
    {
        if (token.size() == 1 && ((token[0] >= 'A' && token[0] <= 'Z') || (token[0] >= '0' && token[0] <= '9')))
        {
            virtual_key = static_cast<UINT>(token[0]);
            return true;
        }

        if (token.size() >= 2 && token.size() <= 3 && (token[0] == 'F' || token[0] == 'f'))
        {
            int number = 0;

            for (size_t i = 1; i < token.size(); ++i)
            {
                if (token[i] < '0' || token[i] > '9')
                {
                    return false;
                }

                number = number * 10 + (token[i] - '0');
            }

            if (number >= 1 && number <= 24)
            {
                virtual_key = static_cast<UINT>(VK_F1 + number - 1);
                return true;
            }

            return false;
        }

        if (IsEqualIgnoreCase(token, "Space"))
        {
            virtual_key = VK_SPACE;
            return true;
        }

        if (IsEqualIgnoreCase(token, "Enter"))
        {
            virtual_key = VK_RETURN;
            return true;
        }

        if (IsEqualIgnoreCase(token, "Tab"))
        {
            virtual_key = VK_TAB;
            return true;
        }

        if (IsEqualIgnoreCase(token, "Esc") || IsEqualIgnoreCase(token, "Escape"))
        {
            virtual_key = VK_ESCAPE;
            return true;
        }

        return false;
    }

    auto ParseAccelerator(std::string_view text, UINT& modifiers, UINT& virtual_key, std::string& canonical)
        -> FamStatus
    {
        modifiers = 0;
        virtual_key = 0;
        canonical.clear();

        if (text.empty())
        {
            return FamStatus_Invalid;
        }

        std::vector<std::string_view> tokens;
        size_t start = 0;

        while (true)
        {
            const size_t plus = text.find('+', start);

            if (plus == std::string_view::npos)
            {
                tokens.push_back(text.substr(start));
                break;
            }

            tokens.push_back(text.substr(start, plus - start));
            start = plus + 1;
        }

        if (tokens.size() < 2)
        {
            return FamStatus_Invalid; // a bare key would take that key from every app
        }

        std::string key_name;
        bool have_key = false;

        for (size_t i = 0; i < tokens.size(); ++i)
        {
            const std::string_view token = tokens[i];

            if (i + 1 == tokens.size() && !have_key)
            {
                if (!ParseKey(token, virtual_key))
                {
                    return FamStatus_Invalid;
                }

                key_name.assign(token);
                have_key = true;
                continue;
            }

            if (IsEqualIgnoreCase(token, "Ctrl") || IsEqualIgnoreCase(token, "Control"))
            {
                modifiers |= MOD_CONTROL;
                canonical += "Ctrl+";
            }
            else if (IsEqualIgnoreCase(token, "Alt"))
            {
                modifiers |= MOD_ALT;
                canonical += "Alt+";
            }
            else if (IsEqualIgnoreCase(token, "Shift"))
            {
                modifiers |= MOD_SHIFT;
                canonical += "Shift+";
            }
            else if (IsEqualIgnoreCase(token, "Win") || IsEqualIgnoreCase(token, "Super"))
            {
                modifiers |= MOD_WIN;
                canonical += "Win+";
            }
            else
            {
                return FamStatus_Invalid;
            }
        }

        if (!have_key || modifiers == 0)
        {
            return FamStatus_Invalid;
        }

        canonical += key_name;
        return FamStatus_Ok;
    }
} // namespace

namespace Famulus::Windows
{
    HotkeyWorker::HotkeyWorker(PlatformEnvironment& environment)
        : Environment(environment)
    {
    }

    HotkeyWorker::~HotkeyWorker()
    {
        Stop();
    }

    auto HotkeyWorker::Bind(std::string_view accelerator, uint32_t& out_id) -> FamStatus
    {
        UINT modifiers = 0;
        UINT virtual_key = 0;
        std::string canonical;

        const FamStatus parsed = ParseAccelerator(accelerator, modifiers, virtual_key, canonical);

        if (parsed != FamStatus_Ok)
        {
            return parsed;
        }

        if (!Start())
        {
            return FamStatus_Io;
        }

        FamStatus result = FamStatus_Io;
        uint32_t id = 0;

        const std::function<void()> body = [&] {
            id = NextId++;

            if (::RegisterHotKey(WindowHandle, static_cast<int>(id), modifiers, virtual_key) == 0)
            {
                result = FamStatus_Busy; // usually ERROR_HOTKEY_ALREADY_REGISTERED
                return;
            }

            Bindings[id] = Binding{canonical};
            result = FamStatus_Ok;
        };

        if (!RunOnThread(body))
        {
            return FamStatus_Io;
        }

        if (result == FamStatus_Ok)
        {
            out_id = id;
        }

        return result;
    }

    auto HotkeyWorker::Unbind(uint32_t id) -> FamStatus
    {
        FamStatus result = FamStatus_Io;

        const std::function<void()> body = [&] {
            if (::UnregisterHotKey(WindowHandle, static_cast<int>(id)) == 0)
            {
                result = FamStatus_Invalid;
                return;
            }

            Bindings.erase(id);
            result = FamStatus_Ok;
        };

        return RunOnThread(body) ? result : FamStatus_Io;
    }

    void HotkeyWorker::Stop()
    {
        DWORD thread_id = 0;

        {
            const std::lock_guard<std::mutex> guard(Mutex);

            if (!Started)
            {
                return;
            }

            thread_id = ThreadId;
        }

        if (thread_id != 0)
        {
            ::PostThreadMessageW(thread_id, WM_QUIT, 0, 0);
        }

        if (Thread.joinable())
        {
            Thread.join();
        }

        const std::lock_guard<std::mutex> guard(Mutex);
        Started = false;
        Ready = false;
        ThreadId = 0;
    }

    auto HotkeyWorker::Start() -> bool
    {
        std::unique_lock<std::mutex> lock(Mutex);

        if (Started)
        {
            return Ready;
        }

        Thread = std::thread([this] { Run(); });
        Started = true;

        StartedSignal.wait(lock, [this] { return ThreadId != 0; });

        return Ready;
    }

    auto HotkeyWorker::RunOnThread(const std::function<void()>& body) -> bool
    {
        const HWND window = WindowHandle;

        if (window == nullptr)
        {
            return false;
        }

        std::atomic<bool> completed{false};
        Job job{&body, &completed};

        if (::PostMessageW(window, JobMessage, 0, reinterpret_cast<LPARAM>(&job)) == 0)
        {
            return false;
        }

        while (!completed.load())
        {
            ::SwitchToThread();
        }

        return true;
    }

    auto HotkeyWorker::Run() -> void
    {
        WNDCLASSW window_class{};
        window_class.lpfnWndProc = &HotkeyWorker::WindowProcedure;
        window_class.hInstance = ::GetModuleHandleW(nullptr);
        window_class.lpszClassName = WindowClassName;
        ::RegisterClassW(&window_class);

        WindowHandle = ::CreateWindowExW(0,
                                         WindowClassName,
                                         L"",
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         HWND_MESSAGE,
                                         nullptr,
                                         window_class.hInstance,
                                         this);

        {
            const std::lock_guard<std::mutex> guard(Mutex);
            ThreadId = ::GetCurrentThreadId();
            Ready = WindowHandle != nullptr;
        }

        StartedSignal.notify_all();

        if (WindowHandle == nullptr)
        {
            return;
        }

        MSG message{};

        while (::GetMessageW(&message, nullptr, 0, 0) > 0)
        {
            ::TranslateMessage(&message);
            ::DispatchMessageW(&message);
        }

        ::UnregisterClassW(WindowClassName, window_class.hInstance);
        WindowHandle = nullptr;
    }

    auto HotkeyWorker::HandleMessage(HWND window, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        if (message == WM_HOTKEY)
        {
            PostPress(static_cast<uint32_t>(wparam));
            return 0;
        }

        if (message == JobMessage)
        {
            auto* job = reinterpret_cast<Job*>(lparam);

            (*job->body)();
            job->completed->store(true);

            return 0;
        }

        return ::DefWindowProcW(window, message, wparam, lparam);
    }

    void HotkeyWorker::PostPress(uint32_t id)
    {
        const auto found = Bindings.find(id);

        if (found == Bindings.end())
        {
            return;
        }

        const FamHost* host = Environment.GetHost();

        if (host == nullptr)
        {
            return;
        }

        const std::string payload = "{\"accelerator\":\"" + found->second.accelerator + "\"}";
        const FamEvent event{"hotkey.pressed", payload.c_str(), payload.size(), GetWallMilliseconds()};

        host->post_event(&event);
    }

    auto CALLBACK HotkeyWorker::WindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        if (message == WM_NCCREATE)
        {
            auto* created = reinterpret_cast<CREATESTRUCTW*>(lparam);
            ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(created->lpCreateParams));
        }

        auto* self = reinterpret_cast<HotkeyWorker*>(::GetWindowLongPtrW(window, GWLP_USERDATA));

        if (self != nullptr)
        {
            return self->HandleMessage(window, message, wparam, lparam);
        }

        return ::DefWindowProcW(window, message, wparam, lparam);
    }
} // namespace Famulus::Windows
