// Regression: cast<yyjson::value> and aggregate reflection of DTOs with
// yyjson::value fields must deep-copy the JSON subtree.
//
// Before the fix, default_caster<yyjson::value> fell into the scalar string
// branch (because std::constructible_from<value, string_view> spuriously
// evaluated to true via inherited constructors), throwing
// "is not constructible from JSON string" for object/array input on MSVC
// (and for direct cast<value> on clang too). The library now deep-copies its
// own value-owning types ahead of the scalar/string branches.
#include <cassert>
#include <cpp_yyjson.hpp>

#include <gtest/gtest.h>
#include <string>
namespace yyjson_value_cast_test
{
    struct TaskDto
    {
        std::string   task_name;
        std::string   entry;
        yyjson::value pipeline_override;
    };
} // namespace yyjson_value_cast_test

template <>
struct yyjson::field_name_rule<yyjson_value_cast_test::TaskDto>
{
    using type = yyjson::snake_to_camel_transform;
};

TEST(ValueCast, DirectObjectSubtreeDeepCopies)
{
    auto parsed = yyjson::read(std::string_view(
        R"({"taskName":"T","entry":"E","pipelineOverride":{"Stage":{"value":"one"}}})"));
    auto v = yyjson::cast<yyjson::value>(parsed);
    EXPECT_TRUE(v.is_object());
}

TEST(ValueCast, DirectScalarSubtreeDeepCopies)
{
    auto parsed = yyjson::read(std::string_view("42"));
    auto v = yyjson::cast<yyjson::value>(parsed);
    EXPECT_TRUE(v.is_uint());
}

TEST(ValueCast, AggregateDtoWithValueField)
{
    auto parsed = yyjson::read(std::string_view(
        R"({"taskName":"T","entry":"E","pipelineOverride":{"Stage":{"value":"one"}}})"));
    auto t = yyjson::cast<yyjson_value_cast_test::TaskDto>(parsed);
    EXPECT_EQ(t.task_name, "T");
    EXPECT_EQ(t.entry, "E");
    EXPECT_TRUE(t.pipeline_override.is_object());
}

TEST(ValueCast, StringCastUnaffectedBySfinaeTightening)
{
    auto parsed = yyjson::read(std::string_view("\"hello\""));
    EXPECT_EQ(yyjson::cast<std::string>(parsed), "hello");
}
