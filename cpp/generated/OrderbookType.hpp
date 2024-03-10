#pragma once

#include <string>
#include <ostream>
#include <cstdint>

namespace my_models
{
enum class OrderbookType : std::uint8_t
{
    Snapshot = 1,
    Delta = 2,
};
}; // namespace my_models

inline std::string to_string(my_models::OrderbookType value)
{
    switch (value)
    {
        case my_models::OrderbookType::Snapshot:
            return "Snapshot";
        case my_models::OrderbookType::Delta:
            return "Delta";
        default:
            return "Unknown";
    }
}

inline std::ostream& operator<<(std::ostream& os, const my_models::OrderbookType& obj)
{
    os << to_string(obj);
    return os;
}
