// ============================================================================
//  src/core/schedule/condition.hpp - the trigger of a schedule
//
//  A node compares a signal, combines children, or both:
//
//      Expected only        compare one signal
//      Children only        combine them by Kind (All / Any / Not)
//      Both                 this signal holds and the children do too
//      Neither              All is true, Any is false, Not is false
// ============================================================================

#pragma once

#include <map>
#include <memory>
#include <set>
#include <typeindex>
#include <vector>

#include "signal.hpp"

namespace Famulus
{
    // What the sources currently believe, by kind.
    using CurrentSignals = std::map<SignalKind, std::shared_ptr<Signal>>;

    class Condition
    {
    public:
        enum class Combine
        {
            All,
            Any,
            Not,
        };

        // The signal this node compares, carrying the expected value. Empty when
        // the node only combines children.
        std::unique_ptr<Signal> Expected;
        bool MatchArrival = false;

        Combine Kind = Combine::All;
        std::vector<Condition> Children;

        // Whether this node holds. `arriving` is the signal that caused this
        // evaluation, or nullptr when nothing did. A kind no source reported is
        // unknown and never satisfies a requirement.
        auto Evaluate(const CurrentSignals& current, const Signal* arriving) const -> bool;

        // Which signals this node waits for, once each, keyed by class: the kind
        // is what a source needs to know, and every kind is its own class. The
        // value is not part of the answer - a source reports what it sees and the
        // condition decides whether it matches.
        auto CollectSignalsWanted() const -> std::set<std::type_index>;

    private:
        auto ExpectedHolds(const CurrentSignals& current, const Signal* arriving) const -> bool;
    };
}
