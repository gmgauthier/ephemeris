/* SPDX-License-Identifier: Unlicense */

#include "notepad_page.hpp"

#include <glib.h>

#include <algorithm>

namespace ephemeris {
namespace {

const Glib::ustring kBullet = "• ";

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

bool range_all_tag(Gtk::TextIter a, Gtk::TextIter b, const Glib::RefPtr<Gtk::TextTag>& tag)
{
  if (!tag || a.compare(b) >= 0)
    return false;
  for (; a.compare(b) < 0; a.forward_char()) {
    if (!a.has_tag(tag))
      return false;
  }
  return true;
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
  view_.signal_changed().connect([this]() {
    if (!suppress_)
      refresh();
  });
  btns->pack_start(*add, Gtk::PACK_SHRINK);
  btns->pack_start(*del, Gtk::PACK_SHRINK);
  btns->pack_end(view_, Gtk::PACK_SHRINK);
  left->pack_start(*btns, Gtk::PACK_SHRINK);

  search_.set_placeholder_text("Search notes");
  search_.signal_changed().connect([this]() {
    if (!suppress_)
      refresh();
  });
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
  journal_.set_width_chars(18);
  journal_.set_placeholder_text("8 October 2026");
  journal_.signal_changed().connect([this]() { save_current(); });
  journal_.signal_focus_out_event().connect([this](GdkEventFocus*) {
    Glib::Date d;
    if (!parse_journal_text(journal_.get_text(), d))
      return false;
    const Glib::ustring written = format_written_date(d);
    if (journal_.get_text() == written)
      return false;
    const bool was = suppress_;
    suppress_ = true;
    journal_.set_text(written);
    suppress_ = was;
    return false;
  });
  auto* dates = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 12));
  dates->pack_start(stamp_, Gtk::PACK_SHRINK);
  dates->pack_end(journal_, Gtk::PACK_SHRINK);
  dates->pack_end(*Gtk::manage(new Gtk::Label("Journal date")), Gtk::PACK_SHRINK);
  right->pack_start(*dates, Gtk::PACK_SHRINK);

  colour_.append("yellow", "Yellow");
  colour_.append("blue", "Blue");
  colour_.append("green", "Green");
  colour_.append("pink", "Pink");
  colour_.append("white", "White");
  colour_.set_active_id("white");
  colour_.signal_changed().connect([this]() { save_current(); });
  category_.signal_changed().connect([this]() { save_current(); });
  if (Gtk::Entry* e = category_.get_entry()) {
    e->signal_changed().connect([this]() { save_current(); });
    e->signal_activate().connect([this]() { commit_category(); });
    e->signal_focus_out_event().connect([this](GdkEventFocus*) {
      commit_category();
      return false;
    });
  }
  categories_.signal_clicked().connect(sigc::mem_fun(*this, &NotepadPage::edit_categories));
  auto* meta = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  meta->pack_start(*Gtk::manage(new Gtk::Label("Colour")), Gtk::PACK_SHRINK);
  meta->pack_start(colour_, Gtk::PACK_SHRINK);
  meta->pack_start(*Gtk::manage(new Gtk::Label("Category")), Gtk::PACK_SHRINK);
  category_.set_hexpand(true);
  meta->pack_start(category_, Gtk::PACK_EXPAND_WIDGET);
  meta->pack_start(categories_, Gtk::PACK_SHRINK);
  right->pack_start(*meta, Gtk::PACK_SHRINK);

  bold_btn_.set_focus_on_click(false);
  italic_btn_.set_focus_on_click(false);
  bullet_btn_.set_focus_on_click(false);
  bold_btn_.signal_toggled().connect(sigc::mem_fun(*this, &NotepadPage::on_bold_toggled));
  italic_btn_.signal_toggled().connect(sigc::mem_fun(*this, &NotepadPage::on_italic_toggled));
  bullet_btn_.signal_toggled().connect(sigc::mem_fun(*this, &NotepadPage::on_bullet_toggled));
  auto* format = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 6));
  format->pack_start(bold_btn_, Gtk::PACK_SHRINK);
  format->pack_start(italic_btn_, Gtk::PACK_SHRINK);
  format->pack_start(bullet_btn_, Gtk::PACK_SHRINK);
  right->pack_start(*format, Gtk::PACK_SHRINK);

  paper_ = Gtk::manage(new Gtk::ScrolledWindow());
  paper_->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  ensure_tags();
  body_.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
  body_.set_left_margin(12);
  body_.set_right_margin(12);
  body_.set_top_margin(8);
  body_.set_bottom_margin(8);
  body_.get_buffer()->signal_changed().connect([this]() { save_current(); });
  body_.get_buffer()->signal_insert().connect(sigc::mem_fun(*this, &NotepadPage::on_insert), false);
  body_.get_buffer()->signal_mark_set().connect(sigc::mem_fun(*this, &NotepadPage::on_mark_set));
  body_.signal_key_press_event().connect(sigc::mem_fun(*this, &NotepadPage::on_body_key), false);
  paper_->add(body_);
  right->pack_start(*paper_, Gtk::PACK_EXPAND_WIDGET);
  pack_start(*right, Gtk::PACK_EXPAND_WIDGET);
  apply_paper(NoteColour::white);
  set_editor_open(false);
}

void NotepadPage::ensure_tags()
{
  auto buf = body_.get_buffer();
  bold_tag_ = buf->create_tag("bold");
  bold_tag_->property_weight() = Pango::WEIGHT_BOLD;
  italic_tag_ = buf->create_tag("italic");
  italic_tag_->property_style() = Pango::STYLE_ITALIC;
  bullet_tag_ = buf->create_tag("bullet-mark");
}

void NotepadPage::apply_paper(NoteColour colour)
{
  static const char* kColours[] = {"yellow", "blue", "green", "pink", "white"};
  auto paint = [&](Gtk::Widget& w) {
    auto ctx = w.get_style_context();
    ctx->add_class("ephemeris-note-paper");
    for (const char* name : kColours)
      ctx->remove_class(Glib::ustring("ephemeris-note-paper-") + name);
    ctx->add_class(Glib::ustring("ephemeris-note-paper-") + colour_attr(colour));
  };
  paint(body_);
  if (paper_)
    paint(*paper_);
}

void NotepadPage::set_editor_open(bool on)
{
  title_.set_sensitive(on);
  body_.set_sensitive(on);
  stamp_.set_sensitive(on);
  journal_.set_sensitive(on);
  colour_.set_sensitive(on);
  category_.set_sensitive(on);
  bold_btn_.set_sensitive(on);
  italic_btn_.set_sensitive(on);
  bullet_btn_.set_sensitive(on);
  if (!on) {
    suppress_format_ = true;
    bold_btn_.set_active(false);
    italic_btn_.set_active(false);
    bullet_btn_.set_active(false);
    suppress_format_ = false;
    type_bold_ = false;
    type_italic_ = false;
  }
}

void NotepadPage::set_binder(Binder* b)
{
  binder_ = b;
}

void NotepadPage::commit_category()
{
  if (suppress_ || !binder_)
    return;
  if (!binder_->add_note_category(combo_text(category_)))
    return;
  fill_category_combo();
  signal_changed_.emit();
}

void NotepadPage::fill_category_combo()
{
  const Glib::ustring keep = combo_text(category_);
  const bool was = suppress_;
  suppress_ = true;
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
  suppress_ = was;
}

bool NotepadPage::ask_name(Gtk::Window& parent, const Glib::ustring& title, Glib::ustring& name)
{
  Gtk::Dialog dlg(title, parent, true);
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dlg.add_button("_OK", Gtk::RESPONSE_OK);
  dlg.set_default_response(Gtk::RESPONSE_OK);
  auto* entry = Gtk::manage(new Gtk::Entry());
  entry->set_text(name);
  entry->set_activates_default(true);
  entry->set_width_chars(24);
  dlg.get_content_area()->set_border_width(10);
  dlg.get_content_area()->pack_start(*entry, Gtk::PACK_SHRINK);
  dlg.show_all();
  const int resp = dlg.run();
  dlg.hide();
  if (resp != Gtk::RESPONSE_OK)
    return false;
  name = entry->get_text();
  return !name.empty();
}

void NotepadPage::edit_categories()
{
  if (!binder_)
    return;
  auto* win = dynamic_cast<Gtk::Window*>(get_toplevel());
  if (!win)
    return;
  save_current();
  Gtk::Dialog dlg("Categories", *win, true);
  dlg.add_button("_Close", Gtk::RESPONSE_CLOSE);
  auto* box = dlg.get_content_area();
  box->set_border_width(10);
  box->set_spacing(8);
  auto* scroll = Gtk::manage(new Gtk::ScrolledWindow());
  scroll->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  scroll->set_min_content_height(160);
  scroll->set_min_content_width(260);
  auto* list = Gtk::manage(new Gtk::ListBox());
  scroll->add(*list);
  box->pack_start(*scroll, Gtk::PACK_EXPAND_WIDGET);
  Glib::ustring selected;
  list->signal_row_selected().connect([&](Gtk::ListBoxRow* row) {
    selected.clear();
    if (!row)
      return;
    if (auto* lab = dynamic_cast<Gtk::Label*>(row->get_child()))
      selected = lab->get_text();
  });
  auto refill = [&]() {
    const Glib::ustring keep = selected;
    for (auto* ch : list->get_children())
      list->remove(*ch);
    selected.clear();
    Gtk::ListBoxRow* sel = nullptr;
    for (const auto& name : binder_->note_categories()) {
      auto* lab = Gtk::manage(new Gtk::Label(name));
      lab->set_xalign(0);
      lab->set_margin_start(6);
      lab->set_margin_top(2);
      lab->set_margin_bottom(2);
      auto* row = Gtk::manage(new Gtk::ListBoxRow());
      row->add(*lab);
      list->append(*row);
      if (name == keep)
        sel = row;
    }
    list->show_all();
    if (sel)
      list->select_row(*sel);
  };
  auto* buttons = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 6));
  auto* add = Gtk::manage(new Gtk::Button("Add"));
  auto* ren = Gtk::manage(new Gtk::Button("Rename"));
  auto* del = Gtk::manage(new Gtk::Button("Delete"));
  add->signal_clicked().connect([&]() {
    Glib::ustring name;
    if (!ask_name(dlg, "Add category", name))
      return;
    binder_->add_note_category(name);
    selected = name;
    refill();
  });
  ren->signal_clicked().connect([&]() {
    if (selected.empty())
      return;
    Glib::ustring name = selected;
    const Glib::ustring from = selected;
    if (!ask_name(dlg, "Rename category", name) || name == from)
      return;
    if (!binder_->rename_note_category(from, name))
      return;
    selected = name;
    refill();
  });
  del->signal_clicked().connect([&]() {
    if (selected.empty())
      return;
    bool used = false;
    for (const auto& n : binder_->notes()) {
      if (n.category == selected) {
        used = true;
        break;
      }
    }
    if (used) {
      Gtk::MessageDialog msg(dlg, "Remove this category?", false, Gtk::MESSAGE_QUESTION,
                             Gtk::BUTTONS_NONE, true);
      msg.set_secondary_text("Notes in this category will have it cleared.");
      msg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
      msg.add_button("_Remove", Gtk::RESPONSE_OK);
      if (msg.run() != Gtk::RESPONSE_OK)
        return;
    }
    binder_->remove_note_category(selected);
    selected.clear();
    refill();
  });
  buttons->pack_start(*add, Gtk::PACK_SHRINK);
  buttons->pack_start(*ren, Gtk::PACK_SHRINK);
  buttons->pack_start(*del, Gtk::PACK_SHRINK);
  box->pack_start(*buttons, Gtk::PACK_SHRINK);
  refill();
  dlg.show_all();
  dlg.run();
  dlg.hide();
  signal_changed_.emit();
  refresh();
}

bool NotepadPage::starts_bullet_at(const Gtk::TextIter& line) const
{
  auto end = line;
  // forward_chars() returns false when the move lands on the buffer end, which is
  // exactly an empty bullet on the last line. The offset shows whether it advanced.
  end.forward_chars(static_cast<int>(kBullet.length()));
  if (end.get_offset() - line.get_offset() != static_cast<int>(kBullet.length()))
    return false;
  return body_.get_buffer()->get_text(line, end) == kBullet;
}

std::vector<NotePara> NotepadPage::paras_from_view()
{
  std::vector<NotePara> paras;
  auto buf = body_.get_buffer();
  if (buf->get_char_count() == 0)
    return paras;
  Gtk::TextIter line = buf->begin();
  while (true) {
    Gtk::TextIter line_end = line;
    if (!line_end.ends_line())
      line_end.forward_to_line_end();
    NotePara para;
    Gtk::TextIter content = line;
    if (starts_bullet_at(line)) {
      para.bullet = true;
      content.forward_chars(static_cast<int>(kBullet.length()));
    }
    for (Gtk::TextIter i = content; i.compare(line_end) < 0;) {
      const bool bold = bold_tag_ && i.has_tag(bold_tag_);
      const bool italic = italic_tag_ && i.has_tag(italic_tag_);
      Gtk::TextIter j = i;
      j.forward_char();
      while (j.compare(line_end) < 0 && (bold_tag_ && j.has_tag(bold_tag_)) == bold &&
             (italic_tag_ && j.has_tag(italic_tag_)) == italic)
        j.forward_char();
      const Glib::ustring text = buf->get_text(i, j);
      if (!text.empty())
        para.spans.push_back(NoteSpan{text, bold, italic});
      i = j;
    }
    paras.push_back(std::move(para));
    if (line_end.is_end())
      break;
    line = line_end;
    if (!line.forward_char())
      break;
  }
  return paras;
}

void NotepadPage::fill_body(const Note* n)
{
  auto buf = body_.get_buffer();
  skip_type_tag_ = true;
  buf->set_text("");
  if (!n) {
    skip_type_tag_ = false;
    return;
  }
  if (n->paras.empty()) {
    if (!n->body.empty())
      buf->set_text(n->body);
    skip_type_tag_ = false;
    return;
  }
  bool first = true;
  for (const auto& para : n->paras) {
    if (!first)
      buf->insert(buf->end(), "\n");
    first = false;
    if (para.bullet) {
      auto at = buf->end();
      buf->insert_with_tag(at, kBullet, bullet_tag_);
    }
    for (const auto& span : para.spans) {
      std::vector<Glib::RefPtr<Gtk::TextTag>> tags;
      if (span.bold && bold_tag_)
        tags.push_back(bold_tag_);
      if (span.italic && italic_tag_)
        tags.push_back(italic_tag_);
      auto at = buf->end();
      if (tags.empty())
        buf->insert(at, span.text);
      else
        buf->insert_with_tags(at, span.text, tags);
    }
  }
  skip_type_tag_ = false;
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
  n.paras = paras_from_view();
  n.body = note_plain(n.paras);
  n.colour = parse_colour(colour_.get_active_id().raw());
  n.category = combo_text(category_);
  Glib::Date journal;
  if (parse_journal_text(journal_.get_text(), journal))
    n.journal = date_iso(journal);
  apply_paper(n.colour);
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
  fill_body(n);
  if (n && !n->stamped.empty())
    stamp_.set_text("Added " + format_written_stamp(n->stamped));
  else
    stamp_.set_text("");
  Glib::Date journal;
  if (n && note_journal(*n, journal))
    journal_.set_text(format_written_date(journal));
  else
    journal_.set_text("");
  const NoteColour colour = n ? n->colour : NoteColour::white;
  colour_.set_active_id(colour_attr(colour));
  apply_paper(colour);
  fill_category_combo();
  if (n) {
    if (Gtk::Entry* e = category_.get_entry())
      e->set_text(n->category);
    if (!n->category.empty())
      category_.set_active_id(n->category);
  } else if (Gtk::Entry* e = category_.get_entry()) {
    e->set_text("");
  }
  type_bold_ = false;
  type_italic_ = false;
  set_editor_open(n != nullptr);
  suppress_ = false;
  if (n)
    adopt_format();
}

void NotepadPage::on_insert(const Gtk::TextIter& pos, const Glib::ustring& text, int)
{
  if (suppress_ || skip_type_tag_ || text.empty())
    return;
  if (!type_bold_ && !type_italic_)
    return;
  const int start = pos.get_offset();
  const int len = static_cast<int>(text.length());
  const bool bold = type_bold_;
  const bool italic = type_italic_;
  ++pending_type_;
  Glib::signal_idle().connect_once([this, start, len, bold, italic]() {
    auto buf = body_.get_buffer();
    const int n = buf->get_char_count();
    if (start >= 0 && start < n && len > 0) {
      auto s = buf->get_iter_at_offset(start);
      auto e = buf->get_iter_at_offset(std::min(start + len, n));
      if (bold && bold_tag_)
        buf->apply_tag(bold_tag_, s, e);
      if (italic && italic_tag_)
        buf->apply_tag(italic_tag_, s, e);
    }
    if (pending_type_ > 0)
      --pending_type_;
    if (pending_type_ == 0)
      adopt_format();
  });
}

void NotepadPage::on_mark_set(const Gtk::TextIter&, const Glib::RefPtr<Gtk::TextBuffer::Mark>& mark)
{
  if (!mark || !body_.get_buffer() || mark != body_.get_buffer()->get_insert())
    return;
  adopt_format();
}

void NotepadPage::adopt_format()
{
  if (suppress_ || skip_type_tag_ || pending_type_ > 0)
    return;
  auto buf = body_.get_buffer();
  Gtk::TextIter a;
  Gtk::TextIter b;
  if (buf->get_selection_bounds(a, b)) {
    type_bold_ = range_all_tag(a, b, bold_tag_);
    type_italic_ = range_all_tag(a, b, italic_tag_);
    sync_format_buttons();
    return;
  }
  auto it = buf->get_iter_at_mark(buf->get_insert());
  const bool at_bullet =
      starts_bullet_at(buf->get_iter_at_line(it.get_line())) && it.get_line_offset() <= 2;
  if (at_bullet && it.get_line_offset() < 2) {
    type_bold_ = false;
    type_italic_ = false;
  } else if ((at_bullet && it.get_line_offset() == 2) || it.get_line_offset() == 0) {
    if (!it.ends_line() && !it.is_end()) {
      type_bold_ = bold_tag_ && it.has_tag(bold_tag_);
      type_italic_ = italic_tag_ && it.has_tag(italic_tag_);
    } else if (!at_bullet) {
      type_bold_ = false;
      type_italic_ = false;
    }
  } else {
    auto probe = it;
    probe.backward_char();
    type_bold_ = bold_tag_ && probe.has_tag(bold_tag_);
    type_italic_ = italic_tag_ && probe.has_tag(italic_tag_);
  }
  sync_format_buttons();
}

bool NotepadPage::selection_is_bullets()
{
  auto buf = body_.get_buffer();
  Gtk::TextIter a;
  Gtk::TextIter b;
  if (!buf->get_selection_bounds(a, b)) {
    a = buf->get_iter_at_mark(buf->get_insert());
    b = a;
  }
  int line0 = a.get_line();
  int line1 = b.get_line();
  if (line1 > line0 && b.get_line_offset() == 0)
    --line1;
  if (line1 < line0)
    return false;
  for (int ln = line0; ln <= line1; ++ln) {
    if (!starts_bullet_at(buf->get_iter_at_line(ln)))
      return false;
  }
  return true;
}

void NotepadPage::sync_format_buttons()
{
  suppress_format_ = true;
  bold_btn_.set_active(type_bold_);
  italic_btn_.set_active(type_italic_);
  bullet_btn_.set_active(selection_is_bullets());
  suppress_format_ = false;
}

void NotepadPage::on_bold_toggled()
{
  if (suppress_format_)
    return;
  auto buf = body_.get_buffer();
  Gtk::TextIter a;
  Gtk::TextIter b;
  if (buf->get_selection_bounds(a, b)) {
    if (bold_btn_.get_active())
      buf->apply_tag(bold_tag_, a, b);
    else
      buf->remove_tag(bold_tag_, a, b);
  }
  type_bold_ = bold_btn_.get_active();
}

void NotepadPage::on_italic_toggled()
{
  if (suppress_format_)
    return;
  auto buf = body_.get_buffer();
  Gtk::TextIter a;
  Gtk::TextIter b;
  if (buf->get_selection_bounds(a, b)) {
    if (italic_btn_.get_active())
      buf->apply_tag(italic_tag_, a, b);
    else
      buf->remove_tag(italic_tag_, a, b);
  }
  type_italic_ = italic_btn_.get_active();
}

void NotepadPage::apply_bullets(bool want)
{
  auto buf = body_.get_buffer();
  Gtk::TextIter a;
  Gtk::TextIter b;
  if (!buf->get_selection_bounds(a, b)) {
    a = buf->get_iter_at_mark(buf->get_insert());
    b = a;
  }
  int line0 = a.get_line();
  int line1 = b.get_line();
  if (line1 > line0 && b.get_line_offset() == 0)
    --line1;
  skip_type_tag_ = true;
  suppress_ = true;
  for (int ln = line1; ln >= line0; --ln) {
    auto it = buf->get_iter_at_line(ln);
    const bool has = starts_bullet_at(it);
    if (want && !has)
      buf->insert_with_tag(it, kBullet, bullet_tag_);
    else if (!want && has) {
      auto end = it;
      end.forward_chars(static_cast<int>(kBullet.length()));
      buf->erase(it, end);
    }
  }
  skip_type_tag_ = false;
  suppress_ = false;
  save_current();
  sync_format_buttons();
}

void NotepadPage::on_bullet_toggled()
{
  if (suppress_format_)
    return;
  apply_bullets(bullet_btn_.get_active());
}

bool NotepadPage::on_body_key(GdkEventKey* event)
{
  if (!event || !body_.get_sensitive())
    return false;
  const guint mods = event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_SHIFT_MASK);
  if (mods == GDK_CONTROL_MASK && (event->keyval == GDK_KEY_b || event->keyval == GDK_KEY_B)) {
    bold_btn_.set_active(!bold_btn_.get_active());
    return true;
  }
  if (mods == GDK_CONTROL_MASK && (event->keyval == GDK_KEY_i || event->keyval == GDK_KEY_I)) {
    italic_btn_.set_active(!italic_btn_.get_active());
    return true;
  }
  auto buf = body_.get_buffer();
  if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) {
    if (buf->get_has_selection())
      return false;
    auto cur = buf->get_iter_at_mark(buf->get_insert());
    auto line = buf->get_iter_at_line(cur.get_line());
    if (!starts_bullet_at(line))
      return false;
    auto line_end = line;
    if (!line_end.ends_line())
      line_end.forward_to_line_end();
    if (buf->get_text(line, line_end) == kBullet) {
      // Drop the marker and stay on this line, so the list ends and the button turns off.
      skip_type_tag_ = true;
      auto end = line;
      end.forward_chars(static_cast<int>(kBullet.length()));
      buf->erase(line, end);
      skip_type_tag_ = false;
      sync_format_buttons();
      return true;
    }
    skip_type_tag_ = true;
    buf->insert_at_cursor("\n");
    auto at = buf->get_iter_at_mark(buf->get_insert());
    buf->insert_with_tag(at, kBullet, bullet_tag_);
    skip_type_tag_ = false;
    sync_format_buttons();
    return true;
  }
  if (event->keyval == GDK_KEY_BackSpace && mods == 0) {
    if (buf->get_has_selection())
      return false;
    auto cur = buf->get_iter_at_mark(buf->get_insert());
    if (cur.get_line_offset() != static_cast<int>(kBullet.length()))
      return false;
    auto line = buf->get_iter_at_line(cur.get_line());
    if (!starts_bullet_at(line))
      return false;
    skip_type_tag_ = true;
    auto end = line;
    end.forward_chars(static_cast<int>(kBullet.length()));
    buf->erase(line, end);
    skip_type_tag_ = false;
    sync_format_buttons();
    return true;
  }
  return false;
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
      if (!note_journal(n, d) || !in_last_days(d, today, 7))
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
    std::sort(out.begin(), out.end(), [](const Note& a, const Note& b) {
      Glib::Date da;
      Glib::Date db;
      const bool va = note_journal(a, da);
      const bool vb = note_journal(b, db);
      if (va && vb && da.compare(db) != 0)
        return da.compare(db) > 0;
      if (va != vb)
        return va;
      if (a.stamped != b.stamped)
        return a.stamped > b.stamped;
      return a.id > b.id;
    });
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

void NotepadPage::show_note(int id)
{
  if (!binder_ || id <= 0 || !binder_->find_note(id))
    return;
  save_current();
  suppress_ = true;
  view_.set_active_id("list");
  search_.set_text("");
  current_id_ = id;
  suppress_ = false;
  refresh();
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
