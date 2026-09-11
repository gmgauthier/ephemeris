/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "month_page.hpp"
#include "rings.hpp"
#include "tab_strip.hpp"
#include "todo_page.hpp"

#include <gtkmm.h>

namespace ephemeris {

class MainWindow : public Gtk::Window {
 public:
  MainWindow();

 private:
  void load_css();
  void build_menu();
  void build_toolbar();
  void show_section(Section s);
  void on_today();
  void on_prev();
  void on_next();
  void on_quit();
  void on_about();
  void on_not_yet(const Glib::ustring& feature);
  void on_day(const Glib::Date& date);

  Gtk::MenuItem* add_item(Gtk::Menu& menu, const Glib::ustring& label,
                          const sigc::slot<void()>& slot, guint key = 0,
                          Gdk::ModifierType mods = Gdk::ModifierType(0));

  Gtk::Box root_{Gtk::ORIENTATION_VERTICAL, 0};
  Gtk::MenuBar menubar_;
  Gtk::Box toolbar_{Gtk::ORIENTATION_HORIZONTAL, 4};
  Gtk::Box book_{Gtk::ORIENTATION_HORIZONTAL, 0};
  Rings rings_;
  Gtk::Stack pages_;
  MonthPage month_;
  TodoPage todo_;
  TabStrip tabs_;
  Gtk::Statusbar status_;
  guint status_ctx_ = 0;
  Glib::RefPtr<Gtk::AccelGroup> accel_;
  Gtk::RadioButtonGroup section_group_;
  Gtk::RadioMenuItem* cal_item_ = nullptr;
  Gtk::RadioMenuItem* todo_item_ = nullptr;
  bool suppress_section_ = false;
};

}  // namespace ephemeris
