#pragma once

#include <string>
#include <ostream>
#include <utility>
#include <cstdint>

namespace my_models
{
enum class OrderbookType : std::uint8_t
{
    Snapshot = 1,
    Delta = 2,
};
}; // namespace my_models

[[nodiscard]] constexpr std::string_view to_string(my_models::OrderbookType value) noexcept
{
    switch (value)
    {
        case my_models::OrderbookType::Snapshot:
            return "Snapshot";
        case my_models::OrderbookType::Delta:
            return "Delta";
    }
    std::unreachable();
}

std::ostream& operator<<(std::ostream& os, const my_models::OrderbookType& obj)
{
    os << to_string(obj);
    return os;
}
