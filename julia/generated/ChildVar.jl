import Base.show
import Base.finalizer
using StringViews

"""
Binary serializable data container.

This container has variable size.

Setter methods from the first variable-sized field onwards
MUST be called in order.

The `finalize!()` method MUST be called after all setter methods have been called.

It is the responsibility of the caller to ensure that the buffer is
large enough to hold the serialized data.
"""
mutable struct ChildVar
    buffer::Ptr{UInt8}
    buffer_size::UInt64
    owns_buffer::Bool

    function ChildVar(buffer::Ptr{UInt8}, buffer_size::UInt64, owns_buffer::Bool)
        new(buffer, buffer_size, owns_buffer)
    end
end

function Base.finalizer(obj::ChildVar)
    if obj.owns_buffer && obj.buffer != C_NULL
        Base.Libc.free(obj.buffer)
        obj.buffer = C_NULL
    end
    nothing
end

@inline function field1(obj::ChildVar)::Int32
    return unsafe_load(reinterpret(Ptr{Int32}, obj.buffer + fastbin_field1_offset(obj)))
end

@inline function field1!(obj::ChildVar, value::Int32)
    unsafe_store!(reinterpret(Ptr{Int32}, obj.buffer + fastbin_field1_offset(obj)), value)
end

@inline function fastbin_field1_offset(obj::ChildVar)::UInt64
    return 8
end

@inline function fastbin_field1_size(obj::ChildVar)::UInt64
    return 8
end

@inline function field2(obj::ChildVar)::StringView
    ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, obj.buffer + fastbin_field2_offset(obj))
    unaligned_size::UInt64 = fastbin_field2_size_unaligned(obj)
    n_bytes::UInt64 = unaligned_size - 8
    count::UInt64 = n_bytes
    return StringView(unsafe_wrap(Vector{UInt8}, ptr + 8, count, own=false))
end

@inline function field2!(obj::ChildVar, value::T) where {T<:AbstractString}
    offset::UInt64 = fastbin_field2_offset(obj)
    unaligned_size::UInt64 = 8 + length(value) * sizeof(UInt8)
    aligned_size::UInt64 = (unaligned_size + 7) & ~7
    aligned_diff::UInt64 = aligned_size - unaligned_size
    aligned_size_high::UInt64 = aligned_size | (aligned_diff << 56)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), aligned_size_high)
    el_ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, obj.buffer + offset + 8)
    unsafe_copyto!(el_ptr, pointer(value), length(value))
end

@inline function fastbin_field2_offset(obj::ChildVar)::UInt64
    return fastbin_field1_offset(obj) + fastbin_field1_size(obj)
end

@inline function fastbin_field2_size(obj::ChildVar)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_field2_offset(obj)))
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size
end

@inline function fastbin_field2_size_unaligned(obj::ChildVar)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_field2_offset(obj)))
    aligned_diff::UInt64 = stored_size >> 56
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size - aligned_diff
end

@inline function fastbin_compute_binary_size(obj::ChildVar)::UInt64
    return fastbin_field2_offset(obj) + fastbin_field2_size(obj)
end

@inline function fastbin_finalize!(obj::ChildVar)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer), fastbin_compute_binary_size(obj))
    nothing
end

@inline function fastbin_binary_size(obj::ChildVar)::UInt64
    return unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer))
end

function show(io::IO, obj::ChildVar)
    print(io, "[my_models::ChildVar]")
    print(io, "\n    field1: ")
    print(io, field1(obj))
    print(io, "\n    field2: ")
    print(io, field2(obj))
    println(io)
end
