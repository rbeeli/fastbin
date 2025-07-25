import Base.show
import Base.finalizer
using StringViews

"""
https://bybit-exchange.github.io/docs/v5/websocket/public/trade

------------------------------------------------------------

Binary serializable data container generated by `fastbin`.

This container has variable size.
All setter methods starting from the first variable-sized member and afterwards MUST be called in order.

Members in order
================
- `server_time::Int64`     (fixed)
- `recv_time::Int64`       (fixed)
- `symbol::StringView`     (variable)
- `fill_time::Int64`       (fixed)
- `side::TradeSide.T`      (fixed)
- `price::Float64`         (fixed)
- `price_chg_dir::TickDirection.T` (fixed)
- `size::Float64`          (fixed)
- `trade_id::StringView`   (variable)
- `block_trade::Bool`      (fixed)

The `fastbin_finalize!()` method MUST be called after all setter methods have been called.

It is the responsibility of the caller to ensure that the buffer is
large enough to hold all data.
"""
mutable struct StreamTrade
    buffer::Ptr{UInt8}
    buffer_size::UInt64
    owns_buffer::Bool

    function StreamTrade(buffer::Ptr{UInt8}, buffer_size::Integer, owns_buffer::Bool)
        owns_buffer || (@assert iszero(UInt(buffer) & 0x7) "Buffer not 8 byte aligned")
        new(buffer, UInt64(buffer_size), owns_buffer)
    end

    function StreamTrade(buffer_size::Integer)
        buffer = reinterpret(Ptr{UInt8}, Base.Libc.malloc(buffer_size))
        obj = new(buffer, buffer_size, true)
        finalizer(_finalize!, obj)
    end
end

_finalize!(obj::StreamTrade) = Base.Libc.free(obj.buffer)


# Member: server_time::Int64

@inline function server_time(obj::StreamTrade)::Int64
    return unsafe_load(reinterpret(Ptr{Int64}, obj.buffer + _server_time_offset(obj)))
end

@inline function server_time!(obj::StreamTrade, value::Int64)
    unsafe_store!(reinterpret(Ptr{Int64}, obj.buffer + _server_time_offset(obj)), value)
end

@inline function _server_time_offset(obj::StreamTrade)::UInt64
    return 8
end

@inline function _server_time_size_aligned(obj::StreamTrade)::UInt64
    return 8
end

@inline function _server_time_calc_size_aligned(::Type{StreamTrade}, value::Int64)::UInt64
    return 8
end


# Member: recv_time::Int64

@inline function recv_time(obj::StreamTrade)::Int64
    return unsafe_load(reinterpret(Ptr{Int64}, obj.buffer + _recv_time_offset(obj)))
end

@inline function recv_time!(obj::StreamTrade, value::Int64)
    unsafe_store!(reinterpret(Ptr{Int64}, obj.buffer + _recv_time_offset(obj)), value)
end

@inline function _recv_time_offset(obj::StreamTrade)::UInt64
    return 16
end

@inline function _recv_time_size_aligned(obj::StreamTrade)::UInt64
    return 8
end

@inline function _recv_time_calc_size_aligned(::Type{StreamTrade}, value::Int64)::UInt64
    return 8
end


# Member: symbol::StringView

@inline function symbol(obj::StreamTrade)::StringView
    ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, obj.buffer + _symbol_offset(obj))
    unaligned_size::UInt64 = _symbol_size_unaligned(obj)
    n_bytes::UInt64 = unaligned_size - 8
    count::UInt64 = n_bytes
    return StringView(unsafe_wrap(Vector{UInt8}, ptr + 8, count, own=false))
end

@inline function symbol!(obj::StreamTrade, value::T) where {T<:AbstractString}
    offset::UInt64 = _symbol_offset(obj)
    contents_size::UInt64 = length(value) * 1
    unaligned_size::UInt64 = 8 + contents_size
    aligned_size::UInt64 = (unaligned_size + 7) & ~7
    aligned_diff::UInt64 = aligned_size - unaligned_size
    aligned_size_high::UInt64 = aligned_size | (aligned_diff << 56)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), aligned_size_high)
    dest_ptr::Ptr{UInt8} = obj.buffer + offset + 8
    src_ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, pointer(value))
    unsafe_copyto!(dest_ptr, src_ptr, contents_size)
end

@inline function _symbol_offset(obj::StreamTrade)::UInt64
    return 24
end

@inline function _symbol_size_aligned(obj::StreamTrade)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + _symbol_offset(obj)))
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size
end

@inline function _symbol_calc_size_aligned(::Type{StreamTrade}, value::T)::UInt64 where {T<:AbstractString}
    contents_size::UInt64 = length(value) * 1
    unaligned_size::UInt64 = 8 + contents_size
    return (unaligned_size + 7) & ~7
end

@inline function _symbol_size_unaligned(obj::StreamTrade)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + _symbol_offset(obj)))
    aligned_diff::UInt64 = stored_size >> 56
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size - aligned_diff
end

# Member: fill_time::Int64

@inline function fill_time(obj::StreamTrade)::Int64
    return unsafe_load(reinterpret(Ptr{Int64}, obj.buffer + _fill_time_offset(obj)))
end

@inline function fill_time!(obj::StreamTrade, value::Int64)
    unsafe_store!(reinterpret(Ptr{Int64}, obj.buffer + _fill_time_offset(obj)), value)
end

@inline function _fill_time_offset(obj::StreamTrade)::UInt64
    return _symbol_offset(obj) + _symbol_size_aligned(obj)
end

@inline function _fill_time_size_aligned(obj::StreamTrade)::UInt64
    return 8
end

@inline function _fill_time_calc_size_aligned(::Type{StreamTrade}, value::Int64)::UInt64
    return 8
end


# Member: side::TradeSide.T

@inline function side(obj::StreamTrade)::TradeSide.T
    return unsafe_load(reinterpret(Ptr{TradeSide.T}, obj.buffer + _side_offset(obj)))
end

@inline function side!(obj::StreamTrade, value::TradeSide.T)
    unsafe_store!(reinterpret(Ptr{TradeSide.T}, obj.buffer + _side_offset(obj)), value)
end

@inline function _side_offset(obj::StreamTrade)::UInt64
    return _fill_time_offset(obj) + _fill_time_size_aligned(obj)
end

@inline function _side_size_aligned(obj::StreamTrade)::UInt64
    return 8
end

@inline function _side_calc_size_aligned(::Type{StreamTrade}, value::TradeSide.T)::UInt64
    return 8
end


# Member: price::Float64

@inline function price(obj::StreamTrade)::Float64
    return unsafe_load(reinterpret(Ptr{Float64}, obj.buffer + _price_offset(obj)))
end

@inline function price!(obj::StreamTrade, value::Float64)
    unsafe_store!(reinterpret(Ptr{Float64}, obj.buffer + _price_offset(obj)), value)
end

@inline function _price_offset(obj::StreamTrade)::UInt64
    return _side_offset(obj) + _side_size_aligned(obj)
end

@inline function _price_size_aligned(obj::StreamTrade)::UInt64
    return 8
end

@inline function _price_calc_size_aligned(::Type{StreamTrade}, value::Float64)::UInt64
    return 8
end


# Member: price_chg_dir::TickDirection.T

@inline function price_chg_dir(obj::StreamTrade)::TickDirection.T
    return unsafe_load(reinterpret(Ptr{TickDirection.T}, obj.buffer + _price_chg_dir_offset(obj)))
end

@inline function price_chg_dir!(obj::StreamTrade, value::TickDirection.T)
    unsafe_store!(reinterpret(Ptr{TickDirection.T}, obj.buffer + _price_chg_dir_offset(obj)), value)
end

@inline function _price_chg_dir_offset(obj::StreamTrade)::UInt64
    return _price_offset(obj) + _price_size_aligned(obj)
end

@inline function _price_chg_dir_size_aligned(obj::StreamTrade)::UInt64
    return 8
end

@inline function _price_chg_dir_calc_size_aligned(::Type{StreamTrade}, value::TickDirection.T)::UInt64
    return 8
end


# Member: size::Float64

@inline function size(obj::StreamTrade)::Float64
    return unsafe_load(reinterpret(Ptr{Float64}, obj.buffer + _size_offset(obj)))
end

@inline function size!(obj::StreamTrade, value::Float64)
    unsafe_store!(reinterpret(Ptr{Float64}, obj.buffer + _size_offset(obj)), value)
end

@inline function _size_offset(obj::StreamTrade)::UInt64
    return _price_chg_dir_offset(obj) + _price_chg_dir_size_aligned(obj)
end

@inline function _size_size_aligned(obj::StreamTrade)::UInt64
    return 8
end

@inline function _size_calc_size_aligned(::Type{StreamTrade}, value::Float64)::UInt64
    return 8
end


# Member: trade_id::StringView

@inline function trade_id(obj::StreamTrade)::StringView
    ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, obj.buffer + _trade_id_offset(obj))
    unaligned_size::UInt64 = _trade_id_size_unaligned(obj)
    n_bytes::UInt64 = unaligned_size - 8
    count::UInt64 = n_bytes
    return StringView(unsafe_wrap(Vector{UInt8}, ptr + 8, count, own=false))
end

@inline function trade_id!(obj::StreamTrade, value::T) where {T<:AbstractString}
    offset::UInt64 = _trade_id_offset(obj)
    contents_size::UInt64 = length(value) * 1
    unaligned_size::UInt64 = 8 + contents_size
    aligned_size::UInt64 = (unaligned_size + 7) & ~7
    aligned_diff::UInt64 = aligned_size - unaligned_size
    aligned_size_high::UInt64 = aligned_size | (aligned_diff << 56)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), aligned_size_high)
    dest_ptr::Ptr{UInt8} = obj.buffer + offset + 8
    src_ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, pointer(value))
    unsafe_copyto!(dest_ptr, src_ptr, contents_size)
end

@inline function _trade_id_offset(obj::StreamTrade)::UInt64
    return _size_offset(obj) + _size_size_aligned(obj)
end

@inline function _trade_id_size_aligned(obj::StreamTrade)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + _trade_id_offset(obj)))
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size
end

@inline function _trade_id_calc_size_aligned(::Type{StreamTrade}, value::T)::UInt64 where {T<:AbstractString}
    contents_size::UInt64 = length(value) * 1
    unaligned_size::UInt64 = 8 + contents_size
    return (unaligned_size + 7) & ~7
end

@inline function _trade_id_size_unaligned(obj::StreamTrade)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + _trade_id_offset(obj)))
    aligned_diff::UInt64 = stored_size >> 56
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size - aligned_diff
end

# Member: block_trade::Bool

@inline function block_trade(obj::StreamTrade)::Bool
    return unsafe_load(reinterpret(Ptr{Bool}, obj.buffer + _block_trade_offset(obj)))
end

@inline function block_trade!(obj::StreamTrade, value::Bool)
    unsafe_store!(reinterpret(Ptr{Bool}, obj.buffer + _block_trade_offset(obj)), value)
end

@inline function _block_trade_offset(obj::StreamTrade)::UInt64
    return _trade_id_offset(obj) + _trade_id_size_aligned(obj)
end

@inline function _block_trade_size_aligned(obj::StreamTrade)::UInt64
    return 8
end

@inline function _block_trade_calc_size_aligned(::Type{StreamTrade}, value::Bool)::UInt64
    return 8
end


# --------------------------------------------------------------------

@inline function fastbin_calc_binary_size(obj::StreamTrade)::UInt64
    return _block_trade_offset(obj) + _block_trade_size_aligned(obj)
end

@inline function fastbin_calc_binary_size(::Type{StreamTrade},
    symbol::StringView,
    trade_id::StringView
)
    return 72 +
        _symbol_calc_size_aligned(StreamTrade, symbol) +
        _trade_id_calc_size_aligned(StreamTrade, trade_id)
end

"""
Returns the stored (aligned) binary size of the object.
This function should only be called after `fastbin_finalize!(obj)`.
"""
@inline function fastbin_binary_size(obj::StreamTrade)::UInt64
    return unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer))
end

"""
Finalizes the object by writing the binary size to the beginning of its buffer.
After calling this function, the underlying buffer can be used for serialization.
To get the actual buffer size, call `fastbin_binary_size(obj)`.
"""
@inline function fastbin_finalize!(obj::StreamTrade)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer), fastbin_calc_binary_size(obj))
    nothing
end

function show(io::IO, obj::StreamTrade)
    print(io, "[my_models::StreamTrade]")
    print(io, "\n    server_time: ")
    print(io, server_time(obj))
    print(io, "\n    recv_time: ")
    print(io, recv_time(obj))
    print(io, "\n    symbol: ")
    print(io, symbol(obj))
    print(io, "\n    fill_time: ")
    print(io, fill_time(obj))
    print(io, "\n    side: ")
    print(io, side(obj))
    print(io, "\n    price: ")
    print(io, price(obj))
    print(io, "\n    price_chg_dir: ")
    print(io, price_chg_dir(obj))
    print(io, "\n    size: ")
    print(io, size(obj))
    print(io, "\n    trade_id: ")
    print(io, trade_id(obj))
    print(io, "\n    block_trade: ")
    print(io, block_trade(obj))
    println(io)
end
