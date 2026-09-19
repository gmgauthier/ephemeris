/* SPDX-License-Identifier: Unlicense */

#include "day_spread.hpp"
#include "mention.hpp"

#include <glib.h>

namespace ephemeris {
namespace {

constexpr int kDayStart = 8 * 60;
constexpr int kDayEnd = 18 * 60;
constexpr int kStep = 30;

bool all_day(const Appointment& a)
{
  return a.start_min == 0 && a.end_min >= 24 * 60;
}

bool in_grid(const Appointment& a)
{
  return !all_day(a) && a.start_min >= kDayStart && a.start_min < kDayEnd;
}

const Appointment* covering(const std::vector<Appointment>& list, int mins)
{
  for (const auto& a : list) {
    if (all_day(a) || !in_grid(a))
      continue;
    if (mins >= a.start_min && mins < a.end_min)
      return &a;
  }
  return nullptr;
}

}  // namespace

DaySpread::DaySpread()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 8)
{
  get_style_context()->add_class("ephemeris-page");
  set_margin_start(16);
  set_margin_end(20);
  set_margin_top(12);
  set_margin_bottom(16);
  cols_.set_homogeneous(true);
  pack_start(cols_, Gtk::PACK_EXPAND_WIDGET);
  left_.set_time_current();
}

void DaySpread::set_binder(Binder* b)
{
  binder_ = b;
}

void DaySpread::set_remote(RemoteCalendars* r)
{
  remote_ = r;
}

void DaySpread::set_left_date(const Glib::Date& d)
{
  left_ = d;
  refresh();
}

void DaySpread::prev_spread()
{
  left_.subtract_days(2);
  refresh();
}

void DaySpread::next_spread()
{
  left_.add_days(2);
  refresh();
}

void DaySpread::today()
{
  left_.set_time_current();
  refresh();
}

void DaySpread::refresh()
{
  for (auto* ch : cols_.get_children())
    cols_.remove(*ch);
  Glib::Date right = left_;
  right.add_days(1);
  cols_.pack_start(*build_day(left_, false), Gtk::PACK_EXPAND_WIDGET);
  cols_.pack_start(*build_day(right, true), Gtk::PACK_EXPAND_WIDGET);
  cols_.show_all();
}

Gtk::Widget* DaySpread::build_day(const Glib::Date& date, bool right)
{
  auto* col = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
  col->get_style_context()->add_class("ephemeris-day-col");
  if (right)
    col->get_style_context()->add_class("ephemeris-day-col-right");
  char buf[64];
  g_date_strftime(buf, sizeof(buf), "%A %d %B", const_cast<GDate*>(date.gobj()));
  auto* head = Gtk::manage(new Gtk::Label());
  head->set_markup(Glib::ustring::compose("<b>%1</b>", buf));
  head->set_halign(Gtk::ALIGN_START);
  head->get_style_context()->add_class("ephemeris-month-head");
  col->pack_start(*head, Gtk::PACK_SHRINK);

  auto* scroll = Gtk::manage(new Gtk::ScrolledWindow());
  scroll->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  auto* list = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
  std::vector<Appointment> items;
  if (binder_)
    items = binder_->for_date(date);
  if (remote_) {
    const auto rem = remote_->for_date(date);
    items.insert(items.end(), rem.begin(), rem.end());
  }

  for (const auto& a : items) {
    if (!all_day(a))
      continue;
    auto* ev = Gtk::manage(new Gtk::EventBox());
    ev->set_visible_window(true);
    ev->get_style_context()->add_class("ephemeris-slot");
    ev->get_style_context()->add_class("ephemeris-slot-remote");
    auto* lab = Gtk::manage(new Gtk::Label("All day  " + a.text));
    lab->set_xalign(0.0);
    lab->set_ellipsize(Pango::ELLIPSIZE_END);
    ev->add(*lab);
    const Appointment copy = a;
    ev->add_events(Gdk::BUTTON_PRESS_MASK);
    ev->signal_button_press_event().connect([this, copy](GdkEventButton* e) {
      if (!e || e->button != 1)
        return false;
      show_remote(copy);
      return true;
    });
    list->pack_start(*ev, Gtk::PACK_SHRINK);
  }

  for (int m = kDayStart; m < kDayEnd; m += kStep) {
    const Appointment* hit = covering(items, m);
    auto* ev = Gtk::manage(new Gtk::EventBox());
    ev->set_visible_window(true);
    ev->get_style_context()->add_class("ephemeris-slot");
    Glib::ustring label = format_hm(m);
    if (hit && hit->start_min == m) {
      ev->get_style_context()->add_class(hit->remote ? "ephemeris-slot-remote"
                                                     : "ephemeris-slot-busy");
      label += "  ";
      label += hit->text;
    } else if (hit) {
      ev->get_style_context()->add_class(hit->remote ? "ephemeris-slot-remote"
                                                     : "ephemeris-slot-busy");
      label += "  ·";
    }
    auto* lab = Gtk::manage(new Gtk::Label(label));
    lab->set_xalign(0.0);
    lab->set_margin_start(6);
    lab->set_ellipsize(Pango::ELLIPSIZE_END);
    ev->add(*lab);
    const Glib::Date d = date;
    const int start = m;
    const int id = hit ? hit->id : 0;
    const bool is_remote = hit && hit->remote;
    Appointment remote_copy;
    if (is_remote)
      remote_copy = *hit;
    ev->add_events(Gdk::BUTTON_PRESS_MASK);
    ev->signal_button_press_event().connect(
        [this, d, start, id, is_remote, remote_copy](GdkEventButton* e) {
          if (!e || e->button != 1)
            return false;
          if (is_remote)
            show_remote(remote_copy);
          else
            edit_slot(d, start, id);
          return true;
        });
    list->pack_start(*ev, Gtk::PACK_SHRINK);
  }

  for (const auto& a : items) {
    if (all_day(a) || in_grid(a))
      continue;
    auto* ev = Gtk::manage(new Gtk::EventBox());
    ev->set_visible_window(true);
    ev->get_style_context()->add_class("ephemeris-slot");
    if (a.remote)
      ev->get_style_context()->add_class("ephemeris-slot-remote");
    else
      ev->get_style_context()->add_class("ephemeris-slot-busy");
    Glib::ustring label = format_hm(a.start_min) + "–" + format_hm(a.end_min) + "  " + a.text;
    auto* lab = Gtk::manage(new Gtk::Label(label));
    lab->set_xalign(0.0);
    lab->set_ellipsize(Pango::ELLIPSIZE_END);
    ev->add(*lab);
    const Appointment copy = a;
    const Glib::Date d = date;
    ev->add_events(Gdk::BUTTON_PRESS_MASK);
    ev->signal_button_press_event().connect([this, copy, d](GdkEventButton* e) {
      if (!e || e->button != 1)
        return false;
      if (copy.remote)
        show_remote(copy);
      else
        edit_slot(d, copy.start_min, copy.id);
      return true;
    });
    list->pack_start(*ev, Gtk::PACK_SHRINK);
  }
  scroll->add(*list);
  col->pack_start(*scroll, Gtk::PACK_EXPAND_WIDGET);

  if (binder_) {
    const auto blocks = binder_->planner_on(date);
    if (!blocks.empty()) {
      auto* thru = Gtk::manage(new Gtk::Label());
      thru->set_markup("<b>Planner</b>");
      thru->set_halign(Gtk::ALIGN_START);
      thru->get_style_context()->add_class("ephemeris-showthrough");
      col->pack_start(*thru, Gtk::PACK_SHRINK);
      const auto keys = binder_->planner_keys();
      for (const auto& p : blocks) {
        Glib::ustring line = keys[static_cast<size_t>(p.category)];
        if (!p.text.empty()) {
          line += " — ";
          line += p.text;
        }
        auto* lab = Gtk::manage(new Gtk::Label(line));
        lab->set_xalign(0.0);
        lab->set_ellipsize(Pango::ELLIPSIZE_END);
        auto* ev = Gtk::manage(new Gtk::EventBox());
        ev->override_background_color(Gdk::RGBA(planner_color(p.category)));
        ev->add(*lab);
        col->pack_start(*ev, Gtk::PACK_SHRINK);
      }
    }
  }

  if (binder_) {
    const auto due = binder_->todos_due_on(date);
    if (!due.empty()) {
      auto* thru = Gtk::manage(new Gtk::Label());
      thru->set_markup("<b>To Do due today</b>");
      thru->set_halign(Gtk::ALIGN_START);
      thru->get_style_context()->add_class("ephemeris-showthrough");
      col->pack_start(*thru, Gtk::PACK_SHRINK);
      for (const Todo& t : due) {
        Glib::ustring line = t.done ? "☑ " : "☐ ";
        if (t.priority > 0)
          line += Glib::ustring::format(t.priority) + " ";
        line += t.text;
        auto* lab = Gtk::manage(new Gtk::Label(line));
        lab->set_xalign(0.0);
        lab->set_ellipsize(Pango::ELLIPSIZE_END);
        if (t.done)
          lab->set_markup("<s>" + Glib::Markup::escape_text(line) + "</s>");
        auto* ev = Gtk::manage(new Gtk::EventBox());
        ev->add(*lab);
        ev->add_events(Gdk::BUTTON_PRESS_MASK);
        ev->signal_button_press_event().connect([this](GdkEventButton* e) {
          if (!e || e->button != 1)
            return false;
          signal_goto_todo_.emit();
          return true;
        });
        col->pack_start(*ev, Gtk::PACK_SHRINK);
      }
    }
  }
  return col;
}

void DaySpread::edit_slot(const Glib::Date& date, int start_min, int appt_id)
{
  if (!binder_)
    return;
  auto* win = dynamic_cast<Gtk::Window*>(get_toplevel());
  if (!win)
    return;

  Appointment a;
  bool existing = false;
  if (appt_id > 0) {
    if (const Appointment* found = binder_->find(appt_id)) {
      a = *found;
      existing = true;
    }
  }
  if (!existing) {
    a.date = date;
    a.start_min = start_min;
    a.end_min = start_min + kStep;
  }

  Appointment edited = a;
  Gtk::Dialog dlg(existing ? "Edit appointment" : "New appointment", *win, true);
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  if (existing)
    dlg.add_button("_Delete", Gtk::RESPONSE_REJECT);
  dlg.add_button("_OK", Gtk::RESPONSE_ACCEPT);
  dlg.set_default_response(Gtk::RESPONSE_ACCEPT);
  auto* box = dlg.get_content_area();
  box->set_border_width(10);
  box->set_spacing(8);
  auto* text = Gtk::manage(new Gtk::Entry());
  text->set_placeholder_text("What — type @ to mention a contact");
  text->set_text(edited.text);
  text->set_activates_default(true);
  attach_mentions(*text, binder_);
  box->pack_start(*Gtk::manage(new Gtk::Label("Text", Gtk::ALIGN_START)), Gtk::PACK_SHRINK);
  box->pack_start(*text, Gtk::PACK_SHRINK);
  auto* start = Gtk::manage(new Gtk::ComboBoxText());
  auto* end = Gtk::manage(new Gtk::ComboBoxText());
  constexpr int kComboStart = 7 * 60;
  constexpr int kComboEnd = 22 * 60;
  for (int m = kComboStart; m <= kComboEnd; m += kStep) {
    const Glib::ustring hm = format_hm(m);
    start->append(hm);
    if (m > kComboStart)
      end->append(hm);
  }
  start->set_active_text(format_hm(edited.start_min));
  if (start->get_active_text().empty())
    start->set_active_text(format_hm(kDayStart));
  int e = edited.end_min <= edited.start_min ? edited.start_min + kStep : edited.end_min;
  end->set_active_text(format_hm(e));
  if (end->get_active_text().empty())
    end->set_active_text(format_hm(kDayStart + kStep));
  auto* row = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  row->pack_start(*Gtk::manage(new Gtk::Label("From")), Gtk::PACK_SHRINK);
  row->pack_start(*start, Gtk::PACK_SHRINK);
  row->pack_start(*Gtk::manage(new Gtk::Label("to")), Gtk::PACK_SHRINK);
  row->pack_start(*end, Gtk::PACK_SHRINK);
  box->pack_start(*row, Gtk::PACK_SHRINK);

  auto* recur = Gtk::manage(new Gtk::ComboBoxText());
  recur->append("none", "Does not repeat");
  recur->append("daily", "Daily");
  recur->append("weekly", "Weekly");
  recur->append("monthly", "Monthly");
  recur->append("yearly", "Yearly");
  const char* rid = recur_attr(edited.recur);
  recur->set_active_id(rid && *rid ? rid : "none");
  auto* interval = Gtk::manage(new Gtk::SpinButton());
  interval->set_range(1, 99);
  interval->set_increments(1, 1);
  interval->set_value(edited.recur_interval > 0 ? edited.recur_interval : 1);
  auto* until_on = Gtk::manage(new Gtk::CheckButton("Until"));
  auto* until = Gtk::manage(new Gtk::Entry());
  until->set_placeholder_text("YYYY-MM-DD");
  until->set_width_chars(12);
  if (edited.has_until && edited.until.valid()) {
    until_on->set_active(true);
    until->set_text(date_iso(edited.until));
  }
  auto* rrow = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  rrow->pack_start(*Gtk::manage(new Gtk::Label("Repeat")), Gtk::PACK_SHRINK);
  rrow->pack_start(*recur, Gtk::PACK_SHRINK);
  rrow->pack_start(*Gtk::manage(new Gtk::Label("every")), Gtk::PACK_SHRINK);
  rrow->pack_start(*interval, Gtk::PACK_SHRINK);
  box->pack_start(*rrow, Gtk::PACK_SHRINK);
  auto* urow = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  urow->pack_start(*until_on, Gtk::PACK_SHRINK);
  urow->pack_start(*until, Gtk::PACK_SHRINK);
  box->pack_start(*urow, Gtk::PACK_SHRINK);
  dlg.show_all();
  const int resp = dlg.run();
  dlg.hide();
  if (resp == Gtk::RESPONSE_REJECT && existing) {
    binder_->remove_appointment(edited.id);
    signal_changed_.emit();
    refresh();
    return;
  }
  if (resp != Gtk::RESPONSE_ACCEPT)
    return;
  edited.text = text->get_text();
  if (edited.text.empty())
    return;
  edited.start_min = parse_hm(start->get_active_text());
  edited.end_min = parse_hm(end->get_active_text());
  if (edited.end_min <= edited.start_min)
    edited.end_min = edited.start_min + kStep;
  edited.recur = parse_recur(recur->get_active_id().raw());
  edited.recur_interval = std::max(1, interval->get_value_as_int());
  edited.has_until = until_on->get_active();
  if (edited.has_until) {
    if (!date_from_iso(until->get_text().raw(), edited.until))
      edited.has_until = false;
  }
  if (existing)
    binder_->update_appointment(edited);
  else
    binder_->add_appointment(edited);
  signal_changed_.emit();
  refresh();
}

void DaySpread::show_remote(const Appointment& a)
{
  auto* win = dynamic_cast<Gtk::Window*>(get_toplevel());
  if (!win)
    return;
  Glib::ustring title = a.calendar.empty() ? Glib::ustring("Subscribed calendar") : a.calendar;
  Gtk::MessageDialog dlg(*win, title, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
  Glib::ustring body = format_hm(a.start_min);
  if (a.end_min > a.start_min) {
    body += "–";
    body += format_hm(a.end_min);
  }
  if (a.start_min == 0 && a.end_min >= 24 * 60)
    body = "All day";
  body += "\n";
  body += a.text;
  body += "\n\nRead-only. Change this event in the calendar that published the URL.";
  dlg.set_secondary_text(body);
  dlg.run();
}

}  // namespace ephemeris
