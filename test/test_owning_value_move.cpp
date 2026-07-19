// Regression: owning yyjson::value (mutable_value_base<mutable_document>) must
// be movable/assignable so it can live in standard containers and be reordered
// by std::sort / std::swap.
//
// Before the fix, operator=(value&&) unconditionally used set_value (node
// content overwrite + cross-doc keepalive), which:
//   - accumulated the source's whole document into dst.doc_.children on every
//     move-assign (memory bloat under std::sort's repeated shuffling);
//   - dereferenced this->has_parent_ (a shared_ptr<bool>) that move-construction
//     leaves empty on the moved-from object -> null-pointer dereference when
//     std::sort's insertion sort move-assigns into a moved-from slot.
//
// The fix routes owning move-assign through document ownership transfer
// (doc_/val_/has_parent_) and makes get_has_parent() defensively rehydrate an
// empty shared_ptr. These tests pin both the contract and the crash scenario.
#include <algorithm>
#include <cassert>
#include <cpp_yyjson.hpp>
#include <gtest/gtest.h>
#include <type_traits>
#include <utility>
#include <vector>

// 编译期契约:owning value 必须可移动构造/赋值(nothrow)。用 is_*_v trait 而非
// concept(MSVC 对复杂模板的 concept/SFINAE 检查不可靠,见 test.cpp 在 MSVC 跳过)。
static_assert(std::is_move_constructible_v<yyjson::value>);
static_assert(std::is_move_assignable_v<yyjson::value>);
static_assert(std::is_nothrow_move_constructible_v<yyjson::value>);
static_assert(std::is_nothrow_move_assignable_v<yyjson::value>);

namespace
{
    yyjson::value make_int_value(int i)
    {
        yyjson::value v;
        v = i;
        return v;
    }
}

// 缺陷①:moved-from 对象被重新 move-assign 不应 null-deref。
TEST(OwningValueMove, MovedFromCanBeReassigned)
{
    auto a = make_int_value(1);
    auto b = make_int_value(2);
    auto tmp = std::move(a);   // a 变 moved-from(has_parent_ 被移空)
    a = std::move(b);          // 修复前:operator=(value&&) 解引用空 has_parent_ -> 💥
    EXPECT_EQ(a.as_sint().value_or(-1), 2);
    (void)tmp;
}

// 缺陷②:vector<value> + std::sort 不崩且结果正确。
TEST(OwningValueMove, VectorSortDoesNotCrash)
{
    std::vector<yyjson::value> v;
    for (int i = 3; i >= 0; --i)   // 逆序,确保 sort 触发 insertion 搬运
    {
        v.push_back(make_int_value(i));
    }
    std::sort(v.begin(), v.end(), [](const yyjson::value& x, const yyjson::value& y) {
        return x.as_sint().value_or(0) < y.as_sint().value_or(0);
    });
    ASSERT_EQ(v.size(), 4u);
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_EQ(v[i].as_sint().value_or(-1), i) << "index " << i;
    }
}

// 基本 move 语义:内容随所有权转移。
TEST(OwningValueMove, MoveTransfersContent)
{
    auto a = make_int_value(42);
    yyjson::value b = std::move(a);
    EXPECT_EQ(b.as_sint().value_or(-1), 42);
}
