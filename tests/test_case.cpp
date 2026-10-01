/* SPDX-License-Identifier: Unlicense */

#include "ics.hpp"
#include "check.hpp"

#include <string>

namespace {

Glib::Date day(int d, Glib::Date::Month m, int y)
{
  return Glib::Date(static_cast<Glib::Date::Day>(d), m, static_cast<Glib::Date::Year>(y));
}

}  // namespace

int main()
{
  const Glib::Date from = day(1, Glib::Date::JANUARY, 2026);
  const Glib::Date to = day(31, Glib::Date::DECEMBER, 2026);

  {
    const auto parsed = ephemeris::parse_ics("not a calendar", from, to);
    CHECK(!parsed.error.empty());
    CHECK(parsed.items.empty());
  }

  {
    const char* text =
        "begin:vcalendar\n"
        "end:vcalendar\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
    CHECK(parsed.items.empty());
  }

  {
    const char* text =
        "begin:vcalendar\n"
        "x-wr-calname:Desk\n"
        "begin:vevent\n"
        "dtstart:20260302t090000\n"
        "dtend:20260302t100000\n"
        "summary:Meet\n"
        "end:vevent\n"
        "end:vcalendar\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
    CHECK(parsed.title == "Desk");
    CHECK(parsed.items.size() == 1);
    CHECK(parsed.items[0].text == "Meet");
    CHECK(parsed.items[0].date.compare(day(2, Glib::Date::MARCH, 2026)) == 0);
    CHECK(parsed.items[0].start_min == 9 * 60);
    CHECK(parsed.items[0].end_min == 10 * 60);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260302T090000\n"
        "DTEND:20260302T100000\n"
        "summary:meet\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 1);
    CHECK(parsed.items[0].text == "meet");
  }

  {
    const char* text =
        "Begin:VCALENDAR\n"
        "Begin:Vevent\n"
        "Dtstart:20260302T110000\n"
        "Dtend:20260302T113000\n"
        "Summary;Language=en:Team huddle\n"
        "End:Vevent\n"
        "End:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 1);
    CHECK(parsed.items[0].text == "Team huddle");
    CHECK(parsed.items[0].start_min == 11 * 60);
    CHECK(parsed.items[0].end_min == 11 * 60 + 30);
  }

  {
    const char* text =
        "begin:vcalendar\n"
        "begin:vevent\n"
        "dtstart:20260302t090000\n"
        "dtend:20260302t100000\n"
        "summary:Repeat\n"
        "rrule:freq=daily;count=2\n"
        "end:vevent\n"
        "end:vcalendar\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 2);
    CHECK(parsed.items[0].text == "Repeat");
    CHECK(parsed.items[0].date.compare(day(2, Glib::Date::MARCH, 2026)) == 0);
    CHECK(parsed.items[1].date.compare(day(3, Glib::Date::MARCH, 2026)) == 0);
    CHECK(parsed.items[0].start_min == 9 * 60);
  }

  {
    const char* text =
        "begin:vcalendar\n"
        "begin:vevent\n"
        "dtstart;value=date:20260928\n"
        "summary:Days\n"
        "rrule:freq=weekly;byday=mo,we;count=2\n"
        "end:vevent\n"
        "end:vcalendar\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 2);
    CHECK(parsed.items[0].text == "Days");
    CHECK(parsed.items[0].date.compare(day(28, Glib::Date::SEPTEMBER, 2026)) == 0);
    CHECK(parsed.items[1].date.compare(day(30, Glib::Date::SEPTEMBER, 2026)) == 0);
    CHECK(parsed.items[0].start_min == 0);
    CHECK(parsed.items[0].end_min == 24 * 60);
  }

  {
    const char* text =
        "begin:vcalendar\n"
        "begin:vevent\n"
        "dtstart:20260302t090000\n"
        "summary:Gone\n"
        "status:cancelled\n"
        "end:vevent\n"
        "end:vcalendar\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
    CHECK(parsed.items.empty());
  }

  {
    const char* text =
        "begin:vcalendar\n"
        "begin:vevent\n"
        "dtstart:20260302t090000\n"
        "duration:pt3h\n"
        "summary:Long\n"
        "end:vevent\n"
        "end:vcalendar\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 1);
    CHECK(parsed.items[0].text == "Long");
    CHECK(parsed.items[0].start_min == 9 * 60);
    CHECK(parsed.items[0].end_min == 12 * 60);
  }

  {
    const char* text =
        "begin:vcalendar\n"
        "begin:vevent\n"
        "dtstart;value=date:20260928t090000\n"
        "summary:All day\n"
        "end:vevent\n"
        "end:vcalendar\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 1);
    CHECK(parsed.items[0].text == "All day");
    CHECK(parsed.items[0].date.compare(day(28, Glib::Date::SEPTEMBER, 2026)) == 0);
    CHECK(parsed.items[0].start_min == 0);
    CHECK(parsed.items[0].end_min == 24 * 60);
  }

  return suite_test::done("casefold");
}
