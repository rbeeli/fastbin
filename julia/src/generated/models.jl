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
export fastbin_finalize!,
    symbol!,
    bid_quantities,
    size,
    trade_id,
    bid_prices,
    field2!,
    server_time,
    from_string,
    symbol,
    type,
    price!,
    side,
    fill_time!,
    price_chg_dir!,
    update_id!,
    side!,
    ask_prices!,
    server_time!,
    block_trade!,
    price,
    child2,
    field2,
    child1!,
    ask_quantities,
    child1,
    trade_id!,
    ask_prices,
    recv_time,
    depth!,
    field1,
    type!,
    child2!,
    bid_quantities!,
    recv_time!,
    cts!,
    bid_prices!,
    values,
    fastbin_binary_size,
    price_chg_dir,
    fill_time,
    seq_num!,
    block_trade,
    ask_quantities!,
    depth,
    fastbin_calc_binary_size,
    values!,
    str,
    cts,
    size!,
    field1!,
    update_id,
    seq_num,
    str!

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
