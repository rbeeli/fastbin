#pragma once

#include <string>
#include <ostream>
#include <utility>
#include <cstdint>

namespace my_models
{
enum class TradeSide : std::uint8_t
{
    Sell = 0,
    Buy = 1,
};
}; // namespace my_models

[[nodiscard]] constexpr std::string_view to_string(my_models::TradeSide value) noexcept
{
    switch (value)
    {
        case my_models::TradeSide::Sell:
            return "Sell";
        case my_models::TradeSide::Buy:
            return "Buy";
    }
    std::unreachable();
}

std::ostream& operator<<(std::ostream& os, const my_models::TradeSide& obj)
{
    os << to_string(obj);
    return os;
}
