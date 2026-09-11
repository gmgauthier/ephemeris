/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <gtkmm.h>

namespace ephemeris {

enum class Section { calendar, todo, contacts };

class TabStrip : public Gtk::Box {
 public:
  TabStrip();

  void set_section(Section s);
  void set_open_count(int n);
  sigc::signal<void, Section>& signal_section() { return signal_section_; }

 private:
  void on_cal();
  void on_todo();
  void on_contacts();
  void restyle();
  void update_todo_label();

  Gtk::Button cal_{"Calendar"};
  Gtk::Button todo_{"To Do"};
  Gtk::Button contacts_{"Contacts"};
  Section section_ = Section::calendar;
  int open_count_ = 0;
  sigc::signal<void, Section> signal_section_;
};

}  // namespace ephemeris
