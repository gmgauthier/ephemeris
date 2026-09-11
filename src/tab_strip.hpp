/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <gtkmm.h>

namespace ephemeris {

enum class Section { calendar, todo };

class TabStrip : public Gtk::Box {
 public:
  TabStrip();

  void set_section(Section s);
  sigc::signal<void, Section>& signal_section() { return signal_section_; }

 private:
  void on_cal();
  void on_todo();
  void restyle();

  Gtk::Button cal_{"Calendar"};
  Gtk::Button todo_{"To Do"};
  Section section_ = Section::calendar;
  sigc::signal<void, Section> signal_section_;
};

}  // namespace ephemeris
