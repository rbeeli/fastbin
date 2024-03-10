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
mutable struct Parent
    buffer::Ptr{UInt8}
    buffer_size::UInt64
    owns_buffer::Bool

    function Parent(buffer::Ptr{UInt8}, buffer_size::UInt64, owns_buffer::Bool)
        new(buffer, buffer_size, owns_buffer)
    end
end

function Base.finalizer(obj::Parent)
    if obj.owns_buffer && obj.buffer != C_NULL
        Base.Libc.free(obj.buffer)
        obj.buffer = C_NULL
    end
    nothing
end

@inline function field1(obj::Parent)::Int32
    return unsafe_load(reinterpret(Ptr{Int32}, obj.buffer + fastbin_field1_offset(obj)))
end

@inline function field1!(obj::Parent, value::Int32)
    unsafe_store!(reinterpret(Ptr{Int32}, obj.buffer + fastbin_field1_offset(obj)), value)
end

@inline function fastbin_field1_offset(obj::Parent)::UInt64
    return 8
end

@inline function fastbin_field1_size(obj::Parent)::UInt64
    return 8
end

@inline function field2(obj::Parent)::StringView
    ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, obj.buffer + fastbin_field2_offset(obj))
    unaligned_size::UInt64 = fastbin_field2_size_unaligned(obj)
    n_bytes::UInt64 = unaligned_size - 8
    count::UInt64 = n_bytes
    return StringView(unsafe_wrap(Vector{UInt8}, ptr + 8, count, own=false))
end

@inline function field2!(obj::Parent, value::T) where {T<:AbstractString}
    offset::UInt64 = fastbin_field2_offset(obj)
    unaligned_size::UInt64 = 8 + length(value) * sizeof(UInt8)
    aligned_size::UInt64 = (unaligned_size + 7) & ~7
    aligned_diff::UInt64 = aligned_size - unaligned_size
    aligned_size_high::UInt64 = aligned_size | (aligned_diff << 56)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), aligned_size_high)
    el_ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, obj.buffer + offset + 8)
    unsafe_copyto!(el_ptr, pointer(value), length(value))
end

@inline function fastbin_field2_offset(obj::Parent)::UInt64
    return fastbin_field1_offset(obj) + fastbin_field1_size(obj)
end

@inline function fastbin_field2_size(obj::Parent)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_field2_offset(obj)))
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size
end

@inline function fastbin_field2_size_unaligned(obj::Parent)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_field2_offset(obj)))
    aligned_diff::UInt64 = stored_size >> 56
    aligned_size::UInt64 = stored_size & 0x00FFFFFFFFFFFFFF
    return aligned_size - aligned_diff
end

@inline function child1(obj::Parent)::ChildVar
    ptr::Ptr{UInt8} = obj.buffer + fastbin_child1_offset(obj)
    return ChildVar(ptr, fastbin_child1_size(obj), false)
end

@inline function child1!(obj::Parent, value::ChildVar)
    @assert fastbin_binary_size(value) > 0 "Cannot set struct value, struct ChildVar not finalized, call fastbin_finalize!() on struct after creation."
    offset::UInt64 = fastbin_child1_offset(obj)
    size::UInt64 = fastbin_binary_size(value)
    unsafe_copyto!(obj.buffer + offset, value.buffer, size)
end

@inline function fastbin_child1_offset(obj::Parent)::UInt64
    return fastbin_field2_offset(obj) + fastbin_field2_size(obj)
end

@inline function fastbin_child1_size(obj::Parent)::UInt64
    return unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_child1_offset(obj)))
end

@inline function child2(obj::Parent)::ChildFixed
    ptr::Ptr{UInt8} = obj.buffer + fastbin_child2_offset(obj)
    return ChildFixed(ptr, fastbin_child2_size(obj), false)
end

@inline function child2!(obj::Parent, value::ChildFixed)
    @assert fastbin_binary_size(value) > 0 "Cannot set struct value, struct ChildFixed not finalized, call fastbin_finalize!() on struct after creation."
    offset::UInt64 = fastbin_child2_offset(obj)
    size::UInt64 = fastbin_binary_size(value)
    unsafe_copyto!(obj.buffer + offset, value.buffer, size)
end

@inline function fastbin_child2_offset(obj::Parent)::UInt64
    return fastbin_child1_offset(obj) + fastbin_child1_size(obj)
end

@inline function fastbin_child2_size(obj::Parent)::UInt64
    return 16
end

@inline function fastbin_compute_binary_size(obj::Parent)::UInt64
    return fastbin_child2_offset(obj) + fastbin_child2_size(obj)
end

@inline function fastbin_finalize!(obj::Parent)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer), fastbin_compute_binary_size(obj))
    nothing
end

@inline function fastbin_binary_size(obj::Parent)::UInt64
    return unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer))
end

function show(io::IO, obj::Parent)
    print(io, "[my_models::Parent]")
    print(io, "\n    field1: ")
    print(io, field1(obj))
    print(io, "\n    field2: ")
    print(io, field2(obj))
    print(io, "\n    child1: ")
    show(io, child1(obj))
    print(io, "\n    child2: ")
    show(io, child2(obj))
    println(io)
end
