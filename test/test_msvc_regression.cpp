// Regression tests for MSVC-specific issues discovered in DuskAutoScript.
// These scenarios work on clang/GCC but previously failed on MSVC.

#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <vector>

#include "cpp_yyjson.hpp"

struct NestedDto { std::string name; int32_t count = 0; bool operator==(const NestedDto&) const = default; };
struct OptionalFieldsDto { std::string required_field; std::optional<int64_t> optional_int; std::optional<std::string> optional_string; bool operator==(const OptionalFieldsDto&) const = default; };
struct ComplexAggregateDto { int32_t id = 0; std::string name; std::optional<int64_t> version; NestedDto nested; std::vector<std::string> tags; std::optional<std::string> description; bool active = false; bool operator==(const ComplexAggregateDto&) const = default; };
struct WithJsonValueDto { std::string type; yyjson::value data; std::string status = "ok"; bool operator==(const WithJsonValueDto&) const = default; };
struct SignalsDto { bool succeeded = false; bool failed = false; bool cancelled = false; bool operator==(const SignalsDto&) const = default; };
struct DiagnosticDto { std::string severity; std::string code; std::string message; std::optional<std::string> path; std::optional<int64_t> provider_code; bool operator==(const DiagnosticDto&) const = default; };
struct ResultDto { int32_t version = 1; std::string status; std::vector<std::string> completed_tasks; bool stopped = false; std::vector<DiagnosticDto> diagnostics; SignalsDto signals; bool operator==(const ResultDto&) const = default; };

TEST(MsvcRegression, OptionalFieldsRoundTrip) {
    using namespace yyjson;
    OptionalFieldsDto original{.required_field = "test", .optional_int = 42, .optional_string = "hello"};
    auto str = yyjson::writer::value(original).write();
    EXPECT_EQ(str, R"({"required_field":"test","optional_int":42,"optional_string":"hello"})");
    auto restored = cast<OptionalFieldsDto>(*read(str).as_object());
    EXPECT_EQ(restored.required_field, "test");
    EXPECT_TRUE(restored.optional_int.has_value());
    EXPECT_EQ(*restored.optional_int, 42);
    EXPECT_TRUE(restored.optional_string.has_value());
    EXPECT_EQ(*restored.optional_string, "hello");
}

TEST(MsvcRegression, OptionalFieldsNullopt) {
    using namespace yyjson;
    OptionalFieldsDto original{.required_field = "test"};
    auto str = yyjson::writer::value(original).write();
    EXPECT_TRUE(str.find("required_field") != std::string::npos);
    auto restored = cast<OptionalFieldsDto>(*read(str).as_object());
    EXPECT_EQ(restored.required_field, "test");
    EXPECT_FALSE(restored.optional_int.has_value());
    EXPECT_FALSE(restored.optional_string.has_value());
}

TEST(MsvcRegression, NestedAggregateRoundTrip) {
    using namespace yyjson;
    ComplexAggregateDto original{.id = 1, .name = "test", .version = 3, .nested = {"inner", 5}, .tags = {"a", "b"}, .description = "desc", .active = true};
    auto str = yyjson::writer::value(original).write();
    auto restored = cast<ComplexAggregateDto>(*read(str).as_object());
    EXPECT_EQ(restored.id, 1);
    EXPECT_EQ(restored.name, "test");
    EXPECT_TRUE(restored.version.has_value()); EXPECT_EQ(*restored.version, 3);
    EXPECT_EQ(restored.nested.name, "inner"); EXPECT_EQ(restored.nested.count, 5);
    EXPECT_EQ(restored.tags.size(), 2u);
    EXPECT_TRUE(restored.description.has_value()); EXPECT_EQ(*restored.description, "desc");
    EXPECT_TRUE(restored.active);
}

TEST(MsvcRegression, WithJsonValueFieldSerialization) {
    using namespace yyjson;
    writer::object inner_obj;
    inner_obj.emplace("key", "value");
    WithJsonValueDto original{.type = "test", .data = writer::value(std::move(inner_obj)), .status = "ok"};
    auto str = yyjson::writer::value(original).write();
    EXPECT_TRUE(str.find(R"("type":"test")") != std::string::npos);
    EXPECT_TRUE(str.find(R"("key":"value")") != std::string::npos);
    EXPECT_TRUE(str.find(R"("status":"ok")") != std::string::npos);
}

TEST(MsvcRegression, DeeplyNestedDto) {
    using namespace yyjson;
    ResultDto original{.version = 1, .status = "completed", .completed_tasks = {"task1", "task2"}, .stopped = false, .diagnostics = {DiagnosticDto{.severity = "error", .code = "E001", .message = "fail", .path = "/file.cpp", .provider_code = 42}}, .signals = {.succeeded = true, .failed = false, .cancelled = false}};
    auto str = yyjson::writer::value(original).write();
    auto restored = cast<ResultDto>(*read(str).as_object());
    EXPECT_EQ(restored.version, 1);
    EXPECT_EQ(restored.status, "completed");
    EXPECT_EQ(restored.completed_tasks.size(), 2u);
    EXPECT_FALSE(restored.stopped);
    EXPECT_EQ(restored.diagnostics.size(), 1u);
    EXPECT_EQ(restored.diagnostics[0].severity, "error");
    EXPECT_EQ(restored.diagnostics[0].code, "E001");
    EXPECT_TRUE(restored.diagnostics[0].path.has_value());
    EXPECT_EQ(*restored.diagnostics[0].path, "/file.cpp");
    EXPECT_TRUE(restored.diagnostics[0].provider_code.has_value());
    EXPECT_EQ(*restored.diagnostics[0].provider_code, 42);
    EXPECT_TRUE(restored.signals.succeeded);
    EXPECT_FALSE(restored.signals.failed);
    EXPECT_FALSE(restored.signals.cancelled);
}

TEST(MsvcRegression, CopyStringWithOptionalFields) {
    using namespace yyjson;
    OptionalFieldsDto original{.required_field = std::string("test"), .optional_int = 42, .optional_string = std::string("hello")};
    auto str = yyjson::writer::value(original, copy_string).write();
    EXPECT_EQ(str, R"({"required_field":"test","optional_int":42,"optional_string":"hello"})");
}

TEST(MsvcRegression, EmptyAggregateNotSerialized) {
    using namespace yyjson;
    static_assert(!std::constructible_from<value, copy_string_t>);
    static_assert(!std::constructible_from<object, copy_string_t>);
}

TEST(MsvcRegression, NestedAggregateWithCopyString) {
    using namespace yyjson;
    ComplexAggregateDto original{.id = 1, .name = std::string("test"), .nested = {"inner", 5}, .description = std::string("desc"), .active = true};
    auto str = yyjson::writer::value(original, copy_string).write();
    auto restored = cast<ComplexAggregateDto>(*read(str).as_object());
    EXPECT_EQ(restored.name, "test");
    EXPECT_FALSE(restored.version.has_value());
    EXPECT_EQ(restored.nested.name, "inner");
    EXPECT_TRUE(restored.description.has_value());
    EXPECT_EQ(*restored.description, "desc");
}
