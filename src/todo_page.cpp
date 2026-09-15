/* SPDX-License-Identifier: Unlicense */

#include "todo_page.hpp"
#include "mention.hpp"

namespace ephemeris {
namespace {

enum class TodoDlg { cancel, ok, del };

void calendar_set(Gtk::Calendar& cal, const Glib::Date& d)
{
  const guint month = static_cast<guint>(static_cast<int>(d.get_month()) - 1);
  cal.select_month(month, static_cast<guint>(d.get_year()));
  cal.select_day(static_cast<guint>(d.get_day()));
}

void calendar_get(const Gtk::Calendar& cal, Glib::Date& d)
{
  guint y = 0, m = 0, day = 0;
  cal.get_date(y, m, day);
  d.set_dmy(static_cast<Glib::Date::Day>(day), static_cast<Glib::Date::Month>(m + 1),
            static_cast<Glib::Date::Year>(y));
}

TodoDlg run_todo_dialog(Gtk::Window& parent, Todo& t, bool existing, Binder* binder)
{
  Gtk::Dialog dlg(existing ? "Edit To Do" : "New To Do", parent, true);
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  if (existing)
    dlg.add_button("_Delete", Gtk::RESPONSE_REJECT);
  dlg.add_button("_OK", Gtk::RESPONSE_ACCEPT);
  dlg.set_default_response(Gtk::RESPONSE_ACCEPT);

  auto* box = dlg.get_content_area();
  box->set_border_width(10);
  box->set_spacing(8);

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

  auto* due_on = Gtk::manage(new Gtk::CheckButton("Due date"));
  auto* cal = Gtk::manage(new Gtk::Calendar());
  cal->set_display_options(Gtk::CALENDAR_SHOW_HEADING | Gtk::CALENDAR_SHOW_DAY_NAMES);
  Glib::Date seed;
  if (t.has_due)
    seed = t.due;
  else
    seed.set_time_current();
  calendar_set(*cal, seed);
  due_on->set_active(t.has_due);
  cal->set_sensitive(due_on->get_active());
  due_on->signal_toggled().connect([due_on, cal]() { cal->set_sensitive(due_on->get_active()); });

  auto* row = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  row->pack_start(*Gtk::manage(new Gtk::Label("Priority")), Gtk::PACK_SHRINK);
  row->pack_start(*pri, Gtk::PACK_SHRINK);
  box->pack_start(*row, Gtk::PACK_SHRINK);
  box->pack_start(*due_on, Gtk::PACK_SHRINK);
  box->pack_start(*cal, Gtk::PACK_SHRINK);
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
  t.has_due = due_on->get_active();
  if (t.has_due)
    calendar_get(*cal, t.due);
  return TodoDlg::ok;
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
  auto* add = Gtk::manage(new Gtk::Button("Add"));
  add->signal_clicked().connect(sigc::mem_fun(*this, &TodoPage::on_add_task));
  top->pack_start(*head, Gtk::PACK_EXPAND_WIDGET);
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

void TodoPage::refresh()
{
  for (auto* ch : list_.get_children())
    list_.remove(*ch);
  if (!binder_)
    return;
  for (const Todo& t : binder_->todos())
    list_.pack_start(*make_row(t), Gtk::PACK_SHRINK);
  list_.show_all();
}

Gtk::Widget* TodoPage::make_row(const Todo& t)
{
  auto* row = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  row->get_style_context()->add_class("ephemeris-todo-line");
  auto* ck = Gtk::manage(new Gtk::CheckButton());
  ck->set_active(t.done);
  const int id = t.id;
  ck->signal_toggled().connect([this, ck, id]() {
    if (!binder_)
      return;
    const Todo* cur = binder_->find_todo(id);
    if (!cur)
      return;
    Todo n = *cur;
    n.done = ck->get_active();
    binder_->update_todo(n);
    signal_changed_.emit();
    refresh();
  });
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
  row->pack_end(*due_lab, Gtk::PACK_SHRINK);
  return row;
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
  binder_->update_todo(t);
  signal_changed_.emit();
  refresh();
}

}  // namespace ephemeris
