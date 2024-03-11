#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstddef>
#include <iostream>
#include "../generated/models.hpp"

using std::vector;
using std::byte;
using std::string;

static string symbol = "BTCUSDT";
static vector<double> prices{
    123.45, 123.46, 123.47, 123.48, 123.49, 123.50, 123.51, 123.52, 123.53, 123.54, 123.49,
    123.50, 123.51, 123.52, 123.53, 123.54, 123.49, 123.50, 123.51, 123.52, 123.53, 123.54,
};
static vector<double> qtys{
    0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5,
    0.6, 0.7, 0.8, 0.9, 1.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
};


TEST(fastbin, ser_de_StreamOrderbook)
{
    const size_t buffer_size = 1024;
    byte* buffer = new byte[buffer_size]();
    bool owns_buffer = false;

    my_models::StreamOrderbook ob(buffer, buffer_size, owns_buffer);
    ob.type(my_models::OrderbookType::Delta);
    ob.server_time(748949849849);
    ob.recv_time(748949849852);
    ob.symbol(symbol);
    ob.update_id(335553355335);
    ob.seq_num(9999999999);
    ob.bid_prices(std::span<double>(prices.data(), prices.size()));
    ob.bid_quantities(std::span<double>(qtys.data(), qtys.size()));
    ob.ask_prices(std::span<double>(prices.data(), prices.size()));
    ob.ask_quantities(std::span<double>(qtys.data(), qtys.size()));
    ob.fastbin_finalize();

    EXPECT_EQ(ob.fastbin_binary_size(), ob.fastbin_compute_binary_size());
    EXPECT_EQ(ob.type(), my_models::OrderbookType::Delta);
    EXPECT_EQ(ob.server_time(), 748949849849);
    EXPECT_EQ(ob.recv_time(), 748949849852);
    EXPECT_EQ(ob.symbol(), symbol);
    EXPECT_EQ(ob.update_id(), 335553355335);
    EXPECT_EQ(ob.seq_num(), 9999999999);
    EXPECT_EQ(ob.bid_prices().size(), prices.size());
    EXPECT_EQ(ob.bid_quantities().size(), qtys.size());
    EXPECT_EQ(ob.ask_prices().size(), prices.size());
    EXPECT_EQ(ob.ask_quantities().size(), qtys.size());

    EXPECT_NE(ob.bid_prices().data(), prices.data());
    EXPECT_NE(ob.bid_quantities().data(), qtys.data());
    EXPECT_NE(ob.ask_prices().data(), prices.data());
    EXPECT_NE(ob.ask_quantities().data(), qtys.data());

    for (size_t i = 0; i < prices.size(); ++i)
    {
        EXPECT_EQ(ob.bid_prices()[i], prices[i]);
        EXPECT_EQ(ob.ask_prices()[i], prices[i]);
        EXPECT_EQ(ob.bid_quantities()[i], qtys[i]);
        EXPECT_EQ(ob.ask_quantities()[i], qtys[i]);
    }

    EXPECT_EQ(
        ob.fastbin_ask_quantities_offset() + ob.fastbin_ask_quantities_size(),
        ob.fastbin_binary_size()
    );

    delete[] buffer;
}

TEST(fastbin, ser_de_Nested)
{
    const size_t buffer_size = 1024;
    byte* buffer = new byte[buffer_size]();

    my_models::Parent p(buffer, buffer_size, true);
    p.field1(123);

    p.child1().field1(456);
    p.child1().field2(789);
    p.child1().fastbin_finalize();

    p.child2().field1(789);
    p.child2().field2("test");
    p.child2().fastbin_finalize();

    p.str("str");

    p.fastbin_finalize();

    EXPECT_EQ(p.fastbin_binary_size(), p.fastbin_compute_binary_size());
    EXPECT_EQ(p.field1(), 123);
    EXPECT_EQ(p.child1().field1(), 456);
    EXPECT_EQ(p.child1().field2(), 789);
    EXPECT_EQ(p.child2().field1(), 789);
    EXPECT_EQ(p.child2().field2(), "test");
    EXPECT_EQ(p.str(), "str");
}

TEST(fastbin, ser_de_UInt32Vector)
{
    const size_t buffer_size = 1024;
    byte* buffer = new byte[buffer_size]();

    my_models::UInt32Vector v(buffer, buffer_size, true); // owns buffer
    uint32_t count = 23;
    vector<uint32_t> values(count);
    for (uint32_t i = 0; i < count; ++i)
        values[i] = i;
    v.values(std::span<uint32_t>(values.data(), values.size()));
    v.count(count);
    v.fastbin_finalize();

    EXPECT_EQ(v.count(), count);
    EXPECT_EQ(v.fastbin_binary_size(), v.fastbin_compute_binary_size());
    EXPECT_EQ(v.fastbin_count_offset() + v.fastbin_count_size(), v.fastbin_binary_size());

    EXPECT_NE(v.values().data(), reinterpret_cast<uint32_t*>(values.data()));
    EXPECT_EQ(v.values().size(), values.size());
    for (size_t i = 0; i < values.size(); ++i)
        EXPECT_EQ(v.values()[i], values[i]);
}
