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

int count_on(const ephemeris::ParsedIcs& parsed, const std::string& text, const Glib::Date& date)
{
  int n = 0;
  for (const auto& item : parsed.items) {
    if (item.text == text && item.date.compare(date) == 0)
      ++n;
  }
  return n;
}

bool full_day(const ephemeris::ParsedIcs& parsed, const std::string& text, const Glib::Date& date)
{
  for (const auto& item : parsed.items) {
    if (item.text == text && item.date.compare(date) == 0 && item.start_min == 0 &&
        item.end_min == 24 * 60)
      return true;
  }
  return false;
}

}  // namespace

int main()
{
  const Glib::Date jan = day(1, Glib::Date::JANUARY, 2026);
  const Glib::Date dec = day(31, Glib::Date::DECEMBER, 2026);
  const Glib::Date d28 = day(28, Glib::Date::SEPTEMBER, 2026);
  const Glib::Date d29 = day(29, Glib::Date::SEPTEMBER, 2026);
  const Glib::Date d30 = day(30, Glib::Date::SEPTEMBER, 2026);
  const Glib::Date oct1 = day(1, Glib::Date::OCTOBER, 2026);
  const Glib::Date oct2 = day(2, Glib::Date::OCTOBER, 2026);

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260928\n"
        "DTEND;VALUE=DATE:20260930\n"
        "SUMMARY:Away\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(parsed.error.empty());
    CHECK(count_text(parsed, "Away") == 2);
    CHECK(full_day(parsed, "Away", d28));
    CHECK(full_day(parsed, "Away", d29));
    CHECK(count_on(parsed, "Away", d30) == 0);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260928\n"
        "DTEND;VALUE=DATE:20261002\n"
        "SUMMARY:Vacation\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "Vacation") == 4);
    CHECK(full_day(parsed, "Vacation", d28));
    CHECK(full_day(parsed, "Vacation", d29));
    CHECK(full_day(parsed, "Vacation", d30));
    CHECK(full_day(parsed, "Vacation", oct1));
    CHECK(count_on(parsed, "Vacation", oct2) == 0);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260928\n"
        "DTEND;VALUE=DATE:20260929\n"
        "SUMMARY:One\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "One") == 1);
    CHECK(full_day(parsed, "One", d28));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260928\n"
        "SUMMARY:Bare\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "Bare") == 1);
    CHECK(full_day(parsed, "Bare", d28));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260928\n"
        "DTEND;VALUE=DATE:20260930\n"
        "SUMMARY:Tail\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, d29, d29);
    CHECK(count_text(parsed, "Tail") == 1);
    CHECK(full_day(parsed, "Tail", d29));
    CHECK(count_on(parsed, "Tail", d28) == 0);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260928\n"
        "DTEND;VALUE=DATE:20260930\n"
        "SUMMARY:Once\n"
        "RRULE:FREQ=DAILY;COUNT=1\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "Once") == 2);
    CHECK(full_day(parsed, "Once", d28));
    CHECK(full_day(parsed, "Once", d29));
    CHECK(count_on(parsed, "Once", d30) == 0);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260928\n"
        "DTEND;VALUE=DATE:20260930\n"
        "SUMMARY:Repeat\n"
        "RRULE:FREQ=DAILY;COUNT=2\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "Repeat") == 4);
    CHECK(count_on(parsed, "Repeat", d28) == 1);
    CHECK(count_on(parsed, "Repeat", d29) == 2);
    CHECK(count_on(parsed, "Repeat", d30) == 1);
    CHECK(full_day(parsed, "Repeat", d28));
    CHECK(full_day(parsed, "Repeat", d30));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260928\n"
        "SUMMARY:Days\n"
        "RRULE:FREQ=DAILY;COUNT=3\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "Days") == 3);
    CHECK(full_day(parsed, "Days", d28));
    CHECK(full_day(parsed, "Days", d29));
    CHECK(full_day(parsed, "Days", d30));
  }

  return suite_test::done("allday");
}
