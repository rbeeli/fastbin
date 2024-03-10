#pragma once

#include <cstddef>
#include <cassert>
#include <ostream>
#include <span>
#include <cstdint>
#include <string_view>
#include "OrderbookType.hpp"

namespace my_models
{
/**
 * https://bybit-exchange.github.io/docs/v5/websocket/public/orderbook
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
struct StreamOrderbook
{
    std::byte* buffer{nullptr};
    size_t buffer_size{0};
    bool owns_buffer{false};

    explicit StreamOrderbook(std::byte* buffer, size_t binary_size, bool owns_buffer) noexcept
        : buffer(buffer), buffer_size(binary_size), owns_buffer(owns_buffer)
    {
    }

    ~StreamOrderbook() noexcept
    {
        if (owns_buffer && buffer != nullptr)
        {
            delete[] buffer;
            buffer = nullptr;
        }
    }

    // disable copy
    StreamOrderbook(const StreamOrderbook&) = delete;
    StreamOrderbook& operator=(const StreamOrderbook&) = delete;

    // enable move
    StreamOrderbook(StreamOrderbook&& other) noexcept
        : buffer(other.buffer), buffer_size(other.buffer_size), owns_buffer(other.owns_buffer)
    {
        other.buffer = nullptr;
        other.buffer_size = 0;
    }
    StreamOrderbook& operator=(StreamOrderbook&& other) noexcept
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

    inline std::int64_t cts() const noexcept
    {
        return *reinterpret_cast<const std::int64_t*>(buffer + fastbin_cts_offset());
    }

    inline void cts(const std::int64_t value) noexcept
    {
        *reinterpret_cast<std::int64_t*>(buffer + fastbin_cts_offset()) = value;
    }

    constexpr inline size_t fastbin_cts_offset() const noexcept
    {
        return fastbin_recv_time_offset() + fastbin_recv_time_size();
    }

    constexpr inline size_t fastbin_cts_size() const noexcept
    {
        return 8;
    }

    inline OrderbookType type() const noexcept
    {
        return static_cast<OrderbookType>(*reinterpret_cast<const OrderbookType*>(buffer + fastbin_type_offset()));
    }

    inline void type(const OrderbookType value) noexcept
    {
        *reinterpret_cast<OrderbookType*>(buffer + fastbin_type_offset()) = static_cast<OrderbookType>(value);
    }

    constexpr inline size_t fastbin_type_offset() const noexcept
    {
        return fastbin_cts_offset() + fastbin_cts_size();
    }

    constexpr inline size_t fastbin_type_size() const noexcept
    {
        return 8;
    }

    inline std::uint16_t depth() const noexcept
    {
        return *reinterpret_cast<const std::uint16_t*>(buffer + fastbin_depth_offset());
    }

    inline void depth(const std::uint16_t value) noexcept
    {
        *reinterpret_cast<std::uint16_t*>(buffer + fastbin_depth_offset()) = value;
    }

    constexpr inline size_t fastbin_depth_offset() const noexcept
    {
        return fastbin_type_offset() + fastbin_type_size();
    }

    constexpr inline size_t fastbin_depth_size() const noexcept
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
        return fastbin_depth_offset() + fastbin_depth_size();
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

    inline std::uint64_t update_id() const noexcept
    {
        return *reinterpret_cast<const std::uint64_t*>(buffer + fastbin_update_id_offset());
    }

    inline void update_id(const std::uint64_t value) noexcept
    {
        *reinterpret_cast<std::uint64_t*>(buffer + fastbin_update_id_offset()) = value;
    }

    constexpr inline size_t fastbin_update_id_offset() const noexcept
    {
        return fastbin_symbol_offset() + fastbin_symbol_size();
    }

    constexpr inline size_t fastbin_update_id_size() const noexcept
    {
        return 8;
    }

    inline std::uint64_t seq_num() const noexcept
    {
        return *reinterpret_cast<const std::uint64_t*>(buffer + fastbin_seq_num_offset());
    }

    inline void seq_num(const std::uint64_t value) noexcept
    {
        *reinterpret_cast<std::uint64_t*>(buffer + fastbin_seq_num_offset()) = value;
    }

    constexpr inline size_t fastbin_seq_num_offset() const noexcept
    {
        return fastbin_update_id_offset() + fastbin_update_id_size();
    }

    constexpr inline size_t fastbin_seq_num_size() const noexcept
    {
        return 8;
    }

    inline std::span<double> bid_prices() const noexcept
    {
        size_t n_bytes = fastbin_bid_prices_size_unaligned() - 8;
        size_t count = n_bytes >> 3;
        auto ptr = reinterpret_cast<double*>(buffer + fastbin_bid_prices_offset() + 8);
        return std::span<double>(ptr, count);
    }

    inline void bid_prices(const std::span<double> value) noexcept
    {
        size_t offset = fastbin_bid_prices_offset();
        *reinterpret_cast<size_t*>(buffer + offset) = 8 + value.size() * sizeof(double);
        auto el_ptr = reinterpret_cast<double*>(buffer + offset + 8);
        std::copy(value.begin(), value.end(), el_ptr);
    }

    constexpr inline size_t fastbin_bid_prices_offset() const noexcept
    {
        return fastbin_seq_num_offset() + fastbin_seq_num_size();
    }

    constexpr inline size_t fastbin_bid_prices_size() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_bid_prices_offset());
        return stored_size;
    }

    constexpr inline size_t fastbin_bid_prices_size_unaligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_bid_prices_offset());
        return stored_size;
    }

    inline std::span<double> bid_quantities() const noexcept
    {
        size_t n_bytes = fastbin_bid_quantities_size_unaligned() - 8;
        size_t count = n_bytes >> 3;
        auto ptr = reinterpret_cast<double*>(buffer + fastbin_bid_quantities_offset() + 8);
        return std::span<double>(ptr, count);
    }

    inline void bid_quantities(const std::span<double> value) noexcept
    {
        size_t offset = fastbin_bid_quantities_offset();
        *reinterpret_cast<size_t*>(buffer + offset) = 8 + value.size() * sizeof(double);
        auto el_ptr = reinterpret_cast<double*>(buffer + offset + 8);
        std::copy(value.begin(), value.end(), el_ptr);
    }

    constexpr inline size_t fastbin_bid_quantities_offset() const noexcept
    {
        return fastbin_bid_prices_offset() + fastbin_bid_prices_size();
    }

    constexpr inline size_t fastbin_bid_quantities_size() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_bid_quantities_offset());
        return stored_size;
    }

    constexpr inline size_t fastbin_bid_quantities_size_unaligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_bid_quantities_offset());
        return stored_size;
    }

    inline std::span<double> ask_prices() const noexcept
    {
        size_t n_bytes = fastbin_ask_prices_size_unaligned() - 8;
        size_t count = n_bytes >> 3;
        auto ptr = reinterpret_cast<double*>(buffer + fastbin_ask_prices_offset() + 8);
        return std::span<double>(ptr, count);
    }

    inline void ask_prices(const std::span<double> value) noexcept
    {
        size_t offset = fastbin_ask_prices_offset();
        *reinterpret_cast<size_t*>(buffer + offset) = 8 + value.size() * sizeof(double);
        auto el_ptr = reinterpret_cast<double*>(buffer + offset + 8);
        std::copy(value.begin(), value.end(), el_ptr);
    }

    constexpr inline size_t fastbin_ask_prices_offset() const noexcept
    {
        return fastbin_bid_quantities_offset() + fastbin_bid_quantities_size();
    }

    constexpr inline size_t fastbin_ask_prices_size() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_ask_prices_offset());
        return stored_size;
    }

    constexpr inline size_t fastbin_ask_prices_size_unaligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_ask_prices_offset());
        return stored_size;
    }

    inline std::span<double> ask_quantities() const noexcept
    {
        size_t n_bytes = fastbin_ask_quantities_size_unaligned() - 8;
        size_t count = n_bytes >> 3;
        auto ptr = reinterpret_cast<double*>(buffer + fastbin_ask_quantities_offset() + 8);
        return std::span<double>(ptr, count);
    }

    inline void ask_quantities(const std::span<double> value) noexcept
    {
        size_t offset = fastbin_ask_quantities_offset();
        *reinterpret_cast<size_t*>(buffer + offset) = 8 + value.size() * sizeof(double);
        auto el_ptr = reinterpret_cast<double*>(buffer + offset + 8);
        std::copy(value.begin(), value.end(), el_ptr);
    }

    constexpr inline size_t fastbin_ask_quantities_offset() const noexcept
    {
        return fastbin_ask_prices_offset() + fastbin_ask_prices_size();
    }

    constexpr inline size_t fastbin_ask_quantities_size() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_ask_quantities_offset());
        return stored_size;
    }

    constexpr inline size_t fastbin_ask_quantities_size_unaligned() const noexcept
    {
        size_t stored_size = *reinterpret_cast<size_t*>(buffer + fastbin_ask_quantities_offset());
        return stored_size;
    }

    constexpr inline size_t fastbin_compute_binary_size() const noexcept
    {
        return fastbin_ask_quantities_offset() + fastbin_ask_quantities_size();
    }

    inline void fastbin_finalize() const noexcept
    {
        *reinterpret_cast<size_t*>(buffer) = fastbin_compute_binary_size();
    }
};
}; // namespace my_models

inline std::ostream& operator<<(std::ostream& os, const my_models::StreamOrderbook& obj)
{
    os << "[my_models::StreamOrderbook size=" << obj.fastbin_binary_size() << " bytes]\n";
    os << "    server_time: " << obj.server_time() << "\n";
    os << "    recv_time: " << obj.recv_time() << "\n";
    os << "    cts: " << obj.cts() << "\n";
    os << "    type: " << obj.type() << "\n";
    os << "    depth: " << obj.depth() << "\n";
    os << "    symbol: " << std::string(obj.symbol()) << "\n";
    os << "    update_id: " << obj.update_id() << "\n";
    os << "    seq_num: " << obj.seq_num() << "\n";
    os << "    bid_prices: " << "[vector<float64> count=" << obj.bid_prices().size() << "]" << "\n";
    os << "    bid_quantities: " << "[vector<float64> count=" << obj.bid_quantities().size() << "]" << "\n";
    os << "    ask_prices: " << "[vector<float64> count=" << obj.ask_prices().size() << "]" << "\n";
    os << "    ask_quantities: " << "[vector<float64> count=" << obj.ask_quantities().size() << "]" << "\n";
    return os;
}
