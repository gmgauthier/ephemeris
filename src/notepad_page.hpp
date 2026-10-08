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
  void show_note(int id);

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
  void commit_category();
  void edit_categories();
  bool ask_name(Gtk::Window& parent, const Glib::ustring& title, Glib::ustring& name);
  void apply_paper(NoteColour colour);
  void ensure_tags();
  void fill_body(const Note* n);
  std::vector<NotePara> paras_from_view();
  bool starts_bullet_at(const Gtk::TextIter& line) const;
  void on_insert(const Gtk::TextIter& pos, const Glib::ustring& text, int bytes);
  void on_mark_set(const Gtk::TextIter& loc, const Glib::RefPtr<Gtk::TextBuffer::Mark>& mark);
  bool on_body_key(GdkEventKey* event);
  void adopt_format();
  void sync_format_buttons();
  void on_bold_toggled();
  void on_italic_toggled();
  void on_bullet_toggled();
  void apply_bullets(bool want);
  bool selection_is_bullets();
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
  Gtk::Entry journal_;
  Gtk::ComboBoxText colour_;
  Gtk::ComboBoxText category_{true};
  Gtk::Button categories_{"Categories…"};
  Gtk::ToggleButton bold_btn_{"Bold"};
  Gtk::ToggleButton italic_btn_{"Italic"};
  Gtk::ToggleButton bullet_btn_{"Bullets"};
  Gtk::ScrolledWindow* paper_ = nullptr;
  Gtk::TextView body_;
  Glib::RefPtr<Gtk::TextTag> bold_tag_;
  Glib::RefPtr<Gtk::TextTag> italic_tag_;
  Glib::RefPtr<Gtk::TextTag> bullet_tag_;
  bool type_bold_ = false;
  bool type_italic_ = false;
  int pending_type_ = 0;
  bool skip_type_tag_ = false;
  bool suppress_format_ = false;
  bool suppress_ = false;
  sigc::signal<void> signal_changed_;
};

}  // namespace ephemeris
