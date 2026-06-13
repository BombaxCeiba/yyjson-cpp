// Regression test: constructing yyjson::object / yyjson::array from an
// aggregate type via function-style cast.
//
// MSVC cannot inherit constrained template constructors through `using base::base`,
// so mutable_object_base/mutable_array_base must re-declare the create_*_callable
// constructors explicitly. Before the fix this fails on MSVC cl with C2440.

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "cpp_yyjson.hpp"

// Mimics Das::GraphRuntime::Dto::GraphDocumentDto — an aggregate reflected via boost::pfr
struct AggregateDto
{
    int id;
    std::string name;
    bool active;
};

struct NestedDto
{
    int version;
    std::vector<int> tags;
    AggregateDto inner;
};

// object(aggregate) — function-style cast, exercises mutable_object_base ctor
TEST(AggregateCtor, ObjectFromAggregate)
{
    using namespace yyjson;
    AggregateDto dto{.id = 7, .name = "graph", .active = true};
    object obj(dto);

    const auto str = writer::value(std::move(obj)).write();
    EXPECT_TRUE(str.find("\"id\":7") != std::string::npos);
    EXPECT_TRUE(str.find("\"name\":\"graph\"") != std::string::npos);
    EXPECT_TRUE(str.find("\"active\":true") != std::string::npos);
}

// object(aggregate, copy_string) — copy_string tag overload
TEST(AggregateCtor, ObjectFromAggregateCopyString)
{
    using namespace yyjson;
    AggregateDto dto{.id = 9, .name = "copy", .active = false};
    object obj(dto, copy_string);

    const auto str = writer::value(std::move(obj)).write();
    EXPECT_TRUE(str.find("\"id\":9") != std::string::npos);
    EXPECT_TRUE(str.find("\"name\":\"copy\"") != std::string::npos);
}

// array(range) — exercises mutable_array_base ctor
TEST(AggregateCtor, ArrayFromVector)
{
    using namespace yyjson;
    std::vector<int> nums{1, 2, 3};
    array arr(nums);

    const auto str = writer::value(std::move(arr)).write();
    EXPECT_TRUE(str.find("[1,2,3]") != std::string::npos);
}

// Nested aggregate round-trip
TEST(AggregateCtor, NestedAggregateObject)
{
    using namespace yyjson;
    NestedDto dto{
        .version = 2,
        .tags = {10, 20},
        .inner = {.id = 99, .name = "inner", .active = true}};
    object obj(dto);

    const auto str = writer::value(std::move(obj)).write();
    EXPECT_TRUE(str.find("\"version\":2") != std::string::npos);
    EXPECT_TRUE(str.find("\"id\":99") != std::string::npos);
}
