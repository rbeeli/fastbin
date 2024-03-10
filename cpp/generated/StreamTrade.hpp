#pragma once

#include <cstddef>
#include <cassert>
#include <ostream>
#include <span>
#include <cstdint>
#include <string_view>
#include "TradeSide.hpp"
#include "TickDirection.hpp"

namespace my_models
{
/**
 * https://bybit-exchange.github.io/docs/v5/websocket/public/trade
 *
 * ------------------------------------------------------------
 *
 * Binary serializable data container.
 * 
 * This container has variable size.
 * 
 * Setter methods from the first variable-sized field onwards
 * MUST be called in order.
 * 
 * The `finalize()` method MUST be called after all setter methods have been called.
 * 
 * It is the responsibility of the caller to ensure that the buffer is
 * large enough to hold the serialized data.
 */
struct StreamTrade
{
    std::byte* buffer{nullptr};
    size_t buffer_size{0};
    bool owns_buffer{false};

    explicit StreamTrade(std::byte* buffer, size_t binary_size, bool owns_buffer) noexcept
        : buffer(buffer), buffer_size(binary_size), owns_buffer(owns_buffer)
    {
    }

    ~StreamTrade() noexcept
    {
        if (owns_buffer && buffer != nullptr)
        {
            delete[] buffer;
            buffer = nullptr;
        }
    }

    // disable copy
    StreamTrade(const StreamTrade&) = delete;
    StreamTrade& operator=(const StreamTrade&) = delete;

    // enable move
    StreamTrade(StreamTrade&& other) noexcept
        : buffer(other.buffer), buffer_size(other.buffer_size), owns_buffer(other.owns_buffer)
    {
        other.buffer = nullptr;
        other.buffer_size = 0;
    }
    StreamTrade& operator=(StreamTrade&& other) noexcept
    {
        if (this != &other)
        {
            delete[] buffer;
            buffer = other.buffer;
            buffer_size = other.buffer_size;
            owns_buffer = other.owns_buffer;
            other.buffer = nullptr;
            other.buffer_size = 0;
        }
        return *this;
    }

    constexpr inline size_t fastbin_binary_size() const noexcept
    {
        return *reinterpret_cast<size_t*>(buffer);
    }

    inline std::int64_t server_time() const noexcept
    {
        return *reinterpret_cast<const std::int64_t*>(buffer + fastbin_server_time_offset());
    }

    inline void server_time(const std::int64_t value) noexcept
    {
        *reinterpret_cast<std::int64_t*>(buffer + fastbin_server_time_offset()) = value;
    }

    constexpr inline size_t fastbin_server_time_offset() const noexcept
    {
        return 8;
    }

    constexpr inline size_t fastbin_server_time_size() const noexcept
    {
        return 8;
    }

    inline std::int64_t recv_time() const noexcept
    {
        return *reinterpret_cast<const std::int64_t*>(buffer + fastbin_recv_time_offset());
    }

    inline void recv_time(const std::int64_t value) noexcept
    {
        *reinterpret_cast<std::int64_t*>(buffer + fastbin_recv_time_offset()) = value;
    }

    constexpr inline size_t fastbin_recv_time_offset() const noexcept
    {
        return fastbin_server_time_offset() + fastbin_server_time_size();
    }

    constexpr inline size_t fastbin_recv_time_size() const noexcept
    {
        return 8;
    }

    inline std::string_view symbol() const noexcept
    {
        size_t n_bytes = fastbin_symbol_size_unaligned() - 8;
        size_t count = n_bytes;
        auto ptr = reinterpret_cast<const char*>(buffer + fastbin_symbol_offset() + 8);
        return std::string_view(ptr, count);
    }

    inline void symbol(const std::string_view value) noexcept
    {
        size_t offset = fastbin_symbol_offset();
        size_t unaligned_size = 8 + value.size() * sizeof(char);
        size_t aligned_size = (unaligned_size + 7) & ~7;
        size_t aligned_diff = aligned_size - unaligned_size;
        size_t aligned_size_high = aligned_size | (aligned_diff << 56);
        *reinterpret_cast<size_t*>(buffer + offset) = aligned_size_high;
        auto el_ptr = reinterpret_cast<char*>(buffer + offset + 8);
        std::copy(value.begin(), value.end(), el_ptr);
    }

    constexpr inline size_t fastbin_symbol_offset() const noexcept
    {
        return fastbin_recv_time_offset() + fastbin_recv_time_size();
    }

    constexpr inline size_t fastbin_symbol_size() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_symbol_offset());
        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;
        return aligned_size;
    }

    constexpr inline size_t fastbin_symbol_size_unaligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_symbol_offset());
        size_t aligned_diff = stored_size >> 56;
        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;
        return aligned_size - aligned_diff;
    }

    inline std::int64_t fill_time() const noexcept
    {
        return *reinterpret_cast<const std::int64_t*>(buffer + fastbin_fill_time_offset());
    }

    inline void fill_time(const std::int64_t value) noexcept
    {
        *reinterpret_cast<std::int64_t*>(buffer + fastbin_fill_time_offset()) = value;
    }

    constexpr inline size_t fastbin_fill_time_offset() const noexcept
    {
        return fastbin_symbol_offset() + fastbin_symbol_size();
    }

    constexpr inline size_t fastbin_fill_time_size() const noexcept
    {
        return 8;
    }

    inline TradeSide side() const noexcept
    {
        return static_cast<TradeSide>(*reinterpret_cast<const TradeSide*>(buffer + fastbin_side_offset()));
    }

    inline void side(const TradeSide value) noexcept
    {
        *reinterpret_cast<TradeSide*>(buffer + fastbin_side_offset()) = static_cast<TradeSide>(value);
    }

    constexpr inline size_t fastbin_side_offset() const noexcept
    {
        return fastbin_fill_time_offset() + fastbin_fill_time_size();
    }

    constexpr inline size_t fastbin_side_size() const noexcept
    {
        return 8;
    }

    inline double price() const noexcept
    {
        return *reinterpret_cast<const double*>(buffer + fastbin_price_offset());
    }

    inline void price(const double value) noexcept
    {
        *reinterpret_cast<double*>(buffer + fastbin_price_offset()) = value;
    }

    constexpr inline size_t fastbin_price_offset() const noexcept
    {
        return fastbin_side_offset() + fastbin_side_size();
    }

    constexpr inline size_t fastbin_price_size() const noexcept
    {
        return 8;
    }

    inline TickDirection price_chg_dir() const noexcept
    {
        return static_cast<TickDirection>(*reinterpret_cast<const TickDirection*>(buffer + fastbin_price_chg_dir_offset()));
    }

    inline void price_chg_dir(const TickDirection value) noexcept
    {
        *reinterpret_cast<TickDirection*>(buffer + fastbin_price_chg_dir_offset()) = static_cast<TickDirection>(value);
    }

    constexpr inline size_t fastbin_price_chg_dir_offset() const noexcept
    {
        return fastbin_price_offset() + fastbin_price_size();
    }

    constexpr inline size_t fastbin_price_chg_dir_size() const noexcept
    {
        return 8;
    }

    inline double size() const noexcept
    {
        return *reinterpret_cast<const double*>(buffer + fastbin_size_offset());
    }

    inline void size(const double value) noexcept
    {
        *reinterpret_cast<double*>(buffer + fastbin_size_offset()) = value;
    }

    constexpr inline size_t fastbin_size_offset() const noexcept
    {
        return fastbin_price_chg_dir_offset() + fastbin_price_chg_dir_size();
    }

    constexpr inline size_t fastbin_size_size() const noexcept
    {
        return 8;
    }

    inline std::string_view trade_id() const noexcept
    {
        size_t n_bytes = fastbin_trade_id_size_unaligned() - 8;
        size_t count = n_bytes;
        auto ptr = reinterpret_cast<const char*>(buffer + fastbin_trade_id_offset() + 8);
        return std::string_view(ptr, count);
    }

    inline void trade_id(const std::string_view value) noexcept
    {
        size_t offset = fastbin_trade_id_offset();
        size_t unaligned_size = 8 + value.size() * sizeof(char);
        size_t aligned_size = (unaligned_size + 7) & ~7;
        size_t aligned_diff = aligned_size - unaligned_size;
        size_t aligned_size_high = aligned_size | (aligned_diff << 56);
        *reinterpret_cast<size_t*>(buffer + offset) = aligned_size_high;
        auto el_ptr = reinterpret_cast<char*>(buffer + offset + 8);
        std::copy(value.begin(), value.end(), el_ptr);
    }

    constexpr inline size_t fastbin_trade_id_offset() const noexcept
    {
        return fastbin_size_offset() + fastbin_size_size();
    }

    constexpr inline size_t fastbin_trade_id_size() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_trade_id_offset());
        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;
        return aligned_size;
    }

    constexpr inline size_t fastbin_trade_id_size_unaligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_trade_id_offset());
        size_t aligned_diff = stored_size >> 56;
        size_t aligned_size = stored_size & 0x00FFFFFFFFFFFFFF;
        return aligned_size - aligned_diff;
    }

    inline bool block_trade() const noexcept
    {
        return *reinterpret_cast<const bool*>(buffer + fastbin_block_trade_offset());
    }

    inline void block_trade(const bool value) noexcept
    {
        *reinterpret_cast<bool*>(buffer + fastbin_block_trade_offset()) = value;
    }

    constexpr inline size_t fastbin_block_trade_offset() const noexcept
    {
        return fastbin_trade_id_offset() + fastbin_trade_id_size();
    }

    constexpr inline size_t fastbin_block_trade_size() const noexcept
    {
        return 8;
    }

    constexpr inline size_t fastbin_compute_binary_size() const noexcept
    {
        return fastbin_block_trade_offset() + fastbin_block_trade_size();
    }

    inline void fastbin_finalize() const noexcept
    {
        *reinterpret_cast<size_t*>(buffer) = fastbin_compute_binary_size();
    }
};
}; // namespace my_models

inline std::ostream& operator<<(std::ostream& os, const my_models::StreamTrade& obj)
{
    os << "[my_models::StreamTrade size=" << obj.fastbin_binary_size() << " bytes]\n";
    os << "    server_time: " << obj.server_time() << "\n";
    os << "    recv_time: " << obj.recv_time() << "\n";
    os << "    symbol: " << std::string(obj.symbol()) << "\n";
    os << "    fill_time: " << obj.fill_time() << "\n";
    os << "    side: " << obj.side() << "\n";
    os << "    price: " << obj.price() << "\n";
    os << "    price_chg_dir: " << obj.price_chg_dir() << "\n";
    os << "    size: " << obj.size() << "\n";
    os << "    trade_id: " << std::string(obj.trade_id()) << "\n";
    os << "    block_trade: " << (obj.block_trade() ? "true" : "false") << "\n";
    return os;
}
