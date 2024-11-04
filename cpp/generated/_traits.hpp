#pragma once

namespace my_models
{
template <typename T>
struct is_variable_size
{
    static constexpr bool value = false;
};
}; // namespace my_models
