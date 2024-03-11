include("generated/models.jl");

using .my_models

function test_StreamTrade()
    buffer_size::UInt64 = 196
    buffer = reinterpret(Ptr{UInt8}, Base.Libc.malloc(buffer_size))

    t = StreamTrade(buffer, buffer_size, true)

    server_time!(t, 1708572597106 * 1_000_000)
    recv_time!(t, 1708572597108 * 1_000_000)
    symbol!(t, "MNTUSDT")
    fill_time!(t, 1708572597104 * 1_000_000)
    side!(t, TradeSide.Sell)
    size!(t, 1.0);
    price!(t, 0.70950)
    price_chg_dir!(t, TickDirection.PlusTick)
    trade_id!(t, "239f859d-d32b-577e-90bc-b1b0dbbb06e0")
    block_trade!(t, false)

    fastbin_finalize!(t)

    show(t)
end
test_StreamTrade()

function test_StreamOrderbook()
    buffer_size::UInt64 = 256
    buffer = reinterpret(Ptr{UInt8}, Base.Libc.malloc(buffer_size))

    t = StreamOrderbook(buffer, buffer_size, true)

    server_time!(t, 1708572597106 * 1_000_000)
    recv_time!(t, 1708572597108 * 1_000_000)
    cts!(t, 1708572597104 * 1_000_000)
    type!(t, OrderbookType.Snapshot)
    depth!(t, UInt16(200))
    symbol!(t, "MNTUSDT")
    update_id!(t, UInt64(123456789))
    seq_num!(t, UInt64(123456789))

    bid_prices!(t, [1.23, 4.56, 7.89])
    bid_quantities!(t, [1.23, 4.56, 7.89].*10)
    ask_prices!(t, [1.23, 4.56, 7.89].*(-1.0))
    ask_quantities!(t, [1.23, 4.56, 7.89].*100)

    fastbin_finalize!(t)

    show(t)
end
test_StreamOrderbook()

function test_UInt32Vector()
    buffer_size::UInt64 = 1024
    buffer = reinterpret(Ptr{UInt8}, Base.Libc.malloc(buffer_size))

    v = UInt32Vector(buffer, buffer_size, true)
    n_items::UInt32 = 23
    vals::Vector{UInt32} = [i for i in 0:n_items-1]
    values!(v, vals)
    my_models.count!(v, n_items)
    fastbin_finalize!(v)

    @assert my_models.count(v) == n_items
    @assert all(my_models.values(v) .== vals)
    @assert fastbin_binary_size(v) == fastbin_compute_binary_size(v)
    @assert fastbin_count_offset(v) + fastbin_count_size(v) == fastbin_binary_size(v)

    show(v)
end
test_UInt32Vector()


function test_StructVector()
    buffer_size::UInt64 = 1024
    buffer = reinterpret(Ptr{UInt8}, Base.Libc.malloc(buffer_size))

    v = StructVector(buffer, buffer_size, true)
    n_items::UInt32 = 23
    vals::Vector{ChildFixed} = Vector{ChildFixed}()
    for i in 0:n_items-1
        c = ChildFixed(16)
        field1!(c, Int32(i))
        field2!(c, Int32(i))
        fastbin_finalize!(c)
        push!(vals, c)
    end
    values!(v, vals)
    my_models.count!(v, n_items)
    fastbin_finalize!(v)

    @assert my_models.count(v) == n_items
    # @assert all(my_models.values(v) .== vals)
    # @assert fastbin_binary_size(v) == fastbin_compute_binary_size(v)
    # @assert fastbin_count_offset(v) + fastbin_count_size(v) == fastbin_binary_size(v)

    show(v)
end
test_StructVector()
