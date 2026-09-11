/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "binder.hpp"

#include <gtkmm.h>

#include <vector>

namespace ephemeris {

class ContactsPage : public Gtk::Box {
 public:
  ContactsPage();

  void set_binder(Binder* b);
  void refresh();

  sigc::signal<void>& signal_changed() { return signal_changed_; }

 private:
  void on_add_contact();
  void edit_item(int id);
  void jump_to(gunichar letter);
  void rebuild_az(const std::vector<Contact>& people);
  Gtk::Widget* make_row(const Contact& c);

  Binder* binder_ = nullptr;
  Gtk::Box az_{Gtk::ORIENTATION_HORIZONTAL, 0};
  Gtk::Box list_{Gtk::ORIENTATION_VERTICAL, 2};
  Gtk::ScrolledWindow scroll_;
  std::vector<std::pair<gunichar, Gtk::Widget*>> rows_;
  sigc::signal<void> signal_changed_;
};

}  // namespace ephemeris
