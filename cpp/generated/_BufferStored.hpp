#pragma once

#include <cstddef>

#include "_traits.hpp"

namespace my_models
{
template <typename T>
class BufferStored
{
public:
    std::byte* buffer{nullptr};
    size_t buffer_size{0};
    bool owns_buffer{false};

    // destructor
    ~BufferStored() noexcept
    {
        if (owns_buffer && buffer)
        {
            delete[] buffer;
        }
    }

protected:
    // constructor
    BufferStored(std::byte* buffer, size_t buffer_size, bool owns_buffer) noexcept
        : buffer(buffer), buffer_size(buffer_size), owns_buffer(owns_buffer)
    {
    }

    // disable copy
    BufferStored(const BufferStored&) = delete;
    BufferStored& operator=(const BufferStored&) = delete;

    // enable move
    BufferStored(BufferStored&& other) noexcept
        : buffer(other.buffer), buffer_size(other.buffer_size), owns_buffer(other.owns_buffer)
    {
        other.buffer = nullptr;
        other.buffer_size = 0;
        other.owns_buffer = false;
    }
    BufferStored& operator=(BufferStored&& other) noexcept
    {
        if (this != &other)
        {
            if (owns_buffer && buffer)
            {
                delete[] buffer;
            }
            buffer = other.buffer;
            buffer_size = other.buffer_size;
            owns_buffer = other.owns_buffer;
            other.buffer = nullptr;
            other.buffer_size = 0;
            other.owns_buffer = false;
        }
        return *this;
    }

public:
    // Open & create factory methods
    static T create(std::span<std::byte> buffer, bool owns_buffer) noexcept
    {
        return create(buffer.data(), buffer.size(), owns_buffer);
    }

    static T open(std::byte* buffer, size_t buffer_size, bool owns_buffer) noexcept
    {
        return {buffer, buffer_size, owns_buffer};
    }

    static T open(std::span<std::byte> buffer, bool owns_buffer) noexcept
    {
        return open(buffer.data(), buffer.size(), owns_buffer);
    }

    // Copy methods
    [[nodiscard]] T copy(
        std::byte* dest_buffer, size_t dest_buffer_size, bool owns_buffer
    ) const noexcept
    {
        assert(dest_buffer_size >= buffer_size && "New buffer size too small");
        std::memcpy(dest_buffer, buffer, buffer_size);
        return {dest_buffer, dest_buffer_size, owns_buffer};
    }

    [[nodiscard]] T copy() const noexcept
    {
        auto dest_buffer = new std::byte[buffer_size];
        std::memcpy(dest_buffer, buffer, buffer_size);
        return {dest_buffer, buffer_size, true};
    }
};
} // namespace my_models
