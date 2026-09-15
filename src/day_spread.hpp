/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "binder.hpp"

#include <gtkmm.h>
#include <glibmm/date.h>

namespace ephemeris {

class DaySpread : public Gtk::Box {
 public:
  DaySpread();

  void set_binder(Binder* b);
  void set_left_date(const Glib::Date& d);
  Glib::Date left_date() const
  {
    return left_;
  }
  void refresh();
  void prev_spread();
  void next_spread();
  void today();

  sigc::signal<void>& signal_changed()
  {
    return signal_changed_;
  }
  sigc::signal<void>& signal_goto_todo()
  {
    return signal_goto_todo_;
  }

 private:
  Gtk::Widget* build_day(const Glib::Date& date, bool right);
  void edit_slot(const Glib::Date& date, int start_min, int appt_id);

  Binder* binder_ = nullptr;
  Glib::Date left_;
  Gtk::Box cols_{Gtk::ORIENTATION_HORIZONTAL, 16};
  sigc::signal<void> signal_changed_;
  sigc::signal<void> signal_goto_todo_;
};

}  // namespace ephemeris
