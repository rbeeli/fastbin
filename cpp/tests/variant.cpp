#include <gtest/gtest.h>
#include <string_view>
#include <vector>
#include <cstddef>
#include <span>
#include <iostream>
#include "../templates/Variant.hpp"

using std::byte;
using std::string_view;
using std::span;
using namespace YOUR_NAMESPACE;

TEST(fastbin, variant_single_primitive)
{
    const size_t buffer_size = 64;
    byte* buffer = new byte[buffer_size]();

    auto var1 = Variant<int32_t>::create(buffer, buffer_size, true);

    EXPECT_EQ(decltype(var1)::types_count(), 1);
    EXPECT_EQ(var1.types_count(), 1);
    EXPECT_EQ(var1.empty(), true);

    var1.set<int32_t>(42);

    EXPECT_EQ(var1.empty(), false);
    EXPECT_EQ(var1.index(), 0);
    EXPECT_EQ(var1.holds_alternative<int32_t>(), true);
    EXPECT_EQ(var1.get<int32_t>(), 42);
    EXPECT_EQ(var1.fastbin_binary_size(), 8 + 4); // size_and_index + sizeof(int32_t)

    byte* buffer2 = new byte[buffer_size]();
    auto var1_copy = var1.copy(buffer2, buffer_size, true);
    EXPECT_EQ(var1_copy.empty(), false);
    EXPECT_EQ(var1_copy.index(), 0);
    EXPECT_EQ(var1_copy.holds_alternative<int32_t>(), true);
    EXPECT_EQ(var1_copy.get<int32_t>(), 42);
    EXPECT_EQ(var1_copy.fastbin_binary_size(), 8 + 4); // size_and_index + sizeof(int32_t)
}

TEST(fastbin, variant_single_string_view)
{
    const size_t buffer_size = 64;
    byte* buffer = new byte[buffer_size]();

    auto var1 = Variant<string_view>::create(buffer, buffer_size, true);

    EXPECT_EQ(decltype(var1)::types_count(), 1);
    EXPECT_EQ(var1.types_count(), 1);
    EXPECT_EQ(var1.empty(), true);

    var1.set<string_view>("test1");

    EXPECT_EQ(var1.empty(), false);
    EXPECT_EQ(var1.index(), 0);
    EXPECT_EQ(var1.holds_alternative<string_view>(), true);
    EXPECT_EQ(var1.get<string_view>(), "test1");
    EXPECT_EQ(var1.fastbin_binary_size(), 8 + 5); // size_and_index + "test1".size()

    byte* buffer2 = new byte[buffer_size]();
    auto var1_copy = var1.copy(buffer2, buffer_size, true);
    EXPECT_EQ(var1_copy.empty(), false);
    EXPECT_EQ(var1_copy.index(), 0);
    EXPECT_EQ(var1_copy.holds_alternative<string_view>(), true);
    EXPECT_EQ(var1_copy.get<string_view>(), "test1");
    EXPECT_EQ(var1_copy.fastbin_binary_size(), 8 + 5); // size_and_index + "test1".size()
}

TEST(fastbin, variant_single_span_int32)
{
    const size_t buffer_size = 64;
    byte* buffer = new byte[buffer_size]();

    auto var1 = Variant<span<int32_t>>::create(buffer, buffer_size, true);

    EXPECT_EQ(decltype(var1)::types_count(), 1);
    EXPECT_EQ(var1.types_count(), 1);
    EXPECT_EQ(var1.empty(), true);

    std::vector<int32_t> vec = {1, 2, 3, 4, 5};
    var1.set<span<int32_t>>(span<int32_t>(vec));

    EXPECT_EQ(var1.empty(), false);
    EXPECT_EQ(var1.index(), 0);
    EXPECT_EQ(var1.holds_alternative<span<int32_t>>(), true);
    auto stored = var1.get<span<int32_t>>();
    EXPECT_EQ(stored.size(), vec.size());
    for (size_t i = 0; i < vec.size(); ++i)
    {
        EXPECT_EQ(stored[i], vec[i]);
    }
    EXPECT_EQ(var1.fastbin_binary_size(), 8 + vec.size() * sizeof(int32_t));

    // byte* buffer2 = new byte[buffer_size]();
    // auto var1_copy = var1.copy(buffer2, buffer_size, true);
    // EXPECT_EQ(var1_copy.empty(), false);
    // EXPECT_EQ(var1_copy.index(), 0);
    // EXPECT_EQ(var1_copy.holds_alternative<string_view>(), true);
    // EXPECT_EQ(var1_copy.get<string_view>(), "test1");
    // EXPECT_EQ(var1_copy.fastbin_binary_size(), 8 + 5); // size_and_index + "test1".size()
}

TEST(fastbin, variant_primitive_string_view)
{
    const size_t buffer_size = 64;
    byte* buffer = new byte[buffer_size]();

    auto var1 = Variant<int8_t, string_view>::create(buffer, buffer_size, true);

    EXPECT_EQ(decltype(var1)::types_count(), 2);
    EXPECT_EQ(var1.types_count(), 2);
    EXPECT_EQ(var1.empty(), true);

    var1.set<string_view>("test1");

    EXPECT_EQ(var1.empty(), false);
    EXPECT_EQ(var1.index(), 1);
    EXPECT_EQ(var1.holds_alternative<string_view>(), true);
    EXPECT_EQ(var1.get<string_view>(), "test1");
    EXPECT_EQ(var1.fastbin_binary_size(), 8 + 5); // size_and_index + "test1".size()

    var1.set<int8_t>(99);

    EXPECT_EQ(var1.empty(), false);
    EXPECT_EQ(var1.index(), 0);
    EXPECT_EQ(var1.holds_alternative<int8_t>(), true);
    EXPECT_EQ(var1.get<int8_t>(), 99);
    EXPECT_EQ(var1.fastbin_binary_size(), 8 + 1); // size_and_index + sizeof(int8_t)
}

TEST(fastbin, variant_primitives)
{
    const size_t buffer_size = 64;
    byte* buffer = new byte[buffer_size]();

    auto var1 = Variant<int32_t, int64_t, uint8_t>::create(buffer, buffer_size, true);

    EXPECT_EQ(decltype(var1)::types_count(), 3);
    EXPECT_EQ(var1.types_count(), 3);
    EXPECT_EQ(var1.empty(), true);

    var1.set<int32_t>(42);

    EXPECT_EQ(var1.empty(), false);
    EXPECT_EQ(var1.index(), 0);
    EXPECT_EQ(var1.holds_alternative<int32_t>(), true);
    EXPECT_EQ(var1.get<int32_t>(), 42);
    EXPECT_EQ(var1.fastbin_binary_size(), 8 + 4); // size_and_index + sizeof(int32_t)

    byte* buffer2 = new byte[buffer_size]();
    auto var1_copy = var1.copy(buffer2, buffer_size, true);
    EXPECT_EQ(var1_copy.empty(), false);
    EXPECT_EQ(var1_copy.index(), 0);
    EXPECT_EQ(var1_copy.holds_alternative<int32_t>(), true);
    EXPECT_EQ(var1_copy.get<int32_t>(), 42);
    EXPECT_EQ(var1_copy.fastbin_binary_size(), 8 + 4); // size_and_index + sizeof(int32_t)
}

TEST(fastbin, variant_primitives_2)
{
    const size_t buffer_size = 64;
    byte* buffer = new byte[buffer_size]();

    auto var1 = Variant<int32_t, int64_t, uint8_t>::create(buffer, buffer_size, true);

    EXPECT_EQ(decltype(var1)::types_count(), 3);
    EXPECT_EQ(var1.types_count(), 3);
    EXPECT_EQ(var1.empty(), true);

    var1.set<int64_t>(42);

    EXPECT_EQ(var1.empty(), false);
    EXPECT_EQ(var1.index(), 1);
    EXPECT_EQ(var1.holds_alternative<int64_t>(), true);
    EXPECT_EQ(var1.get<int64_t>(), 42);
    EXPECT_EQ(var1.fastbin_binary_size(), 8 + sizeof(int64_t)); // size_and_index + sizeof(int64_t)

    byte* buffer2 = new byte[buffer_size]();
    auto var1_copy = var1.copy(buffer2, buffer_size, true);
    EXPECT_EQ(var1_copy.empty(), false);
    EXPECT_EQ(var1_copy.index(), 1);
    EXPECT_EQ(var1_copy.holds_alternative<int64_t>(), true);
    EXPECT_EQ(var1_copy.get<int64_t>(), 42);
    EXPECT_EQ(var1_copy.fastbin_binary_size(), 8 + sizeof(int64_t)); // size_and_index + sizeof(int64_t)
}

TEST(fastbin, variant_primitives_3)
{
    const size_t buffer_size = 64;
    byte* buffer = new byte[buffer_size]();

    auto var1 = Variant<int32_t, int64_t, uint8_t>::create(buffer, buffer_size, true);

    EXPECT_EQ(decltype(var1)::types_count(), 3);
    EXPECT_EQ(var1.types_count(), 3);
    EXPECT_EQ(var1.empty(), true);

    var1.set<uint8_t>(42);

    EXPECT_EQ(var1.empty(), false);
    EXPECT_EQ(var1.index(), 2);
    EXPECT_EQ(var1.holds_alternative<uint8_t>(), true);
    EXPECT_EQ(var1.get<uint8_t>(), 42);
    EXPECT_EQ(var1.fastbin_binary_size(), 8 + sizeof(uint8_t)); // size_and_index + sizeof(uint8_t)

    byte* buffer2 = new byte[buffer_size]();
    auto var1_copy = var1.copy(buffer2, buffer_size, true);
    EXPECT_EQ(var1_copy.empty(), false);
    EXPECT_EQ(var1_copy.index(), 2);
    EXPECT_EQ(var1_copy.holds_alternative<uint8_t>(), true);
    EXPECT_EQ(var1_copy.get<uint8_t>(), 42);
    EXPECT_EQ(var1_copy.fastbin_binary_size(), 8 + sizeof(uint8_t)); // size_and_index + sizeof(uint8_t)
}
