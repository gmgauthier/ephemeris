/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "binder.hpp"

#include <gtkmm.h>
#include <glibmm/date.h>

namespace ephemeris {

class PlannerPage : public Gtk::Box {
 public:
  PlannerPage();

  void set_binder(Binder* b);
  void set_year(Glib::Date::Year year);
  Glib::Date::Year year() const
  {
    return year_;
  }
  void today();
  void prev_year();
  void next_year();
  void refresh();

  sigc::signal<void>& signal_changed()
  {
    return signal_changed_;
  }

 private:
  void rebuild();
  void paint_range(const Glib::Date& a, const Glib::Date& b);
  void edit_event(int id);
  void rename_key(int index);
  Gtk::Widget* make_day(int month, int day);

  Binder* binder_ = nullptr;
  Glib::Date::Year year_ = 2026;
  int category_ = 0;
  bool dragging_ = false;
  Glib::Date drag_start_;
  Gtk::Label head_;
  Gtk::Grid grid_;
  Gtk::Box legend_{Gtk::ORIENTATION_HORIZONTAL, 8};
  Gtk::Label hint_;
  sigc::signal<void> signal_changed_;
};

}  // namespace ephemeris
