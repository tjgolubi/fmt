#include <tjg/color.hpp>

#include <print>
#include <utility>

using tjg::color::styled_text;
using tjg::color::text_style;
using tjg::color::fg;
using tjg::color::bg;
using tc = tjg::color::term_color;
using tjg::color::rgb;

namespace {

// Each column has a distinct background color.
constexpr auto service_color = fg(tc::white).bg(rgb::dark_blue);
constexpr auto status_color  = fg(tc::white).bg(rgb::dark_green);
constexpr auto latency_color = fg(tc::white).bg(rgb::dark_golden_rod);
constexpr auto notes_color   = fg(tc::white).bg(rgb::indigo);

// An absolute style replaces inherited styling.
constexpr auto critical_style =
                      fg(rgb::red).bg(rgb::white).bold().blink().absolute();

// Apply column defaults without overriding existing styles.
auto cell(styled_text text, text_style column) -> styled_text {
  return underlay(column, text);
}

// Each field has equal width and precision.
// Long content is truncated; short content is padded.
void print_row(styled_text service, styled_text status,
               styled_text latency, styled_text notes,
               text_style row = {})
{
  tjg::color::println(row, "{:<16.16} {:^12.12} {:>12.12} {:<30.30}",
               cell(std::move(service), service_color),
               cell(std::move(status),  status_color),
               cell(std::move(latency), latency_color),
               cell(std::move(notes),   notes_color));
}

} // local

int main() {
  std::println("SYSTEM HEALTH DASHBOARD\n");

  // Column headings inherit bold and underline from the row.
  print_row("Service", "Status", "Latency", "Notes", tjg::color::bold().underline());

  // Normal row.
  const auto ok = styled_text{"OK", fg(tc::bright_green)};
  const auto nas_note = styled_text{"All", fg(tc::red).bg(tc::cyan)}
                      + " volumes " + styled_text{"healthy", fg(tc::green)};
  print_row("NAS", ok, "12 ms", nas_note);

  // UTF-8 display widths: "東京" is 6 bytes but 4 terminal columns.
  // "Café" is 5 bytes but 4 terminal columns.
  print_row("東京 NAS", ok, "9 ms", "Café 東京: all systems normal");

  // Two fragments with different foreground colors.
  const auto vpn_note = "Encrypted "
                      + styled_text{"connection", fg(rgb::aquamarine)};
  print_row("VPN", ok, "38 ms", vpn_note);

  // Bold row with an independently colored warning.
  const auto warn = styled_text{"WARN", fg(tc::yellow)};
  const auto backup_note = styled_text{"Slow", fg(rgb::yellow)}
                         + " - retry scheduled";
  print_row("Backup", warn, "215 ms", backup_note, tjg::color::bold());

  // An absolute red-on-white fragment within the Notes column.
  const auto fail = styled_text{"FAIL", fg(tc::bright_red)};
  const auto db_note = "Connection refused: "
                     + styled_text{"CRITICAL!", critical_style}
                     + " Database service is unavailable";
  print_row("Database", fail, "--", db_note, tjg::color::bold());

  // Faint row emphasis, retaining the column backgrounds.
  print_row("Old server", "OFFLINE", "--", "Maintenance mode", tjg::color::faint());
} // main
