#pragma once

#include <cstddef>
#include <cstring>
#include <type_traits>

namespace my_models
{
template <class T>
concept Blittable = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

template <Blittable T>
[[gnu::always_inline]] [[nodiscard]]
inline T load_trivial(const std::byte* buffer, std::size_t offset) noexcept
{
    T out;
    std::memcpy(&out, buffer + offset, sizeof(T));
    return out;
}

template <Blittable T>
[[gnu::always_inline]]
inline void store_trivial(std::byte* buffer, std::size_t offset, T value) noexcept
{
    std::memcpy(buffer + offset, &value, sizeof(T));
}
}; // namespace my_models
