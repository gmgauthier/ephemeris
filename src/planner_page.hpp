/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "binder.hpp"

#include <gtkmm.h>
#include <glibmm/date.h>

#include <vector>

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
  void open_covering(const Glib::Date& date);
  void edit_event(int id, bool created);
  void rename_key(int index);
  Gtk::Widget* make_day(int month, int day);
  Glib::Date date_at_root(gdouble x_root, gdouble y_root) const;
  void sync_drag_style();

  struct DayCell {
    Gtk::EventBox* box = nullptr;
    Glib::Date date;
  };

  Binder* binder_ = nullptr;
  Glib::Date::Year year_ = 2026;
  int category_ = 0;
  bool dragging_ = false;
  Glib::Date drag_start_;
  Glib::Date drag_end_;
  std::vector<DayCell> cells_;
  Gtk::Label head_;
  Gtk::Grid grid_;
  Gtk::Box legend_{Gtk::ORIENTATION_HORIZONTAL, 8};
  Gtk::Label hint_;
  sigc::signal<void> signal_changed_;
};

/* Covering planner events in insertion order. The last one is painted on top. */
struct PlannerHit {
  int id = 0;
};

std::vector<PlannerHit> planner_hits(const std::vector<PlannerEvent>& ev, const Glib::Date& day);

}  // namespace ephemeris
