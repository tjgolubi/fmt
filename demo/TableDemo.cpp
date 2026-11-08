#include "tjg/styled_text.h"
#include "tjg/color.h"

#include <print>

#include <utility>

using tjg::styled_text;
using tjg::text_style;
using tjg::fg;
using tjg::bg;
using tc = tjg::term_color;
namespace color = tjg::color;

// Each column has a distinct background color.
constexpr auto service_color = fg(tc::white).bg(color::dark_blue);
constexpr auto status_color  = fg(tc::white).bg(color::dark_green);
constexpr auto latency_color = fg(tc::white).bg(color::dark_golden_rod);
constexpr auto notes_color   = fg(tc::white).bg(color::indigo);

// An absolute style replaces inherited styling.
constexpr auto critical_style =
                      fg(color::red).bg(color::white).bold().blink().absolute();

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
  tjg::println(row, "{:<16.16} {:^12.12} {:>12.12} {:<30.30}",
               cell(std::move(service), service_color),
               cell(std::move(status),  status_color),
               cell(std::move(latency), latency_color),
               cell(std::move(notes),   notes_color));
}

int main() {
  std::println("SYSTEM HEALTH DASHBOARD\n");

  // Column headings inherit bold and underline from the row.
  print_row("Service", "Status", "Latency", "Notes", tjg::bold().underline());

  // Normal row.
  auto ok = styled_text{"OK", fg(tc::bright_green)};
  auto nas_note = styled_text{"All", fg(tc::red).bg(tc::cyan)}
                + " volumes "
                + styled_text{"healthy", fg(tc::green)};
  print_row("NAS", ok, "12 ms", nas_note);

  // UTF-8 display widths: "東京" is 6 bytes but 4 terminal columns.
  // "Café" is 5 bytes but 4 terminal columns.
  print_row("東京 NAS", ok, "9 ms", "Café 東京: all systems normal");

  // Two fragments with different foreground colors.
  auto vpn_note = styled_text{"Encrypted "}
                + styled_text{"connection", fg(color::aquamarine)};
  print_row("VPN", ok, "38 ms", vpn_note);

  // Bold row with an independently colored warning.
  auto warn = styled_text{"WARN", fg(tc::yellow)};
  auto backup_note = styled_text{"Slow", fg(color::yellow)}
                   + " - retry scheduled";
  print_row("Backup", warn, "215 ms", backup_note, tjg::bold());

  // An absolute red-on-white fragment within the Notes column.
  auto fail = styled_text{"FAIL", fg(tc::bright_red)};
  auto db_note = styled_text{"Connection refused: "}
               + styled_text{"CRITICAL!", critical_style}
               + " Database service is unavailable";
  print_row("Database", fail, "--", db_note, tjg::bold());

  // Faint row emphasis, retaining the column backgrounds.
  print_row("Old server", "OFFLINE", "--", "Maintenance mode", tjg::faint());
} // main
