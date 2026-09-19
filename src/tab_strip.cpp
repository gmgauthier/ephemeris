/* SPDX-License-Identifier: Unlicense */

#include "tab_strip.hpp"

namespace ephemeris {

TabStrip::TabStrip()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4)
{
  set_valign(Gtk::ALIGN_START);
  set_margin_top(28);
  set_margin_start(0);
  set_margin_end(4);
  set_spacing(6);
  auto prep = [](Gtk::Button& b, const char* klass) {
    b.set_size_request(80, 40);
    b.set_relief(Gtk::RELIEF_NONE);
    b.set_margin_start(0);
    b.set_margin_end(0);
    b.get_style_context()->add_class("ephemeris-tab");
    b.get_style_context()->add_class(klass);
  };
  prep(cal_, "ephemeris-tab-cal");
  prep(todo_, "ephemeris-tab-todo");
  prep(contacts_, "ephemeris-tab-contacts");
  prep(planner_, "ephemeris-tab-planner");
  prep(notepad_, "ephemeris-tab-notepad");
  cal_.signal_clicked().connect(sigc::mem_fun(*this, &TabStrip::on_cal));
  todo_.signal_clicked().connect(sigc::mem_fun(*this, &TabStrip::on_todo));
  contacts_.signal_clicked().connect(sigc::mem_fun(*this, &TabStrip::on_contacts));
  planner_.signal_clicked().connect(sigc::mem_fun(*this, &TabStrip::on_planner));
  notepad_.signal_clicked().connect(sigc::mem_fun(*this, &TabStrip::on_notepad));
  pack_start(cal_, Gtk::PACK_SHRINK);
  pack_start(todo_, Gtk::PACK_SHRINK);
  pack_start(contacts_, Gtk::PACK_SHRINK);
  pack_start(planner_, Gtk::PACK_SHRINK);
  pack_start(notepad_, Gtk::PACK_SHRINK);
  restyle();
}

void TabStrip::set_section(Section s)
{
  section_ = s;
  restyle();
}

void TabStrip::set_open_count(int n)
{
  open_count_ = n < 0 ? 0 : n;
  update_todo_label();
}

void TabStrip::update_todo_label()
{
  if (open_count_ > 0)
    todo_.set_label(Glib::ustring::compose("To Do (%1)", open_count_));
  else
    todo_.set_label("To Do");
}

void TabStrip::on_cal()
{
  section_ = Section::calendar;
  restyle();
  signal_section_.emit(section_);
}

void TabStrip::on_todo()
{
  section_ = Section::todo;
  restyle();
  signal_section_.emit(section_);
}

void TabStrip::on_contacts()
{
  section_ = Section::contacts;
  restyle();
  signal_section_.emit(section_);
}

void TabStrip::on_planner()
{
  section_ = Section::planner;
  restyle();
  signal_section_.emit(section_);
}

void TabStrip::on_notepad()
{
  section_ = Section::notepad;
  restyle();
  signal_section_.emit(section_);
}

void TabStrip::restyle()
{
  auto set_on = [](Gtk::Button& b, bool on) {
    if (on)
      b.get_style_context()->add_class("ephemeris-tab-active");
    else
      b.get_style_context()->remove_class("ephemeris-tab-active");
  };
  set_on(cal_, section_ == Section::calendar);
  set_on(todo_, section_ == Section::todo);
  set_on(contacts_, section_ == Section::contacts);
  set_on(planner_, section_ == Section::planner);
  set_on(notepad_, section_ == Section::notepad);
}

}  // namespace ephemeris
