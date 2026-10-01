/* SPDX-License-Identifier: Unlicense */

#include "check.hpp"
#include "planner_page.hpp"

#include <cstdio>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

Glib::Date day(int d, Glib::Date::Month m, int y)
{
  return Glib::Date(static_cast<Glib::Date::Day>(d), m, static_cast<Glib::Date::Year>(y));
}

ephemeris::PlannerEvent span(int id, const char* text, const Glib::Date& start,
                             const Glib::Date& end, int category)
{
  ephemeris::PlannerEvent e;
  e.id = id;
  e.text = text;
  e.start = start;
  e.end = end;
  e.category = category;
  return e;
}

bool has_id(const std::vector<ephemeris::PlannerHit>& hits, int id)
{
  for (const auto& hit : hits) {
    if (hit.id == id)
      return true;
  }
  return false;
}

int top_id(const std::vector<ephemeris::PlannerHit>& hits)
{
  return hits.empty() ? 0 : hits.back().id;
}

class TempBook {
 public:
  TempBook()
  {
    char tmpl[] = "/tmp/ephemeris-planner-XXXXXX";
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
  const Glib::Date jan1 = day(1, Glib::Date::JANUARY, 2026);
  const Glib::Date jan5 = day(5, Glib::Date::JANUARY, 2026);
  const Glib::Date jan7 = day(7, Glib::Date::JANUARY, 2026);
  const Glib::Date jan8 = day(8, Glib::Date::JANUARY, 2026);
  const Glib::Date jan10 = day(10, Glib::Date::JANUARY, 2026);
  const Glib::Date jan11 = day(11, Glib::Date::JANUARY, 2026);
  const auto trip = span(1, "Trip", jan1, jan10, 0);
  const auto holiday = span(2, "Holiday", jan5, jan7, 1);

  {
    const std::vector<ephemeris::PlannerEvent> ev{trip, holiday};
    const auto first = ephemeris::planner_hits(ev, jan1);
    CHECK(first.size() == 1);
    CHECK(top_id(first) == 1);
    const auto mid = ephemeris::planner_hits(ev, jan5);
    CHECK(mid.size() == 2);
    CHECK(mid[0].id == 1);
    CHECK(top_id(mid) == 2);
    const auto last_overlap = ephemeris::planner_hits(ev, jan7);
    CHECK(last_overlap.size() == 2);
    CHECK(top_id(last_overlap) == 2);
    CHECK(has_id(last_overlap, 1));
    const auto after = ephemeris::planner_hits(ev, jan8);
    CHECK(after.size() == 1);
    CHECK(top_id(after) == 1);
    CHECK(ephemeris::planner_hits(ev, jan11).empty());
    CHECK(ephemeris::planner_hits(ev, day(31, Glib::Date::DECEMBER, 2025)).empty());
  }

  {
    const auto again = span(3, "Again", jan1, jan10, 2);
    const std::vector<ephemeris::PlannerEvent> ev{trip, holiday, again};
    const auto mid = ephemeris::planner_hits(ev, jan5);
    CHECK(mid.size() == 3);
    CHECK(mid[0].id == 1);
    CHECK(mid[1].id == 2);
    CHECK(top_id(mid) == 3);
    const auto tail = ephemeris::planner_hits(ev, jan8);
    CHECK(tail.size() == 2);
    CHECK(top_id(tail) == 3);
    CHECK(!has_id(tail, 2));
  }

  {
    ephemeris::PlannerEvent broken = trip;
    broken.start.clear();
    const std::vector<ephemeris::PlannerEvent> ev{broken, holiday};
    const auto mid = ephemeris::planner_hits(ev, jan5);
    CHECK(mid.size() == 1);
    CHECK(top_id(mid) == 2);
  }

  {
    const auto one_day = span(4, "Meet", jan5, jan5, 3);
    const std::vector<ephemeris::PlannerEvent> ev{one_day};
    CHECK(top_id(ephemeris::planner_hits(ev, jan5)) == 4);
    CHECK(ephemeris::planner_hits(ev, day(4, Glib::Date::JANUARY, 2026)).empty());
    CHECK(ephemeris::planner_hits(ev, day(6, Glib::Date::JANUARY, 2026)).empty());
  }

  {
    TempBook tmp;
    CHECK(!tmp.path().empty());
    ephemeris::Binder book;
    ephemeris::PlannerEvent first = trip;
    ephemeris::PlannerEvent second = holiday;
    ephemeris::PlannerEvent march;
    march.text = "March";
    march.category = 4;
    march.start = day(1, Glib::Date::MARCH, 2026);
    march.end = day(3, Glib::Date::MARCH, 2026);
    const int trip_id = book.add_planner(first);
    const int holiday_id = book.add_planner(second);
    const int march_id = book.add_planner(march);
    CHECK(trip_id > 0);
    CHECK(holiday_id > trip_id);
    CHECK(book.save_as(tmp.path()));

    ephemeris::Binder loaded;
    CHECK(loaded.open(tmp.path()));
    const auto stored = loaded.planner_events();
    const auto mid = ephemeris::planner_hits(stored, jan5);
    CHECK(mid.size() == 2);
    CHECK(has_id(mid, trip_id));
    CHECK(top_id(mid) == holiday_id);
    const auto painted = loaded.find_planner(top_id(mid));
    CHECK(painted != nullptr);
    if (painted) {
      CHECK(painted->text == "Holiday");
      CHECK(painted->category == 1);
    }
    const auto only_trip = ephemeris::planner_hits(stored, jan8);
    CHECK(only_trip.size() == 1);
    CHECK(top_id(only_trip) == trip_id);
    const auto march_hits = ephemeris::planner_hits(loaded.planner_on(march.start), march.start);
    CHECK(march_hits.size() == 1);
    CHECK(top_id(march_hits) == march_id);
    CHECK(ephemeris::planner_hits(stored, jan11).empty());
  }

  return suite_test::done("planner");
}
