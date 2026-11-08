#include "tjg/color.h"
#include "test-support.h"

TEST(color_wide_test, format) {
#if TJG_COLOR_COMPACT_SGR == 1
  EXPECT_EQ(tjg::format(fg(tjg::rgb(255, 20, 30)), L"rgb(255,20,30) wide"),
            L"\x1b[38;2;255;20;30mrgb(255,20,30) wide\x1b[0m");
  const auto style = tjg::text_style{tjg::emphasis::bold, tjg::rgb{0, 9, 255},
                                     tjg::cube_color{0}};
  EXPECT_EQ(tjg::format(style, L"wide"),
            L"\x1b[1;38;2;0;9;255;48;5;16mwide\x1b[0m");
  EXPECT_EQ(std::format(L"{:>5}plain", tjg::styled(42, style)),
            L"\x1b[1;38;2;0;9;255;48;5;16m   42\x1b[0mplain");
#else
  EXPECT_EQ(tjg::format(fg(tjg::rgb(255, 20, 30)), L"rgb(255,20,30) wide"),
            L"\x1b[38;2;255;020;030mrgb(255,20,30) wide\x1b[0m");
#endif
}
