// ============================================================================
//  src/core/schedule/condition.cpp - one node, then its children
// ============================================================================

#include "condition.hpp"

#include <algorithm>

namespace Famulus
{
    auto Condition::ExpectedHolds(const CurrentSignals& current, const Signal* arriving) const -> bool
    {
        if (MatchArrival)
        {
            return arriving != nullptr && *arriving == *Expected;
        }

        const auto found = current.find(Expected->GetKind());

        // Unknown is neither false nor true: no source reported this kind, so the
        // requirement does not hold.
        return found != current.end() && found->second != nullptr && *found->second == *Expected;
    }

    auto Condition::Evaluate(const CurrentSignals& current, const Signal* arriving) const -> bool
    {
        const bool holds = Expected != nullptr ? ExpectedHolds(current, arriving) : true;

        switch (Kind)
        {
        case Combine::All:
            return holds && std::all_of(Children.begin(),
                                        Children.end(),
                                        [&current, arriving](const Condition& child)
                                        { return child.Evaluate(current, arriving); });

        case Combine::Any:
            return holds && !Children.empty() &&
                   std::any_of(Children.begin(),
                               Children.end(),
                               [&current, arriving](const Condition& child)
                               { return child.Evaluate(current, arriving); });

        case Combine::Not:
            return holds && Children.size() == 1 && !Children.front().Evaluate(current, arriving);
        }

        return false;
    }

    auto Condition::CollectSignalsWanted() const -> std::set<std::type_index>
    {
        std::set<std::type_index> wanted;

        if (Expected != nullptr)
        {
            wanted.insert(std::type_index(typeid(*Expected)));
        }

        for (const Condition& child : Children)
        {
            const auto fromChild = child.CollectSignalsWanted();
            wanted.insert(fromChild.begin(), fromChild.end());
        }

        return wanted;
    }
} // namespace Famulus
