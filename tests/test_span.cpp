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

bool has_slice(const ephemeris::ParsedIcs& parsed, const std::string& text, const Glib::Date& date,
               int start_min, int end_min)
{
  for (const auto& item : parsed.items) {
    if (item.text == text && item.date.compare(date) == 0 && item.start_min == start_min &&
        item.end_min == end_min)
      return true;
  }
  return false;
}

}  // namespace

int main()
{
  const Glib::Date jan = day(1, Glib::Date::JANUARY, 2026);
  const Glib::Date dec = day(31, Glib::Date::DECEMBER, 2026);
  const Glib::Date mon = day(5, Glib::Date::JANUARY, 2026);
  const Glib::Date tue = day(6, Glib::Date::JANUARY, 2026);
  const Glib::Date wed = day(7, Glib::Date::JANUARY, 2026);

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T220000\n"
        "DTEND:20260106T010000\n"
        "SUMMARY:Night\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(parsed.error.empty());
    CHECK(count_text(parsed, "Night") == 2);
    CHECK(has_slice(parsed, "Night", mon, 22 * 60, 24 * 60));
    CHECK(has_slice(parsed, "Night", tue, 0, 60));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T220000\n"
        "DTEND:20260106T000000\n"
        "SUMMARY:ToMidnight\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "ToMidnight") == 1);
    CHECK(has_slice(parsed, "ToMidnight", mon, 22 * 60, 24 * 60));
    CHECK(!has_slice(parsed, "ToMidnight", tue, 0, 0));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T220000\n"
        "DURATION:PT3H\n"
        "SUMMARY:ThreeHours\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "ThreeHours") == 2);
    CHECK(has_slice(parsed, "ThreeHours", mon, 22 * 60, 24 * 60));
    CHECK(has_slice(parsed, "ThreeHours", tue, 0, 60));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T220000\n"
        "DURATION:PT2H\n"
        "SUMMARY:UntilMidnight\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "UntilMidnight") == 1);
    CHECK(has_slice(parsed, "UntilMidnight", mon, 22 * 60, 24 * 60));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T220000\n"
        "DTEND:20260106T010000\n"
        "SUMMARY:Once\n"
        "RRULE:FREQ=DAILY;COUNT=1\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "Once") == 2);
    CHECK(has_slice(parsed, "Once", mon, 22 * 60, 24 * 60));
    CHECK(has_slice(parsed, "Once", tue, 0, 60));
    CHECK(!has_slice(parsed, "Once", tue, 22 * 60, 24 * 60));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T220000\n"
        "DTEND:20260106T010000\n"
        "SUMMARY:Shift\n"
        "RRULE:FREQ=DAILY;COUNT=2\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "Shift") == 4);
    CHECK(has_slice(parsed, "Shift", mon, 22 * 60, 24 * 60));
    CHECK(has_slice(parsed, "Shift", tue, 0, 60));
    CHECK(has_slice(parsed, "Shift", tue, 22 * 60, 24 * 60));
    CHECK(has_slice(parsed, "Shift", wed, 0, 60));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T220000\n"
        "DTEND:20260107T010000\n"
        "SUMMARY:TwoNights\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "TwoNights") == 3);
    CHECK(has_slice(parsed, "TwoNights", mon, 22 * 60, 24 * 60));
    CHECK(has_slice(parsed, "TwoNights", tue, 0, 24 * 60));
    CHECK(has_slice(parsed, "TwoNights", wed, 0, 60));
  }

  {
    const Glib::Date only_tue = day(6, Glib::Date::JANUARY, 2026);
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T220000\n"
        "DTEND:20260106T010000\n"
        "SUMMARY:Tail\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, only_tue, only_tue);
    CHECK(count_text(parsed, "Tail") == 1);
    CHECK(has_slice(parsed, "Tail", tue, 0, 60));
    CHECK(!has_slice(parsed, "Tail", mon, 22 * 60, 24 * 60));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T220000\n"
        "DTEND:20260105T230000\n"
        "DURATION:PT5H\n"
        "SUMMARY:EndWins\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "EndWins") == 1);
    CHECK(has_slice(parsed, "EndWins", mon, 22 * 60, 23 * 60));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T090000\n"
        "DTEND:20260105T103000\n"
        "SUMMARY:SameDay\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "SameDay") == 1);
    CHECK(has_slice(parsed, "SameDay", mon, 9 * 60, 10 * 60 + 30));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T220000\n"
        "DURATION:P1DT3H\n"
        "SUMMARY:DayAndHours\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, jan, dec);
    CHECK(count_text(parsed, "DayAndHours") == 3);
    CHECK(has_slice(parsed, "DayAndHours", mon, 22 * 60, 24 * 60));
    CHECK(has_slice(parsed, "DayAndHours", tue, 0, 24 * 60));
    CHECK(has_slice(parsed, "DayAndHours", wed, 0, 60));
  }

  return suite_test::done("span");
}
