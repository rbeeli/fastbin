#pragma once

#include <string>
#include <ostream>
#include <cstdint>

namespace my_models
{
/**
 * https://bybit-exchange.github.io/docs/v5/enum#tickdirection
 */
enum class TickDirection : std::uint8_t
{
    Unknown = 0,

    /**
     * Price rise.
     */
    PlusTick = 1,

    /**
     * Trade occurs at the same price as the previous trade,
     * which occurred at a price lower than that for the trade preceding it.
     * 
     * Example price series: 100 -> 99 -> 99
     */
    ZeroPlusTick = 2,

    /**
     * Price drop.
     */
    MinusTick = 3,

    /**
     * Trade occurs at the same price as the previous trade,
     * which occurred at a price higher than that for the trade preceding it.
     * 
     * Example price series: 100 -> 101 -> 101
     */
    ZeroMinusTick = 4,
};
}; // namespace my_models

inline std::string to_string(my_models::TickDirection value)
{
    switch (value)
    {
        case my_models::TickDirection::Unknown:
            return "Unknown";
        case my_models::TickDirection::PlusTick:
            return "PlusTick";
        case my_models::TickDirection::ZeroPlusTick:
            return "ZeroPlusTick";
        case my_models::TickDirection::MinusTick:
            return "MinusTick";
        case my_models::TickDirection::ZeroMinusTick:
            return "ZeroMinusTick";
        default:
            return "Unknown";
    }
}

inline std::ostream& operator<<(std::ostream& os, const my_models::TickDirection& obj)
{
    os << to_string(obj);
    return os;
}
