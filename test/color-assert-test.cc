#include "tjg/color.hpp"
#include "test-support.h"

#if GTEST_HAS_DEATH_TEST && !defined(NDEBUG)
TEST(color_assert_test, argument_ranges) {
  EXPECT_DEATH((void)(tjg::color::rgb{-1, 0, 0}), "channel out of range");
  EXPECT_DEATH((void)(tjg::color::gray_color{24}), "index out of range");
  EXPECT_DEATH((void)(tjg::color::cube_color{0, 6, 0}), "channel out of range");
  EXPECT_DEATH((void)(tjg::color::cube_color{216}), "index out of range");
  tjg::color::cube_color color;
  EXPECT_DEATH((void)color.b(-1), "channel out of range");
}
#endif
