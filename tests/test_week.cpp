/* SPDX-License-Identifier: Unlicense */

#include "binder.hpp"
#include "check.hpp"
#include "todo_page.hpp"

#include <cstdio>
#include <string>
#include <unistd.h>

namespace {

Glib::Date day(int d, Glib::Date::Month m, int y)
{
  return Glib::Date(static_cast<Glib::Date::Day>(d), m, static_cast<Glib::Date::Year>(y));
}

class TempBook {
 public:
  TempBook()
  {
    char tmpl[] = "/tmp/ephemeris-week-XXXXXX";
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

const ephemeris::Todo* kept(ephemeris::Binder& binder, int id, const char* text)
{
  const ephemeris::Todo* t = binder.find_todo(id);
  CHECK(t != nullptr);
  if (t)
    CHECK(t->text == text);
  return t;
}

}  // namespace

int main()
{
  const Glib::Date today = day(28, Glib::Date::JANUARY, 2026);
  const Glib::Date yesterday = day(27, Glib::Date::JANUARY, 2026);
  const Glib::Date tomorrow = day(29, Glib::Date::JANUARY, 2026);
  const Glib::Date sixth = day(3, Glib::Date::FEBRUARY, 2026);
  const Glib::Date seventh = day(4, Glib::Date::FEBRUARY, 2026);

  CHECK(ephemeris::in_next_days(today, today, 7));
  CHECK(ephemeris::in_next_days(tomorrow, today, 7));
  CHECK(ephemeris::in_next_days(sixth, today, 7));
  CHECK(!ephemeris::in_next_days(seventh, today, 7));
  CHECK(!ephemeris::in_next_days(yesterday, today, 7));

  Glib::Date blank;
  blank.clear();
  CHECK(!ephemeris::in_next_days(blank, today, 7));
  CHECK(!ephemeris::in_next_days(today, blank, 7));
  CHECK(!ephemeris::in_next_days(today, today, 0));
  CHECK(!ephemeris::in_next_days(today, today, -3));
  CHECK(ephemeris::in_next_days(today, today, 1));
  CHECK(!ephemeris::in_next_days(tomorrow, today, 1));

  const Glib::Date dec30 = day(30, Glib::Date::DECEMBER, 2026);
  const Glib::Date jan5 = day(5, Glib::Date::JANUARY, 2027);
  const Glib::Date jan6 = day(6, Glib::Date::JANUARY, 2027);
  CHECK(ephemeris::in_next_days(dec30, dec30, 7));
  CHECK(ephemeris::in_next_days(jan5, dec30, 7));
  CHECK(!ephemeris::in_next_days(jan6, dec30, 7));

  const Glib::Date six_back = day(22, Glib::Date::JANUARY, 2026);
  const Glib::Date seven_back = day(21, Glib::Date::JANUARY, 2026);
  CHECK(ephemeris::in_last_days(today, today, 7));
  CHECK(ephemeris::in_last_days(yesterday, today, 7));
  CHECK(ephemeris::in_last_days(six_back, today, 7));
  CHECK(!ephemeris::in_last_days(seven_back, today, 7));
  CHECK(!ephemeris::in_last_days(tomorrow, today, 7));
  CHECK(!ephemeris::in_last_days(blank, today, 7));
  CHECK(!ephemeris::in_last_days(today, blank, 7));
  CHECK(!ephemeris::in_last_days(today, today, 0));

  TempBook book;
  CHECK(!book.path().empty());
  ephemeris::Binder binder;
  auto add = [&](const char* text, bool has_due, const Glib::Date& due) {
    ephemeris::Todo t;
    t.text = text;
    t.has_due = has_due;
    if (has_due)
      t.due = due;
    return binder.add_todo(t);
  };
  const int id_today = add("Due today", true, today);
  const int id_last = add("Last day", true, sixth);
  const int id_after = add("Day after", true, seventh);
  const int id_over = add("Yesterday", true, yesterday);
  const int id_none = add("No due", false, today);
  CHECK(binder.save_as(book.path()));

  ephemeris::Binder loaded;
  CHECK(loaded.open(book.path()));
  const ephemeris::Todo* due_today = kept(loaded, id_today, "Due today");
  const ephemeris::Todo* last_day = kept(loaded, id_last, "Last day");
  const ephemeris::Todo* day_after = kept(loaded, id_after, "Day after");
  const ephemeris::Todo* overdue = kept(loaded, id_over, "Yesterday");
  const ephemeris::Todo* no_due = kept(loaded, id_none, "No due");
  CHECK(due_today && due_today->has_due && ephemeris::in_next_days(due_today->due, today, 7));
  CHECK(last_day && last_day->has_due && ephemeris::in_next_days(last_day->due, today, 7));
  CHECK(day_after && day_after->has_due && !ephemeris::in_next_days(day_after->due, today, 7));
  CHECK(overdue && overdue->has_due && !ephemeris::in_next_days(overdue->due, today, 7));
  CHECK(no_due && !no_due->has_due && !ephemeris::in_next_days(no_due->due, today, 7));

  return suite_test::done("week");
}
