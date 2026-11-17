#include <tjg/color.hpp>

#include <iomanip>
#include <iostream>
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

// Apply column defaults without overriding existing styles.
auto cell(styled_text text, text_style column, text_style row) -> styled_text {
  text.underlay(column.underlay(row));
  return text;
}

// Each field has equal width and precision.
// Long content is not truncated; short content is padded.
void print_row(std::ostream& out,
               styled_text service, styled_text status,
               styled_text latency, styled_text notes,
               text_style row = {})
{
  // setw() applies to one cell. Width is a minimum, so use notes that fit.
  out        << std::left << std::setw(16)
             << cell(std::move(service), service_color, row)
      << ' ' << std::setw(12) << cell(std::move(status), status_color, row)
      << ' ' << std::right << std::setw(12)
             << cell(std::move(latency), latency_color, row)
      << ' ' << std::left << std::setw(30)
             << cell(std::move(notes), notes_color, row) << '\n';
}

} // local

int main() {
  auto& out = std::cout;
  out << "SYSTEM HEALTH DASHBOARD\n\n";

  // Column headings inherit bold and underline from the row.
  print_row(out, "Service", "Status", "Latency", "Notes", tjg::color::bold().underline());

  // Normal row.
  const auto ok = styled_text{"OK", fg(tc::bright_green)};
  const auto nas_note = styled_text{"All", fg(tc::red).bg(tc::cyan)}
                      + " volumes " + styled_text{"healthy", fg(tc::green)};
  print_row(out, "NAS", ok, "12 ms", nas_note);

  // UTF-8 display widths: "東京" is 6 bytes but 4 terminal columns.
  // "Café" is 5 bytes but 4 terminal columns.
  print_row(out, "東京 NAS", ok, "9 ms", "Café 東京: all systems normal");

  // Two fragments with different foreground colors.
  const auto vpn_note = styled_text{"Encrypted "}
                      + styled_text{"connection", fg(rgb::aquamarine)};
  print_row(out, "VPN", ok, "38 ms", vpn_note);

  // Bold row with an independently colored warning.
  const auto warn = styled_text{"WARN", fg(tc::yellow)};
  const auto backup_note = styled_text{"Slow", fg(rgb::yellow)}
                         + " - retry scheduled";
  print_row(out, "Backup", warn, "215 ms", backup_note, tjg::color::bold());

  // An absolute red-on-white fragment within the Notes column.
  const auto fail = styled_text{"FAIL", fg(tc::bright_red)};
  const auto critical_style =
                      fg(rgb::red).bg(rgb::white).bold().blink().absolute();
  const auto db_note = styled_text{"Connection refused: "}
                     + styled_text{"CRITICAL!", critical_style};
  print_row(out, "Database", fail, "--", db_note, tjg::color::bold());

  // Faint row emphasis, retaining the column backgrounds.
  print_row(out, "Old server", "OFFLINE", "--", "Maintenance mode", tjg::color::faint());

  // The underlying integer inserter handles the base, fill, and field width.
  out << "\nRequest ID: " << std::hex << std::right << std::setfill('0')
      << std::setw(4) << tjg::color::styled(42, fg(tc::bright_cyan))
      << std::dec << std::setfill(' ') << '\n';
}
