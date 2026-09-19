/* SPDX-License-Identifier: Unlicense */

#include "planner_page.hpp"

#include <glib.h>

namespace ephemeris {
namespace {

const char* kMonthName[] = {"",        "January",  "February", "March",  "April",
                            "May",     "June",     "July",     "August", "September",
                            "October", "November", "December"};

int covering_id(const std::vector<PlannerEvent>& ev, const Glib::Date& d)
{
  for (const auto& e : ev) {
    if (e.start.valid() && e.end.valid() && d.compare(e.start) >= 0 && d.compare(e.end) <= 0)
      return e.id;
  }
  return 0;
}

}  // namespace

PlannerPage::PlannerPage()
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 6)
{
  get_style_context()->add_class("ephemeris-page");
  head_.get_style_context()->add_class("ephemeris-month-head");
  head_.set_halign(Gtk::ALIGN_START);
  head_.set_margin_top(12);
  head_.set_margin_start(16);
  pack_start(head_, Gtk::PACK_SHRINK);

  hint_.set_text("Pick a colour, then click and drag across days.");
  hint_.set_halign(Gtk::ALIGN_START);
  hint_.set_margin_start(16);
  pack_start(hint_, Gtk::PACK_SHRINK);

  grid_.set_row_spacing(2);
  grid_.set_column_spacing(1);
  grid_.set_margin_start(12);
  grid_.set_margin_end(16);
  grid_.set_margin_top(8);
  pack_start(grid_, Gtk::PACK_EXPAND_WIDGET);

  legend_.set_margin_start(16);
  legend_.set_margin_bottom(8);
  pack_start(legend_, Gtk::PACK_SHRINK);

  Glib::Date now;
  now.set_time_current();
  year_ = now.get_year();
}

void PlannerPage::set_binder(Binder* b)
{
  binder_ = b;
}

void PlannerPage::set_year(Glib::Date::Year year)
{
  year_ = year;
  rebuild();
}

void PlannerPage::today()
{
  Glib::Date now;
  now.set_time_current();
  set_year(now.get_year());
}

void PlannerPage::prev_year()
{
  set_year(year_ - 1);
}

void PlannerPage::next_year()
{
  set_year(year_ + 1);
}

void PlannerPage::refresh()
{
  rebuild();
}

Gtk::Widget* PlannerPage::make_day(int month, int day)
{
  const int dim = Glib::Date::get_days_in_month(static_cast<Glib::Date::Month>(month), year_);
  auto* ev = Gtk::manage(new Gtk::EventBox());
  ev->set_visible_window(true);
  ev->set_size_request(24, 22);
  ev->get_style_context()->add_class("ephemeris-planner-day");
  auto* lab =
      Gtk::manage(new Gtk::Label(day <= dim ? Glib::ustring::format(day) : Glib::ustring()));
  lab->set_xalign(0.5);
  ev->add(*lab);
  if (day > dim)
    return ev;

  Glib::Date date(static_cast<Glib::Date::Day>(day), static_cast<Glib::Date::Month>(month), year_);
  int hit = 0;
  if (binder_)
    hit = covering_id(binder_->planner_on(date), date);
  if (hit > 0) {
    if (const PlannerEvent* p = binder_->find_planner(hit)) {
      ev->get_style_context()->add_class("ephemeris-planner-hit");
      ev->get_style_context()->add_class(
          Glib::ustring::compose("ephemeris-planner-hit-%1", p->category).c_str());
    }
  }

  ev->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::BUTTON_MOTION_MASK);
  ev->signal_button_press_event().connect([this, date, hit](GdkEventButton* e) {
    if (!e || e->button != 1)
      return false;
    if (e->type == GDK_2BUTTON_PRESS && hit > 0) {
      edit_event(hit);
      return true;
    }
    if (hit > 0) {
      edit_event(hit);
      return true;
    }
    dragging_ = true;
    drag_start_ = date;
    return true;
  });
  ev->signal_button_release_event().connect([this, date](GdkEventButton* e) {
    if (!e || e->button != 1 || !dragging_)
      return false;
    dragging_ = false;
    paint_range(drag_start_, date);
    return true;
  });
  return ev;
}

void PlannerPage::rebuild()
{
  for (auto* ch : grid_.get_children())
    grid_.remove(*ch);
  for (auto* ch : legend_.get_children())
    legend_.remove(*ch);

  head_.set_text(Glib::ustring::compose("Planner %1", static_cast<int>(year_)));

  for (int d = 1; d <= 31; ++d) {
    auto* lab = Gtk::manage(new Gtk::Label(Glib::ustring::format(d)));
    lab->get_style_context()->add_class("ephemeris-dow");
    lab->set_xalign(0.5);
    grid_.attach(*lab, d, 0, 1, 1);
  }
  for (int m = 1; m <= 12; ++m) {
    auto* name = Gtk::manage(new Gtk::Label(kMonthName[m]));
    name->set_xalign(1.0);
    name->set_margin_end(6);
    grid_.attach(*name, 0, m, 1, 1);
    for (int d = 1; d <= 31; ++d)
      grid_.attach(*make_day(m, d), d, m, 1, 1);
  }

  if (binder_) {
    const auto keys = binder_->planner_keys();
    for (int i = 0; i < kPlannerKeyCount; ++i) {
      auto* b = Gtk::manage(new Gtk::Button(keys[static_cast<size_t>(i)]));
      b->set_relief(Gtk::RELIEF_NONE);
      b->get_style_context()->add_class("ephemeris-planner-key");
      b->get_style_context()->add_class(
          Glib::ustring::compose("ephemeris-planner-key-%1", i).c_str());
      if (i == category_)
        b->get_style_context()->add_class("ephemeris-planner-key-on");
      const int idx = i;
      b->signal_clicked().connect([this, idx]() {
        category_ = idx;
        rebuild();
      });
      b->signal_button_press_event().connect([this, idx](GdkEventButton* e) {
        if (e && e->type == GDK_2BUTTON_PRESS) {
          rename_key(idx);
          return true;
        }
        return false;
      });
      legend_.pack_start(*b, Gtk::PACK_SHRINK);
    }
  }
  grid_.show_all();
  legend_.show_all();
}

void PlannerPage::paint_range(const Glib::Date& a, const Glib::Date& b)
{
  if (!binder_ || !a.valid() || !b.valid())
    return;
  PlannerEvent e;
  e.start = a;
  e.end = b;
  e.category = category_;
  e.text = binder_->planner_keys()[static_cast<size_t>(category_)];
  binder_->add_planner(e);
  signal_changed_.emit();
  rebuild();
}

void PlannerPage::edit_event(int id)
{
  if (!binder_)
    return;
  const PlannerEvent* found = binder_->find_planner(id);
  if (!found)
    return;
  auto* win = dynamic_cast<Gtk::Window*>(get_toplevel());
  if (!win)
    return;
  PlannerEvent edited = *found;
  Gtk::Dialog dlg("Planner event", *win, true);
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dlg.add_button("_Delete", Gtk::RESPONSE_REJECT);
  dlg.add_button("_OK", Gtk::RESPONSE_ACCEPT);
  auto* box = dlg.get_content_area();
  box->set_border_width(10);
  box->set_spacing(8);
  auto* text = Gtk::manage(new Gtk::Entry());
  text->set_text(edited.text);
  text->set_activates_default(true);
  box->pack_start(*Gtk::manage(new Gtk::Label("Note", Gtk::ALIGN_START)), Gtk::PACK_SHRINK);
  box->pack_start(*text, Gtk::PACK_SHRINK);
  auto* cat = Gtk::manage(new Gtk::ComboBoxText());
  const auto keys = binder_->planner_keys();
  for (int i = 0; i < kPlannerKeyCount; ++i)
    cat->append(Glib::ustring::format(i), keys[static_cast<size_t>(i)]);
  cat->set_active_id(Glib::ustring::format(edited.category));
  box->pack_start(*cat, Gtk::PACK_SHRINK);
  dlg.set_default_response(Gtk::RESPONSE_ACCEPT);
  dlg.show_all();
  const int resp = dlg.run();
  dlg.hide();
  if (resp == Gtk::RESPONSE_REJECT) {
    binder_->remove_planner(id);
    signal_changed_.emit();
    rebuild();
    return;
  }
  if (resp != Gtk::RESPONSE_ACCEPT)
    return;
  edited.text = text->get_text();
  edited.category = static_cast<int>(g_ascii_strtoll(cat->get_active_id().c_str(), nullptr, 10));
  binder_->update_planner(edited);
  signal_changed_.emit();
  rebuild();
}

void PlannerPage::rename_key(int index)
{
  if (!binder_)
    return;
  auto* win = dynamic_cast<Gtk::Window*>(get_toplevel());
  if (!win)
    return;
  Gtk::Dialog dlg("Planner key", *win, true);
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dlg.add_button("_OK", Gtk::RESPONSE_ACCEPT);
  auto* entry = Gtk::manage(new Gtk::Entry());
  entry->set_text(binder_->planner_keys()[static_cast<size_t>(index)]);
  entry->set_activates_default(true);
  dlg.get_content_area()->set_border_width(10);
  dlg.get_content_area()->pack_start(*entry, Gtk::PACK_SHRINK);
  dlg.set_default_response(Gtk::RESPONSE_ACCEPT);
  dlg.show_all();
  if (dlg.run() != Gtk::RESPONSE_ACCEPT)
    return;
  binder_->set_planner_key(index, entry->get_text());
  signal_changed_.emit();
  rebuild();
}

}  // namespace ephemeris
