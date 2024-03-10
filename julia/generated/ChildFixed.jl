import Base.show
import Base.finalizer

"""
Binary serializable data container.

This container has fixed size of 16 bytes.

The `finalize!()` method MUST be called after all setter methods have been called.

It is the responsibility of the caller to ensure that the buffer is
large enough to hold the serialized data.
"""
mutable struct ChildFixed
    buffer::Ptr{UInt8}
    buffer_size::UInt64
    owns_buffer::Bool

    function ChildFixed(buffer::Ptr{UInt8}, buffer_size::UInt64, owns_buffer::Bool)
        new(buffer, buffer_size, owns_buffer)
    end
end

function Base.finalizer(obj::ChildFixed)
    if obj.owns_buffer && obj.buffer != C_NULL
        Base.Libc.free(obj.buffer)
        obj.buffer = C_NULL
    end
    nothing
end

@inline function field1(obj::ChildFixed)::Int32
    return unsafe_load(reinterpret(Ptr{Int32}, obj.buffer + fastbin_field1_offset(obj)))
end

@inline function field1!(obj::ChildFixed, value::Int32)
    unsafe_store!(reinterpret(Ptr{Int32}, obj.buffer + fastbin_field1_offset(obj)), value)
end

@inline function fastbin_field1_offset(obj::ChildFixed)::UInt64
    return 8
end

@inline function fastbin_field1_size(obj::ChildFixed)::UInt64
    return 8
end

@inline function field2(obj::ChildFixed)::Int32
    return unsafe_load(reinterpret(Ptr{Int32}, obj.buffer + fastbin_field2_offset(obj)))
end

@inline function field2!(obj::ChildFixed, value::Int32)
    unsafe_store!(reinterpret(Ptr{Int32}, obj.buffer + fastbin_field2_offset(obj)), value)
end

@inline function fastbin_field2_offset(obj::ChildFixed)::UInt64
    return fastbin_field1_offset(obj) + fastbin_field1_size(obj)
end

@inline function fastbin_field2_size(obj::ChildFixed)::UInt64
    return 8
end

@inline function fastbin_compute_binary_size(obj::ChildFixed)::UInt64
    return 16
end

@inline function fastbin_finalize!(obj::ChildFixed)
end

@inline function fastbin_binary_size(obj::ChildFixed)::UInt64
    return 16
end

function show(io::IO, obj::ChildFixed)
    print(io, "[my_models::ChildFixed]")
    print(io, "\n    field1: ")
    print(io, field1(obj))
    print(io, "\n    field2: ")
    print(io, field2(obj))
    println(io)
end
