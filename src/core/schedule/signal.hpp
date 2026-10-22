// ============================================================================
//  src/core/schedule/signal.hpp - what a source reports
//
//  Signal is the interface; every kind is a derived class that owns the value
//  type only it can produce. Nothing is erased into a variant, so a power source
//  value can only ever be read by code that asked for a power source.
// ============================================================================

#pragma once

#include <cstdint>
#include <utility>
#include <variant>

namespace Famulus
{
    enum class SignalKind
    {
        Time,
        PowerSource,
        SessionState,
        DisplayState,
        UserPresence,
        NetworkOnline,
        DeviceInterface,
        Resumed,
        ClockChanged,
    };

    // The value domain of each kind.
    enum class PowerSource
    {
        Ac,
        Dc,
        Hot,
    };

    enum class SessionState
    {
        LoggedOn,
        Locked,
        LoggedOff,
    };

    enum class DisplayState
    {
        On,
        Off,
        Dim,
    };

    enum class UserPresence
    {
        Present,
        Inactive,
        Absent,
    };

    enum class DeviceInterface
    {
        Arrived,
        Removed,
    };

    class Signal
    {
    public:
        virtual ~Signal() = default;

        virtual auto GetKind() const -> SignalKind = 0;
        virtual auto GetOccurredAtMs() const -> int64_t = 0;

        // Same kind and same value. Each derived class decides what "same value"
        // means for the type it owns, and turning the other signal into its own
        // type is how a mismatch is detected.
        virtual auto Equals(const Signal& other) const -> bool = 0;

    };

    template <SignalKind Kind_, typename Value_>
    class TypedSignal final : public Signal
    {
    public:
        using ValueType = Value_;

        static constexpr SignalKind Kind = Kind_;

        TypedSignal(ValueType value, int64_t occurred_at_ms)
            : OccurredAtMs(occurred_at_ms), Value(std::move(value))
        {
        }

        auto GetKind() const -> SignalKind override
        {
            return Kind_;
        }

        auto GetOccurredAtMs() const -> int64_t override
        {
            return OccurredAtMs;
        }


        auto Equals(const Signal& other) const -> bool override
        {
            const auto* typed = dynamic_cast<const TypedSignal*>(&other);

            return typed != nullptr && typed->Value == Value;
        }

        ValueType Value;

    private:
        int64_t OccurredAtMs;
    };

    using TimeSignal = TypedSignal<SignalKind::Time, int64_t>;
    using PowerSourceSignal = TypedSignal<SignalKind::PowerSource, PowerSource>;
    using SessionStateSignal = TypedSignal<SignalKind::SessionState, SessionState>;
    using DisplayStateSignal = TypedSignal<SignalKind::DisplayState, DisplayState>;
    using UserPresenceSignal = TypedSignal<SignalKind::UserPresence, UserPresence>;
    using NetworkOnlineSignal = TypedSignal<SignalKind::NetworkOnline, bool>;
    using DeviceInterfaceSignal = TypedSignal<SignalKind::DeviceInterface, DeviceInterface>;
    using ResumedSignal = TypedSignal<SignalKind::Resumed, std::monostate>;
    using ClockChangedSignal = TypedSignal<SignalKind::ClockChanged, std::monostate>;

    // Same kind and same value. A free function rather than a member: two derived
    // values comparing through the base would make C++20's rewritten `!=`
    // ambiguous, while one non-member == is a single candidate for both orders.
    inline auto operator==(const Signal& left, const Signal& right) -> bool
    {
        return left.Equals(right);
    }
}
