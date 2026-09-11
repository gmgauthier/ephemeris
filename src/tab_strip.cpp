/* SPDX-License-Identifier: Unlicense */

#include "tab_strip.hpp"

namespace ephemeris {

TabStrip::TabStrip() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4)
{
  set_valign(Gtk::ALIGN_START);
  set_margin_top(24);
  cal_.set_size_request(72, 48);
  todo_.set_size_request(72, 48);
  cal_.set_relief(Gtk::RELIEF_NONE);
  todo_.set_relief(Gtk::RELIEF_NONE);
  cal_.get_style_context()->add_class("ephemeris-tab");
  cal_.get_style_context()->add_class("ephemeris-tab-cal");
  todo_.get_style_context()->add_class("ephemeris-tab");
  todo_.get_style_context()->add_class("ephemeris-tab-todo");
  cal_.signal_clicked().connect(sigc::mem_fun(*this, &TabStrip::on_cal));
  todo_.signal_clicked().connect(sigc::mem_fun(*this, &TabStrip::on_todo));
  pack_start(cal_, Gtk::PACK_SHRINK);
  pack_start(todo_, Gtk::PACK_SHRINK);
  restyle();
}

void TabStrip::set_section(Section s)
{
  section_ = s;
  restyle();
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

void TabStrip::restyle()
{
  auto cal_ctx = cal_.get_style_context();
  auto todo_ctx = todo_.get_style_context();
  if (section_ == Section::calendar) {
    cal_ctx->add_class("ephemeris-tab-active");
    todo_ctx->remove_class("ephemeris-tab-active");
  } else {
    todo_ctx->add_class("ephemeris-tab-active");
    cal_ctx->remove_class("ephemeris-tab-active");
  }
}

}  // namespace ephemeris
