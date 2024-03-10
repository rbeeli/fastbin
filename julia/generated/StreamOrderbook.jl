import Base.show
import Base.finalizer
using StringViews

"""
https://bybit-exchange.github.io/docs/v5/websocket/public/orderbook

------------------------------------------------------------

Binary serializable data container.

This container has variable size.

Setter methods from the first variable-sized field onwards
MUST be called in order.

The `finalize!()` method MUST be called after all setter methods have been called.

It is the responsibility of the caller to ensure that the buffer is
large enough to hold the serialized data.
"""
mutable struct StreamOrderbook
    buffer::Ptr{UInt8}
    buffer_size::UInt64
    owns_buffer::Bool

    function StreamOrderbook(buffer::Ptr{UInt8}, buffer_size::UInt64, owns_buffer::Bool)
        new(buffer, buffer_size, owns_buffer)
    end
end

function Base.finalizer(obj::StreamOrderbook)
    if obj.owns_buffer && obj.buffer != C_NULL
        Base.Libc.free(obj.buffer)
        obj.buffer = C_NULL
    end
    nothing
end

@inline function server_time(obj::StreamOrderbook)::Int64
    return unsafe_load(reinterpret(Ptr{Int64}, obj.buffer + fastbin_server_time_offset(obj)))
end

@inline function server_time!(obj::StreamOrderbook, value::Int64)
    unsafe_store!(reinterpret(Ptr{Int64}, obj.buffer + fastbin_server_time_offset(obj)), value)
end

@inline function fastbin_server_time_offset(obj::StreamOrderbook)::UInt64
    return 8
end

@inline function fastbin_server_time_size(obj::StreamOrderbook)::UInt64
    return 8
end

@inline function recv_time(obj::StreamOrderbook)::Int64
    return unsafe_load(reinterpret(Ptr{Int64}, obj.buffer + fastbin_recv_time_offset(obj)))
end

@inline function recv_time!(obj::StreamOrderbook, value::Int64)
    unsafe_store!(reinterpret(Ptr{Int64}, obj.buffer + fastbin_recv_time_offset(obj)), value)
end

@inline function fastbin_recv_time_offset(obj::StreamOrderbook)::UInt64
    return fastbin_server_time_offset(obj) + fastbin_server_time_size(obj)
end

@inline function fastbin_recv_time_size(obj::StreamOrderbook)::UInt64
    return 8
end

@inline function cts(obj::StreamOrderbook)::Int64
    return unsafe_load(reinterpret(Ptr{Int64}, obj.buffer + fastbin_cts_offset(obj)))
end

@inline function cts!(obj::StreamOrderbook, value::Int64)
    unsafe_store!(reinterpret(Ptr{Int64}, obj.buffer + fastbin_cts_offset(obj)), value)
end

@inline function fastbin_cts_offset(obj::StreamOrderbook)::UInt64
    return fastbin_recv_time_offset(obj) + fastbin_recv_time_size(obj)
end

@inline function fastbin_cts_size(obj::StreamOrderbook)::UInt64
    return 8
end

@inline function type(obj::StreamOrderbook)::OrderbookType.T
    return unsafe_load(reinterpret(Ptr{OrderbookType.T}, obj.buffer + fastbin_type_offset(obj)))
end

@inline function type!(obj::StreamOrderbook, value::OrderbookType.T)
    unsafe_store!(reinterpret(Ptr{OrderbookType.T}, obj.buffer + fastbin_type_offset(obj)), value)
end

@inline function fastbin_type_offset(obj::StreamOrderbook)::UInt64
    return fastbin_cts_offset(obj) + fastbin_cts_size(obj)
end

@inline function fastbin_type_size(obj::StreamOrderbook)::UInt64
    return 8
end

@inline function depth(obj::StreamOrderbook)::UInt16
    return unsafe_load(reinterpret(Ptr{UInt16}, obj.buffer + fastbin_depth_offset(obj)))
end

@inline function depth!(obj::StreamOrderbook, value::UInt16)
    unsafe_store!(reinterpret(Ptr{UInt16}, obj.buffer + fastbin_depth_offset(obj)), value)
end

@inline function fastbin_depth_offset(obj::StreamOrderbook)::UInt64
    return fastbin_type_offset(obj) + fastbin_type_size(obj)
end

@inline function fastbin_depth_size(obj::StreamOrderbook)::UInt64
    return 8
end

@inline function symbol(obj::StreamOrderbook)::StringView
    ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, obj.buffer + fastbin_symbol_offset(obj))
    unaligned_size::UInt64 = fastbin_symbol_size_unaligned(obj)
    n_bytes::UInt64 = unaligned_size - 8
    count::UInt64 = n_bytes
    return StringView(unsafe_wrap(Vector{UInt8}, ptr + 8, count, own=false))
end

@inline function symbol!(obj::StreamOrderbook, value::T) where {T<:AbstractString}
    offset::UInt64 = fastbin_symbol_offset(obj)
    unaligned_size::UInt64 = 8 + length(value) * sizeof(UInt8)
    aligned_size::UInt64 = (unaligned_size + 7) & ~7
    aligned_diff::UInt64 = aligned_size - unaligned_size
    aligned_size_high::UInt64 = aligned_size | (aligned_diff << 56)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), aligned_size_high)
    el_ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, obj.buffer + offset + 8)
    unsafe_copyto!(el_ptr, pointer(value), length(value))
end

@inline function fastbin_symbol_offset(obj::StreamOrderbook)::UInt64
    return fastbin_depth_offset(obj) + fastbin_depth_size(obj)
end

@inline function fastbin_symbol_size(obj::StreamOrderbook)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_symbol_offset(obj)))
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size
end

@inline function fastbin_symbol_size_unaligned(obj::StreamOrderbook)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_symbol_offset(obj)))
    aligned_diff::UInt64 = stored_size >> 56
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size - aligned_diff
end

@inline function update_id(obj::StreamOrderbook)::UInt64
    return unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_update_id_offset(obj)))
end

@inline function update_id!(obj::StreamOrderbook, value::UInt64)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_update_id_offset(obj)), value)
end

@inline function fastbin_update_id_offset(obj::StreamOrderbook)::UInt64
    return fastbin_symbol_offset(obj) + fastbin_symbol_size(obj)
end

@inline function fastbin_update_id_size(obj::StreamOrderbook)::UInt64
    return 8
end

@inline function seq_num(obj::StreamOrderbook)::UInt64
    return unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_seq_num_offset(obj)))
end

@inline function seq_num!(obj::StreamOrderbook, value::UInt64)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_seq_num_offset(obj)), value)
end

@inline function fastbin_seq_num_offset(obj::StreamOrderbook)::UInt64
    return fastbin_update_id_offset(obj) + fastbin_update_id_size(obj)
end

@inline function fastbin_seq_num_size(obj::StreamOrderbook)::UInt64
    return 8
end

@inline function bid_prices(obj::StreamOrderbook)::Vector{Float64}
    ptr::Ptr{Float64} = reinterpret(Ptr{Float64}, obj.buffer + fastbin_bid_prices_offset(obj))
    unaligned_size::UInt64 = fastbin_bid_prices_size_unaligned(obj)
    n_bytes::UInt64 = unaligned_size - 8
    count::UInt64 = n_bytes >> 3
    return unsafe_wrap(Vector{Float64}, ptr + 8, count, own=false)
end

@inline function bid_prices!(obj::StreamOrderbook, value::Vector{Float64})
    offset::UInt64 = fastbin_bid_prices_offset(obj)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), 8 + length(value) * sizeof(Float64))
    el_ptr::Ptr{Float64} = reinterpret(Ptr{Float64}, obj.buffer + offset + 8)
    unsafe_copyto!(el_ptr, pointer(value), length(value))
end

@inline function fastbin_bid_prices_offset(obj::StreamOrderbook)::UInt64
    return fastbin_seq_num_offset(obj) + fastbin_seq_num_size(obj)
end

@inline function fastbin_bid_prices_size(obj::StreamOrderbook)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_bid_prices_offset(obj)))
    return stored_size
end

@inline function fastbin_bid_prices_size_unaligned(obj::StreamOrderbook)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_bid_prices_offset(obj)))
    return stored_size
end

@inline function bid_quantities(obj::StreamOrderbook)::Vector{Float64}
    ptr::Ptr{Float64} = reinterpret(Ptr{Float64}, obj.buffer + fastbin_bid_quantities_offset(obj))
    unaligned_size::UInt64 = fastbin_bid_quantities_size_unaligned(obj)
    n_bytes::UInt64 = unaligned_size - 8
    count::UInt64 = n_bytes >> 3
    return unsafe_wrap(Vector{Float64}, ptr + 8, count, own=false)
end

@inline function bid_quantities!(obj::StreamOrderbook, value::Vector{Float64})
    offset::UInt64 = fastbin_bid_quantities_offset(obj)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), 8 + length(value) * sizeof(Float64))
    el_ptr::Ptr{Float64} = reinterpret(Ptr{Float64}, obj.buffer + offset + 8)
    unsafe_copyto!(el_ptr, pointer(value), length(value))
end

@inline function fastbin_bid_quantities_offset(obj::StreamOrderbook)::UInt64
    return fastbin_bid_prices_offset(obj) + fastbin_bid_prices_size(obj)
end

@inline function fastbin_bid_quantities_size(obj::StreamOrderbook)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_bid_quantities_offset(obj)))
    return stored_size
end

@inline function fastbin_bid_quantities_size_unaligned(obj::StreamOrderbook)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_bid_quantities_offset(obj)))
    return stored_size
end

@inline function ask_prices(obj::StreamOrderbook)::Vector{Float64}
    ptr::Ptr{Float64} = reinterpret(Ptr{Float64}, obj.buffer + fastbin_ask_prices_offset(obj))
    unaligned_size::UInt64 = fastbin_ask_prices_size_unaligned(obj)
    n_bytes::UInt64 = unaligned_size - 8
    count::UInt64 = n_bytes >> 3
    return unsafe_wrap(Vector{Float64}, ptr + 8, count, own=false)
end

@inline function ask_prices!(obj::StreamOrderbook, value::Vector{Float64})
    offset::UInt64 = fastbin_ask_prices_offset(obj)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), 8 + length(value) * sizeof(Float64))
    el_ptr::Ptr{Float64} = reinterpret(Ptr{Float64}, obj.buffer + offset + 8)
    unsafe_copyto!(el_ptr, pointer(value), length(value))
end

@inline function fastbin_ask_prices_offset(obj::StreamOrderbook)::UInt64
    return fastbin_bid_quantities_offset(obj) + fastbin_bid_quantities_size(obj)
end

@inline function fastbin_ask_prices_size(obj::StreamOrderbook)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_ask_prices_offset(obj)))
    return stored_size
end

@inline function fastbin_ask_prices_size_unaligned(obj::StreamOrderbook)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_ask_prices_offset(obj)))
    return stored_size
end

@inline function ask_quantities(obj::StreamOrderbook)::Vector{Float64}
    ptr::Ptr{Float64} = reinterpret(Ptr{Float64}, obj.buffer + fastbin_ask_quantities_offset(obj))
    unaligned_size::UInt64 = fastbin_ask_quantities_size_unaligned(obj)
    n_bytes::UInt64 = unaligned_size - 8
    count::UInt64 = n_bytes >> 3
    return unsafe_wrap(Vector{Float64}, ptr + 8, count, own=false)
end

@inline function ask_quantities!(obj::StreamOrderbook, value::Vector{Float64})
    offset::UInt64 = fastbin_ask_quantities_offset(obj)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), 8 + length(value) * sizeof(Float64))
    el_ptr::Ptr{Float64} = reinterpret(Ptr{Float64}, obj.buffer + offset + 8)
    unsafe_copyto!(el_ptr, pointer(value), length(value))
end

@inline function fastbin_ask_quantities_offset(obj::StreamOrderbook)::UInt64
    return fastbin_ask_prices_offset(obj) + fastbin_ask_prices_size(obj)
end

@inline function fastbin_ask_quantities_size(obj::StreamOrderbook)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_ask_quantities_offset(obj)))
    return stored_size
end

@inline function fastbin_ask_quantities_size_unaligned(obj::StreamOrderbook)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_ask_quantities_offset(obj)))
    return stored_size
end

@inline function fastbin_compute_binary_size(obj::StreamOrderbook)::UInt64
    return fastbin_ask_quantities_offset(obj) + fastbin_ask_quantities_size(obj)
end

@inline function fastbin_finalize!(obj::StreamOrderbook)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer), fastbin_compute_binary_size(obj))
    nothing
end

@inline function fastbin_binary_size(obj::StreamOrderbook)::UInt64
    return unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer))
end

function show(io::IO, obj::StreamOrderbook)
    print(io, "[my_models::StreamOrderbook]")
    print(io, "\n    server_time: ")
    print(io, server_time(obj))
    print(io, "\n    recv_time: ")
    print(io, recv_time(obj))
    print(io, "\n    cts: ")
    print(io, cts(obj))
    print(io, "\n    type: ")
    print(io, type(obj))
    print(io, "\n    depth: ")
    print(io, depth(obj))
    print(io, "\n    symbol: ")
    print(io, symbol(obj))
    print(io, "\n    update_id: ")
    print(io, update_id(obj))
    print(io, "\n    seq_num: ")
    print(io, seq_num(obj))
    print(io, "\n    bid_prices: ")
    show(io, bid_prices(obj))
    print(io, "\n    bid_quantities: ")
    show(io, bid_quantities(obj))
    print(io, "\n    ask_prices: ")
    show(io, ask_prices(obj))
    print(io, "\n    ask_quantities: ")
    show(io, ask_quantities(obj))
    println(io)
end
