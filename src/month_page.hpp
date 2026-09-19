/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <gtkmm.h>
#include <glibmm/date.h>

#include <map>
#include <set>

namespace ephemeris {

class MonthPage : public Gtk::Box {
 public:
  MonthPage();

  void set_month(Glib::Date::Month month, Glib::Date::Year year);
  void today();
  void prev_month();
  void next_month();
  void set_marks(std::set<int> days, std::map<int, Glib::ustring> tips = {});
  Glib::Date::Month month() const
  {
    return month_;
  }
  Glib::Date::Year year() const
  {
    return year_;
  }
  Glib::ustring title() const;

  sigc::signal<void, Glib::Date>& signal_day_chosen()
  {
    return signal_day_chosen_;
  }

 private:
  void rebuild();
  Gtk::Widget* make_day(int day, bool in_month, bool is_today, bool busy, const Glib::ustring& tip);

  Gtk::Label head_;
  Gtk::Grid grid_;
  Glib::Date::Month month_ = Glib::Date::JANUARY;
  Glib::Date::Year year_ = 2026;
  std::set<int> marks_;
  std::map<int, Glib::ustring> tips_;
  sigc::signal<void, Glib::Date> signal_day_chosen_;
};

}  // namespace ephemeris
