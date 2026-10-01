/* SPDX-License-Identifier: Unlicense */

#include "binder.hpp"
#include "check.hpp"

#include <cstdio>
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

class TempBook {
 public:
  TempBook()
  {
    char tmpl[] = "/tmp/ephemeris-keepdate-XXXXXX";
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
  const Glib::Date due = day(10, Glib::Date::JANUARY, 2026);
  const Glib::Date start = day(5, Glib::Date::JANUARY, 2026);
  const Glib::Date until = day(1, Glib::Date::JUNE, 2026);

  {
    Glib::Date out = due;
    CHECK(ephemeris::apply_iso_date(true, "tomorrow", out));
    CHECK(on_date(out, 10, Glib::Date::JANUARY, 2026));
    CHECK(ephemeris::apply_iso_date(true, "2026/02/01", out));
    CHECK(on_date(out, 10, Glib::Date::JANUARY, 2026));
    CHECK(ephemeris::apply_iso_date(true, "", out));
    CHECK(on_date(out, 10, Glib::Date::JANUARY, 2026));
    CHECK(ephemeris::apply_iso_date(true, "2026-02-31", out));
    CHECK(on_date(out, 10, Glib::Date::JANUARY, 2026));
  }

  {
    Glib::Date out = start;
    CHECK(ephemeris::apply_iso_date(true, "tomorrow", out));
    CHECK(on_date(out, 5, Glib::Date::JANUARY, 2026));
    out = until;
    CHECK(ephemeris::apply_iso_date(true, "2026/02/01", out));
    CHECK(on_date(out, 1, Glib::Date::JUNE, 2026));
  }

  {
    Glib::Date out = due;
    CHECK(ephemeris::apply_iso_date(true, "2026-03-15", out));
    CHECK(on_date(out, 15, Glib::Date::MARCH, 2026));
    CHECK(!ephemeris::apply_iso_date(false, "2026-04-01", out));
    CHECK(on_date(out, 15, Glib::Date::MARCH, 2026));
  }

  {
    Glib::Date out;
    CHECK(!ephemeris::apply_iso_date(true, "tomorrow", out));
    CHECK(!out.valid());
    CHECK(!ephemeris::apply_iso_date(true, "2026/02/01", out));
    CHECK(!out.valid());
    CHECK(ephemeris::apply_iso_date(true, "2026-04-30", out));
    CHECK(on_date(out, 30, Glib::Date::APRIL, 2026));
  }

  {
    TempBook tmp;
    CHECK(!tmp.path().empty());
    ephemeris::Binder book;
    ephemeris::Todo t;
    t.text = "Pay";
    t.has_due = true;
    t.due = due;
    t.has_start = true;
    t.start = start;
    t.has_until = true;
    t.until = until;
    t.recur = ephemeris::Recur::monthly;
    const int id = book.add_todo(t);
    ephemeris::Appointment a;
    a.date = due;
    a.text = "Meet";
    a.recur = ephemeris::Recur::weekly;
    a.has_until = true;
    a.until = until;
    const int aid = book.add_appointment(a);
    CHECK(book.save_as(tmp.path()));

    ephemeris::Todo edited = *book.find_todo(id);
    edited.has_due = ephemeris::apply_iso_date(true, "tomorrow", edited.due);
    edited.has_start = ephemeris::apply_iso_date(true, "2026/02/01", edited.start);
    edited.has_until = ephemeris::apply_iso_date(true, "next week", edited.until);
    CHECK(book.update_todo(edited));
    ephemeris::Appointment edited_a = *book.find(aid);
    edited_a.has_until = ephemeris::apply_iso_date(true, "tomorrow", edited_a.until);
    CHECK(book.update_appointment(edited_a));
    CHECK(book.save());

    ephemeris::Binder loaded;
    CHECK(loaded.open(tmp.path()));
    const ephemeris::Todo* kept = loaded.find_todo(id);
    CHECK(kept != nullptr);
    if (kept) {
      CHECK(kept->has_due);
      CHECK(on_date(kept->due, 10, Glib::Date::JANUARY, 2026));
      CHECK(kept->has_start);
      CHECK(on_date(kept->start, 5, Glib::Date::JANUARY, 2026));
      CHECK(kept->has_until);
      CHECK(on_date(kept->until, 1, Glib::Date::JUNE, 2026));
      CHECK(kept->text == "Pay");
    }
    const ephemeris::Appointment* kept_a = loaded.find(aid);
    CHECK(kept_a != nullptr);
    if (kept_a) {
      CHECK(kept_a->has_until);
      CHECK(on_date(kept_a->until, 1, Glib::Date::JUNE, 2026));
      CHECK(kept_a->text == "Meet");
    }

    ephemeris::Todo next = *loaded.find_todo(id);
    next.has_due = ephemeris::apply_iso_date(true, "2026-03-15", next.due);
    next.has_start = ephemeris::apply_iso_date(true, "2026-02-31", next.start);
    CHECK(loaded.update_todo(next));
    ephemeris::Appointment next_a = *loaded.find(aid);
    next_a.has_until = ephemeris::apply_iso_date(true, "2026-08-01", next_a.until);
    CHECK(loaded.update_appointment(next_a));
    CHECK(loaded.save());

    ephemeris::Binder again;
    CHECK(again.open(tmp.path()));
    const ephemeris::Todo* moved = again.find_todo(id);
    CHECK(moved != nullptr);
    if (moved) {
      CHECK(on_date(moved->due, 15, Glib::Date::MARCH, 2026));
      CHECK(on_date(moved->start, 5, Glib::Date::JANUARY, 2026));
      CHECK(on_date(moved->until, 1, Glib::Date::JUNE, 2026));
    }
    const ephemeris::Appointment* moved_a = again.find(aid);
    CHECK(moved_a != nullptr);
    if (moved_a)
      CHECK(on_date(moved_a->until, 1, Glib::Date::AUGUST, 2026));

    ephemeris::Todo cleared = *again.find_todo(id);
    cleared.has_due = ephemeris::apply_iso_date(false, "2026-01-10", cleared.due);
    CHECK(again.update_todo(cleared));
    CHECK(again.save());
    ephemeris::Binder off;
    CHECK(off.open(tmp.path()));
    const ephemeris::Todo* undated = off.find_todo(id);
    CHECK(undated != nullptr);
    if (undated) {
      CHECK(!undated->has_due);
      CHECK(undated->has_start);
      CHECK(on_date(undated->start, 5, Glib::Date::JANUARY, 2026));
    }

    ephemeris::Todo fresh;
    fresh.text = "New";
    fresh.has_due = ephemeris::apply_iso_date(true, "tomorrow", fresh.due);
    const int nid = off.add_todo(fresh);
    CHECK(off.save());
    ephemeris::Binder blank;
    CHECK(blank.open(tmp.path()));
    const ephemeris::Todo* created = blank.find_todo(nid);
    CHECK(created != nullptr);
    if (created) {
      CHECK(created->text == "New");
      CHECK(!created->has_due);
      CHECK(!created->due.valid());
    }
  }

  return suite_test::done("keepdate");
}
