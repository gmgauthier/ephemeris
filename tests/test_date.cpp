/* SPDX-License-Identifier: Unlicense */

#include "binder.hpp"
#include "check.hpp"

#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>

namespace {

Glib::Date day(int d, Glib::Date::Month m, int y)
{
  return Glib::Date(static_cast<Glib::Date::Day>(d), m, static_cast<Glib::Date::Year>(y));
}

bool on_date(const Glib::Date& d, int dd, Glib::Date::Month m, int y)
{
  return d.valid() && static_cast<int>(d.get_day()) == dd && d.get_month() == m &&
         static_cast<int>(d.get_year()) == y;
}

bool unchanged(const Glib::Date& d, guint32 julian)
{
  return d.valid() && d.get_julian() == julian;
}

class TempBook {
 public:
  TempBook()
  {
    char tmpl[] = "/tmp/ephemeris-date-XXXXXX";
    if (char* made = mkdtemp(tmpl))
      dir_ = made;
    if (!dir_.empty())
      path_ = dir_ + "/book.ephemeris";
  }

  ~TempBook()
  {
    if (!path_.empty())
      std::remove(path_.c_str());
    if (!dir_.empty())
      rmdir(dir_.c_str());
  }

  const std::string& path() const
  {
    return path_;
  }

 private:
  std::string dir_;
  std::string path_;
};

}  // namespace

int main()
{
  const Glib::Date tenth = day(10, Glib::Date::JANUARY, 2026);
  const guint32 tenth_julian = tenth.get_julian();

  {
    Glib::Date out = tenth;
    CHECK(!ephemeris::date_from_iso("2026-02-31", out));
    CHECK(unchanged(out, tenth_julian));
  }

  {
    Glib::Date out;
    CHECK(!out.valid());
    CHECK(!ephemeris::date_from_iso("2026-02-31", out));
    CHECK(!out.valid());
  }

  {
    const char* rejected[] = {"2026-02-29", "2026-02-30",  "2026-04-31", "2026-06-31",
                              "2026-09-31", "2026-11-31",  "1900-02-29", "2100-02-29",
                              "0-01-01",    "10000-01-01", "2026/02/01", "tomorrow",
                              "",           "2026-02-00",  "2026-13-01", "2026-00-10"};
    for (const char* text : rejected) {
      Glib::Date out = tenth;
      CHECK(!ephemeris::date_from_iso(text, out));
      CHECK(unchanged(out, tenth_julian));
    }
  }

  {
    Glib::Date out = tenth;
    CHECK(ephemeris::date_from_iso("2024-02-29", out));
    CHECK(on_date(out, 29, Glib::Date::FEBRUARY, 2024));
    CHECK(ephemeris::date_from_iso("2000-02-29", out));
    CHECK(on_date(out, 29, Glib::Date::FEBRUARY, 2000));
    CHECK(ephemeris::date_from_iso("2026-02-28", out));
    CHECK(on_date(out, 28, Glib::Date::FEBRUARY, 2026));
    CHECK(ephemeris::date_from_iso("2026-04-30", out));
    CHECK(on_date(out, 30, Glib::Date::APRIL, 2026));
    CHECK(ephemeris::date_from_iso("2026-01-31", out));
    CHECK(on_date(out, 31, Glib::Date::JANUARY, 2026));
    CHECK(ephemeris::date_from_iso("2026-12-31", out));
    CHECK(on_date(out, 31, Glib::Date::DECEMBER, 2026));
    CHECK(ephemeris::date_iso(out) == "2026-12-31");
  }

  {
    Glib::Date out = tenth;
    CHECK(!ephemeris::stamp_to_date("2026-02-31T12:00", out));
    CHECK(unchanged(out, tenth_julian));
    CHECK(ephemeris::stamp_to_date("2026-01-15T08:30", out));
    CHECK(on_date(out, 15, Glib::Date::JANUARY, 2026));
    CHECK(!ephemeris::stamp_to_date("2026-02", out));
    CHECK(on_date(out, 15, Glib::Date::JANUARY, 2026));
  }

  {
    TempBook tmp;
    CHECK(!tmp.path().empty());
    {
      ephemeris::Binder book;
      ephemeris::Appointment kept;
      kept.date = day(10, Glib::Date::JANUARY, 2026);
      kept.text = "Kept";
      kept.start_min = 9 * 60;
      kept.end_min = 9 * 60 + 30;
      book.add_appointment(kept);
      ephemeris::Todo real;
      real.text = "Real";
      real.has_due = true;
      real.due = day(30, Glib::Date::APRIL, 2026);
      book.add_todo(real);
      ephemeris::Todo leap;
      leap.text = "Leap";
      leap.has_due = true;
      leap.due = day(29, Glib::Date::FEBRUARY, 2024);
      book.add_todo(leap);
      ephemeris::PlannerEvent span;
      span.start = day(1, Glib::Date::MARCH, 2026);
      span.end = day(2, Glib::Date::MARCH, 2026);
      span.text = "Real span";
      book.add_planner(span);
      CHECK(book.save_as(tmp.path()));
    }
    {
      ephemeris::Binder loaded;
      CHECK(loaded.open(tmp.path()));
      const auto appts = loaded.for_date(day(10, Glib::Date::JANUARY, 2026));
      CHECK(appts.size() == 1);
      CHECK(appts[0].text == "Kept");
      const auto due = loaded.todos_due_on(day(30, Glib::Date::APRIL, 2026));
      CHECK(due.size() == 1);
      CHECK(due[0].text == "Real");
      const auto leaps = loaded.todos_due_on(day(29, Glib::Date::FEBRUARY, 2024));
      CHECK(leaps.size() == 1);
      CHECK(leaps[0].text == "Leap");
      const auto spans = loaded.planner_on(day(1, Glib::Date::MARCH, 2026));
      CHECK(spans.size() == 1);
      CHECK(spans[0].text == "Real span");
    }
  }

  {
    TempBook tmp;
    CHECK(!tmp.path().empty());
    const std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ephemeris version=\"1\">\n"
        "  <appointment id=\"1\" date=\"2026-01-10\" start=\"09:00\" "
        "end=\"09:30\">Kept</appointment>\n"
        "  <appointment id=\"2\" date=\"2026-02-31\" start=\"09:00\" "
        "end=\"09:30\">Impossible</appointment>\n"
        "  <todo id=\"1\" done=\"false\" priority=\"0\" status=\"not_started\" percent=\"0\" "
        "text=\"Real\" due=\"2026-04-30\"></todo>\n"
        "  <todo id=\"2\" done=\"false\" priority=\"0\" status=\"not_started\" percent=\"0\" "
        "text=\"Bad\" due=\"2026-02-31\"></todo>\n"
        "  <todo id=\"3\" done=\"false\" priority=\"0\" status=\"not_started\" percent=\"0\" "
        "text=\"Leap\" due=\"2024-02-29\"></todo>\n"
        "  <planner id=\"1\" start=\"2026-06-31\" end=\"2026-07-01\">Bad span</planner>\n"
        "  <planner id=\"2\" start=\"2026-03-01\" end=\"2026-03-02\">Real span</planner>\n"
        "</ephemeris>\n";
    {
      std::ofstream out(tmp.path());
      CHECK(static_cast<bool>(out));
      out << xml;
      CHECK(static_cast<bool>(out));
    }
    ephemeris::Binder book;
    CHECK(book.open(tmp.path()));
    const auto kept = book.for_date(day(10, Glib::Date::JANUARY, 2026));
    CHECK(kept.size() == 1);
    CHECK(kept[0].text == "Kept");
    CHECK(book.for_date(day(31, Glib::Date::JANUARY, 2026)).empty());
    const ephemeris::Appointment* impossible = book.find(2);
    CHECK(impossible != nullptr);
    if (impossible) {
      CHECK(impossible->text == "Impossible");
      CHECK(!impossible->date.valid());
    }
    const ephemeris::Todo* bad = book.find_todo(2);
    CHECK(bad != nullptr);
    if (bad) {
      CHECK(bad->text == "Bad");
      CHECK(!bad->has_due);
      CHECK(!bad->due.valid());
    }
    const auto real = book.todos_due_on(day(30, Glib::Date::APRIL, 2026));
    CHECK(real.size() == 1);
    CHECK(real[0].text == "Real");
    const auto leap = book.todos_due_on(day(29, Glib::Date::FEBRUARY, 2024));
    CHECK(leap.size() == 1);
    CHECK(leap[0].text == "Leap");
    bool bad_span = false;
    bool real_span = false;
    for (const auto& p : book.planner_events()) {
      if (p.text == "Bad span") {
        bad_span = true;
        CHECK(!p.start.valid());
        CHECK(on_date(p.end, 1, Glib::Date::JULY, 2026));
      }
      if (p.text == "Real span") {
        real_span = true;
        CHECK(on_date(p.start, 1, Glib::Date::MARCH, 2026));
        CHECK(on_date(p.end, 2, Glib::Date::MARCH, 2026));
      }
    }
    CHECK(bad_span);
    CHECK(real_span);
    CHECK(book.planner_on(day(1, Glib::Date::JULY, 2026)).empty());
  }

  return suite_test::done("date");
}
