/* SPDX-License-Identifier: Unlicense */

#include "day_spread.hpp"

#include <glib.h>

namespace ephemeris {
namespace {

constexpr int kDayStart = 8 * 60;
constexpr int kDayEnd = 18 * 60;
constexpr int kStep = 30;

const Appointment* covering(const std::vector<Appointment>& list, int mins)
{
  for (const auto& a : list) {
    if (mins >= a.start_min && mins < a.end_min)
      return &a;
  }
  return nullptr;
}

}  // namespace

DaySpread::DaySpread() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 8)
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
  cols_.pack_start(*build_day(left_), Gtk::PACK_EXPAND_WIDGET);
  cols_.pack_start(*build_day(right), Gtk::PACK_EXPAND_WIDGET);
  cols_.show_all();
}

Gtk::Widget* DaySpread::build_day(const Glib::Date& date)
{
  auto* col = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
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

  for (int m = kDayStart; m < kDayEnd; m += kStep) {
    const Appointment* hit = covering(items, m);
    auto* ev = Gtk::manage(new Gtk::EventBox());
    ev->set_visible_window(true);
    ev->get_style_context()->add_class("ephemeris-slot");
    Glib::ustring label = format_hm(m);
    if (hit && hit->start_min == m) {
      ev->get_style_context()->add_class("ephemeris-slot-busy");
      label += "  ";
      label += hit->text;
    } else if (hit) {
      ev->get_style_context()->add_class("ephemeris-slot-busy");
      label += "  ·";
    }
    auto* lab = Gtk::manage(new Gtk::Label(label));
    lab->set_xalign(0.0);
    lab->set_ellipsize(Pango::ELLIPSIZE_END);
    ev->add(*lab);
    const Glib::Date d = date;
    const int start = m;
    const int id = hit ? hit->id : 0;
    ev->add_events(Gdk::BUTTON_PRESS_MASK);
    ev->signal_button_press_event().connect([this, d, start, id](GdkEventButton* e) {
      if (!e || e->button != 1)
        return false;
      edit_slot(d, start, id);
      return true;
    });
    list->pack_start(*ev, Gtk::PACK_SHRINK);
  }
  scroll->add(*list);
  col->pack_start(*scroll, Gtk::PACK_EXPAND_WIDGET);

  if (binder_) {
    const auto due = binder_->todos_due_on(date);
    if (!due.empty()) {
      auto* thru = Gtk::manage(new Gtk::Label());
      thru->set_markup("<b>To Do due today</b>");
      thru->set_halign(Gtk::ALIGN_START);
      thru->set_margin_top(8);
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
  text->set_placeholder_text("What");
  text->set_text(edited.text);
  text->set_activates_default(true);
  box->pack_start(*Gtk::manage(new Gtk::Label("Text", Gtk::ALIGN_START)), Gtk::PACK_SHRINK);
  box->pack_start(*text, Gtk::PACK_SHRINK);
  auto* start = Gtk::manage(new Gtk::ComboBoxText());
  auto* end = Gtk::manage(new Gtk::ComboBoxText());
  for (int m = kDayStart; m <= kDayEnd; m += kStep) {
    const Glib::ustring hm = format_hm(m);
    if (m < kDayEnd)
      start->append(hm);
    if (m > kDayStart)
      end->append(hm);
  }
  start->set_active_text(format_hm(edited.start_min));
  int e = edited.end_min <= edited.start_min ? edited.start_min + kStep : edited.end_min;
  if (e > kDayEnd)
    e = kDayEnd;
  end->set_active_text(format_hm(e));
  auto* row = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  row->pack_start(*Gtk::manage(new Gtk::Label("From")), Gtk::PACK_SHRINK);
  row->pack_start(*start, Gtk::PACK_SHRINK);
  row->pack_start(*Gtk::manage(new Gtk::Label("to")), Gtk::PACK_SHRINK);
  row->pack_start(*end, Gtk::PACK_SHRINK);
  box->pack_start(*row, Gtk::PACK_SHRINK);
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
  if (existing)
    binder_->update_appointment(edited);
  else
    binder_->add_appointment(edited);
  signal_changed_.emit();
  refresh();
}

}  // namespace ephemeris
