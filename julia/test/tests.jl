using TestItems


@testitem "StreamTrade" begin
	using StringViews
	using fastbin.my_models

	buffer_size = 196
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

	@assert server_time(t) == 1708572597106 * 1_000_000
	@assert recv_time(t) == 1708572597108 * 1_000_000
	@assert symbol(t) == "MNTUSDT"
	@assert fill_time(t) == 1708572597104 * 1_000_000
	@assert side(t) == TradeSide.Sell
	@assert my_models.size(t) == 1.0
	@assert price(t) == 0.70950
	@assert price_chg_dir(t) == TickDirection.PlusTick
	@assert trade_id(t) == "239f859d-d32b-577e-90bc-b1b0dbbb06e0"
	@assert block_trade(t) == false

	@assert fastbin_calc_binary_size(
		StreamTrade,
		StringView("MNTUSDT"),
		StringView("239f859d-d32b-577e-90bc-b1b0dbbb06e0")) == fastbin_binary_size(t)

	show(t)
end

@testitem "StreamOrderbook" begin
	using StringViews
	using fastbin.my_models

	buffer_size = 256
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
	bid_quantities!(t, [1.23, 4.56, 7.89] .* 10)
	ask_prices!(t, [1.23, 4.56, 7.89] .* (-1.0))
	ask_quantities!(t, [1.23, 4.56, 7.89] .* 100)

	fastbin_finalize!(t)

	@assert all(bid_prices(t) .== [1.23, 4.56, 7.89])
	@assert all(bid_quantities(t) .== [1.23, 4.56, 7.89] .* 10)
	@assert all(ask_prices(t) .== [1.23, 4.56, 7.89] .* (-1.0))
	@assert all(ask_quantities(t) .== [1.23, 4.56, 7.89] .* 100)

	@assert fastbin_calc_binary_size(
		StreamOrderbook,
		StringView("MNTUSDT"),
		bid_prices(t),
		bid_quantities(t),
		ask_prices(t),
		ask_quantities(t)) == fastbin_binary_size(t)

	show(t)
end

@testitem "VectorOfUInt32" begin
	using StringViews
	using fastbin.my_models

	buffer_size = 1024
	buffer = reinterpret(Ptr{UInt8}, Base.Libc.malloc(buffer_size))

	v = VectorOfUInt32(buffer, buffer_size, true)
	n_items::UInt32 = 23
	vals::Vector{UInt32} = [i for i in 0:(n_items-1)]
	str_::StringView = "TEST"
	values!(v, vals)
	str!(v, "TEST")

	fastbin_finalize!(v)

	@assert length(my_models.values(v)) == n_items
	@assert str(v) == str_
	@assert all(my_models.values(v) .== vals)

	@assert fastbin_binary_size(v) == fastbin_calc_binary_size(v)

	@assert fastbin_calc_binary_size(VectorOfUInt32, vals, str_) == fastbin_binary_size(v)

	show(v)
end

@testitem "VectorOfFixedSizedStructs" begin
	using StringViews
	using fastbin.my_models

	buffer_size = 1024
	buffer = reinterpret(Ptr{UInt8}, Base.Libc.malloc(buffer_size))

	v = VectorOfFixedSizedStructs(buffer, buffer_size, true)
	n_items::UInt32 = 23
	vals::Vector{ChildFixed} = Vector{ChildFixed}()
	for i in 0:(n_items-1)
		c = ChildFixed(16)
		field1!(c, Int32(i))
		field2!(c, Int32(i))
		fastbin_finalize!(c)
		push!(vals, c)
	end
	values!(v, vals)
	str_::StringView = "TEST"
	str!(v, "TEST")

	fastbin_finalize!(v)

	@assert length(my_models.values(v)) == n_items
	@assert str(v) == str_
	@assert all(my_models.values(v) .== vals)

	@assert fastbin_binary_size(v) == fastbin_calc_binary_size(v)
	@assert fastbin_calc_binary_size(VectorOfFixedSizedStructs, vals, str_) == fastbin_binary_size(v)
end

@testitem "Parent round-trip (nested var/fixed)" begin
	using StringViews
	using fastbin.my_models

	# --- build children
	cvar = ChildVar(64)
	field1!(cvar, Int32(42))
	field2!(cvar, "hello")
	fastbin_finalize!(cvar)

	cfix = ChildFixed(16)
	field1!(cfix, Int32(-11))
	field2!(cfix, Int32(+11))
	fastbin_finalize!(cfix)

	# --- build parent--
	buf = reinterpret(Ptr{UInt8}, Base.Libc.malloc(128))
	p   = Parent(buf, 128, true)
	field1!(p, Int32(123))
	child1!(p, cfix)
	child2!(p, cvar)
	str!(p, "parent")
	fastbin_finalize!(p)

	@test field1(p) == 123
	@test field1(child1(p)) == -11
	@test field2(child2(p)) == "hello"
	@test str(p) == "parent"

	# binary length consistency
	@test fastbin_binary_size(p) ==
		  fastbin_calc_binary_size(Parent, child2(p), str(p))
end

@testitem "Zero-length vector / string edge-case" begin
	using StringViews
	using fastbin.my_models

	buf = reinterpret(Ptr{UInt8}, Base.Libc.malloc(128))
	v   = VectorOfUInt32(buf, 128, true)

	values!(v, UInt32[])      # empty vector
	str!(v, "")               # empty string
	fastbin_finalize!(v)

	@test length(my_models.values(v)) == 0
	@test str(v) == ""

	# 8 bytes for struct size
	# 8 bytes for values count
	# 8 bytes for string length
	@test fastbin_binary_size(v) == 24
end

@testitem "Binary copy reopen" begin
	using fastbin.my_models
	using StringViews

	old = ChildFixed(16)
	field1!(old, Int32(7))
	field2!(old, Int32(8))
	fastbin_finalize!(old)

	newbuf = reinterpret(Ptr{UInt8}, Base.Libc.malloc(16))
	unsafe_copyto!(newbuf, old.buffer, 16)

	reopened = ChildFixed(newbuf, 16, true)   # takes ownership
	@test field1(reopened) == 7
	@test field2(reopened) == 8
end

@testitem "Alignment assertion fires" begin
	using fastbin.my_models
	using Test

	raw = Base.Libc.malloc(64)         # 8‑aligned block we own
	buf = reinterpret(Ptr{UInt8}, raw) # Ptr to first byte
	mis = buf + 1                      # intentionally mis‑aligned

	@test_throws AssertionError Parent(mis, 64, false)

	Base.Libc.free(raw)
end
