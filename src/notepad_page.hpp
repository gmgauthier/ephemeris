/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "binder.hpp"

#include <gtkmm.h>

namespace ephemeris {

class NotepadPage : public Gtk::Box {
 public:
  NotepadPage();

  void set_binder(Binder* b);
  void refresh();

  sigc::signal<void>& signal_changed()
  {
    return signal_changed_;
  }

 private:
  void save_current();
  void load_id(int id);
  void add_page();
  void delete_page();
  void on_select();
  void set_editor_open(bool on);
  void fill_category_combo();
  std::vector<Note> visible() const;
  void rebuild_list();
  void rebuild_icons();
  Gtk::Widget* make_chip(const Note& n);

  Binder* binder_ = nullptr;
  int current_id_ = 0;
  Gtk::ComboBoxText view_;
  Gtk::Entry search_;
  Gtk::ScrolledWindow list_scroll_;
  Gtk::ScrolledWindow icon_scroll_;
  Gtk::ListBox list_;
  Gtk::FlowBox icons_;
  Gtk::Entry title_;
  Gtk::Label stamp_;
  Gtk::ComboBoxText colour_;
  Gtk::ComboBoxText category_{true};
  Gtk::TextView body_;
  bool suppress_ = false;
  sigc::signal<void> signal_changed_;
};

}  // namespace ephemeris
