/* SPDX-License-Identifier: Unlicense */

#include "binder.hpp"
#include "check.hpp"

#include <cstdio>
#include <string>
#include <unistd.h>
#include <vector>

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

ephemeris::Todo due_todo(const char* text, Glib::Date due, ephemeris::Recur recur, int interval)
{
  ephemeris::Todo t;
  t.text = text;
  t.due = due;
  t.has_due = true;
  t.recur = recur;
  t.recur_interval = interval;
  t.priority = 2;
  t.category = "Home";
  t.notes = "keep";
  return t;
}

const ephemeris::Todo* only_open(const std::vector<ephemeris::Todo>& todos)
{
  const ephemeris::Todo* open = nullptr;
  for (const auto& t : todos) {
    if (t.done)
      continue;
    if (open)
      return nullptr;
    open = &t;
  }
  return open;
}

class TempBook {
 public:
  TempBook()
  {
    char tmpl[] = "/tmp/ephemeris-recur-XXXXXX";
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
  {
    ephemeris::Binder book;
    const int id = book.add_todo(
        due_todo("Rent", day(31, Glib::Date::JANUARY, 2026), ephemeris::Recur::monthly, 1));
    CHECK(book.complete_todo(id));
    const auto todos = book.todos();
    CHECK(todos.size() == 2);
    CHECK(todos[1].done);
    CHECK(todos[1].id == id);
    CHECK(on_date(todos[1].due, 31, Glib::Date::JANUARY, 2026));
    CHECK(todos[1].percent == 100);
    const ephemeris::Todo* open = only_open(todos);
    CHECK(open != nullptr);
    if (open) {
      CHECK(on_date(open->due, 31, Glib::Date::MARCH, 2026));
      CHECK(open->text == "Rent");
      CHECK(open->notes == "keep");
      CHECK(open->category == "Home");
      CHECK(open->priority == 2);
      CHECK(open->recur == ephemeris::Recur::monthly);
      CHECK(open->recur_interval == 1);
      CHECK(open->percent == 0);
      CHECK(!open->done);
      const int next = open->id;
      CHECK(book.complete_todo(id));
      CHECK(book.open_todo_count() == 1);
      CHECK(book.complete_todo(next));
      const auto again = book.todos();
      const ephemeris::Todo* third = only_open(again);
      CHECK(third != nullptr);
      if (third)
        CHECK(on_date(third->due, 31, Glib::Date::MAY, 2026));
    }
  }

  {
    ephemeris::Binder book;
    const int id = book.add_todo(
        due_todo("Rent", day(31, Glib::Date::MARCH, 2026), ephemeris::Recur::monthly, 1));
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open)
      CHECK(on_date(open->due, 31, Glib::Date::MAY, 2026));
  }

  {
    ephemeris::Binder book;
    const int id = book.add_todo(
        due_todo("Rent", day(31, Glib::Date::JANUARY, 2026), ephemeris::Recur::monthly, 2));
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open)
      CHECK(on_date(open->due, 31, Glib::Date::MARCH, 2026));
  }

  {
    ephemeris::Binder book;
    const int id = book.add_todo(
        due_todo("Rent", day(31, Glib::Date::JANUARY, 2026), ephemeris::Recur::monthly, 3));
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open)
      CHECK(on_date(open->due, 31, Glib::Date::JULY, 2026));
  }

  {
    ephemeris::Binder book;
    const int id = book.add_todo(
        due_todo("Bills", day(30, Glib::Date::JANUARY, 2026), ephemeris::Recur::monthly, 1));
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open)
      CHECK(on_date(open->due, 30, Glib::Date::MARCH, 2026));
  }

  {
    ephemeris::Binder book;
    const int id = book.add_todo(
        due_todo("Bills", day(28, Glib::Date::JANUARY, 2026), ephemeris::Recur::monthly, 1));
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open)
      CHECK(on_date(open->due, 28, Glib::Date::FEBRUARY, 2026));
  }

  {
    ephemeris::Binder book;
    const int id = book.add_todo(
        due_todo("Leap", day(29, Glib::Date::JANUARY, 2024), ephemeris::Recur::monthly, 1));
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open)
      CHECK(on_date(open->due, 29, Glib::Date::FEBRUARY, 2024));
  }

  {
    ephemeris::Binder book;
    const int id = book.add_todo(
        due_todo("Leap", day(29, Glib::Date::JANUARY, 2025), ephemeris::Recur::monthly, 1));
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open)
      CHECK(on_date(open->due, 29, Glib::Date::MARCH, 2025));
  }

  {
    ephemeris::Binder book;
    ephemeris::Todo t =
        due_todo("Leap", day(29, Glib::Date::FEBRUARY, 2024), ephemeris::Recur::yearly, 1);
    const int id = book.add_todo(t);
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open) {
      CHECK(on_date(open->due, 29, Glib::Date::FEBRUARY, 2028));
      CHECK(book.complete_todo(open->id));
      const auto held_next = book.todos();
      const ephemeris::Todo* next = only_open(held_next);
      CHECK(next != nullptr);
      if (next)
        CHECK(on_date(next->due, 29, Glib::Date::FEBRUARY, 2032));
    }
  }

  {
    ephemeris::Binder book;
    const int id = book.add_todo(
        due_todo("Leap", day(29, Glib::Date::FEBRUARY, 2024), ephemeris::Recur::yearly, 4));
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open)
      CHECK(on_date(open->due, 29, Glib::Date::FEBRUARY, 2028));
  }

  {
    ephemeris::Binder book;
    const int id = book.add_todo(
        due_todo("Century", day(29, Glib::Date::FEBRUARY, 2000), ephemeris::Recur::yearly, 100));
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open)
      CHECK(on_date(open->due, 29, Glib::Date::FEBRUARY, 2400));
  }

  {
    ephemeris::Binder book;
    const int id = book.add_todo(
        due_todo("Tax", day(31, Glib::Date::JANUARY, 2026), ephemeris::Recur::yearly, 1));
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open)
      CHECK(on_date(open->due, 31, Glib::Date::JANUARY, 2027));
  }

  {
    ephemeris::Binder book;
    const int id = book.add_todo(
        due_todo("Daily", day(31, Glib::Date::JANUARY, 2026), ephemeris::Recur::daily, 1));
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open) {
      CHECK(on_date(open->due, 1, Glib::Date::FEBRUARY, 2026));
      CHECK(book.complete_todo(open->id));
      const auto held_next = book.todos();
      const ephemeris::Todo* next = only_open(held_next);
      CHECK(next != nullptr);
      if (next)
        CHECK(on_date(next->due, 2, Glib::Date::FEBRUARY, 2026));
    }
  }

  {
    ephemeris::Binder book;
    const int id = book.add_todo(
        due_todo("Daily", day(31, Glib::Date::JANUARY, 2026), ephemeris::Recur::daily, 2));
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open)
      CHECK(on_date(open->due, 2, Glib::Date::FEBRUARY, 2026));
  }

  {
    ephemeris::Binder book;
    const int id = book.add_todo(
        due_todo("Week", day(31, Glib::Date::JANUARY, 2026), ephemeris::Recur::weekly, 1));
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open) {
      CHECK(on_date(open->due, 7, Glib::Date::FEBRUARY, 2026));
      CHECK(open->due.get_weekday() == Glib::Date::SATURDAY);
    }
  }

  {
    ephemeris::Binder book;
    ephemeris::Todo t =
        due_todo("Span", day(31, Glib::Date::JANUARY, 2026), ephemeris::Recur::monthly, 1);
    t.has_start = true;
    t.start = day(15, Glib::Date::JANUARY, 2026);
    const int id = book.add_todo(t);
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open) {
      CHECK(on_date(open->start, 15, Glib::Date::FEBRUARY, 2026));
      CHECK(on_date(open->due, 31, Glib::Date::MARCH, 2026));
    }
  }

  {
    ephemeris::Binder book;
    ephemeris::Todo t;
    t.text = "Leap start";
    t.has_start = true;
    t.start = day(29, Glib::Date::FEBRUARY, 2024);
    t.recur = ephemeris::Recur::yearly;
    t.recur_interval = 1;
    const int id = book.add_todo(t);
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open) {
      CHECK(!open->has_due);
      CHECK(on_date(open->start, 29, Glib::Date::FEBRUARY, 2028));
    }
  }

  {
    ephemeris::Binder book;
    ephemeris::Todo t =
        due_todo("Until", day(31, Glib::Date::JANUARY, 2026), ephemeris::Recur::monthly, 1);
    t.has_until = true;
    t.until = day(28, Glib::Date::FEBRUARY, 2026);
    const int id = book.add_todo(t);
    CHECK(book.complete_todo(id));
    CHECK(book.todos().size() == 1);
    CHECK(book.open_todo_count() == 0);
    CHECK(book.todos()[0].done);
    CHECK(on_date(book.todos()[0].due, 31, Glib::Date::JANUARY, 2026));
  }

  {
    ephemeris::Binder book;
    ephemeris::Todo t =
        due_todo("Until", day(31, Glib::Date::JANUARY, 2026), ephemeris::Recur::monthly, 1);
    t.has_until = true;
    t.until = day(31, Glib::Date::MARCH, 2026);
    const int id = book.add_todo(t);
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open)
      CHECK(on_date(open->due, 31, Glib::Date::MARCH, 2026));
  }

  {
    ephemeris::Binder book;
    ephemeris::Todo t =
        due_todo("Once", day(31, Glib::Date::JANUARY, 2026), ephemeris::Recur::none, 1);
    const int id = book.add_todo(t);
    CHECK(book.complete_todo(id));
    CHECK(book.todos().size() == 1);
    CHECK(book.todos()[0].done);
    CHECK(on_date(book.todos()[0].due, 31, Glib::Date::JANUARY, 2026));
    CHECK(!book.complete_todo(404));
    CHECK(book.complete_todo(id));
    CHECK(book.todos().size() == 1);
  }

  {
    ephemeris::Binder book;
    ephemeris::Todo t =
        due_todo("Zero", day(31, Glib::Date::JANUARY, 2026), ephemeris::Recur::monthly, 0);
    const int id = book.add_todo(t);
    const ephemeris::Todo* stored = book.find_todo(id);
    CHECK(stored != nullptr);
    if (stored)
      CHECK(stored->recur_interval == 1);
    CHECK(book.complete_todo(id));
    const auto held = book.todos();
    const ephemeris::Todo* open = only_open(held);
    CHECK(open != nullptr);
    if (open)
      CHECK(on_date(open->due, 31, Glib::Date::MARCH, 2026));
  }

  {
    TempBook tmp;
    CHECK(!tmp.path().empty());
    ephemeris::Binder book;
    const int rent = book.add_todo(
        due_todo("Rent", day(31, Glib::Date::JANUARY, 2026), ephemeris::Recur::monthly, 1));
    const int leap = book.add_todo(
        due_todo("Leap", day(29, Glib::Date::FEBRUARY, 2024), ephemeris::Recur::yearly, 1));
    CHECK(book.save_as(tmp.path()));
    CHECK(book.complete_todo(rent));
    CHECK(book.complete_todo(leap));
    CHECK(book.save());

    ephemeris::Binder loaded;
    CHECK(loaded.open(tmp.path()));
    const auto loaded_todos = loaded.todos();
    CHECK(loaded_todos.size() == 4);
    const ephemeris::Todo* open_rent = nullptr;
    const ephemeris::Todo* open_leap = nullptr;
    for (const auto& t : loaded_todos) {
      if (t.done && t.text == "Rent")
        CHECK(on_date(t.due, 31, Glib::Date::JANUARY, 2026));
      if (t.done && t.text == "Leap")
        CHECK(on_date(t.due, 29, Glib::Date::FEBRUARY, 2024));
      if (!t.done && t.text == "Rent")
        open_rent = &t;
      if (!t.done && t.text == "Leap")
        open_leap = &t;
    }
    CHECK(open_rent != nullptr);
    CHECK(open_leap != nullptr);
    int rent_id = 0;
    if (open_rent && open_leap) {
      CHECK(on_date(open_rent->due, 31, Glib::Date::MARCH, 2026));
      CHECK(open_rent->notes == "keep");
      CHECK(open_rent->category == "Home");
      CHECK(on_date(open_leap->due, 29, Glib::Date::FEBRUARY, 2028));
      CHECK(open_leap->notes == "keep");
      rent_id = open_rent->id;
    }
    if (rent_id) {
      CHECK(loaded.complete_todo(rent_id));
      CHECK(loaded.save());
      ephemeris::Binder again;
      CHECK(again.open(tmp.path()));
      const auto again_todos = again.todos();
      CHECK(again_todos.size() == 5);
      const ephemeris::Todo* newest = nullptr;
      int leap_open = 0;
      for (const auto& t : again_todos) {
        if (!t.done && t.text == "Rent")
          newest = &t;
        if (!t.done && t.text == "Leap") {
          ++leap_open;
          CHECK(on_date(t.due, 29, Glib::Date::FEBRUARY, 2028));
        }
      }
      CHECK(leap_open == 1);
      CHECK(newest != nullptr);
      if (newest)
        CHECK(on_date(newest->due, 31, Glib::Date::MAY, 2026));
    }
  }

  return suite_test::done("recur");
}
