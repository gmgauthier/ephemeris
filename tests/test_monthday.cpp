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

bool has_on(const ephemeris::ParsedIcs& parsed, const std::string& text, const Glib::Date& date)
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
  const Glib::Date from = day(1, Glib::Date::JANUARY, 2024);
  const Glib::Date to = day(31, Glib::Date::DECEMBER, 2032);

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260131T090000\n"
        "DTEND:20260131T100000\n"
        "SUMMARY:ThirtyFirst\n"
        "RRULE:FREQ=MONTHLY;COUNT=3\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
    CHECK(count_text(parsed, "ThirtyFirst") == 3);
    CHECK(has_on(parsed, "ThirtyFirst", day(31, Glib::Date::JANUARY, 2026)));
    CHECK(has_on(parsed, "ThirtyFirst", day(31, Glib::Date::MARCH, 2026)));
    CHECK(has_on(parsed, "ThirtyFirst", day(31, Glib::Date::MAY, 2026)));
    CHECK(!has_on(parsed, "ThirtyFirst", day(28, Glib::Date::FEBRUARY, 2026)));
    CHECK(!has_on(parsed, "ThirtyFirst", day(30, Glib::Date::APRIL, 2026)));
    CHECK(parsed.items[0].start_min == 9 * 60);
    CHECK(parsed.items[0].end_min == 10 * 60);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260130T090000\n"
        "DTEND:20260130T100000\n"
        "SUMMARY:Thirtieth\n"
        "RRULE:FREQ=MONTHLY;COUNT=4\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(count_text(parsed, "Thirtieth") == 4);
    CHECK(has_on(parsed, "Thirtieth", day(30, Glib::Date::JANUARY, 2026)));
    CHECK(has_on(parsed, "Thirtieth", day(30, Glib::Date::MARCH, 2026)));
    CHECK(has_on(parsed, "Thirtieth", day(30, Glib::Date::APRIL, 2026)));
    CHECK(has_on(parsed, "Thirtieth", day(30, Glib::Date::MAY, 2026)));
    CHECK(!has_on(parsed, "Thirtieth", day(28, Glib::Date::FEBRUARY, 2026)));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260128T090000\n"
        "DTEND:20260128T100000\n"
        "SUMMARY:TwentyEighth\n"
        "RRULE:FREQ=MONTHLY;COUNT=3\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(count_text(parsed, "TwentyEighth") == 3);
    CHECK(has_on(parsed, "TwentyEighth", day(28, Glib::Date::JANUARY, 2026)));
    CHECK(has_on(parsed, "TwentyEighth", day(28, Glib::Date::FEBRUARY, 2026)));
    CHECK(has_on(parsed, "TwentyEighth", day(28, Glib::Date::MARCH, 2026)));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20240229T090000\n"
        "DTEND:20240229T100000\n"
        "SUMMARY:Leap\n"
        "RRULE:FREQ=YEARLY\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const Glib::Date leap_to = day(31, Glib::Date::DECEMBER, 2028);
    const auto parsed = ephemeris::parse_ics(text, from, leap_to);
    CHECK(count_text(parsed, "Leap") == 2);
    CHECK(has_on(parsed, "Leap", day(29, Glib::Date::FEBRUARY, 2024)));
    CHECK(has_on(parsed, "Leap", day(29, Glib::Date::FEBRUARY, 2028)));
    CHECK(!has_on(parsed, "Leap", day(28, Glib::Date::FEBRUARY, 2025)));
    CHECK(!has_on(parsed, "Leap", day(28, Glib::Date::FEBRUARY, 2026)));
    CHECK(!has_on(parsed, "Leap", day(28, Glib::Date::FEBRUARY, 2027)));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20240229T090000\n"
        "DTEND:20240229T100000\n"
        "SUMMARY:LeapCount\n"
        "RRULE:FREQ=YEARLY;COUNT=3\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(count_text(parsed, "LeapCount") == 3);
    CHECK(has_on(parsed, "LeapCount", day(29, Glib::Date::FEBRUARY, 2024)));
    CHECK(has_on(parsed, "LeapCount", day(29, Glib::Date::FEBRUARY, 2028)));
    CHECK(has_on(parsed, "LeapCount", day(29, Glib::Date::FEBRUARY, 2032)));
    CHECK(!has_on(parsed, "LeapCount", day(28, Glib::Date::FEBRUARY, 2025)));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260131T090000\n"
        "DTEND:20260131T100000\n"
        "SUMMARY:Hidden\n"
        "RRULE:FREQ=MONTHLY\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const Glib::Date feb1 = day(1, Glib::Date::FEBRUARY, 2026);
    const Glib::Date feb28 = day(28, Glib::Date::FEBRUARY, 2026);
    const auto parsed = ephemeris::parse_ics(text, feb1, feb28);
    CHECK(count_text(parsed, "Hidden") == 0);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260131T090000\n"
        "DTEND:20260131T100000\n"
        "SUMMARY:EachYear\n"
        "RRULE:FREQ=YEARLY;COUNT=2\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(count_text(parsed, "EachYear") == 2);
    CHECK(has_on(parsed, "EachYear", day(31, Glib::Date::JANUARY, 2026)));
    CHECK(has_on(parsed, "EachYear", day(31, Glib::Date::JANUARY, 2027)));
  }

  return suite_test::done("monthday");
}
