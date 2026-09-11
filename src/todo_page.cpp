/* SPDX-License-Identifier: Unlicense */

#include "todo_page.hpp"

namespace ephemeris {

TodoPage::TodoPage() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4)
{
  get_style_context()->add_class("ephemeris-page");
  set_border_width(12);
  auto* head = Gtk::manage(new Gtk::Label());
  head->set_markup("<b>To Do</b>");
  head->set_halign(Gtk::ALIGN_START);
  head->get_style_context()->add_class("ephemeris-month-head");
  pack_start(*head, Gtk::PACK_SHRINK);
  auto* hint = Gtk::manage(new Gtk::Label("Tasks come in M2. This page is the lined list."));
  hint->set_halign(Gtk::ALIGN_START);
  hint->get_style_context()->add_class("ephemeris-todo-line");
  pack_start(*hint, Gtk::PACK_SHRINK);
  for (int i = 0; i < 12; ++i) {
    auto* line = Gtk::manage(new Gtk::Label("☐  ··································"));
    line->set_halign(Gtk::ALIGN_START);
    line->get_style_context()->add_class("ephemeris-todo-line");
    pack_start(*line, Gtk::PACK_SHRINK);
  }
}

}  // namespace ephemeris
