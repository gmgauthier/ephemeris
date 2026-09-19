/* SPDX-License-Identifier: Unlicense */

#include "notepad_page.hpp"

#include <glib.h>

namespace ephemeris {

NotepadPage::NotepadPage()
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8)
{
  get_style_context()->add_class("ephemeris-page");
  set_margin_start(16);
  set_margin_end(16);
  set_margin_top(12);
  set_margin_bottom(12);

  auto* left = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 6));
  auto* btns = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 6));
  auto* add = Gtk::manage(new Gtk::Button("Add"));
  auto* del = Gtk::manage(new Gtk::Button("Delete"));
  add->signal_clicked().connect(sigc::mem_fun(*this, &NotepadPage::add_page));
  del->signal_clicked().connect(sigc::mem_fun(*this, &NotepadPage::delete_page));
  btns->pack_start(*add, Gtk::PACK_SHRINK);
  btns->pack_start(*del, Gtk::PACK_SHRINK);
  left->pack_start(*btns, Gtk::PACK_SHRINK);
  auto* scroll = Gtk::manage(new Gtk::ScrolledWindow());
  scroll->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  scroll->add(list_);
  scroll->set_min_content_width(180);
  left->pack_start(*scroll, Gtk::PACK_EXPAND_WIDGET);
  list_.signal_row_selected().connect([this](Gtk::ListBoxRow*) { on_select(); });
  pack_start(*left, Gtk::PACK_SHRINK);

  auto* right = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 6));
  title_.set_placeholder_text("Title");
  title_.signal_changed().connect([this]() { save_current(); });
  right->pack_start(title_, Gtk::PACK_SHRINK);
  stamp_.set_halign(Gtk::ALIGN_START);
  stamp_.get_style_context()->add_class("ephemeris-note-stamp");
  right->pack_start(stamp_, Gtk::PACK_SHRINK);
  auto* bscroll = Gtk::manage(new Gtk::ScrolledWindow());
  bscroll->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  body_.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
  body_.get_buffer()->signal_changed().connect([this]() { save_current(); });
  bscroll->add(body_);
  right->pack_start(*bscroll, Gtk::PACK_EXPAND_WIDGET);
  pack_start(*right, Gtk::PACK_EXPAND_WIDGET);
  set_editor_open(false);
}

void NotepadPage::set_editor_open(bool on)
{
  title_.set_sensitive(on);
  body_.set_sensitive(on);
  stamp_.set_sensitive(on);
}

void NotepadPage::set_binder(Binder* b)
{
  binder_ = b;
}

void NotepadPage::save_current()
{
  if (suppress_ || !binder_ || current_id_ <= 0)
    return;
  Note n;
  if (const Note* old = binder_->find_note(current_id_))
    n = *old;
  n.id = current_id_;
  n.title = title_.get_text();
  n.body = body_.get_buffer()->get_text();
  binder_->update_note(n);
  signal_changed_.emit();
  /* refresh list labels without losing selection */
  for (auto* row : list_.get_children()) {
    auto* r = dynamic_cast<Gtk::ListBoxRow*>(row);
    if (!r)
      continue;
    if (GPOINTER_TO_INT(r->get_data("id")) != current_id_)
      continue;
    if (auto* lab = dynamic_cast<Gtk::Label*>(r->get_child()))
      lab->set_text(n.title.empty() ? Glib::ustring("(untitled)") : n.title);
  }
}

void NotepadPage::load_id(int id)
{
  suppress_ = true;
  current_id_ = id;
  const Note* n = binder_ ? binder_->find_note(id) : nullptr;
  title_.set_text(n ? n->title : Glib::ustring());
  body_.get_buffer()->set_text(n ? n->body : Glib::ustring());
  if (n && !n->stamped.empty())
    stamp_.set_text("Added " + n->stamped);
  else
    stamp_.set_text("");
  set_editor_open(n != nullptr);
  suppress_ = false;
}

void NotepadPage::refresh()
{
  const int keep = current_id_;
  for (auto* ch : list_.get_children())
    list_.remove(*ch);
  if (!binder_)
    return;
  Gtk::ListBoxRow* sel = nullptr;
  for (const auto& n : binder_->notes()) {
    auto* lab =
        Gtk::manage(new Gtk::Label(n.title.empty() ? Glib::ustring("(untitled)") : n.title));
    lab->set_xalign(0.0);
    lab->set_ellipsize(Pango::ELLIPSIZE_END);
    auto* row = Gtk::manage(new Gtk::ListBoxRow());
    row->set_data("id", GINT_TO_POINTER(n.id));
    row->add(*lab);
    list_.append(*row);
    if (n.id == keep)
      sel = row;
  }
  list_.show_all();
  if (sel) {
    list_.select_row(*sel);
    load_id(keep);
  } else
    load_id(0);
}

void NotepadPage::add_page()
{
  if (!binder_)
    return;
  save_current();
  Note n;
  n.title = "New page";
  n.stamped = now_stamp();
  current_id_ = binder_->add_note(n);
  signal_changed_.emit();
  refresh();
}

void NotepadPage::delete_page()
{
  if (!binder_ || current_id_ <= 0)
    return;
  binder_->remove_note(current_id_);
  current_id_ = 0;
  signal_changed_.emit();
  refresh();
}

void NotepadPage::on_select()
{
  auto* row = list_.get_selected_row();
  if (!row)
    return;
  const int id = GPOINTER_TO_INT(row->get_data("id"));
  if (id == current_id_ && title_.get_sensitive())
    return;
  if (id != current_id_)
    save_current();
  load_id(id);
}

}  // namespace ephemeris
