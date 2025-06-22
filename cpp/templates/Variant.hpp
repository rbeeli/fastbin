#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>
#include <span>
#include <type_traits>

#include "_traits.hpp"
#include "_BufferStored.hpp"

namespace my_models
{
// Support primitive types / enums
template <typename T>
    requires is_primitive_or_enum<T>::value
struct is_variant_supported_type<T> : std::true_type
{
};

// Support std::string_view
template <>
struct is_variant_supported_type<std::string_view> : std::true_type
{
};

// Support span<is_primitive_or_enum> (recursive)
template <typename T>
struct is_variant_supported_type<std::span<T>> : is_primitive_or_enum<T>
{
};

// Support structs, struct arrays (T must inherit from CRTP type BufferStored<T>)
template <typename T>
    requires is_buffer_stored<T>::value
struct is_variant_supported_type<T> : std::true_type
{
};

/**
 * A variant type that can hold one of multiple types.
 * The data is always aligned to 8 bytes.
 * 
 * Memory Layout:
 * [size_t size_and_index][data: T]
 * ^                      ^
 * buffer points here     bufferptr() points here
 * 
 * - size_and_index stores the size of the buffer and the index of the active type in a single size_t.
 * - The active index is stored in the top 1 byte of the size_and_index, allowing for 256 different types.
 * - Consequently, the maximum size of the storage buffer is 2^56 bytes (~72 petabytes).
 * - The size itself includes the 8 bytes of size_and_index.
 */
template <typename... Types>
class Variant final : public BufferStored<Variant<Types...>>
{
    static_assert(
        (is_variant_supported_type<Types>::value && ...), "Some Variant types are not supported"
    );
    static_assert(sizeof...(Types) <= 255, "Maximum 255 types supported");

public:
    using BufferStored<Variant<Types...>>::buffer;
    using BufferStored<Variant<Types...>>::buffer_size;
    using BufferStored<Variant<Types...>>::owns_buffer;

protected:
    friend class BufferStored<Variant<Types...>>;

    // constructor forwards to base class
    Variant(std::byte* buffer, size_t buffer_size, bool owns_buffer) noexcept
        : BufferStored<Variant<Types...>>(buffer, buffer_size, owns_buffer)
    {
    }

    /**
     * Sets the active index of the variant in the top byte of size_and_index.
     */
    inline void index(size_t index) noexcept
    {
        size_t size_and_index = *reinterpret_cast<size_t*>(buffer);
        size_and_index &= 0xFFFFFFFFFFFFFF00; // Clear the top byte
        size_and_index |= index;              // Set the index
        *reinterpret_cast<size_t*>(buffer) = size_and_index;
    }

    /**
     * Returns the total size of the buffer in bytes.
     */
    inline size_t size() const noexcept
    {
        return *reinterpret_cast<size_t*>(buffer) >> 8;
    }

    /**
     * Returns the size of the stored type data in bytes.
     */
    inline size_t data_size() const noexcept
    {
        return size() - sizeof(size_t);
    }

    inline void size(size_t size) noexcept
    {
        size_t size_and_index = *reinterpret_cast<size_t*>(buffer);
        size_and_index &= 0xFF;        // Clear the size
        size_and_index |= (size << 8); // Shift size to the left by 8 bits
        *reinterpret_cast<size_t*>(buffer) = size_and_index;
    }

    inline std::byte* bufferptr() const noexcept
    {
        return buffer + sizeof(size_t);
    }

    template <typename T>
    static constexpr size_t type_index()
    {
        size_t idx = 0;
        bool found = false;
        ((found |= std::is_same_v<T, Types>, found ? 0 : ++idx), ...);
        return idx;
    }

public:
    // copy
    Variant(const Variant&) = delete;
    Variant& operator=(const Variant&) = delete;

    // move
    Variant(Variant&&) noexcept = default;
    Variant& operator=(Variant&&) noexcept = default;

    // Static creation methods
    static Variant create(std::byte* buffer, size_t buffer_size, bool owns_buffer) noexcept
    {
        // Zero-initialize the buffer
        std::memset(buffer, 0, buffer_size);

        // Create Variant object in the buffer
        Variant arr(buffer, buffer_size, owns_buffer);

        // Initial size is sizeof(size_t) for size_and_index
        arr.size(sizeof(size_t));

        return arr;
    }

    /**
     * Returns the number of types in the variant.
     */
    static constexpr size_t types_count() noexcept
    {
        return sizeof...(Types);
    }

    /**
     * Index of the active type in the variant.
     */
    inline size_t index() const noexcept
    {
        return *reinterpret_cast<size_t*>(buffer) & 0xFF;
    }

    /**
     * Indicates whether the variant is empty.
     */
    bool empty() const noexcept
    {
        return data_size() == 0;
    }

    template <typename T>
    bool holds_alternative() const
    {
        return !empty() && index() == type_index<T>();
    }

    // Get/set primitive values
    template <typename T>
        requires is_primitive_or_enum<T>::value && (std::is_same_v<T, Types> || ...)
    void set(const T& value)
    {
        // Set the active index
        index(type_index<T>());

        // Set the size of the buffer
        size_t total_size = sizeof(size_t) + sizeof(T);
        assert(buffer_size >= total_size && "Buffer size too small");
        size(total_size);

        // Copy the value to the buffer
        std::memcpy(bufferptr(), &value, sizeof(T));
    }

    template <typename T>
        requires is_primitive_or_enum<T>::value && (std::is_same_v<T, Types> || ...)
    T get() const
    {
        assert(!empty() && "Variant is empty");
        assert(holds_alternative<T>() && "Invalid type");
        return *reinterpret_cast<const T*>(bufferptr());
    }

    // Get/set std::string_view
    template <typename T>
        requires std::is_same_v<T, std::string_view> && (std::is_same_v<T, Types> || ...)
    void set(const T& value)
    {
        // Set the active index
        index(type_index<T>());

        // Set the size of the buffer
        size_t total_size = sizeof(size_t) + value.size();
        assert(buffer_size >= total_size && "Buffer size too small");
        size(total_size);

        // Copy the string to the buffer
        std::memcpy(bufferptr(), value.data(), value.size());
    }

    template <typename T>
        requires std::is_same_v<T, std::string_view> && (std::is_same_v<T, Types> || ...)
    T get() const
    {
        assert(!empty() && "Variant is empty");
        assert(holds_alternative<T>() && "Invalid type");
        return T(reinterpret_cast<const char*>(bufferptr()), data_size());
    }

    // Get/set std::span<T> for is_primitive_or_enum<T>
    template <typename T>
        requires(requires(T t) {
            requires std::is_same_v<T, std::span<typename T::value_type>>; // Must be a span
            requires is_primitive_or_enum<
                typename T::value_type>::value;        // Element type must be primitive/enum
            requires(std::is_same_v<T, Types> || ...); // Must be in variant's type list
        })
    void set(const T& value)
    {
        // Set the active index
        index(type_index<T>());

        // Set the size of the buffer
        size_t total_size = sizeof(size_t) + (value.size_bytes());
        assert(buffer_size >= total_size && "Buffer size too small");
        size(total_size);

        // Copy the span data to the buffer
        std::memcpy(bufferptr(), value.data(), value.size_bytes());
    }

    template <typename T>
        requires(requires(T t) {
            requires std::is_same_v<T, std::span<typename T::value_type>>; // Must be a span
            requires is_primitive_or_enum<
                typename T::value_type>::value;        // Element type must be primitive/enum
            requires(std::is_same_v<T, Types> || ...); // Must be in variant's type list
        })
    T get() const
    {
        assert(!empty() && "Variant is empty");
        assert(holds_alternative<T>() && "Invalid type");
        return T(
            reinterpret_cast<typename T::value_type*>(bufferptr()),
            data_size() / sizeof(typename T::value_type)
        );
    }

    // Get/set for is_buffer_stored<T>
    template <typename T>
        requires is_buffer_stored<T>::value && (std::is_same_v<T, Types> || ...)
    void set(const T& value)
    {
        // Set the active index
        index(type_index<T>());

        // Set the size of the buffer
        size_t total_size = sizeof(size_t) + value.fastbin_binary_size();
        assert(buffer_size >= total_size && "Buffer size too small");
        size(total_size);

        // Copy the value to the buffer
        std::memcpy(bufferptr(), value.buffer, value.fastbin_binary_size());
    }

    template <typename T>
        requires is_buffer_stored<T>::value && (std::is_same_v<T, Types> || ...)
    T get() const
    {
        assert(!empty() && "Variant is empty");
        assert(holds_alternative<T>() && "Invalid type");
        return T::open(bufferptr(), data_size(), false);
    }

    /**
     * Returns the stored binary size of the object.
     */
    inline size_t fastbin_calc_binary_size() const noexcept
    {
        return size();
    }

    /**
     * Calculates the binary size of a Variant for a given element type.
     * This function is used to pre-calculate the size before creating the Variant.
     */
    template <typename T>
    static inline size_t fastbin_calc_binary_size(const T& item) noexcept
    {
        return sizeof(size_t) + item.fastbin_calc_binary_size();
    }

    inline size_t fastbin_binary_size() const noexcept
    {
        return size();
    }

    inline void fastbin_finalize() noexcept
    {
        // no-op
    }
};

// Type traits
template <typename... Types>
struct is_variable_size<Variant<Types...>>
{
    static constexpr bool value = true;
};
} // namespace my_models
