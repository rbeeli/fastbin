using EnumX

"""
https://bybit-exchange.github.io/docs/v5/enum#tickdirection
"""
@enumx TickDirection::UInt8 begin
    Unknown = 0

    """
    Price rise.
    """
    PlusTick = 1

    """
    Trade occurs at the same price as the previous trade,
    which occurred at a price lower than that for the trade preceding it.
    
    Example price series: 100 -> 99 -> 99
    """
    ZeroPlusTick = 2

    """
    Price drop.
    """
    MinusTick = 3

    """
    Trade occurs at the same price as the previous trade,
    which occurred at a price higher than that for the trade preceding it.
    
    Example price series: 100 -> 101 -> 101
    """
    ZeroMinusTick = 4
end

function from_string(::Type{TickDirection.T}, str::T) where T <: AbstractString
    str == "Unknown" && return TickDirection.Unknown
    str == "PlusTick" && return TickDirection.PlusTick
    str == "ZeroPlusTick" && return TickDirection.ZeroPlusTick
    str == "MinusTick" && return TickDirection.MinusTick
    str == "ZeroMinusTick" && return TickDirection.ZeroMinusTick
    throw(ArgumentError("Invalid string value for enum my_models.TickDirection: $str"))
end
