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
export bid_prices,
    ask_quantities!,
    fastbin_binary_size,
    seq_num!,
    update_id,
    field2,
    server_time,
    str,
    bid_quantities!,
    recv_time!,
    server_time!,
    size!,
    bid_prices!,
    price_chg_dir,
    field1,
    depth,
    symbol!,
    price!,
    field2!,
    price,
    child1,
    fill_time,
    bid_quantities,
    ask_prices,
    size,
    cts!,
    child2!,
    fill_time!,
    block_trade!,
    update_id!,
    depth!,
    values,
    ask_prices!,
    price_chg_dir!,
    fastbin_finalize!,
    side,
    type,
    recv_time,
    type!,
    trade_id,
    field1!,
    str!,
    side!,
    ask_quantities,
    child1!,
    symbol,
    values!,
    block_trade,
    cts,
    child2,
    trade_id!,
    from_string,
    seq_num,
    fastbin_calc_binary_size

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
