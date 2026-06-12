#include <gtest/gtest.h>
#include <string>
#include "cpp_yyjson.hpp"

// ---- Test structs (file scope with prefix to avoid ODR / boost::pfr C7631) ----

struct NameTransformIdentityStruct
{
    int user_name;
    bool is_active;
};

struct NameTransformCamelStruct
{
    int user_name;
    bool is_active;
};

struct NameTransformPrefixStruct
{
    int m_user_name;
    bool p_is_active;
};

struct NameTransformSuffixStruct
{
    int value_;
    std::string name_;
};

struct NameTransformCombinedStruct
{
    int m_first_name_;
    std::string m_last_name_;
};

// ---- field_name_rule specializations (must be in global or yyjson namespace) ----

template <>
struct yyjson::field_name_rule<NameTransformCamelStruct>
{
    using type = yyjson::snake_to_camel_transform;
};

template <>
struct yyjson::field_name_rule<NameTransformPrefixStruct>
{
    using type = yyjson::snake_to_camel_transform;
    static constexpr std::array<std::string_view, 2> prefixes{"m_", "p_"};
};

template <>
struct yyjson::field_name_rule<NameTransformSuffixStruct>
{
    using type = yyjson::identity_transform;
    static constexpr std::array<std::string_view, 1> suffixes{"_"};
};

template <>
struct yyjson::field_name_rule<NameTransformCombinedStruct>
{
    using type = yyjson::snake_to_camel_transform;
    static constexpr std::array<std::string_view, 1> prefixes{"m_"};
    static constexpr std::array<std::string_view, 1> suffixes{"_"};
};

// ---- Identity tests ----

TEST(NameTransform, IdentitySerialization)
{
    using namespace yyjson;

    NameTransformIdentityStruct s{.user_name = 42, .is_active = true};
    auto str = writer::value(s).write();

    EXPECT_TRUE(str.find("\"user_name\"") != std::string::npos);
    EXPECT_TRUE(str.find("\"is_active\"") != std::string::npos);
}

TEST(NameTransform, IdentityRoundTrip)
{
    using namespace yyjson;

    NameTransformIdentityStruct original{.user_name = 42, .is_active = true};
    auto str = writer::value(original).write();

    auto val = read(str);
    auto restored = cast<NameTransformIdentityStruct>(*val.as_object());

    EXPECT_EQ(restored.user_name, 42);
    EXPECT_EQ(restored.is_active, true);
}

// ---- snake_to_camel tests ----

TEST(NameTransform, SnakeToCamelSerialization)
{
    using namespace yyjson;

    NameTransformCamelStruct s{.user_name = 42, .is_active = true};
    auto str = writer::value(s).write();

    EXPECT_TRUE(str.find("\"userName\"") != std::string::npos);
    EXPECT_TRUE(str.find("\"isActive\"") != std::string::npos);
    EXPECT_TRUE(str.find("\"user_name\"") == std::string::npos);
    EXPECT_TRUE(str.find("\"is_active\"") == std::string::npos);
}

TEST(NameTransform, SnakeToCamelRoundTrip)
{
    using namespace yyjson;

    NameTransformCamelStruct original{.user_name = 42, .is_active = true};
    auto str = writer::value(original).write();

    auto val = read(str);
    auto restored = cast<NameTransformCamelStruct>(*val.as_object());

    EXPECT_EQ(restored.user_name, 42);
    EXPECT_EQ(restored.is_active, true);
}

TEST(NameTransform, SnakeToCamelDeserialization)
{
    using namespace yyjson;

    const char* json_str = R"({"userName": 99, "isActive": false})";
    auto val = read(json_str);
    auto restored = cast<NameTransformCamelStruct>(*val.as_object());

    EXPECT_EQ(restored.user_name, 99);
    EXPECT_EQ(restored.is_active, false);
}

// ---- Prefix stripping tests ----

TEST(NameTransform, PrefixStripping)
{
    using namespace yyjson;

    NameTransformPrefixStruct s{.m_user_name = 10, .p_is_active = false};
    auto str = writer::value(s).write();

    EXPECT_TRUE(str.find("\"userName\"") != std::string::npos);
    EXPECT_TRUE(str.find("\"isActive\"") != std::string::npos);
}

TEST(NameTransform, PrefixRoundTrip)
{
    using namespace yyjson;

    NameTransformPrefixStruct original{.m_user_name = 10, .p_is_active = false};
    auto str = writer::value(original).write();

    auto val = read(str);
    auto restored = cast<NameTransformPrefixStruct>(*val.as_object());

    EXPECT_EQ(restored.m_user_name, 10);
    EXPECT_EQ(restored.p_is_active, false);
}

// ---- Suffix stripping tests ----

TEST(NameTransform, SuffixStripping)
{
    using namespace yyjson;

    NameTransformSuffixStruct s{.value_ = 42, .name_ = "test"};
    auto str = writer::value(s).write();

    EXPECT_TRUE(str.find("\"value\"") != std::string::npos);
    EXPECT_TRUE(str.find("\"name\"") != std::string::npos);
}

TEST(NameTransform, SuffixRoundTrip)
{
    using namespace yyjson;

    NameTransformSuffixStruct original{.value_ = 42, .name_ = "test"};
    auto str = writer::value(original).write();

    auto val = read(str);
    auto restored = cast<NameTransformSuffixStruct>(*val.as_object());

    EXPECT_EQ(restored.value_, 42);
    EXPECT_EQ(restored.name_, "test");
}

// ---- Combined pipeline tests ----

TEST(NameTransform, CombinedPipeline)
{
    using namespace yyjson;

    NameTransformCombinedStruct s{.m_first_name_ = 1, .m_last_name_ = "Smith"};
    auto str = writer::value(s).write();

    EXPECT_TRUE(str.find("\"firstName\"") != std::string::npos);
    EXPECT_TRUE(str.find("\"lastName\"") != std::string::npos);
}

TEST(NameTransform, CombinedRoundTrip)
{
    using namespace yyjson;

    NameTransformCombinedStruct original{.m_first_name_ = 1, .m_last_name_ = "Smith"};
    auto str = writer::value(original).write();

    auto val = read(str);
    auto restored = cast<NameTransformCombinedStruct>(*val.as_object());

    EXPECT_EQ(restored.m_first_name_, 1);
    EXPECT_EQ(restored.m_last_name_, "Smith");
}
