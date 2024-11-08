#pragma once

namespace YOUR_NAMESPACE
{
template <typename T>
struct is_variable_size
{
    static constexpr bool value = false;
};

// Helper to detect if type derives from BufferStored<T>
template <typename T>
struct is_buffer_stored : std::false_type
{
};

// All supported primitive and enum types
template <typename T>
struct is_primitive_or_enum
{
    static constexpr bool value = //
        std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, int32_t> ||
        std::is_same_v<T, int64_t> || std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> ||
        std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t> || std::is_same_v<T, float> ||
        std::is_same_v<T, double> || std::is_same_v<T, char> || std::is_same_v<T, std::byte> ||
        std::is_same_v<T, bool> || std::is_enum_v<T>;
};

// Helper to detect if type is supported by Variant
template <typename T>
struct is_variant_supported_type : std::false_type
{
};
}; // namespace YOUR_NAMESPACE
