using EnumX

@enumx TradeSide::UInt8 begin
    Sell = 0
    Buy = 1
end

function from_string(::Type{TradeSide.T}, str::T) where T <: AbstractString
    str == "Sell" && return TradeSide.Sell
    str == "Buy" && return TradeSide.Buy
    throw(ArgumentError("Invalid string value for enum my_models.TradeSide: $str"))
end
