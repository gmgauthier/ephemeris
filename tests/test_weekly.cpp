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
  const Glib::Date jan = day(1, Glib::Date::JANUARY, 2026);
  const Glib::Date dec = day(31, Glib::Date::DECEMBER, 2026);
  const Glib::Date tue = day(6, Glib::Date::JANUARY, 2026);
  const Glib::Date sun = day(11, Glib::Date::JANUARY, 2026);
  const Glib::Date tue2 = day(13, Glib::Date::JANUARY, 2026);

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260106T090000\n"
        "DTEND:20260106T100000\n"
        "SUMMARY:One\n"
        "RRULE:FREQ=WEEKLY;BYDAY=SU,TU;COUNT=1\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "One") == 1);
    CHECK(has_on(parsed, "One", tue));
    CHECK(!has_on(parsed, "One", sun));
    CHECK(parsed.items[0].start_min == 9 * 60);
    CHECK(parsed.items[0].end_min == 10 * 60);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260106T090000\n"
        "DTEND:20260106T100000\n"
        "SUMMARY:Two\n"
        "RRULE:FREQ=WEEKLY;BYDAY=SU,TU;COUNT=2\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "Two") == 2);
    CHECK(has_on(parsed, "Two", tue));
    CHECK(has_on(parsed, "Two", sun));
    CHECK(!has_on(parsed, "Two", tue2));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260106T090000\n"
        "DTEND:20260106T100000\n"
        "SUMMARY:Ordered\n"
        "RRULE:FREQ=WEEKLY;BYDAY=TU,SU;COUNT=1\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "Ordered") == 1);
    CHECK(has_on(parsed, "Ordered", tue));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260111T090000\n"
        "DTEND:20260111T100000\n"
        "SUMMARY:FromSunday\n"
        "RRULE:FREQ=WEEKLY;BYDAY=SU,TU;COUNT=1\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "FromSunday") == 1);
    CHECK(has_on(parsed, "FromSunday", sun));
    CHECK(!has_on(parsed, "FromSunday", tue));
    CHECK(!has_on(parsed, "FromSunday", tue2));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260106T090000\n"
        "DTEND:20260106T100000\n"
        "SUMMARY:Dup\n"
        "RRULE:FREQ=WEEKLY;BYDAY=TU,TU,SU;COUNT=3\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "Dup") == 3);
    CHECK(has_on(parsed, "Dup", tue));
    CHECK(has_on(parsed, "Dup", sun));
    CHECK(has_on(parsed, "Dup", tue2));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260928\n"
        "SUMMARY:Days\n"
        "RRULE:FREQ=WEEKLY;BYDAY=MO,WE;COUNT=4\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "Days") == 4);
    CHECK(has_on(parsed, "Days", day(28, Glib::Date::SEPTEMBER, 2026)));
    CHECK(has_on(parsed, "Days", day(30, Glib::Date::SEPTEMBER, 2026)));
    CHECK(has_on(parsed, "Days", day(5, Glib::Date::OCTOBER, 2026)));
    CHECK(has_on(parsed, "Days", day(7, Glib::Date::OCTOBER, 2026)));
  }

  return suite_test::done("weekly");
}
