import Base.show
import Base.finalizer

"""
Binary serializable data container.

This container has variable size.

Setter methods from the first variable-sized field onwards
MUST be called in order.

The `finalize!()` method MUST be called after all setter methods have been called.

It is the responsibility of the caller to ensure that the buffer is
large enough to hold the serialized data.
"""
mutable struct UInt32Vector
    buffer::Ptr{UInt8}
    buffer_size::UInt64
    owns_buffer::Bool

    function UInt32Vector(buffer::Ptr{UInt8}, buffer_size::UInt64, owns_buffer::Bool)
        new(buffer, buffer_size, owns_buffer)
    end
end

function Base.finalizer(obj::UInt32Vector)
    if obj.owns_buffer && obj.buffer != C_NULL
        Base.Libc.free(obj.buffer)
        obj.buffer = C_NULL
    end
    nothing
end

@inline function values(obj::UInt32Vector)::Vector{UInt32}
    ptr::Ptr{UInt32} = reinterpret(Ptr{UInt32}, obj.buffer + fastbin_values_offset(obj))
    unaligned_size::UInt64 = fastbin_values_size_unaligned(obj)
    n_bytes::UInt64 = unaligned_size - 8
    count::UInt64 = n_bytes >> 2
    return unsafe_wrap(Vector{UInt32}, ptr + 8, count, own=false)
end

@inline function values!(obj::UInt32Vector, value::Vector{UInt32})
    offset::UInt64 = fastbin_values_offset(obj)
    unaligned_size::UInt64 = 8 + length(value) * sizeof(UInt32)
    aligned_size::UInt64 = (unaligned_size + 7) & ~7
    aligned_diff::UInt64 = aligned_size - unaligned_size
    aligned_size_high::UInt64 = aligned_size | (aligned_diff << 56)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), aligned_size_high)
    el_ptr::Ptr{UInt32} = reinterpret(Ptr{UInt32}, obj.buffer + offset + 8)
    unsafe_copyto!(el_ptr, pointer(value), length(value))
end

@inline function fastbin_values_offset(obj::UInt32Vector)::UInt64
    return 8
end

@inline function fastbin_values_size(obj::UInt32Vector)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_values_offset(obj)))
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size
end

@inline function fastbin_values_size_unaligned(obj::UInt32Vector)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_values_offset(obj)))
    aligned_diff::UInt64 = stored_size >> 56
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size - aligned_diff
end

@inline function count(obj::UInt32Vector)::UInt32
    return unsafe_load(reinterpret(Ptr{UInt32}, obj.buffer + fastbin_count_offset(obj)))
end

@inline function count!(obj::UInt32Vector, value::UInt32)
    unsafe_store!(reinterpret(Ptr{UInt32}, obj.buffer + fastbin_count_offset(obj)), value)
end

@inline function fastbin_count_offset(obj::UInt32Vector)::UInt64
    return fastbin_values_offset(obj) + fastbin_values_size(obj)
end

@inline function fastbin_count_size(obj::UInt32Vector)::UInt64
    return 8
end

@inline function fastbin_compute_binary_size(obj::UInt32Vector)::UInt64
    return fastbin_count_offset(obj) + fastbin_count_size(obj)
end

@inline function fastbin_finalize!(obj::UInt32Vector)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer), fastbin_compute_binary_size(obj))
    nothing
end

@inline function fastbin_binary_size(obj::UInt32Vector)::UInt64
    return unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer))
end

function show(io::IO, obj::UInt32Vector)
    print(io, "[my_models::UInt32Vector]")
    print(io, "\n    values: ")
    show(io, values(obj))
    print(io, "\n    count: ")
    print(io, count(obj))
    println(io)
end
