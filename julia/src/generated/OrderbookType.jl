using EnumX

@enumx OrderbookType::UInt8 begin
    Snapshot = 1
    Delta = 2
end

function from_string(::Type{OrderbookType.T}, str::T) where T <: AbstractString
    str == "Snapshot" && return OrderbookType.Snapshot
    str == "Delta" && return OrderbookType.Delta
    throw(ArgumentError("Invalid string value for enum my_models.OrderbookType: $str"))
end
