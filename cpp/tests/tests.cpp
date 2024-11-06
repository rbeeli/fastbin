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

    auto ob = my_models::StreamOrderbook::create(buffer, buffer_size, owns_buffer);
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

    EXPECT_EQ(ob.fastbin_binary_size(), ob.fastbin_calc_binary_size());
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
        ob._ask_quantities_offset() + ob._ask_quantities_size_aligned(), ob.fastbin_binary_size()
    );

    delete[] buffer;
}

TEST(fastbin, ser_de_Nested)
{
    const size_t buffer_size = 1024;
    byte* buffer = new byte[buffer_size]();

    auto p = my_models::Parent::create(buffer, buffer_size, true);
    p.field1(123);

    p.child1().field1(456);
    p.child1().field2(789);
    p.child1().fastbin_finalize();

    p.child2().field1(789);
    p.child2().field2("test");
    p.child2().fastbin_finalize();

    p.str("str");

    p.fastbin_finalize();

    EXPECT_EQ(p.fastbin_binary_size(), p.fastbin_calc_binary_size());
    EXPECT_EQ(p.field1(), 123);
    EXPECT_EQ(p.child1().field1(), 456);
    EXPECT_EQ(p.child1().field2(), 789);
    EXPECT_EQ(p.child2().field1(), 789);
    EXPECT_EQ(p.child2().field2(), "test");
    EXPECT_EQ(p.str(), "str");
}

TEST(fastbin, ser_de_VectorOfUInt32)
{
    const size_t buffer_size = 1024;
    byte* buffer = new byte[buffer_size]();

    auto v = my_models::VectorOfUInt32::create(buffer, buffer_size, true); // owns buffer
    uint32_t count = 23;
    vector<uint32_t> values(count);
    for (uint32_t i = 0; i < count; ++i)
        values[i] = i;
    v.values(std::span<uint32_t>(values.data(), values.size()));
    v.str("test");
    v.fastbin_finalize();

    EXPECT_EQ(v.str(), "test");
    EXPECT_EQ(v.fastbin_binary_size(), v.fastbin_calc_binary_size());
    EXPECT_EQ(v._str_offset() + v._str_size_aligned(), v.fastbin_binary_size());

    EXPECT_NE(v.values().data(), reinterpret_cast<uint32_t*>(values.data()));
    EXPECT_EQ(v.values().size(), values.size());
    for (size_t i = 0; i < values.size(); ++i)
        EXPECT_EQ(v.values()[i], values[i]);
}

TEST(fastbin, ser_de_VectorOfFixedSizedStructs_own_buffer)
{
    const size_t buffer_size = 1024;
    byte* buffer = new byte[buffer_size]();

    // create outer struct
    auto v = my_models::VectorOfFixedSizedStructs::create(buffer, buffer_size, true); // owns buffer

    constexpr size_t child_size = my_models::ChildFixed::fastbin_fixed_size();

    // Calculate total needed size for struct_array:
    // 2 * sizeof(size_t) for size_ and count_
    // Plus space for 3 ChildFixed objects
    const size_t array_buffer_size = 2 * sizeof(size_t) + (3 * child_size);
    byte* array_buffer = new byte[array_buffer_size]();

    // Create struct_array with its own buffer
    auto arr = my_models::struct_array<my_models::ChildFixed>::create(
        array_buffer, array_buffer_size, true // owns its buffer
    );

    vector<my_models::ChildFixed> values;
    for (uint32_t i = 0; i < 3; ++i)
    {
        byte* buf = new byte[child_size]();
        auto child = my_models::ChildFixed::create(buf, child_size, true); // owns buffer
        child.field1(i);
        child.field2(i * 10);
        child.fastbin_finalize();

        arr.append(child);
        values.emplace_back(std::move(child));
    }
    arr.fastbin_finalize();

    // Copy the finalized struct_array to the VectorOfFixedSizedStructs
    v.values(arr);

    EXPECT_EQ(
        my_models::struct_array<my_models::ChildFixed>::fastbin_calc_binary_size(values),
        arr.buffer_size
    );

    v.str("test");
    v.fastbin_finalize();

    EXPECT_EQ(v._values_offset(), 8);
    EXPECT_EQ(v._values_size_aligned(), arr.fastbin_binary_size());
    EXPECT_EQ(v._str_offset(), 8 + arr.fastbin_binary_size());

    EXPECT_EQ(v.str(), "test");
    EXPECT_EQ(v.fastbin_binary_size(), v.fastbin_calc_binary_size());
    EXPECT_EQ(v._str_offset() + v._str_size_aligned(), v.fastbin_binary_size());
    EXPECT_EQ(
        v.fastbin_binary_size(),
        v._values_offset() + arr.fastbin_binary_size() + v._str_size_aligned()
    );

    EXPECT_EQ(v.values().size(), values.size());
    for (size_t i = 0; i < values.size(); ++i)
    {
        EXPECT_EQ(v.values()[i].field1(), i);
        EXPECT_EQ(v.values()[i].field2(), i * 10);
    }
}

TEST(fastbin, ser_de_VectorOfFixedSizedStructs_inplace_buffer)
{
    const size_t buffer_size = 1024;
    byte* buffer = new byte[buffer_size]();

    // create outer struct
    auto v = my_models::VectorOfFixedSizedStructs::create(buffer, buffer_size, true); // owns buffer

    // Create struct_array using outer struct buffer
    auto arr = my_models::struct_array<my_models::ChildFixed>::create(
        v.values().buffer, 1000, false // non-owning directly write to outer buffer
    );

    vector<my_models::ChildFixed> values;
    for (uint32_t i = 0; i < 3; ++i)
    {
        constexpr size_t child_size = my_models::ChildFixed::fastbin_fixed_size();
        byte* buf = new byte[child_size]();
        auto child = my_models::ChildFixed::create(buf, child_size, true); // owns buffer
        child.field1(i);
        child.field2(i * 10);
        child.fastbin_finalize();

        arr.append(child);
        values.emplace_back(std::move(child));
    }
    arr.fastbin_finalize();

    // // Copy not needed anymore, struct_array is inplace
    // v.values(arr);

    v.str("test");
    v.fastbin_finalize();

    EXPECT_EQ(v._values_offset(), 8);
    EXPECT_EQ(v._values_size_aligned(), arr.fastbin_binary_size());
    EXPECT_EQ(v._str_offset(), 8 + arr.fastbin_binary_size());

    EXPECT_EQ(v.str(), "test");
    EXPECT_EQ(v.fastbin_binary_size(), v.fastbin_calc_binary_size());
    EXPECT_EQ(v._str_offset() + v._str_size_aligned(), v.fastbin_binary_size());
    EXPECT_EQ(
        v.fastbin_binary_size(),
        v._values_offset() + arr.fastbin_binary_size() + v._str_size_aligned()
    );

    EXPECT_EQ(v.values().size(), values.size());
    for (size_t i = 0; i < values.size(); ++i)
    {
        EXPECT_EQ(v.values()[i].field1(), i);
        EXPECT_EQ(v.values()[i].field2(), i * 10);
    }
}

TEST(fastbin, ser_de_VectorOfVariableSizedStructs_own_buffer)
{
    // 8 + (8 + string length)
    size_t child_size = my_models::ChildVar::fastbin_calc_binary_size("var_text");
    EXPECT_EQ(child_size, 32);

    // Calculate total needed size for struct_array:
    // 2 * sizeof(size_t) for size_ and count_
    // Plus space for 3 ChildVar objects
    const size_t array_buffer_size = 2 * sizeof(size_t) + (3 * child_size);
    byte* array_buffer = new byte[array_buffer_size]();

    // Create struct_array with its own buffer
    auto arr = my_models::struct_array<my_models::ChildVar>::create(
        array_buffer, array_buffer_size, true // owns its buffer
    );

    std::vector<my_models::ChildVar> values;
    for (uint32_t i = 0; i < 3; ++i)
    {
        byte* buf = new byte[child_size]();
        auto child = my_models::ChildVar::create(buf, child_size, true); // owns buffer
        child.field1(i);
        child.field2("var_text");
        child.fastbin_finalize();

        arr.append(child);
        values.emplace_back(std::move(child));
    }
    arr.fastbin_finalize();

    EXPECT_EQ(
        my_models::struct_array<my_models::ChildVar>::fastbin_calc_binary_size(values),
        arr.buffer_size
    );

    size_t buffer_size = my_models::VectorOfVariableSizedStructs::fastbin_calc_binary_size(
        arr, "test"
    );
    byte* buffer = new byte[buffer_size]();

    // create outer struct
    auto v = my_models::VectorOfVariableSizedStructs::create(
        buffer, buffer_size, true
    ); // owns buffer

    // Copy the finalized struct_array to the VectorOfVariableSizedStructs
    v.values(arr);

    v.str("test");
    v.fastbin_finalize();

    EXPECT_EQ(v._values_offset(), 8);
    EXPECT_EQ(v._values_size_aligned(), arr.fastbin_binary_size());
    EXPECT_EQ(v._str_offset(), 8 + arr.fastbin_binary_size());

    EXPECT_EQ(v.str(), "test");
    EXPECT_EQ(v.fastbin_binary_size(), v.fastbin_calc_binary_size());
    EXPECT_EQ(v._str_offset() + v._str_size_aligned(), v.fastbin_binary_size());
    EXPECT_EQ(
        v.fastbin_binary_size(),
        v._values_offset() + arr.fastbin_binary_size() + v._str_size_aligned()
    );

    EXPECT_EQ(v.values().size(), 3);
    for (size_t i = 0; i < 3; ++i)
    {
        EXPECT_EQ(v.values()[i].field1(), i);
        EXPECT_EQ(v.values()[i].field2(), "var_text");
    }
}

TEST(fastbin, ser_de_VectorOfVariableSizedStructs_inplace_buffer)
{
    const size_t buffer_size = 1024;
    byte* buffer = new byte[buffer_size]();

    // create outer struct
    auto v = my_models::VectorOfVariableSizedStructs::create(
        buffer, buffer_size, true
    ); // owns buffer

    // Create struct_array using outer struct buffer
    auto arr = my_models::struct_array<my_models::ChildVar>::create(
        v.values().buffer, 1000, false // non-owning directly write to outer buffer
    );

    // 8 + (8 + string length)
    size_t child_size = my_models::ChildVar::fastbin_calc_binary_size("var_text");
    EXPECT_EQ(child_size, 32);

    vector<my_models::ChildVar> values;
    for (uint32_t i = 0; i < 3; ++i)
    {
        byte* buf = new byte[child_size]();
        auto child = my_models::ChildVar::create(buf, child_size, true); // owns buffer
        child.field1(i);
        child.field2("var_text");
        child.fastbin_finalize();

        arr.append(child);
        values.emplace_back(std::move(child));
    }
    arr.fastbin_finalize();

    // // Copy not needed anymore, struct_array is inplace
    // v.values(arr);

    v.str("test");
    v.fastbin_finalize();

    EXPECT_EQ(v._values_offset(), 8);
    EXPECT_EQ(v._values_size_aligned(), arr.fastbin_binary_size());
    EXPECT_EQ(v._str_offset(), 8 + arr.fastbin_binary_size());

    EXPECT_EQ(v.str(), "test");
    EXPECT_EQ(v.fastbin_binary_size(), v.fastbin_calc_binary_size());
    EXPECT_EQ(v._str_offset() + v._str_size_aligned(), v.fastbin_binary_size());
    EXPECT_EQ(
        v.fastbin_binary_size(),
        v._values_offset() + arr.fastbin_binary_size() + v._str_size_aligned()
    );

    EXPECT_EQ(v.values().size(), values.size());
    for (size_t i = 0; i < values.size(); ++i)
    {
        EXPECT_EQ(v.values()[i].field1(), i);
        EXPECT_EQ(v.values()[i].field2(), "var_text");
    }
}
