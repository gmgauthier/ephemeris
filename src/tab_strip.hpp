/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <gtkmm.h>

namespace ephemeris {

enum class Section { calendar, todo };

class TabStrip : public Gtk::Box {
 public:
  TabStrip();

  void set_section(Section s);
  void set_open_count(int n);
  sigc::signal<void, Section>& signal_section() { return signal_section_; }

 private:
  void on_cal();
  void on_todo();
  void restyle();
  void update_todo_label();

  Gtk::Button cal_{"Calendar"};
  Gtk::Button todo_{"To Do"};
  Section section_ = Section::calendar;
  int open_count_ = 0;
  sigc::signal<void, Section> signal_section_;
};

}  // namespace ephemeris
