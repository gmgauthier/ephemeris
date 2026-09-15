/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "binder.hpp"

#include <gtkmm.h>

namespace ephemeris {

class TodoPage : public Gtk::Box {
 public:
  TodoPage();

  void set_binder(Binder* b);
  void refresh();

  sigc::signal<void>& signal_changed()
  {
    return signal_changed_;
  }

 private:
  void on_add_task();
  void edit_item(int id);
  Gtk::Widget* make_row(const Todo& t);

  Binder* binder_ = nullptr;
  Gtk::Box list_{Gtk::ORIENTATION_VERTICAL, 2};
  sigc::signal<void> signal_changed_;
};

}  // namespace ephemeris
