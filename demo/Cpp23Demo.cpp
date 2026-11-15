#include <algorithm>
#include <format>
#include <initializer_list>
#include <print>
#include <string>
#include <string_view>

namespace {

struct span {
  std::string_view text;
  std::string_view sgr{};
};

using cell = std::initializer_list<span>;

enum class alignment { left, center, right };

void append_sgr(std::string& out, std::string_view params) {
  out += "\x1b[";
  out += params;
  out += 'm';
}

auto render_cell(cell parts, int width, alignment align,
                 std::string_view colors, std::string_view row) -> std::string
{
  // Lay out the plain text before inserting escape sequences.
  auto plain = std::string{};
  for (auto part : parts) plain += part.text;
  const auto clipped = std::format("{:.{}}", plain, width);

  auto padded = std::string{};
  switch (align) {
  case alignment::left:   padded = std::format("{:<{}}", clipped, width); break;
  case alignment::center: padded = std::format("{:^{}}", clipped, width); break;
  case alignment::right:  padded = std::format("{:>{}}", clipped, width); break;
  }
  const auto padding = padded.size() - clipped.size();
  const auto leading = (align == alignment::left)   ? 0
                     : (align == alignment::center) ? padding / 2 : padding;

  auto out = std::string{};
  auto set_column_style = [&] {
    append_sgr(out, colors);
    if (!row.empty()) append_sgr(out, row);
  };

  set_column_style();
  out.append(padded, 0, leading);
  auto remaining = clipped.size();
  for (auto part : parts) {
    if (remaining == 0) break;
    const auto count = std::min(remaining, part.text.size());
    if (count == 0) continue;
    if (!part.sgr.empty()) append_sgr(out, part.sgr);
    out += part.text.substr(0, count);
    remaining -= count;
    if (!part.sgr.empty()) {
      append_sgr(out, "0");
      set_column_style();
    }
  }
  out.append(padded, leading + clipped.size(), padding - leading);
  append_sgr(out, "0");
  if (!row.empty()) append_sgr(out, row);
  return out;
}

void print_row(cell service, cell status, cell latency, cell notes,
               std::string_view row = {})
{
  // White foreground over dark blue, dark green, dark goldenrod, and indigo.
  using enum alignment;
  std::println("{} {} {} {}\x1b[0m",
               render_cell(service, 16, left,   "37;48;2;0;0;139",    row),
               render_cell(status,  12, center, "37;48;2;0;100;0",    row),
               render_cell(latency, 12, right,  "37;48;2;184;134;11", row),
               render_cell(notes,   30, left,   "37;48;2;75;0;130",   row));
}

} // local

int main() {
  std::println("SYSTEM HEALTH DASHBOARD\n");
  print_row({{"Service"}}, {{"Status"}}, {{"Latency"}}, {{"Notes"}}, "1;4");

  const auto ok = cell{{"OK", "92"}};
  print_row({{"NAS"}}, ok, {{"12 ms"}},
            {{"All", "31;46"}, {" volumes "}, {"healthy", "32"}});
  print_row({{"東京 NAS"}}, ok, {{"9 ms"}}, {{"Café 東京: all systems normal"}});
  print_row({{"VPN"}}, ok, {{"38 ms"}},
            {{"Encrypted "}, {"connection", "38;2;127;255;212"}});
  print_row({{"Backup"}}, {{"WARN", "33"}}, {{"215 ms"}},
            {{"Slow", "38;2;255;255;0"}, {" - retry scheduled"}}, "1");

  // The leading zero clears inherited SGR attributes for the warning.
  print_row({{"Database"}}, {{"FAIL", "91"}}, {{"--"}},
            {{"Connection refused: "},
             {"CRITICAL!", "0;1;5;38;2;255;0;0;48;2;255;255;255"},
             {" Database service is unavailable"}}, "1");
  print_row({{"Old server"}}, {{"OFFLINE"}}, {{"--"}}, {{"Maintenance mode"}}, "2");
} // main
