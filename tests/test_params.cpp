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

}  // namespace

int main()
{
  const Glib::Date from = day(1, Glib::Date::JANUARY, 2026);
  const Glib::Date to = day(31, Glib::Date::DECEMBER, 2026);

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260302T090000\n"
        "DTEND:20260302T100000\n"
        "SUMMARY;LANGUAGE=en:Meet\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
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
        "DTSTART:20260302T110000\n"
        "DTEND:20260302T113000\n"
        "SUMMARY;LANGUAGE=en;X-ALT=short:Team huddle\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 1);
    CHECK(parsed.items[0].text == "Team huddle");
    CHECK(parsed.items[0].start_min == 11 * 60);
    CHECK(parsed.items[0].end_min == 11 * 60 + 30);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260302T140000\n"
        "DTEND:20260302T150000\n"
        "SUMMARY;LANGUAGE=en:Meet\\, room\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 1);
    CHECK(parsed.items[0].text == "Meet, room");
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260304T090000\n"
        "DTEND:20260304T100000\n"
        "SUMMARY;LANGUAGE=en:Folded\n"
        "  note\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 1);
    CHECK(parsed.items[0].text == "Folded note");
    CHECK(parsed.items[0].date.compare(day(4, Glib::Date::MARCH, 2026)) == 0);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260302T090000\n"
        "DTEND:20260302T100000\n"
        "SUMMARY;LANGUAGE=en:Repeat\n"
        "RRULE;X-NAME=keep:FREQ=DAILY;COUNT=2\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(count_text(parsed, "Repeat") == 2);
    CHECK(parsed.items.size() == 2);
    CHECK(parsed.items[0].date.compare(day(2, Glib::Date::MARCH, 2026)) == 0);
    CHECK(parsed.items[1].date.compare(day(3, Glib::Date::MARCH, 2026)) == 0);
    CHECK(parsed.items[0].start_min == 9 * 60);
    CHECK(parsed.items[0].end_min == 10 * 60);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260302T090000\n"
        "DTEND:20260302T100000\n"
        "SUMMARY;LANGUAGE=en:Gone\n"
        "STATUS;LANGUAGE=en:CANCELLED\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
    CHECK(parsed.items.empty());
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260302T090000\n"
        "DURATION;X-FOO=1:PT3H\n"
        "SUMMARY;LANGUAGE=en:Long\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 1);
    CHECK(parsed.items[0].text == "Long");
    CHECK(parsed.items[0].start_min == 9 * 60);
    CHECK(parsed.items[0].end_min == 12 * 60);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260928T090000\n"
        "SUMMARY;LANGUAGE=en:All day\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 1);
    CHECK(parsed.items[0].text == "All day");
    CHECK(parsed.items[0].date.compare(day(28, Glib::Date::SEPTEMBER, 2026)) == 0);
    CHECK(parsed.items[0].start_min == 0);
    CHECK(parsed.items[0].end_min == 24 * 60);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;TZID=Europe/London;VALUE=DATE:20260928\n"
        "DTEND;VALUE=DATE;TZID=Europe/London:20260930\n"
        "SUMMARY;LANGUAGE=en:Away\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(count_text(parsed, "Away") == 2);
    CHECK(parsed.items[0].date.compare(day(28, Glib::Date::SEPTEMBER, 2026)) == 0);
    CHECK(parsed.items[1].date.compare(day(29, Glib::Date::SEPTEMBER, 2026)) == 0);
    CHECK(parsed.items[0].start_min == 0);
    CHECK(parsed.items[0].end_min == 24 * 60);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260302T160000\n"
        "DTEND:20260302T170000\n"
        "SUMMARY:Plain\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 1);
    CHECK(parsed.items[0].text == "Plain");
    CHECK(parsed.items[0].start_min == 16 * 60);
    CHECK(parsed.items[0].end_min == 17 * 60);
  }

  return suite_test::done("params");
}
