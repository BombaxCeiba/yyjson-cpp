#define CPPYYJSON_DEFAULT_TRANSFORM ::yyjson::snake_to_camel_transform

#include <gtest/gtest.h>
#include <string>
#include "cpp_yyjson.hpp"

// ---- Test structs ----

namespace
{

struct DefaultCamelStruct
{
    int user_name;
    bool is_active;
};

struct OverrideStruct
{
    int value_;
    std::string name_;
};

} // anonymous namespace

// OverrideStruct explicitly uses identity (overrides the macro default)
template <>
struct yyjson::field_name_rule<OverrideStruct>
{
    using type = yyjson::identity_transform;
    static constexpr std::array<std::string_view, 1> suffixes{"_"};
};

// ---- Tests ----

TEST(DefaultTransform, InheritedRule)
{
    using namespace yyjson;

    DefaultCamelStruct s{.user_name = 42, .is_active = true};
    auto str = writer::value(s).write();

    EXPECT_TRUE(str.find("\"userName\"") != std::string::npos);
    EXPECT_TRUE(str.find("\"isActive\"") != std::string::npos);
    EXPECT_TRUE(str.find("\"user_name\"") == std::string::npos);
    EXPECT_TRUE(str.find("\"is_active\"") == std::string::npos);
}

TEST(DefaultTransform, InheritedRoundTrip)
{
    using namespace yyjson;

    DefaultCamelStruct original{.user_name = 42, .is_active = true};
    auto str = writer::value(original).write();

    auto val = read(str);
    auto restored = cast<DefaultCamelStruct>(*val.as_object());

    EXPECT_EQ(restored.user_name, 42);
    EXPECT_EQ(restored.is_active, true);
}

TEST(DefaultTransform, InheritedDeserialization)
{
    using namespace yyjson;

    const char* json_str = R"({"userName": 99, "isActive": false})";
    auto val = read(json_str);
    auto restored = cast<DefaultCamelStruct>(*val.as_object());

    EXPECT_EQ(restored.user_name, 99);
    EXPECT_EQ(restored.is_active, false);
}

TEST(DefaultTransform, ExplicitOverrideTakesPriority)
{
    using namespace yyjson;

    OverrideStruct s{.value_ = 10, .name_ = "test"};
    auto str = writer::value(s).write();

    EXPECT_TRUE(str.find("\"value\"") != std::string::npos);
    EXPECT_TRUE(str.find("\"name\"") != std::string::npos);
    EXPECT_TRUE(str.find("\"value_\"") == std::string::npos);
}

TEST(DefaultTransform, ExplicitOverrideRoundTrip)
{
    using namespace yyjson;

    OverrideStruct original{.value_ = 10, .name_ = "test"};
    auto str = writer::value(original).write();

    auto val = read(str);
    auto restored = cast<OverrideStruct>(*val.as_object());

    EXPECT_EQ(restored.value_, 10);
    EXPECT_EQ(restored.name_, "test");
}
