/* SPDX-License-Identifier: Unlicense */

#include "ics.hpp"
#include "check.hpp"

#include <cstdio>
#include <string>

namespace {

Glib::Date day(int d, Glib::Date::Month m, int y)
{
  return Glib::Date(static_cast<Glib::Date::Day>(d), m, static_cast<Glib::Date::Year>(y));
}

std::string ymd(const Glib::Date& d)
{
  char buf[16];
  std::snprintf(buf, sizeof buf, "%04d%02d%02d", d.get_year(), static_cast<int>(d.get_month()),
                d.get_day());
  return buf;
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
  const Glib::Date jan1 = day(1, Glib::Date::JANUARY, 2026);
  const Glib::Date jan2 = day(2, Glib::Date::JANUARY, 2026);
  const Glib::Date jan3 = day(3, Glib::Date::JANUARY, 2026);
  Glib::Date early = jan1;
  early.subtract_days(900);
  const std::string early_ymd = ymd(early);

  {
    const std::string text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:" +
        early_ymd +
        "T090000\n"
        "DTEND:" +
        early_ymd +
        "T093000\n"
        "SUMMARY:Old\n"
        "RRULE:FREQ=DAILY\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan1, jan3);
    CHECK(parsed.error.empty());
    CHECK(count_text(parsed, "Old") == 3);
    CHECK(has_on(parsed, "Old", jan1));
    CHECK(has_on(parsed, "Old", jan2));
    CHECK(has_on(parsed, "Old", jan3));
    for (const auto& item : parsed.items) {
      CHECK(item.start_min == 9 * 60);
      CHECK(item.end_min == 9 * 60 + 30);
    }
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:19900101T090000\n"
        "DTEND:19900101T093000\n"
        "SUMMARY:Years\n"
        "RRULE:FREQ=DAILY\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan1, jan3);
    CHECK(count_text(parsed, "Years") == 3);
    CHECK(has_on(parsed, "Years", jan1));
    CHECK(has_on(parsed, "Years", jan3));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20251230T090000\n"
        "DTEND:20251230T100000\n"
        "SUMMARY:Counted\n"
        "RRULE:FREQ=DAILY;COUNT=3\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan1, jan3);
    CHECK(count_text(parsed, "Counted") == 1);
    CHECK(has_on(parsed, "Counted", jan1));
    CHECK(!has_on(parsed, "Counted", jan2));
  }

  {
    const std::string text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:" +
        early_ymd +
        "T090000\n"
        "DTEND:" +
        early_ymd +
        "T100000\n"
        "SUMMARY:Spent\n"
        "RRULE:FREQ=DAILY;COUNT=3\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan1, jan3);
    CHECK(count_text(parsed, "Spent") == 0);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20251231T090000\n"
        "DTEND:20251231T100000\n"
        "SUMMARY:EveryOther\n"
        "RRULE:FREQ=DAILY;INTERVAL=2\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const Glib::Date jan8 = day(8, Glib::Date::JANUARY, 2026);
    const auto parsed = ephemeris::parse_ics(text, jan1, jan8);
    CHECK(count_text(parsed, "EveryOther") == 4);
    CHECK(!has_on(parsed, "EveryOther", jan1));
    CHECK(has_on(parsed, "EveryOther", jan2));
    CHECK(has_on(parsed, "EveryOther", day(4, Glib::Date::JANUARY, 2026)));
    CHECK(has_on(parsed, "EveryOther", day(6, Glib::Date::JANUARY, 2026)));
    CHECK(has_on(parsed, "EveryOther", jan8));
  }

  {
    Glib::Date night = early;
    Glib::Date morning = early;
    morning.add_days(1);
    const std::string text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:" +
        ymd(night) +
        "T220000\n"
        "DTEND:" +
        ymd(morning) +
        "T010000\n"
        "SUMMARY:Nightly\n"
        "RRULE:FREQ=DAILY\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan1, jan1);
    CHECK(count_text(parsed, "Nightly") == 2);
    bool tail = false;
    bool head = false;
    for (const auto& item : parsed.items) {
      CHECK(item.date.compare(jan1) == 0);
      if (item.start_min == 0 && item.end_min == 60)
        tail = true;
      if (item.start_min == 22 * 60 && item.end_min == 24 * 60)
        head = true;
    }
    CHECK(tail);
    CHECK(head);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260101T090000\n"
        "DTEND:20260101T100000\n"
        "SUMMARY:Capped\n"
        "RRULE:FREQ=DAILY\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan1, day(31, Glib::Date::DECEMBER, 2028));
    CHECK(count_text(parsed, "Capped") == 800);
    CHECK(has_on(parsed, "Capped", jan1));
    Glib::Date last = jan1;
    last.add_days(799);
    CHECK(has_on(parsed, "Capped", last));
    Glib::Date after = last;
    after.add_days(1);
    CHECK(!has_on(parsed, "Capped", after));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20100101T120000\n"
        "DTEND:20100101T130000\n"
        "SUMMARY:Weekly\n"
        "RRULE:FREQ=WEEKLY\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const Glib::Date from = day(1, Glib::Date::JANUARY, 2026);
    const Glib::Date to = day(31, Glib::Date::JANUARY, 2026);
    const auto parsed = ephemeris::parse_ics(text, from, to);
    const Glib::Date anchor = day(1, Glib::Date::JANUARY, 2010);
    int expect = 0;
    for (Glib::Date d = from; d.compare(to) <= 0; d.add_days(1)) {
      const int delta = static_cast<int>(d.get_julian()) - static_cast<int>(anchor.get_julian());
      if (delta % 7 == 0)
        ++expect;
    }
    CHECK(expect >= 4);
    CHECK(count_text(parsed, "Weekly") == expect);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:19900115T090000\n"
        "DTEND:19900115T100000\n"
        "SUMMARY:Month\n"
        "RRULE:FREQ=MONTHLY\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan1, day(31, Glib::Date::JANUARY, 2026));
    CHECK(count_text(parsed, "Month") == 1);
    CHECK(has_on(parsed, "Month", day(15, Glib::Date::JANUARY, 2026)));
  }

  return suite_test::done("window");
}
