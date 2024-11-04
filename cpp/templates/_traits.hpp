#pragma once

namespace YOUR_NAMESPACE
{
template <typename T>
struct is_variable_size
{
    static constexpr bool value = false;
};
}; // namespace YOUR_NAMESPACE
