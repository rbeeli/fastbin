#pragma once

#include <string>
#include <ostream>
#include <cstdint>

namespace my_models
{
enum class TradeSide : std::uint8_t
{
    Sell = 0,
    Buy = 1,
};
}; // namespace my_models

inline std::string to_string(my_models::TradeSide value)
{
    switch (value)
    {
        case my_models::TradeSide::Sell:
            return "Sell";
        case my_models::TradeSide::Buy:
            return "Buy";
        default:
            return "Unknown";
    }
}

inline std::ostream& operator<<(std::ostream& os, const my_models::TradeSide& obj)
{
    os << to_string(obj);
    return os;
}
