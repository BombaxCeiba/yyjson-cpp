// Regression tests for aggregate types being incorrectly matched by
// the generic constructible fallback in default_caster::from_json.
//
// Before the fix, aggregates with int32_t first field would be
// "constructible" from uint64_t/int64_t/bool via aggregate init,
// triggering MSVC C4244 narrowing conversion warnings and silently
// producing wrong values at runtime.

#include <gtest/gtest.h>
#include <cstdint>
#include <optional>
#include <string>

#include "cpp_yyjson.hpp"

// --- Test types ---

// Mimics ChildExecutionSnapshotDto: aggregate, first field is int32_t
struct SnapshotDto
{
    int32_t version = 1;
    int64_t source_entry_id = 0;
    std::string name;
};

// Aggregate whose first field is bool
struct BoolFirstDto
{
    bool active = false;
    int32_t code = 0;
};

// Non-aggregate with converting constructor from int64_t — should still work
// Note: we deliberately use int64_t (not bool) to avoid the pre-existing
// const char* -> bool implicit conversion issue, where T(bool) matches
// the const char* string branch before reaching the bool branch.
struct FromInt
{
    int64_t value;
    FromInt(int64_t v) : value(v) {}  // NOLINT implicit by design
};

// Aggregate with yyjson::value field (like WithJsonValueDto)
struct WithJsonValueAggregate
{
    int32_t version = 1;
    yyjson::value data;
    std::string status = "ok";
};

// --- Tests ---

// Aggregate + uint JSON → must throw bad_cast (not silently narrow)
TEST(AggregateFallback, UintToAggregateThrows)
{
    using namespace yyjson;
    auto doc = reader::read("42");
    EXPECT_THROW(cast<SnapshotDto>(doc), bad_cast);
}

// Aggregate + sint JSON → must throw bad_cast
TEST(AggregateFallback, SintToAggregateThrows)
{
    using namespace yyjson;
    auto doc = reader::read("-100");
    EXPECT_THROW(cast<SnapshotDto>(doc), bad_cast);
}

// Aggregate + bool JSON → must throw bad_cast
TEST(AggregateFallback, BoolToAggregateThrows)
{
    using namespace yyjson;
    auto doc = reader::read("true");
    EXPECT_THROW(cast<SnapshotDto>(doc), bad_cast);
}

// Aggregate with bool first field + bool JSON → must throw bad_cast
TEST(AggregateFallback, BoolToBoolFirstAggregateThrows)
{
    using namespace yyjson;
    auto doc = reader::read("true");
    EXPECT_THROW(cast<BoolFirstDto>(doc), bad_cast);
}

// Aggregate with bool first field + int JSON → must throw bad_cast
TEST(AggregateFallback, IntToBoolFirstAggregateThrows)
{
    using namespace yyjson;
    auto doc = reader::read("1");
    EXPECT_THROW(cast<BoolFirstDto>(doc), bad_cast);
}

// Normal object round-trip for SnapshotDto still works
TEST(AggregateFallback, ObjectRoundTripWorks)
{
    using namespace yyjson;
    SnapshotDto original{.version = 2, .source_entry_id = 999, .name = "test"};
    auto str = writer::value(original).write();
    auto restored = cast<SnapshotDto>(*reader::read(str).as_object());
    EXPECT_EQ(restored.version, 2);
    EXPECT_EQ(restored.source_entry_id, 999);
    EXPECT_EQ(restored.name, "test");
}

// Object round-trip with WithJsonValueAggregate still works
TEST(AggregateFallback, WithJsonValueObjectRoundTrip)
{
    using namespace yyjson;
    writer::object inner;
    inner.emplace("key", "val");
    WithJsonValueAggregate original{
        .version = 3,
        .data = writer::value(std::move(inner)),
        .status = "running"};
    auto str = writer::value(original).write();
    EXPECT_TRUE(str.find(R"("version":3)") != std::string::npos);
    EXPECT_TRUE(str.find(R"("key":"val")") != std::string::npos);
    EXPECT_TRUE(str.find(R"("status":"running")") != std::string::npos);
}

// Non-aggregate type with int64_t constructor — should still work
TEST(AggregateFallback, NonAggregateFromIntWorks)
{
    using namespace yyjson;
    auto doc = reader::read("12345");
    auto result = cast<FromInt>(doc);
    EXPECT_EQ(result.value, 12345);
}

// Large uint value that would overflow int32_t — still throws for aggregate
TEST(AggregateFallback, LargeUintToAggregateThrows)
{
    using namespace yyjson;
    auto doc = reader::read("9999999999");
    EXPECT_THROW(cast<SnapshotDto>(doc), bad_cast);
}

// Verify the fix: aggregate types are correctly rejected by the is_aggregate guard
TEST(AggregateFallback, StaticAssertAggregateCheck)
{
    static_assert(std::is_aggregate_v<SnapshotDto>);
    static_assert(std::is_aggregate_v<BoolFirstDto>);
    static_assert(!std::is_aggregate_v<FromInt>);
}
