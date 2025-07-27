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
export fastbin_binary_size,
    values,
    bid_prices,
    block_trade,
    update_id!,
    update_id,
    price_chg_dir!,
    price!,
    trade_id,
    recv_time!,
    recv_time,
    from_string,
    depth,
    ask_prices!,
    fastbin_finalize!,
    type!,
    seq_num,
    bid_quantities!,
    str,
    trade_id!,
    child2!,
    fastbin_calc_binary_size,
    server_time,
    depth!,
    child1!,
    block_trade!,
    server_time!,
    price_chg_dir,
    price,
    values!,
    cts,
    cts!,
    fill_time!,
    field1!,
    bid_quantities,
    field2!,
    side!,
    fill_time,
    ask_quantities!,
    child2,
    seq_num!,
    field1,
    size!,
    ask_quantities,
    side,
    type,
    field2,
    symbol!,
    symbol,
    bid_prices!,
    ask_prices,
    str!,
    size,
    child1

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
