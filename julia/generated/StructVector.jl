import Base.show
import Base.finalizer

"""
Binary serializable data container generated by `fastbin`.

This container has variable size.
All setter methods starting from the first variable-sized member and afterwards MUST be called in order.

Fields in order
===============
- `values::Vector{ChildFixed}` (variable)
- `count::UInt32`          (fixed)

The `fastbin_finalize!()` method MUST be called after all setter methods have been called.

It is the responsibility of the caller to ensure that the buffer is
large enough to hold all data.
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

# Field: values::Vector{ChildFixed}

@inline function values(obj::StructVector)::Vector{ChildFixed}
    ptr::Ptr{ChildFixed} = reinterpret(Ptr{ChildFixed}, obj.buffer + _values_offset(obj))
    unaligned_size::UInt64 = _values_size_unaligned(obj)
    n_bytes::UInt64 = unaligned_size - 8
    count::UInt64 = n_bytes / 16
    return unsafe_wrap(Vector{ChildFixed}, ptr + 8, count, own=false)
end

@inline function values!(obj::StructVector, value::Vector{ChildFixed})
    offset::UInt64 = _values_offset(obj)
    contents_size::UInt64 = length(value) * 16
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer + offset), 8 + contents_size)
    dest_ptr::Ptr{UInt8} = obj.buffer + offset + 8
    src_ptr::Ptr{UInt8} = reinterpret(Ptr{UInt8}, pointer(value))
    unsafe_copyto!(dest_ptr, src_ptr, contents_size)
end

@inline function _values_offset(obj::StructVector)::UInt64
    return 8
end

@inline function _values_size_aligned(obj::StructVector)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + _values_offset(obj)))
    return stored_size
end

@inline function _values_calc_size_aligned(::Type{StructVector}, value::Vector{ChildFixed})::UInt64
    contents_size::UInt64 = length(value) * 16
    return 8 + contents_size
end

@inline function _values_size_unaligned(obj::StructVector)::UInt64
    stored_size::UInt64 = unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer + _values_offset(obj)))
    return stored_size
end

# Field: count::UInt32

@inline function count(obj::StructVector)::UInt32
    return unsafe_load(reinterpret(Ptr{UInt32}, obj.buffer + _count_offset(obj)))
end

@inline function count!(obj::StructVector, value::UInt32)
    unsafe_store!(reinterpret(Ptr{UInt32}, obj.buffer + _count_offset(obj)), value)
end

@inline function _count_offset(obj::StructVector)::UInt64
    return _values_offset(obj) + _values_size_aligned(obj)
end

@inline function _count_size_aligned(obj::StructVector)::UInt64
    return 8
end

@inline function _count_calc_size_aligned(::Type{StructVector}, value::UInt32)::UInt64
    return 8
end


# --------------------------------------------------------------------

@inline function fastbin_calc_binary_size(obj::StructVector)::UInt64
    return _count_offset(obj) + _count_size_aligned(obj)
end

@inline function fastbin_calc_binary_size(::Type{StructVector},
    values::Vector{ChildFixed}
)
    return 16 +
        _values_calc_size_aligned(StructVector, values)
end

"""
Returns the stored (aligned) binary size of the object.
This function should only be called after `fastbin_finalize!(obj)`.
"""
@inline function fastbin_binary_size(obj::StructVector)::UInt64
    return unsafe_load(reinterpret(Ptr{UInt64}, obj.buffer))
end

"""
Finalizes the object by writing the binary size to the beginning of its buffer.
After calling this function, the underlying buffer can be used for serialization.
To get the actual buffer size, call `fastbin_binary_size(obj)`.
"""
@inline function fastbin_finalize!(obj::StructVector)
    unsafe_store!(reinterpret(Ptr{UInt64}, obj.buffer), fastbin_calc_binary_size(obj))
    nothing
end

function show(io::IO, obj::StructVector)
    print(io, "[my_models::StructVector]")
    print(io, "\n    values: ")
    show(io, values(obj))
    print(io, "\n    count: ")
    print(io, count(obj))
    println(io)
end
