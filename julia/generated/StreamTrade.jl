import Base.show
import Base.finalizer
using StringViews

"""
https://bybit-exchange.github.io/docs/v5/websocket/public/trade

------------------------------------------------------------

Binary serializable data container.

This container has variable size.

Setter methods from the first variable-sized field onwards
MUST be called in order.

The `finalize!()` method MUST be called after all setter methods have been called.

It is the responsibility of the caller to ensure that the buffer is
large enough to hold the serialized data.
"""
mutable struct StreamTrade
    buffer::Ptr{UInt8}
    buffer_size::UInt64
    owns_buffer::Bool

    function StreamTrade(buffer::Ptr{UInt8}, buffer_size::UInt64, owns_buffer::Bool)
        new(buffer, buffer_size, owns_buffer)
    end
end

function Base.finalizer(obj::StreamTrade)
    if obj.owns_buffer && obj.buffer != C_NULL
        Base.Libc.free(obj.buffer)
        obj.buffer = C_NULL
    end
    nothing
end

@inline function server_time(obj::StreamTrade)::Int64
    return unsafe_load(reinterpret(Ptr{Int64}, obj.buffer + fastbin_server_time_offset(obj)))
end

@inline function server_time!(obj::StreamTrade, value::Int64)
    unsafe_store!(reinterpret(Ptr{Int64}, obj.buffer + fastbin_server_time_offset(obj)), value)
end

@inline function fastbin_server_time_offset(obj::StreamTrade)::UInt64
    return 8
end

@inline function fastbin_server_time_size(obj::StreamTrade)::UInt64
    return 8
end

@inline function recv_time(obj::StreamTrade)::Int64
    return unsafe_load(reinterpret(Ptr{Int64}, obj.buffer + fastbin_recv_time_offset(obj)))
end

@inline function recv_time!(obj::StreamTrade, value::Int64)
    unsafe_store!(reinterpret(Ptr{Int64}, obj.buffer + fastbin_recv_time_offset(obj)), value)
end

@inline function fastbin_recv_time_offset(obj::StreamTrade)::UInt64
    return fastbin_server_time_offset(obj) + fastbin_server_time_size(obj)
end

@inline function fastbin_recv_time_size(obj::StreamTrade)::UInt64
    return 8
end

@inline function symbol(obj::StreamTrade)::StringView
    ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, obj.buffer + fastbin_symbol_offset(obj))
    unaligned_size::UInt64 = fastbin_symbol_size_unaligned(obj)
    n_bytes::UInt64 = unaligned_size - 8
    count::UInt64 = n_bytes
    return StringView(unsafe_wrap(Vector{UInt8}, ptr + 8, count, own=false))
end

@inline function symbol!(obj::StreamTrade, value::T) where {T<:AbstractString}
    offset::UInt64 = fastbin_symbol_offset(obj)
    unaligned_size::UInt64 = 8 + length(value) * sizeof(UInt8)
    aligned_size::UInt64 = (unaligned_size + 7) & ~7
    aligned_diff::UInt64 = aligned_size - unaligned_size
    aligned_size_high::UInt64 = aligned_size | (aligned_diff << 56)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), aligned_size_high)
    el_ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, obj.buffer + offset + 8)
    unsafe_copyto!(el_ptr, pointer(value), length(value))
end

@inline function fastbin_symbol_offset(obj::StreamTrade)::UInt64
    return fastbin_recv_time_offset(obj) + fastbin_recv_time_size(obj)
end

@inline function fastbin_symbol_size(obj::StreamTrade)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_symbol_offset(obj)))
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size
end

@inline function fastbin_symbol_size_unaligned(obj::StreamTrade)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_symbol_offset(obj)))
    aligned_diff::UInt64 = stored_size >> 56
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size - aligned_diff
end

@inline function fill_time(obj::StreamTrade)::Int64
    return unsafe_load(reinterpret(Ptr{Int64}, obj.buffer + fastbin_fill_time_offset(obj)))
end

@inline function fill_time!(obj::StreamTrade, value::Int64)
    unsafe_store!(reinterpret(Ptr{Int64}, obj.buffer + fastbin_fill_time_offset(obj)), value)
end

@inline function fastbin_fill_time_offset(obj::StreamTrade)::UInt64
    return fastbin_symbol_offset(obj) + fastbin_symbol_size(obj)
end

@inline function fastbin_fill_time_size(obj::StreamTrade)::UInt64
    return 8
end

@inline function side(obj::StreamTrade)::TradeSide.T
    return unsafe_load(reinterpret(Ptr{TradeSide.T}, obj.buffer + fastbin_side_offset(obj)))
end

@inline function side!(obj::StreamTrade, value::TradeSide.T)
    unsafe_store!(reinterpret(Ptr{TradeSide.T}, obj.buffer + fastbin_side_offset(obj)), value)
end

@inline function fastbin_side_offset(obj::StreamTrade)::UInt64
    return fastbin_fill_time_offset(obj) + fastbin_fill_time_size(obj)
end

@inline function fastbin_side_size(obj::StreamTrade)::UInt64
    return 8
end

@inline function price(obj::StreamTrade)::Float64
    return unsafe_load(reinterpret(Ptr{Float64}, obj.buffer + fastbin_price_offset(obj)))
end

@inline function price!(obj::StreamTrade, value::Float64)
    unsafe_store!(reinterpret(Ptr{Float64}, obj.buffer + fastbin_price_offset(obj)), value)
end

@inline function fastbin_price_offset(obj::StreamTrade)::UInt64
    return fastbin_side_offset(obj) + fastbin_side_size(obj)
end

@inline function fastbin_price_size(obj::StreamTrade)::UInt64
    return 8
end

@inline function price_chg_dir(obj::StreamTrade)::TickDirection.T
    return unsafe_load(reinterpret(Ptr{TickDirection.T}, obj.buffer + fastbin_price_chg_dir_offset(obj)))
end

@inline function price_chg_dir!(obj::StreamTrade, value::TickDirection.T)
    unsafe_store!(reinterpret(Ptr{TickDirection.T}, obj.buffer + fastbin_price_chg_dir_offset(obj)), value)
end

@inline function fastbin_price_chg_dir_offset(obj::StreamTrade)::UInt64
    return fastbin_price_offset(obj) + fastbin_price_size(obj)
end

@inline function fastbin_price_chg_dir_size(obj::StreamTrade)::UInt64
    return 8
end

@inline function size(obj::StreamTrade)::Float64
    return unsafe_load(reinterpret(Ptr{Float64}, obj.buffer + fastbin_size_offset(obj)))
end

@inline function size!(obj::StreamTrade, value::Float64)
    unsafe_store!(reinterpret(Ptr{Float64}, obj.buffer + fastbin_size_offset(obj)), value)
end

@inline function fastbin_size_offset(obj::StreamTrade)::UInt64
    return fastbin_price_chg_dir_offset(obj) + fastbin_price_chg_dir_size(obj)
end

@inline function fastbin_size_size(obj::StreamTrade)::UInt64
    return 8
end

@inline function trade_id(obj::StreamTrade)::StringView
    ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, obj.buffer + fastbin_trade_id_offset(obj))
    unaligned_size::UInt64 = fastbin_trade_id_size_unaligned(obj)
    n_bytes::UInt64 = unaligned_size - 8
    count::UInt64 = n_bytes
    return StringView(unsafe_wrap(Vector{UInt8}, ptr + 8, count, own=false))
end

@inline function trade_id!(obj::StreamTrade, value::T) where {T<:AbstractString}
    offset::UInt64 = fastbin_trade_id_offset(obj)
    unaligned_size::UInt64 = 8 + length(value) * sizeof(UInt8)
    aligned_size::UInt64 = (unaligned_size + 7) & ~7
    aligned_diff::UInt64 = aligned_size - unaligned_size
    aligned_size_high::UInt64 = aligned_size | (aligned_diff << 56)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), aligned_size_high)
    el_ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, obj.buffer + offset + 8)
    unsafe_copyto!(el_ptr, pointer(value), length(value))
end

@inline function fastbin_trade_id_offset(obj::StreamTrade)::UInt64
    return fastbin_size_offset(obj) + fastbin_size_size(obj)
end

@inline function fastbin_trade_id_size(obj::StreamTrade)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_trade_id_offset(obj)))
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size
end

@inline function fastbin_trade_id_size_unaligned(obj::StreamTrade)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_trade_id_offset(obj)))
    aligned_diff::UInt64 = stored_size >> 56
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size - aligned_diff
end

@inline function block_trade(obj::StreamTrade)::Bool
    return unsafe_load(reinterpret(Ptr{Bool}, obj.buffer + fastbin_block_trade_offset(obj)))
end

@inline function block_trade!(obj::StreamTrade, value::Bool)
    unsafe_store!(reinterpret(Ptr{Bool}, obj.buffer + fastbin_block_trade_offset(obj)), value)
end

@inline function fastbin_block_trade_offset(obj::StreamTrade)::UInt64
    return fastbin_trade_id_offset(obj) + fastbin_trade_id_size(obj)
end

@inline function fastbin_block_trade_size(obj::StreamTrade)::UInt64
    return 8
end

@inline function fastbin_compute_binary_size(obj::StreamTrade)::UInt64
    return fastbin_block_trade_offset(obj) + fastbin_block_trade_size(obj)
end

@inline function fastbin_finalize!(obj::StreamTrade)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer), fastbin_compute_binary_size(obj))
    nothing
end

@inline function fastbin_binary_size(obj::StreamTrade)::UInt64
    return unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer))
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
