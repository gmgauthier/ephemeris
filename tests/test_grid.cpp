/* SPDX-License-Identifier: Unlicense */

#include "check.hpp"
#include "day_spread.hpp"

#include <cstdio>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

Glib::Date day(int d, Glib::Date::Month m, int y)
{
  return Glib::Date(static_cast<Glib::Date::Day>(d), m, static_cast<Glib::Date::Year>(y));
}

ephemeris::Appointment appt(const char* text, int start, int end, bool remote = false)
{
  ephemeris::Appointment a;
  a.date = day(2, Glib::Date::OCTOBER, 2026);
  a.text = text;
  a.start_min = start;
  a.end_min = end;
  a.remote = remote;
  return a;
}

int find_text(const std::vector<ephemeris::Appointment>& items, const char* text)
{
  for (size_t i = 0; i < items.size(); ++i) {
    if (items[i].text == text)
      return static_cast<int>(i);
  }
  return -1;
}

bool opens(const std::vector<ephemeris::GridRow>& rows, int slot, int index)
{
  for (const auto& row : rows) {
    if (row.slot_min == slot && row.index == index)
      return true;
  }
  return false;
}

int rows_at(const std::vector<ephemeris::GridRow>& rows, int slot)
{
  int n = 0;
  for (const auto& row : rows) {
    if (row.slot_min == slot)
      ++n;
  }
  return n;
}

class TempBook {
 public:
  TempBook()
  {
    char tmpl[] = "/tmp/ephemeris-grid-XXXXXX";
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
  const int at9 = 9 * 60;
  const int at930 = 9 * 60 + 30;
  const int at10 = 10 * 60;
  const int at1030 = 10 * 60 + 30;

  {
    std::vector<ephemeris::Appointment> items;
    items.push_back(appt("Early", at9, at10));
    items.push_back(appt("Late", at930, at1030));
    const auto rows = ephemeris::day_grid_rows(items);
    const int early = find_text(items, "Early");
    const int late = find_text(items, "Late");
    CHECK(opens(rows, at9, early));
    CHECK(!opens(rows, at9, late));
    CHECK(opens(rows, at930, late));
    CHECK(!opens(rows, at930, early));
    CHECK(opens(rows, at10, late));
    CHECK(!opens(rows, at10, early));
    CHECK(opens(rows, 8 * 60, -1));
  }

  {
    std::vector<ephemeris::Appointment> items;
    items.push_back(appt("One", at9, at930));
    items.push_back(appt("Two", at9, at10));
    const auto rows = ephemeris::day_grid_rows(items);
    CHECK(rows_at(rows, at9) == 2);
    CHECK(opens(rows, at9, find_text(items, "One")));
    CHECK(opens(rows, at9, find_text(items, "Two")));
    CHECK(opens(rows, at930, find_text(items, "Two")));
    CHECK(!opens(rows, at930, find_text(items, "One")));
  }

  {
    std::vector<ephemeris::Appointment> items;
    items.push_back(appt("Only", at9, at10));
    const auto rows = ephemeris::day_grid_rows(items);
    const int only = find_text(items, "Only");
    CHECK(rows_at(rows, at9) == 1);
    CHECK(opens(rows, at9, only));
    CHECK(opens(rows, at930, only));
    CHECK(opens(rows, at10, -1));
  }

  {
    std::vector<ephemeris::Appointment> items;
    items.push_back(appt("All", 0, 24 * 60));
    items.push_back(appt("Dawn", 7 * 60, 8 * 60));
    items.push_back(appt("Evening", 19 * 60, 20 * 60));
    items.push_back(appt("Mid", 9 * 60 + 15, at10));
    const auto rows = ephemeris::day_grid_rows(items);
    CHECK(!opens(rows, 8 * 60, find_text(items, "All")));
    CHECK(!opens(rows, 8 * 60, find_text(items, "Dawn")));
    CHECK(!opens(rows, 17 * 60 + 30, find_text(items, "Evening")));
    CHECK(opens(rows, at9, find_text(items, "Mid")));
    CHECK(rows_at(rows, at9) == 1);
  }

  {
    const auto rows = ephemeris::day_grid_rows({});
    CHECK(rows.size() == 20);
    CHECK(rows.front().slot_min == 8 * 60);
    CHECK(rows.front().index == -1);
    CHECK(rows.back().slot_min == 17 * 60 + 30);
    CHECK(rows.back().index == -1);
  }

  {
    std::vector<ephemeris::Appointment> items;
    items.push_back(appt("Local", at9, at10, false));
    items.push_back(appt("Remote", at930, at1030, true));
    const auto rows = ephemeris::day_grid_rows(items);
    CHECK(opens(rows, at9, find_text(items, "Local")));
    CHECK(opens(rows, at930, find_text(items, "Remote")));
    CHECK(items[static_cast<size_t>(find_text(items, "Remote"))].remote);
  }

  {
    TempBook tmp;
    CHECK(!tmp.path().empty());
    const Glib::Date when = day(2, Glib::Date::OCTOBER, 2026);
    ephemeris::Binder book;
    ephemeris::Appointment early = appt("Early", at9, at10);
    ephemeris::Appointment late = appt("Late", at930, at1030);
    ephemeris::Appointment allday = appt("Holiday", 0, 24 * 60);
    ephemeris::Appointment evening = appt("Concert", 19 * 60, 20 * 60);
    CHECK(book.add_appointment(early) > 0);
    CHECK(book.add_appointment(late) > 0);
    CHECK(book.add_appointment(allday) > 0);
    CHECK(book.add_appointment(evening) > 0);
    CHECK(book.save_as(tmp.path()));

    ephemeris::Binder loaded;
    CHECK(loaded.open(tmp.path()));
    const auto items = loaded.for_date(when);
    CHECK(items.size() == 4);
    const auto rows = ephemeris::day_grid_rows(items);
    const int early_i = find_text(items, "Early");
    const int late_i = find_text(items, "Late");
    CHECK(early_i >= 0);
    CHECK(late_i >= 0);
    CHECK(opens(rows, at9, early_i));
    CHECK(opens(rows, at930, late_i));
    CHECK(!opens(rows, at930, early_i));
    CHECK(!opens(rows, at9, find_text(items, "Holiday")));
    CHECK(!opens(rows, 17 * 60 + 30, find_text(items, "Concert")));
  }

  return suite_test::done("grid");
}
