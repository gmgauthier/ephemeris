/* SPDX-License-Identifier: Unlicense */

#include "notepad_page.hpp"

#include <glib.h>

#include <algorithm>

namespace ephemeris {
namespace {

Glib::ustring combo_text(Gtk::ComboBoxText& combo)
{
  if (Gtk::Entry* e = combo.get_entry())
    return e->get_text();
  return combo.get_active_text();
}

bool note_matches(const Note& n, const Glib::ustring& query)
{
  if (query.empty())
    return true;
  const Glib::ustring q = query.casefold();
  if (n.title.casefold().find(q) != Glib::ustring::npos)
    return true;
  if (n.body.casefold().find(q) != Glib::ustring::npos)
    return true;
  if (n.category.casefold().find(q) != Glib::ustring::npos)
    return true;
  return false;
}

const char* colour_css_class(NoteColour c)
{
  switch (c) {
    case NoteColour::blue:
      return "ephemeris-note-chip-blue";
    case NoteColour::green:
      return "ephemeris-note-chip-green";
    case NoteColour::pink:
      return "ephemeris-note-chip-pink";
    case NoteColour::white:
      return "ephemeris-note-chip-white";
    case NoteColour::yellow:
    default:
      return "ephemeris-note-chip-yellow";
  }
}

}  // namespace

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
  view_.append("icons", "Icons");
  view_.append("list", "List");
  view_.append("week", "Last seven days");
  view_.append("category", "By category");
  view_.append("colour", "By colour");
  view_.set_active_id("list");
  view_.signal_changed().connect([this]() { refresh(); });
  btns->pack_start(*add, Gtk::PACK_SHRINK);
  btns->pack_start(*del, Gtk::PACK_SHRINK);
  btns->pack_end(view_, Gtk::PACK_SHRINK);
  left->pack_start(*btns, Gtk::PACK_SHRINK);

  search_.set_placeholder_text("Search notes");
  search_.signal_changed().connect([this]() { refresh(); });
  left->pack_start(search_, Gtk::PACK_SHRINK);

  list_scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  list_scroll_.add(list_);
  list_scroll_.set_min_content_width(200);
  list_.signal_row_selected().connect([this](Gtk::ListBoxRow*) { on_select(); });
  left->pack_start(list_scroll_, Gtk::PACK_EXPAND_WIDGET);

  icon_scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  icons_.set_selection_mode(Gtk::SELECTION_NONE);
  icons_.set_homogeneous(false);
  icons_.set_max_children_per_line(3);
  icons_.set_row_spacing(6);
  icons_.set_column_spacing(6);
  icon_scroll_.add(icons_);
  icon_scroll_.set_min_content_width(200);
  icon_scroll_.set_no_show_all(true);
  icon_scroll_.hide();
  left->pack_start(icon_scroll_, Gtk::PACK_EXPAND_WIDGET);
  pack_start(*left, Gtk::PACK_SHRINK);

  auto* right = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 6));
  title_.set_placeholder_text("Title");
  title_.signal_changed().connect([this]() { save_current(); });
  right->pack_start(title_, Gtk::PACK_SHRINK);
  stamp_.set_halign(Gtk::ALIGN_START);
  stamp_.get_style_context()->add_class("ephemeris-note-stamp");
  right->pack_start(stamp_, Gtk::PACK_SHRINK);

  colour_.append("yellow", "Yellow");
  colour_.append("blue", "Blue");
  colour_.append("green", "Green");
  colour_.append("pink", "Pink");
  colour_.append("white", "White");
  colour_.set_active_id("yellow");
  colour_.signal_changed().connect([this]() { save_current(); });
  category_.signal_changed().connect([this]() { save_current(); });
  if (Gtk::Entry* e = category_.get_entry())
    e->signal_changed().connect([this]() { save_current(); });
  auto* meta = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  meta->pack_start(*Gtk::manage(new Gtk::Label("Colour")), Gtk::PACK_SHRINK);
  meta->pack_start(colour_, Gtk::PACK_SHRINK);
  meta->pack_start(*Gtk::manage(new Gtk::Label("Category")), Gtk::PACK_SHRINK);
  category_.set_hexpand(true);
  meta->pack_start(category_, Gtk::PACK_EXPAND_WIDGET);
  right->pack_start(*meta, Gtk::PACK_SHRINK);

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
  colour_.set_sensitive(on);
  category_.set_sensitive(on);
}

void NotepadPage::set_binder(Binder* b)
{
  binder_ = b;
}

void NotepadPage::fill_category_combo()
{
  const Glib::ustring keep = combo_text(category_);
  category_.remove_all();
  if (binder_) {
    for (const auto& n : binder_->note_categories())
      category_.append(n, n);
  }
  if (!keep.empty()) {
    bool found = false;
    if (binder_) {
      for (const auto& n : binder_->note_categories()) {
        if (n == keep) {
          found = true;
          break;
        }
      }
    }
    if (!found)
      category_.append(keep, keep);
    category_.set_active_id(keep);
    if (Gtk::Entry* e = category_.get_entry())
      e->set_text(keep);
  }
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
  n.colour = parse_colour(colour_.get_active_id().raw());
  n.category = combo_text(category_);
  binder_->update_note(n);
  signal_changed_.emit();
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
  colour_.set_active_id(n ? colour_attr(n->colour) : "yellow");
  fill_category_combo();
  if (n) {
    if (Gtk::Entry* e = category_.get_entry())
      e->set_text(n->category);
    if (!n->category.empty())
      category_.set_active_id(n->category);
  } else if (Gtk::Entry* e = category_.get_entry()) {
    e->set_text("");
  }
  set_editor_open(n != nullptr);
  suppress_ = false;
}

std::vector<Note> NotepadPage::visible() const
{
  std::vector<Note> out;
  if (!binder_)
    return out;
  const std::string view = view_.get_active_id().raw();
  Glib::Date today;
  today.set_time_current();
  for (const auto& n : binder_->notes()) {
    if (!note_matches(n, search_.get_text()))
      continue;
    if (view == "week") {
      Glib::Date d;
      if (!stamp_to_date(n.stamped, d) || !d.valid())
        continue;
      if (!in_last_days(d, today, 7))
        continue;
    }
    out.push_back(n);
  }
  if (view == "category") {
    std::sort(out.begin(), out.end(), [](const Note& a, const Note& b) {
      const Glib::ustring ca = a.category.casefold();
      const Glib::ustring cb = b.category.casefold();
      if (ca != cb)
        return ca < cb;
      return a.id < b.id;
    });
  } else if (view == "colour") {
    std::sort(out.begin(), out.end(), [](const Note& a, const Note& b) {
      if (a.colour != b.colour)
        return static_cast<int>(a.colour) < static_cast<int>(b.colour);
      return a.id < b.id;
    });
  } else if (view == "week") {
    std::sort(out.begin(), out.end(),
              [](const Note& a, const Note& b) { return a.stamped > b.stamped; });
  }
  return out;
}

Gtk::Widget* NotepadPage::make_chip(const Note& n)
{
  auto* ev = Gtk::manage(new Gtk::EventBox());
  ev->get_style_context()->add_class("ephemeris-note-chip");
  ev->get_style_context()->add_class(colour_css_class(n.colour));
  auto* lab = Gtk::manage(new Gtk::Label(n.title.empty() ? Glib::ustring("(untitled)") : n.title));
  lab->set_line_wrap(true);
  lab->set_xalign(0.0);
  lab->set_yalign(0.0);
  lab->set_margin_start(6);
  lab->set_margin_end(6);
  lab->set_margin_top(6);
  lab->set_margin_bottom(6);
  lab->set_size_request(88, 64);
  ev->add(*lab);
  ev->add_events(Gdk::BUTTON_PRESS_MASK);
  const int id = n.id;
  ev->signal_button_press_event().connect([this, id](GdkEventButton* e) {
    if (!e || e->button != 1)
      return false;
    if (id != current_id_)
      save_current();
    load_id(id);
    return true;
  });
  return ev;
}

void NotepadPage::rebuild_list()
{
  for (auto* ch : list_.get_children())
    list_.remove(*ch);
  const std::string view = view_.get_active_id().raw();
  Gtk::ListBoxRow* sel = nullptr;
  Glib::ustring last_group;
  bool first = true;
  for (const auto& n : visible()) {
    if (view == "category" || view == "colour") {
      Glib::ustring group = view == "category"
                                ? (n.category.empty() ? Glib::ustring("(none)") : n.category)
                                : Glib::ustring(colour_label(n.colour));
      if (first || group != last_group) {
        auto* head = Gtk::manage(new Gtk::Label(group));
        head->set_xalign(0.0);
        head->get_style_context()->add_class("ephemeris-group-head");
        auto* hrow = Gtk::manage(new Gtk::ListBoxRow());
        hrow->set_selectable(false);
        hrow->set_sensitive(false);
        hrow->add(*head);
        list_.append(*hrow);
        last_group = group;
        first = false;
      }
    }
    auto* lab =
        Gtk::manage(new Gtk::Label(n.title.empty() ? Glib::ustring("(untitled)") : n.title));
    lab->set_xalign(0.0);
    lab->set_ellipsize(Pango::ELLIPSIZE_END);
    auto* row = Gtk::manage(new Gtk::ListBoxRow());
    row->set_data("id", GINT_TO_POINTER(n.id));
    row->add(*lab);
    list_.append(*row);
    if (n.id == current_id_)
      sel = row;
  }
  list_.show_all();
  if (sel) {
    list_.select_row(*sel);
    load_id(current_id_);
  } else if (current_id_ > 0 && binder_ && binder_->find_note(current_id_)) {
    load_id(current_id_);
  } else {
    load_id(0);
  }
}

void NotepadPage::rebuild_icons()
{
  for (auto* ch : icons_.get_children())
    icons_.remove(*ch);
  bool keep = false;
  for (const auto& n : visible()) {
    icons_.add(*make_chip(n));
    if (n.id == current_id_)
      keep = true;
  }
  icons_.show_all();
  if (keep)
    load_id(current_id_);
  else
    load_id(0);
}

void NotepadPage::refresh()
{
  const std::string view = view_.get_active_id().raw();
  const bool icons = view == "icons";
  list_scroll_.set_visible(!icons);
  icon_scroll_.set_visible(icons);
  if (icons)
    rebuild_icons();
  else
    rebuild_list();
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
  if (id <= 0)
    return;
  if (id == current_id_ && title_.get_sensitive())
    return;
  if (id != current_id_)
    save_current();
  load_id(id);
}

}  // namespace ephemeris
