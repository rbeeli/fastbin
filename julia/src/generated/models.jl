module my_models

# enums
export TradeSide,
    OrderbookType,
    TickDirection

# structs
export StreamTrade,
    StreamOrderbook,
    ChildVar,
    ChildFixed,
    Parent,
    VectorOfUInt32,
    VectorOfFixedSizedStructs

# functions
export size,
    price_chg_dir,
    child1!,
    symbol,
    side!,
    block_trade,
    side,
    str!,
    recv_time,
    depth!,
    cts!,
    bid_prices!,
    field2!,
    values,
    ask_quantities!,
    ask_quantities,
    fastbin_finalize!,
    bid_prices,
    size!,
    price,
    server_time,
    type,
    child2,
    from_string,
    seq_num,
    field1!,
    fill_time!,
    fastbin_calc_binary_size,
    depth,
    update_id,
    seq_num!,
    recv_time!,
    cts,
    fill_time,
    child2!,
    trade_id,
    symbol!,
    trade_id!,
    block_trade!,
    ask_prices!,
    child1,
    bid_quantities,
    field2,
    ask_prices,
    type!,
    values!,
    fastbin_binary_size,
    price!,
    update_id!,
    str,
    server_time!,
    bid_quantities!,
    field1,
    price_chg_dir!

# enums
include("TradeSide.jl")
include("OrderbookType.jl")
include("TickDirection.jl")

# structs
include("StreamTrade.jl")
include("StreamOrderbook.jl")
include("ChildVar.jl")
include("ChildFixed.jl")
include("Parent.jl")
include("VectorOfUInt32.jl")
include("VectorOfFixedSizedStructs.jl")

end
