/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "binder.hpp"
#include "day_spread.hpp"
#include "month_page.hpp"
#include "rings.hpp"
#include "settings.hpp"
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
  void show_month();
  void show_spread(const Glib::Date& left);
  void refresh_marks();
  void set_status(const Glib::ustring& text);
  void update_title();
  void show_error(const Glib::ustring& message);
  bool confirm_discard();
  bool do_save();
  bool do_save_as();
  std::string ensure_suffix(const std::string& path) const;
  std::string samples_dir() const;

  void on_new();
  void on_open();
  void on_save();
  void on_save_as();
  void on_today();
  void on_prev();
  void on_next();
  void on_month_btn();
  void on_quit();
  void on_about();
  void on_not_yet(const Glib::ustring& feature);
  void on_day(const Glib::Date& date);
  void on_binder_changed();
  void persist();
  void restore_session();
  void on_print();
  void on_print_day();
  void on_print_month();
  void on_print_todos();
  bool in_editable_focus() const;

  Gtk::MenuItem* add_item(Gtk::Menu& menu, const Glib::ustring& label,
                          const sigc::slot<void()>& slot, guint key = 0,
                          Gdk::ModifierType mods = Gdk::ModifierType(0));

 protected:
  bool on_delete_event(GdkEventAny* event) override;
  bool on_key_press_event(GdkEventKey* event) override;

  Gtk::Box root_{Gtk::ORIENTATION_VERTICAL, 0};
  Gtk::MenuBar menubar_;
  Gtk::Box toolbar_{Gtk::ORIENTATION_HORIZONTAL, 4};
  Gtk::Button btn_prev_{"Prev"};
  Gtk::Button btn_next_{"Next"};
  Gtk::Box book_{Gtk::ORIENTATION_HORIZONTAL, 0};
  Rings rings_;
  Gtk::EventBox sheet_;
  Gtk::Stack pages_;
  MonthPage month_;
  DaySpread spread_;
  TodoPage todo_;
  TabStrip tabs_;
  Gtk::Statusbar status_;
  guint status_ctx_ = 0;
  Glib::RefPtr<Gtk::AccelGroup> accel_;
  Gtk::RadioButtonGroup section_group_;
  Gtk::RadioMenuItem* cal_item_ = nullptr;
  Gtk::RadioMenuItem* todo_item_ = nullptr;
  bool suppress_section_ = false;
  enum class CalView { month, spread };
  CalView cal_view_ = CalView::month;
  Binder binder_;
  Settings settings_;
};

}  // namespace ephemeris
