/// @file
/// SGR Support.
/// @copyright 2026 Terry Golubiewski, all rights reserved.

#pragma once

#include <tjg/color/text_style.hpp>
#include <tjg/color/colors.hpp>
#include "emphasis.hpp"
#include <tjg/inline_buffer.hpp>
#include "error_support.hpp"
#include <tjg/utility.hpp>

#include <string>
#include <string_view>

#include <array>
#include <variant>
#include <utility>

#include <cstddef>
#include <cstdint>

namespace tjg::color {

/// Upper bound on style SGR output length.
constexpr auto sgr_max_length = std::size_t{60};

/// Length of the terminal-reset SGR sequence.
constexpr auto sgr_reset_length = std::size_t{4};

template<CharType Char>
using basic_sgr_buffer = basic_inline_buffer<Char, sgr_max_length>;

using sgr_buffer = basic_sgr_buffer<char>;

/**
 * Returns the ANSI SGR sequence that resets colors and emphasis to terminal
 * defaults. The returned view refers to static storage.
 */
template<CharType Char = char>
[[nodiscard]] constexpr
auto sgr_reset() noexcept -> std::basic_string_view<Char> {
  static constexpr auto reset =
                      std::array<Char, sgr_reset_length>{'\x1b', '[', '0', 'm'};
  return {reset.data(), reset.size()};
}

namespace detail {

inline constexpr const char* decimal_strings[256] = {
    "0",   "1",   "2",   "3",   "4",   "5",   "6",   "7",   "8",   "9",
   "10",  "11",  "12",  "13",  "14",  "15",  "16",  "17",  "18",  "19",
   "20",  "21",  "22",  "23",  "24",  "25",  "26",  "27",  "28",  "29",
   "30",  "31",  "32",  "33",  "34",  "35",  "36",  "37",  "38",  "39",
   "40",  "41",  "42",  "43",  "44",  "45",  "46",  "47",  "48",  "49",
   "50",  "51",  "52",  "53",  "54",  "55",  "56",  "57",  "58",  "59",
   "60",  "61",  "62",  "63",  "64",  "65",  "66",  "67",  "68",  "69",
   "70",  "71",  "72",  "73",  "74",  "75",  "76",  "77",  "78",  "79",
   "80",  "81",  "82",  "83",  "84",  "85",  "86",  "87",  "88",  "89",
   "90",  "91",  "92",  "93",  "94",  "95",  "96",  "97",  "98",  "99",
  "100", "101", "102", "103", "104", "105", "106", "107", "108", "109",
  "110", "111", "112", "113", "114", "115", "116", "117", "118", "119",
  "120", "121", "122", "123", "124", "125", "126", "127", "128", "129",
  "130", "131", "132", "133", "134", "135", "136", "137", "138", "139",
  "140", "141", "142", "143", "144", "145", "146", "147", "148", "149",
  "150", "151", "152", "153", "154", "155", "156", "157", "158", "159",
  "160", "161", "162", "163", "164", "165", "166", "167", "168", "169",
  "170", "171", "172", "173", "174", "175", "176", "177", "178", "179",
  "180", "181", "182", "183", "184", "185", "186", "187", "188", "189",
  "190", "191", "192", "193", "194", "195", "196", "197", "198", "199",
  "200", "201", "202", "203", "204", "205", "206", "207", "208", "209",
  "210", "211", "212", "213", "214", "215", "216", "217", "218", "219",
  "220", "221", "222", "223", "224", "225", "226", "227", "228", "229",
  "230", "231", "232", "233", "234", "235", "236", "237", "238", "239",
  "240", "241", "242", "243", "244", "245", "246", "247", "248", "249",
  "250", "251", "252", "253", "254", "255"
};  // decimal_strings

class ansi_color_escape {

  // Maximum color SGR is "\x1b[38;2;255;255;255m"
  // Maximum emphasis SGR is "\x1b[1;2;3;4;5;7;8;9m"
  inline_buffer<20> _buf;

  constexpr void append(char c) noexcept { _buf.push_back(c); }

  constexpr void append(const char* s) noexcept { _buf.append(s); }

  constexpr void append(const char* s, char c) noexcept {
    append(s);
    append(c);
  }

  constexpr void append(uint8_t n, char c) noexcept {
    append(decimal_strings[n]);
    append(c);
  }

  constexpr void append(no_color_t, bool) noexcept {}

  constexpr void append(term_color clr, bool is_bg) noexcept {
    constexpr auto BrightBlack =
        static_cast<int>(std::to_underlying(term_color::bright_black));
    auto value = static_cast<int>(std::to_underlying(clr));
    value += (value < BrightBlack) ? 30 : 90 - BrightBlack;

    // Background ASCII codes are the same as the foreground ones + 10.
    if (is_bg) value += 10;
    if (value >= 100) {
      append('1');
      value %= 100;
    }
    append(narrow_cast<char>('0' + value / 10));
    append(narrow_cast<char>('0' + value % 10));
    append(';');
  }

  constexpr void append(rgb clr, bool is_bg) noexcept {
    append("38;2;");
    if (is_bg) *_buf.data() = '4';
    append(clr.r, ';');
    append(clr.g, ';');
    append(clr.b, ';');
  }

  constexpr void append(cube_color clr, bool is_bg) noexcept {
    append("38;5;");
    if (is_bg) *_buf.data() = '4';
    auto index = narrow_cast<uint8_t>(clr.index() + 16);
    append(index, ';');
  }

  constexpr void append(gray_color clr, bool is_bg) noexcept {
    append("38;5;");
    if (is_bg) *_buf.data() = '4';
    auto index = narrow_cast<uint8_t>(clr.index() + 232);
    append(index, ';');
  }

  struct color_visitor {
    ansi_color_escape& escape;
    bool is_bg = false;

    constexpr void operator()(const no_color_t&) const noexcept {}

    template<typename Color>
    constexpr void operator()(Color clr) const noexcept
    { escape.append(clr, is_bg); }
  }; // color_visitor

 public:
  constexpr ansi_color_escape(color_t text_color, bool is_bg) noexcept {
    if (!text_color.valueless_by_exception())
      std::visit(color_visitor{*this, is_bg}, text_color);
  }

  constexpr ansi_color_escape(emphasis em) noexcept {
    unsigned bits = std::to_underlying(em);
    if (bits == 0) return;

    constexpr char sgr[] = {'1', '2', '3', '4', '5', '7', '8', '9'};

    for (int i = 0; i < 8; ++i) {
      if (!(bits & (1u << i))) continue;
      append(sgr[i]);
      append(';');
    }
  }

  constexpr operator const char*() const noexcept { return _buf.data(); }

  [[nodiscard]] constexpr auto begin() const noexcept -> const char*
  { return _buf.data(); }

  [[nodiscard]] constexpr auto end() const noexcept -> const char*
  { return _buf.data() + _buf.size(); }

  [[nodiscard]] constexpr auto view() const noexcept -> std::string_view
  { return {_buf.data(), _buf.size()}; }
};  // ansi_color_escape

[[nodiscard]] constexpr
auto make_foreground(color_t foreground) noexcept -> ansi_color_escape
{ return ansi_color_escape{foreground, false}; }

[[nodiscard]] constexpr
auto make_background(color_t background) noexcept -> ansi_color_escape
{ return ansi_color_escape{background, true}; }

[[nodiscard]] constexpr
auto make_emphasis(emphasis em) noexcept -> ansi_color_escape
{ return ansi_color_escape{em}; }

template<typename Buffer>
constexpr inline void reset_color(Buffer& buffer) {
  constexpr auto reset_color = sgr_reset();
  buffer.append(reset_color.data(), reset_color.data() + reset_color.size());
}

template<typename Buffer>
constexpr void append_sgr_raw(Buffer& buf, text_style ts) {
  if (ts.empty()) return;
  if (ts.is_absolute()) {
    buf.push_back('0');
    buf.push_back(';');
  }
  auto emphasis = make_emphasis(ts.emph());
  buf.append(emphasis.view());
  auto foreground = make_foreground(ts.fg());
  buf.append(foreground.view());
  auto background = make_background(ts.bg());
  buf.append(background.view());
}  // append_sgr_raw

template<typename Buffer>
constexpr void append_sgr(Buffer& buf, text_style ts) {
  if (ts.empty()) return;
  buf.push_back('\x1b');
  buf.push_back('[');
  append_sgr_raw(buf, ts);
  buf.data()[buf.size() - 1] = 'm';
}  // append_sgr

} // detail

/**
 * Returns one ANSI SGR escape sequence for the specified foreground,
 * background, and emphasis, with unpadded parameters. Absolute styles start
 * with a reset parameter. An absolute-only style emits a reset.
 *
 * Returns an empty string for an empty style.
 * Unspecified color channels emit no color parameters. For absolute styles,
 * the leading reset selects terminal defaults for those channels.
 */
[[nodiscard]] inline
auto to_sgr(const text_style& ts) TJG_NOEXCEPT_UNLESS_BAD_ALLOC -> std::string {
  if (ts.empty()) return {};
  auto sgr = sgr_buffer{};
  detail::append_sgr(sgr, ts);
  return std::string{sgr.view()};
}

namespace detail {

template<typename Buffer>
constexpr void append_transition_code(Buffer& buf, uint8_t code) {
  for (auto cp = decimal_strings[code]; *cp; ++cp)
    buf.push_back(*cp);
  buf.push_back(';');
}

template<typename Buffer>
constexpr
void append_transition(Buffer& buf, text_style target, text_style initial) {
  auto before = static_cast<unsigned>(std::to_underlying(initial.emph()));
  auto after  = static_cast<unsigned>(std::to_underlying(target.emph()));
  auto removed = before & ~after;
  // SGR 22 clears both intensity attributes; reapply any surviving bit.
  if ((removed & 3u) != 0) {
    append_transition_code(buf, 22);
    before &= ~3u;
  }
  constexpr uint8_t off[] = {23, 24, 25, 27, 28, 29};
  for (unsigned i = 0; i < 6; ++i) {
    if (removed & (4u << i)) append_transition_code(buf, off[i]);
  }
  after &= ~before;
  if (after != 0) {
    auto sgr = make_emphasis(enum_cast<emphasis>(after));
    buf.append(sgr.view());
  }
  auto fg = target.fg();
  if (!is_unspecified(fg)) {
    if (fg != initial.fg()) {
      auto sgr = make_foreground(fg);
      buf.append(sgr.view());
    }
  } else if (initial.has_fg()) {
    append_transition_code(buf, 39);
  }
  auto bg = target.bg();
  if (!is_unspecified(bg)) {
    if (bg != initial.bg()) {
      auto sgr = make_background(bg);
      buf.append(sgr.view());
    }
  } else if (initial.has_bg()) {
    append_transition_code(buf, 49);
  }
}  // append_transition

template<typename Buffer>
constexpr
void append_sgr_transition(Buffer& buf, text_style target, text_style initial) {
  if (target.is_absolute()) return append_sgr(buf, target);
  initial.absolute(false);
  if (target == initial) return;
  auto relative = sgr_buffer{};
  append_transition(relative, target, initial);
  if (relative.empty()) return;
  auto absolute = sgr_buffer{};
  append_sgr_raw(absolute, text_style{target}.absolute());
  const auto& parameters = (absolute.size() < relative.size()) ? absolute
                                                               : relative;
  buf.push_back('\x1b');
  buf.push_back('[');
  buf.append(parameters.view());
  buf.data()[buf.size() - 1] = 'm';
} // append_sgr_transition

} // detail

/**
 * Returns the ANSI SGR transition from `initial` to `ts`, without a trailing
 * reset. Both arguments describe complete terminal states: empty color channels
 * mean terminal defaults, and absent emphasis means off. No colors are
 * inherited. The absolute flag on `initial` is ignored. An absolute target
 * always resets and delegates to the one-argument overload, regardless of the
 * initial state. Otherwise returns an empty string when no changes are needed.
 *
 * For relative targets, uses one unpadded sequence, choosing the shorter of
 * selective changes and reset followed by the target attributes.
 * Equal lengths prefer selective changes.
 */
[[nodiscard]] inline
auto to_sgr(const text_style& target, const text_style& initial)
  TJG_NOEXCEPT_UNLESS_BAD_ALLOC -> std::string
{
  auto sgr = sgr_buffer{};
  detail::append_sgr_transition(sgr, target, initial);
  return std::string{sgr.view()};
}

/**
 * Returns a copy of `sv` wrapped in the SGR sequences for `ts` and a terminal
 * reset. Empty text returns an empty string; an empty style returns just the
 * text. The text is copied without formatting or interpreting embedded escapes.
 * The reset restores terminal defaults, not a previously active style.
 */
 template<CharType Char>
[[nodiscard]] inline
auto wrap_sgr(const text_style& ts, std::basic_string_view<Char> sv)
  -> std::basic_string<Char>
{
  if (sv.empty()) return {};
  if (ts.empty()) return std::basic_string<Char>{sv};
  constexpr auto reset = sgr_reset();
  auto sgr = sgr_buffer{};
  detail::append_sgr(sgr, ts);
  auto result = std::basic_string<Char>{};
  result.reserve(sgr.size() + sv.size() + reset.size());
  result.append(sgr.begin(), sgr.end());
  result.append(sv);
  result.append(reset.begin(), reset.end());
  return result;
}  // wrap_sgr

/// Narrow-character convenience overload of `wrap_sgr()`.
[[nodiscard]] inline
auto wrap_sgr(const text_style& ts, std::string_view sv) -> std::string
{ return wrap_sgr<char>(ts, sv); }

} // tjg::color
