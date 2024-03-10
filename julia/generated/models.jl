module my_models

include("TradeSide.jl")
include("OrderbookType.jl")
include("TickDirection.jl")

include("StreamTrade.jl")
include("StreamOrderbook.jl")
include("ChildVar.jl")
include("ChildFixed.jl")
include("Parent.jl")
include("UInt32Vector.jl")

# export all types and functions
for n in names(@__MODULE__; all=true)
    if Base.isidentifier(n) && n ∉ (Symbol(@__MODULE__), :eval, :include)
        println("Exporting: $n")
        @eval export $n
    end
end

end
