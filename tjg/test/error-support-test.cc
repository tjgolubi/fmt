#include "tjg/error_support.h"

#include <stdexcept>

#include "test-support.h"

namespace {
constexpr bool valid_checks() {
  TJG_ASSERT(true, "assert");
  TJG_ENFORCE(true, "enforce");
  TJG_EXPECTS(true, "expects");
  TJG_ENSURES(true, "ensures");
  return true;
}
static_assert(valid_checks());
}  // namespace

TEST(error_support_test, evaluates_condition_once) {
  int count = 0;
  TJG_ASSERT(++count == 1, "assert");
#ifdef NDEBUG
  EXPECT_EQ(count, 0);
#else
  EXPECT_EQ(count, 1);
#endif
  count = 0;
  TJG_ENFORCE(++count == 1, "enforce");
  TJG_EXPECTS(++count == 2, "expects");
  TJG_ENSURES(++count == 3, "ensures");
  EXPECT_EQ(count, 3);
}

TEST(error_support_test, failure_diagnostics) {
#if GTEST_HAS_DEATH_TEST
#ifndef NDEBUG
  EXPECT_DEATH(TJG_ASSERT(false, "assert diagnostic"), "assert diagnostic");
#else
  TJG_ASSERT(false, "disabled assertion");
#endif
  EXPECT_DEATH(TJG_ENFORCE(false, "enforce diagnostic"), "enforce diagnostic");
  EXPECT_DEATH(TJG_EXPECTS(false, "expects diagnostic"), "expects diagnostic");
  EXPECT_DEATH(TJG_ENSURES(false, "ensures diagnostic"), "ensures diagnostic");
#endif
}

TEST(error_support_test, throws_exception) {
  EXPECT_THROW(TJG_THROW(std::runtime_error("throw diagnostic")),
               std::runtime_error);
}
