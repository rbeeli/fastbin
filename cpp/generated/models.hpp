#pragma once

#include <bit>

static_assert(std::endian::native == std::endian::little);
static_assert(sizeof(size_t) == 8, "fastbin requires 64-bit size_t");

#include "TradeSide.hpp"
#include "OrderbookType.hpp"
#include "TickDirection.hpp"

#include "StreamTrade.hpp"
#include "StreamOrderbook.hpp"
#include "ChildVar.hpp"
#include "ChildFixed.hpp"
#include "Parent.hpp"
#include "VectorOfUInt32.hpp"
#include "VectorOfFixedSizedStructs.hpp"
#include "VectorOfVariableSizedStructs.hpp"
#include "Variants.hpp"
