// Port of support/printable.py, adapted from Rust's Unicode table generator.
// https://github.com/rust-lang/rust/blob/master/library/core/src/unicode/printable.py

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

using code_point = std::int32_t;
constexpr code_point unicode_limit = 0x110000;
constexpr code_point plane_size = 0x10000;

struct code_point_range {
  code_point first;
  code_point end;
};

struct plane_tables {
  std::vector<code_point> singletons;
  std::vector<code_point_range> ranges;
};

auto take_field(std::string_view& row) -> std::string_view {
  const auto separator = row.find(';');
  if (separator == std::string_view::npos)
    throw std::runtime_error{"incomplete UnicodeData.txt record"};
  const auto field = row.substr(0, separator);
  row.remove_prefix(separator + 1);
  return field;
}

auto parse_code_point(std::string_view field) -> code_point {
  if (field.empty())
    throw std::runtime_error{"missing code point"};

  code_point value{};
  const auto end = field.data() + field.size();
  const auto [ptr, error] = std::from_chars(field.data(), end, value, 16);
  if (error != std::errc{} || ptr != end || value < 0 || value >= unicode_limit)
    throw std::runtime_error{"invalid code point: " + std::string{field}};
  return value;
}

auto needs_escape(std::string_view category) -> bool {
  constexpr std::array<std::string_view, 8> categories{
    "Cc", "Cf", "Cs", "Co", "Cn", "Zl", "Zp", "Zs",
  };
  return std::ranges::find(categories, category) != categories.end();
}

auto read_unicode_data(std::istream& input) -> std::vector<unsigned char> {
  std::vector<unsigned char> nonprintable(unicode_limit, 1);
  code_point previous = -1;
  code_point first = -1;
  auto first_category = std::string{};

  for (std::string line; std::getline(input, line);) {
    if (line.empty() || line == "\r") continue;

    auto row = std::string_view{line};
    const auto point = parse_code_point(take_field(row));
    const auto name = take_field(row);
    const auto category = take_field(row);
    if (point <= previous)
      throw std::runtime_error{"UnicodeData.txt code points are out of order"};
    previous = point;

    if (first >= 0) {
      if (!name.ends_with(", Last>") || category != first_category)
        throw std::runtime_error{"unmatched UnicodeData.txt range"};
      std::fill(nonprintable.data() + first, nonprintable.data() + point + 1,
                needs_escape(category));
      first = -1;
    } else if (name.ends_with(", First>")) {
      first = point;
      first_category = category;
    } else if (name.ends_with(", Last>")) {
      throw std::runtime_error{"unexpected Last record"};
    } else {
      nonprintable[static_cast<std::size_t>(point)] = needs_escape(category);
    }
  }

  if (input.bad() || !input.eof())
    throw std::runtime_error{"error reading UnicodeData.txt"};
  if (first >= 0 || previous < 0)
    throw std::runtime_error{"incomplete UnicodeData.txt"};

  nonprintable[0x20] = 0;
  return nonprintable;
}

void write_hex_byte(int value) {
  std::print("0x{:02x},", value);
}

void write_singletons(const std::vector<code_point>& points, int plane) {
  std::println("inline constexpr singleton singletons{}[] = {{", plane);
  for (auto first = points.begin(); first != points.end();) {
    const auto upper = *first >> 8;
    const auto end = std::find_if(first, points.end(), [upper](code_point point) {
      return (point >> 8) != upper;
    });
    std::println("    {{0x{:02x}, {}}},", upper, end - first);
    first = end;
  }

  std::println("}};\n\ninline constexpr unsigned char singletons{}_lower[] = {{",
             plane);
  for (std::size_t index = 0; index < points.size(); ++index) {
    std::print("{}", index % 8 == 0 ? "    " : " ");
    write_hex_byte(points[index] & 0xff);
    if (index % 8 == 7 || index + 1 == points.size()) std::println();
  }
  std::println("}};\n");
}

void write_length(code_point length) {
  if (length < 0 || length >= 0x8000)
    throw std::runtime_error{"run length exceeds the table encoding"};

  if (length > 0x7f) {
    write_hex_byte(0x80 | (length >> 8));
    std::print(" ");
  }
  write_hex_byte(length & 0xff);
}

void write_ranges(const std::vector<code_point_range>& ranges, int plane) {
  std::println("inline constexpr unsigned char normal{}[] = {{", plane);
  code_point previous_end = 0;
  for (const auto& range : ranges) {
    std::print("    ");
    write_length(range.first - previous_end);
    std::print(" ");
    write_length(range.end - range.first);
    std::println();
    previous_end = range.end;
  }
  std::println("}};\n");
}

void write_tables(const std::vector<unsigned char>& nonprintable) {
  std::array<plane_tables, 2> planes;
  std::vector<code_point_range> supplementary;

  for (code_point first = 0; first < unicode_limit;) {
    if (nonprintable[static_cast<std::size_t>(first)] == 0) {
      ++first;
      continue;
    }

    const auto plane = first / plane_size;
    const auto limit = plane < 2 ? (plane + 1) * plane_size : unicode_limit;
    auto end = first + 1;
    while (end < limit && nonprintable[static_cast<std::size_t>(end)] != 0)
      ++end;

    if (plane >= 2) {
      supplementary.push_back({first, end});
    } else {
      auto& table = planes[static_cast<std::size_t>(plane)];
      const auto start = first % plane_size;
      const auto length = end - first;
      if (length <= 2) {
        for (code_point offset = 0; offset < length; ++offset)
          table.singletons.push_back(start + offset);
      } else {
        table.ranges.push_back({start, start + length});
      }
    }
    first = end;
  }

  std::print("{}", R"(/**
 * @file UnicodeData.hpp
 * @brief Unicode printability tables.
 *
 * Generated by support/unicode/GenUnicodeTables.cpp from
 * support/unicode/UnicodeData.txt. Do not modify this file directly.
 * See support/unicode/README.md for download and regeneration instructions.
 */

#include <cstdint>
#pragma once

namespace tjg::color::detail::unicode::detail {

struct singleton {
  unsigned char upper;
  unsigned char lower_count;
};

struct code_point_range {
  std::uint32_t first;
  std::uint32_t end;
};

)");
  write_singletons(planes[0].singletons, 0);
  write_singletons(planes[1].singletons, 1);
  write_ranges(planes[0].ranges, 0);
  write_ranges(planes[1].ranges, 1);

  std::println("inline constexpr code_point_range nonprintable_ranges[] = {{");
  for (const auto& [first, end] : supplementary)
    std::println("  {{0x{:x}, 0x{:x}}},", first, end);
  std::println("}};\n\n}} // tjg::color::detail::unicode");
}

} // namespace

int main(int argc, char** argv) try {
  if (argc != 2) {
    std::println(stderr, "Usage: {} UnicodeData.txt", argv[0]);
    return 2;
  }

  std::ifstream input{argv[1]};
  if (!input)
    throw std::runtime_error{"cannot open " + std::string{argv[1]}};

  write_tables(read_unicode_data(input));
} catch (const std::exception& error) {
  std::println(stderr, "{}", error.what());
  return 1;
}
