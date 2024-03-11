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
mutable struct StructVector
    buffer::Ptr{UInt8}
    buffer_size::UInt64
    owns_buffer::Bool

    function StructVector(buffer::Ptr{UInt8}, buffer_size::UInt64, owns_buffer::Bool)
        new(buffer, buffer_size, owns_buffer)
    end

    function StructVector(buffer_size::Integer)
        buffer = reinterpret(Ptr{UInt8}, Base.Libc.malloc(buffer_size))
        new(buffer, buffer_size, true)
    end
end

function Base.finalizer(obj::StructVector)
    if obj.owns_buffer && obj.buffer != C_NULL
        Base.Libc.free(obj.buffer)
        obj.buffer = C_NULL
    end
    nothing
end

@inline function values(obj::StructVector)::Vector{ChildFixed}
    ptr::Ptr{ChildFixed} = reinterpret(Ptr{ChildFixed}, obj.buffer + fastbin_values_offset(obj))
    unaligned_size::UInt64 = fastbin_values_size_unaligned(obj)
    n_bytes::UInt64 = unaligned_size - 8
    count::UInt64 = n_bytes / 16
    return unsafe_wrap(Vector{ChildFixed}, ptr + 8, count, own=false)
end

@inline function values!(obj::StructVector, value::Vector{ChildFixed})
    offset::UInt64 = fastbin_values_offset(obj)
    elements_size::UInt64 = length(value) * 16
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), 8 + elements_size)
    dest_ptr::Ptr{UInt8} = obj.buffer + offset + 8
    src_ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, pointer(value))
    unsafe_copyto!(dest_ptr, src_ptr, elements_size)
end

@inline function fastbin_values_offset(obj::StructVector)::UInt64
    return 8
end

@inline function fastbin_values_size(obj::StructVector)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_values_offset(obj)))
    return stored_size
end

@inline function fastbin_values_size_unaligned(obj::StructVector)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + fastbin_values_offset(obj)))
    return stored_size
end

@inline function count(obj::StructVector)::UInt32
    return unsafe_load(reinterpret(Ptr{UInt32}, obj.buffer + fastbin_count_offset(obj)))
end

@inline function count!(obj::StructVector, value::UInt32)
    unsafe_store!(reinterpret(Ptr{UInt32}, obj.buffer + fastbin_count_offset(obj)), value)
end

@inline function fastbin_count_offset(obj::StructVector)::UInt64
    return fastbin_values_offset(obj) + fastbin_values_size(obj)
end

@inline function fastbin_count_size(obj::StructVector)::UInt64
    return 8
end

@inline function fastbin_compute_binary_size(obj::StructVector)::UInt64
    return fastbin_count_offset(obj) + fastbin_count_size(obj)
end

@inline function fastbin_finalize!(obj::StructVector)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer), fastbin_compute_binary_size(obj))
    nothing
end

@inline function fastbin_binary_size(obj::StructVector)::UInt64
    return unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer))
end

function show(io::IO, obj::StructVector)
    print(io, "[my_models::StructVector]")
    print(io, "\n    values: ")
    show(io, values(obj))
    print(io, "\n    count: ")
    print(io, count(obj))
    println(io)
end
