#include <fmt/color.h>
#include <fmt/format.h>

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

namespace {

using fmt::text_style;
using fmt::fg;
using fmt::bg;
using fmt::styled;
using tc = fmt::terminal_color;
using color = fmt::color;
using emphasis = fmt::emphasis;

constexpr auto service_style     = fg(tc::white) | bg(color::dark_blue);
constexpr auto status_background = bg(color::dark_green);
constexpr auto latency_style     = fg(tc::white) | bg(color::dark_golden_rod);
constexpr auto notes_background  = bg(color::indigo);
constexpr auto notes_style       = fg(tc::white) | notes_background;

struct fragment {
  std::string_view text;
  std::optional<text_style> style{};
  bool absolute = false;
};

using note = std::initializer_list<fragment>;

auto format_note(note parts, text_style row) -> std::string {
  // The 30-column limit applies to the whole cell.
  auto plain = std::string{};
  for (const auto& part : parts)
    plain += part.text;
  const auto clipped = fmt::format("{:.30}", plain);
  const auto padded  = fmt::format("{:<30}", clipped);
  const auto padding = std::string_view{padded}.substr(clipped.size());

  auto result = std::string{};
  auto out = std::back_inserter(result);
  auto remaining = clipped.size();
  for (const auto& part : parts) {
    if (remaining == 0) break;
    const auto count = std::min(remaining, part.text.size());
    if (count == 0) continue;

    // Each styled fragment ends with a reset. Give the next one its full style.
    auto style = part.style.value_or(notes_style);
    if (!part.absolute) style |= row;
    out = fmt::format_to(out, "{}", styled(part.text.substr(0, count), style));
    remaining -= count;
  }
  if (!padding.empty())
    fmt::format_to(out, "{}", styled(padding, notes_style | row));
  return result;
}

void print_row(std::string_view service,
               std::string_view status,
               std::string_view latency, note notes,
               text_style status_foreground = fg(tc::white),
               text_style row = {})
{
  // Argument 1 is the separator, which also inherits the row emphasis.
  fmt::println("{0:<16.16}{1}{2:^12.12}{1}{3:>12.12}{1}{4}",
               styled(service, service_style     | row), styled(" ", row),
               styled(status,  status_foreground | status_background | row),
               styled(latency, latency_style     | row), format_note(notes, row));
}

} // local

int main() {
  fmt::println("SYSTEM HEALTH DASHBOARD\n");
  print_row("Service", "Status", "Latency", {{"Notes"}}, fg(tc::white),
            emphasis::bold | emphasis::underline);

  constexpr auto ok = fg(tc::bright_green);
  print_row("NAS", "OK", "12 ms",
            {{"All", fg(tc::red) | bg(tc::cyan)}, {" volumes "},
              {"healthy", fg(tc::green) | notes_background}},
             ok);
  print_row("東京 NAS", "OK", "9 ms", {{"Café 東京: all systems normal"}}, ok);
  print_row("VPN", "OK", "38 ms",
            {{"Encrypted "},
              {"connection", fg(color::aquamarine) | notes_background}},
            ok);
  print_row("Backup", "WARN", "215 ms",
      {{"Slow", fg(color::yellow) | notes_background}, {" - retry scheduled"}},
            fg(tc::yellow), emphasis::bold);

  constexpr auto critical = fg(color::red) | bg(color::white)
                          | emphasis::bold | emphasis::blink;
  print_row("Database", "FAIL", "--",
            {{"Connection refused: "}, {"CRITICAL!", critical, true},
                {" Database service is unavailable"}},
             fg(tc::bright_red), emphasis::bold);
  print_row("Old server", "OFFLINE", "--", {{"Maintenance mode"}},
            fg(tc::white), emphasis::faint);
} // main
