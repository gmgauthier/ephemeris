/* SPDX-License-Identifier: Unlicense */

#include "ics.hpp"
#include "vcard.hpp"
#include "check.hpp"

#include <string>

namespace {

Glib::Date day(int d, Glib::Date::Month m, int y)
{
  return Glib::Date(static_cast<Glib::Date::Day>(d), m, static_cast<Glib::Date::Year>(y));
}

bool has_day(const ephemeris::ParsedIcs& parsed, const Glib::Date& date)
{
  for (const auto& item : parsed.items) {
    if (item.date.compare(date) == 0)
      return true;
  }
  return false;
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
        "BEGIN:VCALENDAR\n"
        "X-WR-CALNAME:Desk\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260928\n"
        "DTEND;VALUE=DATE:20260929\n"
        "SUMMARY:Meet\n"
        "END:VEVENT\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260928\n"
        "SUMMARY:Gone\n"
        "STATUS:CANCELLED\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
    CHECK(parsed.title == "Desk");
    CHECK(parsed.items.size() == 1);
    CHECK(parsed.items[0].text == "Meet");
    CHECK(has_day(parsed, day(28, Glib::Date::SEPTEMBER, 2026)));
    CHECK(parsed.items[0].start_min == 0);
    CHECK(parsed.items[0].end_min == 24 * 60);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260928T090000\n"
        "DTEND:20260928T103000\n"
        "SUMMARY:Standup\n"
        "RRULE:FREQ=DAILY;COUNT=3\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 3);
    CHECK(has_day(parsed, day(28, Glib::Date::SEPTEMBER, 2026)));
    CHECK(has_day(parsed, day(29, Glib::Date::SEPTEMBER, 2026)));
    CHECK(has_day(parsed, day(30, Glib::Date::SEPTEMBER, 2026)));
    CHECK(parsed.items[0].start_min == 9 * 60);
    CHECK(parsed.items[0].end_min == 10 * 60 + 30);
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
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 4);
    CHECK(has_day(parsed, day(28, Glib::Date::SEPTEMBER, 2026)));
    CHECK(has_day(parsed, day(30, Glib::Date::SEPTEMBER, 2026)));
    CHECK(has_day(parsed, day(5, Glib::Date::OCTOBER, 2026)));
    CHECK(has_day(parsed, day(7, Glib::Date::OCTOBER, 2026)));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260131T090000\n"
        "DTEND:20260131T100000\n"
        "SUMMARY:Pay\n"
        "RRULE:FREQ=MONTHLY;COUNT=3\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.items.size() == 3);
    CHECK(has_day(parsed, day(31, Glib::Date::JANUARY, 2026)));
    CHECK(has_day(parsed, day(31, Glib::Date::MARCH, 2026)));
    CHECK(has_day(parsed, day(31, Glib::Date::MAY, 2026)));
    CHECK(!has_day(parsed, day(28, Glib::Date::FEBRUARY, 2026)));
    CHECK(!has_day(parsed, day(30, Glib::Date::APRIL, 2026)));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "X-WR-CALNAME:Desk\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T090000\n"
        "DTEND:20260105T100000\n"
        "SUMMARY:Board\n"
        "RRULE:FREQ=MONTHLY;BYDAY=1MO;COUNT=3\n"
        "END:VEVENT\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260106T150000\n"
        "DTEND:20260106T160000\n"
        "SUMMARY:Once\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
    CHECK(parsed.title == "Desk");
    int boards = 0;
    int onces = 0;
    for (const auto& item : parsed.items) {
      if (item.text == "Board") {
        ++boards;
        CHECK(item.start_min == 9 * 60);
        CHECK(item.end_min == 10 * 60);
      } else if (item.text == "Once") {
        ++onces;
        CHECK(has_day(parsed, day(6, Glib::Date::JANUARY, 2026)));
        CHECK(item.start_min == 15 * 60);
      }
    }
    CHECK(boards == 3);
    CHECK(onces == 1);
    CHECK(has_day(parsed, day(5, Glib::Date::JANUARY, 2026)));
    CHECK(has_day(parsed, day(2, Glib::Date::FEBRUARY, 2026)));
    CHECK(has_day(parsed, day(2, Glib::Date::MARCH, 2026)));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260901T120000Z\n"
        "SUMMARY:Folded\n"
        "  note\n"
        "RRULE:FREQ=DAILY;UNTIL=20260902T120000Z;INTERVAL=1\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(!parsed.items.empty());
    CHECK(parsed.items[0].text == "Folded note");
    for (const auto& item : parsed.items)
      CHECK(item.date.compare(day(3, Glib::Date::SEPTEMBER, 2026)) < 0);
  }

  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "N:Gauthier;Greg;;;\r\n"
        "EMAIL:gmgauthier@protonmail.com\r\n"
        "TEL:555\r\n"
        "NOTE:hello\\, there\r\n"
        "END:VCARD\r\n"
        "BEGIN:VCARD\r\n"
        "FN:Only Name\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 2);
    CHECK(people[0].last == "Gauthier");
    CHECK(people[0].first == "Greg");
    CHECK(people[0].email == "gmgauthier@protonmail.com");
    CHECK(people[0].phone == "555");
    CHECK(people[0].notes == "hello, there");
    CHECK(people[1].first == "Only Name");
    const std::string dumped = ephemeris::contacts_to_vcf(people);
    const auto again = ephemeris::parse_vcf(dumped);
    CHECK(again.size() == 2);
    CHECK(again[0].last == "Gauthier");
    CHECK(again[0].first == "Greg");
    CHECK(again[0].email == people[0].email);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "X-WR-CALNAME:Desk\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T220000\n"
        "DTEND:20260106T010000\n"
        "SUMMARY:Night\n"
        "END:VEVENT\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260105T090000\n"
        "DTEND:20260105T103000\n"
        "SUMMARY:Standup\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
    CHECK(parsed.title == "Desk");
    int nights = 0;
    int standups = 0;
    for (const auto& item : parsed.items) {
      if (item.text == "Night") {
        ++nights;
        if (item.date.compare(day(5, Glib::Date::JANUARY, 2026)) == 0) {
          CHECK(item.start_min == 22 * 60);
          CHECK(item.end_min == 24 * 60);
        } else if (item.date.compare(day(6, Glib::Date::JANUARY, 2026)) == 0) {
          CHECK(item.start_min == 0);
          CHECK(item.end_min == 60);
        } else {
          CHECK(false);
        }
      } else if (item.text == "Standup") {
        ++standups;
        CHECK(item.date.compare(day(5, Glib::Date::JANUARY, 2026)) == 0);
        CHECK(item.start_min == 9 * 60);
        CHECK(item.end_min == 10 * 60 + 30);
      }
    }
    CHECK(nights == 2);
    CHECK(standups == 1);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "X-WR-CALNAME:Desk\n"
        "BEGIN:VEVENT\n"
        "DTSTART;VALUE=DATE:20260928\n"
        "DTEND;VALUE=DATE:20260930\n"
        "SUMMARY:Vacation\n"
        "END:VEVENT\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260928T090000\n"
        "DTEND:20260928T100000\n"
        "SUMMARY:Meet\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
    CHECK(parsed.title == "Desk");
    int vacations = 0;
    int meets = 0;
    for (const auto& item : parsed.items) {
      if (item.text == "Vacation") {
        ++vacations;
        CHECK(item.start_min == 0);
        CHECK(item.end_min == 24 * 60);
        CHECK(item.date.compare(day(28, Glib::Date::SEPTEMBER, 2026)) == 0 ||
              item.date.compare(day(29, Glib::Date::SEPTEMBER, 2026)) == 0);
      } else if (item.text == "Meet") {
        ++meets;
        CHECK(item.date.compare(day(28, Glib::Date::SEPTEMBER, 2026)) == 0);
        CHECK(item.start_min == 9 * 60);
        CHECK(item.end_min == 10 * 60);
      }
    }
    CHECK(vacations == 2);
    CHECK(meets == 1);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "X-WR-CALNAME:Desk\n"
        "BEGIN:VEVENT\n"
        "DTSTART:19900101T090000\n"
        "DTEND:19900101T093000\n"
        "SUMMARY:Ancient\n"
        "RRULE:FREQ=DAILY\n"
        "END:VEVENT\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260615T150000\n"
        "DTEND:20260615T160000\n"
        "SUMMARY:Meet\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
    CHECK(parsed.title == "Desk");
    int ancient = 0;
    int meets = 0;
    for (const auto& item : parsed.items) {
      if (item.text == "Ancient") {
        ++ancient;
        CHECK(item.start_min == 9 * 60);
        CHECK(item.end_min == 9 * 60 + 30);
      } else if (item.text == "Meet") {
        ++meets;
        CHECK(item.date.compare(day(15, Glib::Date::JUNE, 2026)) == 0);
        CHECK(item.start_min == 15 * 60);
        CHECK(item.end_min == 16 * 60);
      }
    }
    CHECK(ancient == 365);
    CHECK(meets == 1);
    CHECK(has_day(parsed, day(1, Glib::Date::JANUARY, 2026)));
    CHECK(has_day(parsed, day(31, Glib::Date::DECEMBER, 2026)));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "X-WR-CALNAME:Desk\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260106T090000\n"
        "DTEND:20260106T100000\n"
        "SUMMARY:Shift\n"
        "RRULE:FREQ=WEEKLY;BYDAY=SU,TU;COUNT=1\n"
        "END:VEVENT\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260106T150000\n"
        "DTEND:20260106T160000\n"
        "SUMMARY:Once\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
    CHECK(parsed.title == "Desk");
    int shifts = 0;
    int onces = 0;
    for (const auto& item : parsed.items) {
      if (item.text == "Shift") {
        ++shifts;
        CHECK(item.date.compare(day(6, Glib::Date::JANUARY, 2026)) == 0);
        CHECK(item.start_min == 9 * 60);
        CHECK(item.end_min == 10 * 60);
      } else if (item.text == "Once") {
        ++onces;
        CHECK(item.date.compare(day(6, Glib::Date::JANUARY, 2026)) == 0);
        CHECK(item.start_min == 15 * 60);
      }
    }
    CHECK(shifts == 1);
    CHECK(onces == 1);
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "X-WR-CALNAME:Desk\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260131T090000\n"
        "DTEND:20260131T100000\n"
        "SUMMARY:Pay\n"
        "RRULE:FREQ=MONTHLY;COUNT=3\n"
        "END:VEVENT\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260202T150000\n"
        "DTEND:20260202T160000\n"
        "SUMMARY:Once\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
    CHECK(parsed.title == "Desk");
    int pays = 0;
    int onces = 0;
    for (const auto& item : parsed.items) {
      if (item.text == "Pay") {
        ++pays;
        CHECK(item.start_min == 9 * 60);
        CHECK(item.end_min == 10 * 60);
        CHECK(item.date.get_day() == 31);
        CHECK(item.date.get_month() != Glib::Date::FEBRUARY);
      } else if (item.text == "Once") {
        ++onces;
        CHECK(item.date.compare(day(2, Glib::Date::FEBRUARY, 2026)) == 0);
        CHECK(item.start_min == 15 * 60);
        CHECK(item.end_min == 16 * 60);
      }
    }
    CHECK(pays == 3);
    CHECK(onces == 1);
    CHECK(has_day(parsed, day(31, Glib::Date::JANUARY, 2026)));
    CHECK(has_day(parsed, day(31, Glib::Date::MARCH, 2026)));
    CHECK(has_day(parsed, day(31, Glib::Date::MAY, 2026)));
    CHECK(!has_day(parsed, day(28, Glib::Date::FEBRUARY, 2026)));
  }

  {
    const char* text =
        "BEGIN:VCALENDAR\n"
        "X-WR-CALNAME;LANGUAGE=en:Desk\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260302T090000\n"
        "DTEND:20260302T100000\n"
        "SUMMARY;LANGUAGE=en:Meet\n"
        "RRULE;X-NAME=keep:FREQ=DAILY;COUNT=2\n"
        "END:VEVENT\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260302T150000\n"
        "DTEND:20260302T160000\n"
        "SUMMARY:Once\n"
        "END:VEVENT\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260302T180000\n"
        "DTEND:20260302T190000\n"
        "SUMMARY;LANGUAGE=en:Gone\n"
        "STATUS;LANGUAGE=en:CANCELLED\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
    CHECK(parsed.title == "Desk");
    int meets = 0;
    int onces = 0;
    int gones = 0;
    for (const auto& item : parsed.items) {
      if (item.text == "Meet") {
        ++meets;
        CHECK(item.start_min == 9 * 60);
        CHECK(item.end_min == 10 * 60);
      } else if (item.text == "Once") {
        ++onces;
        CHECK(item.date.compare(day(2, Glib::Date::MARCH, 2026)) == 0);
        CHECK(item.start_min == 15 * 60);
        CHECK(item.end_min == 16 * 60);
      } else if (item.text == "Gone") {
        ++gones;
      }
    }
    CHECK(meets == 2);
    CHECK(onces == 1);
    CHECK(gones == 0);
    CHECK(has_day(parsed, day(2, Glib::Date::MARCH, 2026)));
    CHECK(has_day(parsed, day(3, Glib::Date::MARCH, 2026)));
  }

  {
    const char* text =
        "begin:vcalendar\n"
        "x-wr-calname;language=en:Desk\n"
        "begin:vevent\n"
        "dtstart:20260302t090000\n"
        "dtend:20260302t100000\n"
        "summary;language=en:Meet\n"
        "rrule:freq=daily;count=2\n"
        "end:vevent\n"
        "BEGIN:VEVENT\n"
        "DTSTART:20260302T150000\n"
        "DTEND:20260302T160000\n"
        "SUMMARY:Once\n"
        "END:VEVENT\n"
        "begin:vevent\n"
        "dtstart:20260302t180000\n"
        "summary:Gone\n"
        "status:cancelled\n"
        "end:vevent\n"
        "end:vcalendar\n";
    const auto parsed = ephemeris::parse_ics(text, from, to);
    CHECK(parsed.error.empty());
    CHECK(parsed.title == "Desk");
    int meets = 0;
    int onces = 0;
    int gones = 0;
    for (const auto& item : parsed.items) {
      if (item.text == "Meet") {
        ++meets;
        CHECK(item.start_min == 9 * 60);
        CHECK(item.end_min == 10 * 60);
      } else if (item.text == "Once") {
        ++onces;
        CHECK(item.date.compare(day(2, Glib::Date::MARCH, 2026)) == 0);
        CHECK(item.start_min == 15 * 60);
      } else if (item.text == "Gone") {
        ++gones;
      }
    }
    CHECK(meets == 2);
    CHECK(onces == 1);
    CHECK(gones == 0);
    CHECK(has_day(parsed, day(3, Glib::Date::MARCH, 2026)));
  }

  return suite_test::done("calendar");
}
