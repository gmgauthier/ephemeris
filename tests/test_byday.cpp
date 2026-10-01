/* SPDX-License-Identifier: Unlicense */

#include "ics.hpp"
#include "check.hpp"

#include <string>

namespace {

Glib::Date day(int d, Glib::Date::Month m, int y)
{
  return Glib::Date(static_cast<Glib::Date::Day>(d), m, static_cast<Glib::Date::Year>(y));
}

int count_text(const ephemeris::ParsedIcs& parsed, const std::string& text)
{
  int n = 0;
  for (const auto& item : parsed.items) {
    if (item.text == text)
      ++n;
  }
  return n;
}

bool has_text_on(const ephemeris::ParsedIcs& parsed, const std::string& text,
                 const Glib::Date& date)
{
  for (const auto& item : parsed.items) {
    if (item.text == text && item.date.compare(date) == 0)
      return true;
  }
  return false;
}

}  // namespace

int main()
{
  const Glib::Date from = day(1, Glib::Date::JANUARY, 2026);
  const Glib::Date to = day(31, Glib::Date::DECEMBER, 2027);

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T090000\n"
        "DTEND:20260105T100000\n"
        "SUMMARY:Board\n"
        "RRULE:FREQ=MONTHLY;BYDAY=1MO;COUNT=3\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 3);
    CHECK(has_text_on(parsed, "Board", day(5, Glib::Date::JANUARY, 2026)));
    CHECK(has_text_on(parsed, "Board", day(2, Glib::Date::FEBRUARY, 2026)));
    CHECK(has_text_on(parsed, "Board", day(2, Glib::Date::MARCH, 2026)));
    CHECK(!has_text_on(parsed, "Board", day(5, Glib::Date::FEBRUARY, 2026)));
    CHECK(parsed.items[0].start_min == 9 * 60);
    CHECK(parsed.items[0].end_min == 10 * 60);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260105\n"
        "SUMMARY:Shift\n"
        "RRULE:FREQ=MONTHLY;BYDAY=MO,WE,FR;COUNT=6\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 6);
    CHECK(has_text_on(parsed, "Shift", day(5, Glib::Date::JANUARY, 2026)));
    CHECK(has_text_on(parsed, "Shift", day(7, Glib::Date::JANUARY, 2026)));
    CHECK(has_text_on(parsed, "Shift", day(9, Glib::Date::JANUARY, 2026)));
    CHECK(has_text_on(parsed, "Shift", day(12, Glib::Date::JANUARY, 2026)));
    CHECK(has_text_on(parsed, "Shift", day(14, Glib::Date::JANUARY, 2026)));
    CHECK(has_text_on(parsed, "Shift", day(16, Glib::Date::JANUARY, 2026)));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260130\n"
        "SUMMARY:Last Friday\n"
        "RRULE:FREQ=MONTHLY;BYDAY=-1FR;COUNT=2\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 2);
    CHECK(has_text_on(parsed, "Last Friday", day(30, Glib::Date::JANUARY, 2026)));
    CHECK(has_text_on(parsed, "Last Friday", day(27, Glib::Date::FEBRUARY, 2026)));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260105\n"
        "SUMMARY:Year Monday\n"
        "RRULE:FREQ=YEARLY;BYDAY=1MO;COUNT=2\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 2);
    CHECK(has_text_on(parsed, "Year Monday", day(5, Glib::Date::JANUARY, 2026)));
    CHECK(has_text_on(parsed, "Year Monday", day(4, Glib::Date::JANUARY, 2027)));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260105\n"
        "SUMMARY:Mondays\n"
        "RRULE:FREQ=YEARLY;BYDAY=MO;COUNT=3\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 3);
    CHECK(has_text_on(parsed, "Mondays", day(5, Glib::Date::JANUARY, 2026)));
    CHECK(has_text_on(parsed, "Mondays", day(12, Glib::Date::JANUARY, 2026)));
    CHECK(has_text_on(parsed, "Mondays", day(19, Glib::Date::JANUARY, 2026)));
    CHECK(count_text(parsed, "Mondays") == 3);
  }

  return suite_test::done("byday");
}
