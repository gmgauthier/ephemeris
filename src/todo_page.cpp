/* SPDX-License-Identifier: Unlicense */

#include "todo_page.hpp"
#include "mention.hpp"

#include <algorithm>

namespace ephemeris {
namespace {

enum class TodoDlg { cancel, ok, del };

void fill_categories(Gtk::ComboBoxText& combo, const std::vector<Glib::ustring>& names,
                     const Glib::ustring& current)
{
  combo.remove_all();
  for (const auto& n : names)
    combo.append(n, n);
  if (!current.empty()) {
    bool found = false;
    for (const auto& n : names) {
      if (n == current) {
        found = true;
        break;
      }
    }
    if (!found)
      combo.append(current, current);
    combo.set_active_id(current);
    if (combo.get_entry())
      combo.get_entry()->set_text(current);
  }
}

Glib::ustring combo_text(Gtk::ComboBoxText& combo)
{
  if (Gtk::Entry* e = combo.get_entry())
    return e->get_text();
  return combo.get_active_text();
}

TodoDlg run_todo_dialog(Gtk::Window& parent, Todo& t, bool existing, Binder* binder)
{
  Gtk::Dialog dlg(existing ? "Edit To Do" : "New To Do", parent, true);
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  if (existing)
    dlg.add_button("_Delete", Gtk::RESPONSE_REJECT);
  dlg.add_button("_OK", Gtk::RESPONSE_ACCEPT);
  dlg.set_default_response(Gtk::RESPONSE_ACCEPT);
  dlg.set_default_size(460, 0);

  auto* box = dlg.get_content_area();
  box->set_border_width(10);
  box->set_spacing(6);

  auto* text = Gtk::manage(new Gtk::Entry());
  text->set_placeholder_text("Task — type @ to mention a contact");
  text->set_text(t.text);
  text->set_activates_default(true);
  attach_mentions(*text, binder);
  box->pack_start(*Gtk::manage(new Gtk::Label("Text", Gtk::ALIGN_START)), Gtk::PACK_SHRINK);
  box->pack_start(*text, Gtk::PACK_SHRINK);

  auto* pri = Gtk::manage(new Gtk::ComboBoxText());
  pri->append("0", "None");
  pri->append("1", "1");
  pri->append("2", "2");
  pri->append("3", "3");
  pri->set_active_id(Glib::ustring::format(t.priority));

  auto* status = Gtk::manage(new Gtk::ComboBoxText());
  status->append("not_started", "Not started");
  status->append("in_progress", "In progress");
  status->append("waiting", "Waiting");
  status->append("deferred", "Deferred");
  status->append("completed", "Completed");
  status->set_active_id(status_attr(t.status));

  auto* pct = Gtk::manage(new Gtk::SpinButton());
  pct->set_range(0, 100);
  pct->set_increments(5, 25);
  pct->set_value(t.percent);

  auto* row = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  row->pack_start(*Gtk::manage(new Gtk::Label("Priority")), Gtk::PACK_SHRINK);
  row->pack_start(*pri, Gtk::PACK_SHRINK);
  row->pack_start(*Gtk::manage(new Gtk::Label("Status")), Gtk::PACK_SHRINK);
  row->pack_start(*status, Gtk::PACK_SHRINK);
  row->pack_start(*Gtk::manage(new Gtk::Label("%")), Gtk::PACK_SHRINK);
  row->pack_start(*pct, Gtk::PACK_SHRINK);
  box->pack_start(*row, Gtk::PACK_SHRINK);

  auto* cat = Gtk::manage(new Gtk::ComboBoxText(true));
  fill_categories(*cat, binder ? binder->todo_categories() : std::vector<Glib::ustring>{},
                  t.category);
  auto* crow = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  crow->pack_start(*Gtk::manage(new Gtk::Label("Category")), Gtk::PACK_SHRINK);
  cat->set_hexpand(true);
  crow->pack_start(*cat, Gtk::PACK_EXPAND_WIDGET);
  box->pack_start(*crow, Gtk::PACK_SHRINK);

  auto* start_on = Gtk::manage(new Gtk::CheckButton("Start"));
  auto* start = Gtk::manage(new Gtk::Entry());
  start->set_placeholder_text("YYYY-MM-DD");
  start->set_width_chars(12);
  start_on->set_active(t.has_start);
  if (t.has_start && t.start.valid())
    start->set_text(date_iso(t.start));
  start->set_sensitive(start_on->get_active());
  start_on->signal_toggled().connect(
      [start_on, start]() { start->set_sensitive(start_on->get_active()); });

  auto* due_on = Gtk::manage(new Gtk::CheckButton("Due"));
  auto* due = Gtk::manage(new Gtk::Entry());
  due->set_placeholder_text("YYYY-MM-DD");
  due->set_width_chars(12);
  due_on->set_active(t.has_due);
  if (t.has_due && t.due.valid())
    due->set_text(date_iso(t.due));
  due->set_sensitive(due_on->get_active());
  due_on->signal_toggled().connect([due_on, due]() { due->set_sensitive(due_on->get_active()); });

  auto* drow = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  drow->pack_start(*start_on, Gtk::PACK_SHRINK);
  drow->pack_start(*start, Gtk::PACK_SHRINK);
  drow->pack_start(*due_on, Gtk::PACK_SHRINK);
  drow->pack_start(*due, Gtk::PACK_SHRINK);
  box->pack_start(*drow, Gtk::PACK_SHRINK);

  auto* recur = Gtk::manage(new Gtk::ComboBoxText());
  recur->append("none", "Does not repeat");
  recur->append("daily", "Daily");
  recur->append("weekly", "Weekly");
  recur->append("monthly", "Monthly");
  recur->append("yearly", "Yearly");
  const char* rid = recur_attr(t.recur);
  recur->set_active_id(rid && *rid ? rid : "none");
  auto* interval = Gtk::manage(new Gtk::SpinButton());
  interval->set_range(1, 99);
  interval->set_increments(1, 1);
  interval->set_value(t.recur_interval > 0 ? t.recur_interval : 1);
  auto* rrow = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  rrow->pack_start(*Gtk::manage(new Gtk::Label("Repeat")), Gtk::PACK_SHRINK);
  rrow->pack_start(*recur, Gtk::PACK_SHRINK);
  rrow->pack_start(*Gtk::manage(new Gtk::Label("every")), Gtk::PACK_SHRINK);
  rrow->pack_start(*interval, Gtk::PACK_SHRINK);
  box->pack_start(*rrow, Gtk::PACK_SHRINK);

  auto* until_on = Gtk::manage(new Gtk::CheckButton("Until"));
  auto* until = Gtk::manage(new Gtk::Entry());
  until->set_placeholder_text("YYYY-MM-DD");
  until->set_width_chars(12);
  if (t.has_until && t.until.valid()) {
    until_on->set_active(true);
    until->set_text(date_iso(t.until));
  }
  auto* urow = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  urow->pack_start(*until_on, Gtk::PACK_SHRINK);
  urow->pack_start(*until, Gtk::PACK_SHRINK);
  box->pack_start(*urow, Gtk::PACK_SHRINK);

  auto* notes = Gtk::manage(new Gtk::TextView());
  notes->set_wrap_mode(Gtk::WRAP_WORD_CHAR);
  notes->get_buffer()->set_text(t.notes);
  auto* nscroll = Gtk::manage(new Gtk::ScrolledWindow());
  nscroll->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  nscroll->set_min_content_height(90);
  nscroll->add(*notes);
  box->pack_start(*Gtk::manage(new Gtk::Label("Notes", Gtk::ALIGN_START)), Gtk::PACK_SHRINK);
  box->pack_start(*nscroll, Gtk::PACK_EXPAND_WIDGET);
  dlg.show_all();

  const int resp = dlg.run();
  if (resp == Gtk::RESPONSE_REJECT)
    return TodoDlg::del;
  if (resp != Gtk::RESPONSE_ACCEPT)
    return TodoDlg::cancel;
  t.text = text->get_text();
  try {
    t.priority = std::stoi(pri->get_active_id());
  } catch (...) {
    t.priority = 0;
  }
  t.status = parse_status(status->get_active_id().raw());
  t.percent = pct->get_value_as_int();
  t.category = combo_text(*cat);
  t.has_start = apply_iso_date(start_on->get_active(), start->get_text().raw(), t.start);
  t.has_due = apply_iso_date(due_on->get_active(), due->get_text().raw(), t.due);
  t.recur = parse_recur(recur->get_active_id().raw());
  t.recur_interval = std::max(1, interval->get_value_as_int());
  t.has_until = apply_iso_date(until_on->get_active(), until->get_text().raw(), t.until);
  t.notes = notes->get_buffer()->get_text();
  t.done = t.status == TodoStatus::completed || t.percent >= 100;
  return TodoDlg::ok;
}

bool in_next_days(const Glib::Date& due, const Glib::Date& today, int days)
{
  if (!due.valid() || !today.valid())
    return false;
  const long delta = static_cast<long>(due.get_julian()) - static_cast<long>(today.get_julian());
  return delta >= 0 && delta <= days;
}

}  // namespace

TodoPage::TodoPage()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 8)
{
  get_style_context()->add_class("ephemeris-page");
  set_margin_start(16);
  set_margin_end(20);
  set_margin_top(12);
  set_margin_bottom(16);

  auto* top = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  auto* head = Gtk::manage(new Gtk::Label());
  head->set_markup("<b>To Do</b>");
  head->set_halign(Gtk::ALIGN_START);
  head->get_style_context()->add_class("ephemeris-month-head");
  view_.append("today", "Today");
  view_.append("detailed", "Detailed");
  view_.append("active", "Active");
  view_.append("week", "Next seven days");
  view_.append("overdue", "Overdue");
  view_.append("category", "By category");
  view_.set_active_id("active");
  view_.signal_changed().connect([this]() { refresh(); });
  auto* add = Gtk::manage(new Gtk::Button("Add"));
  add->signal_clicked().connect(sigc::mem_fun(*this, &TodoPage::on_add_task));
  top->pack_start(*head, Gtk::PACK_EXPAND_WIDGET);
  top->pack_start(view_, Gtk::PACK_SHRINK);
  top->pack_end(*add, Gtk::PACK_SHRINK);
  pack_start(*top, Gtk::PACK_SHRINK);

  auto* scroll = Gtk::manage(new Gtk::ScrolledWindow());
  scroll->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  scroll->add(list_);
  pack_start(*scroll, Gtk::PACK_EXPAND_WIDGET);
}

void TodoPage::set_binder(Binder* b)
{
  binder_ = b;
  refresh();
}

std::vector<Todo> TodoPage::visible() const
{
  std::vector<Todo> out;
  if (!binder_)
    return out;
  Glib::Date today;
  today.set_time_current();
  const std::string view = view_.get_active_id().raw();
  for (const Todo& t : binder_->todos()) {
    if (view == "today") {
      const bool due_today = t.has_due && t.due.valid() && t.due.get_julian() == today.get_julian();
      const bool open_undated = !t.done && !t.has_due;
      const bool done_today = t.done && t.has_completed && t.completed.valid() &&
                              t.completed.get_julian() == today.get_julian();
      if (!(due_today || open_undated || done_today))
        continue;
    } else if (view == "active") {
      if (t.done)
        continue;
    } else if (view == "week") {
      if (t.done || !t.has_due || !in_next_days(t.due, today, 7))
        continue;
    } else if (view == "overdue") {
      if (t.done || !t.has_due || !t.due.valid() || t.due.compare(today) >= 0)
        continue;
    }
    out.push_back(t);
  }
  if (view == "category") {
    std::sort(out.begin(), out.end(), [](const Todo& a, const Todo& b) {
      const Glib::ustring ca = a.category.casefold();
      const Glib::ustring cb = b.category.casefold();
      if (ca != cb)
        return ca < cb;
      return a.id < b.id;
    });
  }
  return out;
}

void TodoPage::refresh()
{
  for (auto* ch : list_.get_children())
    list_.remove(*ch);
  if (!binder_)
    return;
  const std::string view = view_.get_active_id().raw();
  const bool detailed = view == "detailed";
  const auto items = visible();
  if (view == "category") {
    Glib::ustring last;
    bool first = true;
    for (const Todo& t : items) {
      Glib::ustring cat = t.category.empty() ? Glib::ustring("(none)") : t.category;
      if (first || cat != last) {
        list_.pack_start(*make_group(cat), Gtk::PACK_SHRINK);
        last = cat;
        first = false;
      }
      list_.pack_start(*make_row(t, false), Gtk::PACK_SHRINK);
    }
  } else {
    for (const Todo& t : items)
      list_.pack_start(*make_row(t, detailed), Gtk::PACK_SHRINK);
  }
  list_.show_all();
}

Gtk::Widget* TodoPage::make_group(const Glib::ustring& title)
{
  auto* lab = Gtk::manage(new Gtk::Label(title));
  lab->set_xalign(0.0);
  lab->get_style_context()->add_class("ephemeris-group-head");
  return lab;
}

Gtk::Widget* TodoPage::make_row(const Todo& t, bool detailed)
{
  auto* row = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  row->get_style_context()->add_class("ephemeris-todo-line");
  auto* ck = Gtk::manage(new Gtk::CheckButton());
  ck->set_active(t.done);
  const int id = t.id;
  ck->signal_toggled().connect([this, ck, id]() { on_toggle_done(id, ck->get_active()); });
  auto* pri = Gtk::manage(new Gtk::Label(t.priority > 0 ? Glib::ustring::format(t.priority) : "·"));
  pri->set_width_chars(2);
  Glib::ustring body = t.text;
  if (t.done)
    body = "<s>" + Glib::Markup::escape_text(body) + "</s>";
  else
    body = Glib::Markup::escape_text(body);
  auto* lab = Gtk::manage(new Gtk::Label());
  lab->set_markup(body);
  lab->set_xalign(0.0);
  lab->set_ellipsize(Pango::ELLIPSIZE_END);
  Glib::ustring due = t.has_due ? date_iso(t.due) : "";
  auto* due_lab = Gtk::manage(new Gtk::Label(due));
  due_lab->set_halign(Gtk::ALIGN_END);

  auto* ev = Gtk::manage(new Gtk::EventBox());
  ev->add(*lab);
  ev->add_events(Gdk::BUTTON_PRESS_MASK);
  ev->signal_button_press_event().connect([this, id](GdkEventButton* e) {
    if (!e || e->button != 1)
      return false;
    edit_item(id);
    return true;
  });

  row->pack_start(*ck, Gtk::PACK_SHRINK);
  row->pack_start(*pri, Gtk::PACK_SHRINK);
  row->pack_start(*ev, Gtk::PACK_EXPAND_WIDGET);
  if (detailed) {
    auto* st = Gtk::manage(new Gtk::Label(status_label(t.status)));
    st->set_width_chars(12);
    auto* pc = Gtk::manage(new Gtk::Label(Glib::ustring::format(t.percent) + "%"));
    pc->set_width_chars(4);
    auto* cat = Gtk::manage(new Gtk::Label(t.category));
    cat->set_width_chars(10);
    cat->set_ellipsize(Pango::ELLIPSIZE_END);
    row->pack_start(*st, Gtk::PACK_SHRINK);
    row->pack_start(*pc, Gtk::PACK_SHRINK);
    row->pack_start(*cat, Gtk::PACK_SHRINK);
  }
  row->pack_end(*due_lab, Gtk::PACK_SHRINK);
  return row;
}

void TodoPage::on_toggle_done(int id, bool done)
{
  if (!binder_)
    return;
  const Todo* cur = binder_->find_todo(id);
  if (!cur)
    return;
  if (done) {
    binder_->complete_todo(id);
  } else {
    Todo n = *cur;
    n.done = false;
    n.status = TodoStatus::not_started;
    n.percent = 0;
    n.has_completed = false;
    binder_->update_todo(n);
  }
  signal_changed_.emit();
  refresh();
}

void TodoPage::on_add_task()
{
  if (!binder_)
    return;
  auto* win = dynamic_cast<Gtk::Window*>(get_toplevel());
  if (!win)
    return;
  Todo t;
  t.has_due = true;
  t.due.set_time_current();
  if (run_todo_dialog(*win, t, false, binder_) != TodoDlg::ok)
    return;
  if (t.text.empty())
    return;
  if (t.done)
    t.status = TodoStatus::completed;
  binder_->add_todo(t);
  signal_changed_.emit();
  refresh();
}

void TodoPage::edit_item(int id)
{
  if (!binder_)
    return;
  const Todo* cur = binder_->find_todo(id);
  if (!cur)
    return;
  auto* win = dynamic_cast<Gtk::Window*>(get_toplevel());
  if (!win)
    return;
  const bool was_done = cur->done;
  Todo t = *cur;
  const TodoDlg r = run_todo_dialog(*win, t, true, binder_);
  if (r == TodoDlg::del) {
    binder_->remove_todo(id);
    signal_changed_.emit();
    refresh();
    return;
  }
  if (r != TodoDlg::ok)
    return;
  if (t.text.empty())
    return;
  if (!was_done && t.done) {
    Todo open = t;
    open.done = false;
    if (open.status == TodoStatus::completed)
      open.status = TodoStatus::not_started;
    if (open.percent >= 100)
      open.percent = 0;
    open.has_completed = false;
    binder_->update_todo(open);
    binder_->complete_todo(id);
  } else {
    binder_->update_todo(t);
  }
  signal_changed_.emit();
  refresh();
}

}  // namespace ephemeris
