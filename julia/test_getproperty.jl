using InteractiveUtils
using BenchmarkTools

mutable struct A
    buffer::Ptr{UInt8}
    buffer_size::UInt64
    owns_buffer::Bool

    function A(buffer_size::Integer)
        buffer = reinterpret(Ptr{UInt8}, Base.Libc.malloc(buffer_size))
        new(buffer, buffer_size, true)
    end
end

# functions
@inline function field1(obj::A)::Int32
    return unsafe_load(reinterpret(Ptr{Int32}, obj.buffer + fastbin_field1_offset(obj)))
end

@inline function field1!(obj::A, value::Int32)
    unsafe_store!(reinterpret(Ptr{Int32}, obj.buffer + fastbin_field1_offset(obj)), value)
end

@inline function fastbin_field1_offset(obj::A)::UInt64
    return 8
end

@inline function fastbin_field1_size(obj::A)::UInt64
    return 8
end

# # getproperty
# function Base.getproperty(obj::A, v::Symbol)
#     if v == :buffer
#         return getfield(obj, v)
#     elseif v == :buffer_size
#         return getfield(obj, v)
#     elseif v == :owns_buffer
#         return getfield(obj, v)
#     elseif v == :field1
#         return unsafe_load(reinterpret(Ptr{Int32}, obj.buffer + fastbin_field1_offset(obj)))
#     end
# end


const a = A(64)
field1!(a, Int32(42))
println(field1(a))
# println(a.field1)

# @code_warntype a.field1
# @code_llvm a.field1
# @code_native a.field1
# @code_native a.buffer
# @code_native field1(a)

# @benchmark a.field1
# @benchmark field1(a)





# read access
@inline Base.getproperty(obj::A, ::Val{:field1})::Int32 =
    unsafe_load(reinterpret(Ptr{Int32}, obj.buffer + fastbin_field1_offset(obj)))

@inline Base.getproperty(obj::A, name::Symbol) =
    name === :field1 ? getproperty(obj, Val(name)) : getfield(obj, name)

# write access
@inline function Base.setproperty!(obj::A, ::Val{:field1}, value::Int32)
    unsafe_store!(reinterpret(Ptr{Int32}, obj.buffer + fastbin_field1_offset(obj)), value)
    value
end
@inline function Base.setproperty!(obj::A, name::Symbol, value)
    name === :field1 && return setproperty!(obj, Val(name), value)
    setfield!(obj, name, value)
end



field1!(a, Int32(42))

@benchmark field1($a)

@benchmark $a.field1

new_val = Int32(32)
@benchmark field1!($a, $new_val)

@benchmark $a.field1 = $new_val

@inline function helper(c::A)
    c.field1
end

@inline function helper2(c::A)
    field1(c)
end

@code_native helper(a)

@code_native helper2(a)
