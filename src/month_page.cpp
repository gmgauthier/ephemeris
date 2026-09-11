/* SPDX-License-Identifier: Unlicense */

#include "month_page.hpp"

namespace ephemeris {
namespace {

const char* kMonthName[] = {"",        "January",  "February", "March",  "April",
                            "May",     "June",     "July",     "August", "September",
                            "October", "November", "December"};

const char* kDow[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

int monday_index(Glib::Date::Weekday wd)
{
  /* Glib: Monday=1 ... Sunday=7 */
  const int n = static_cast<int>(wd);
  if (n <= 0)
    return 0;
  return (n + 6) % 7;
}

}  // namespace

MonthPage::MonthPage() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 8)
{
  get_style_context()->add_class("ephemeris-page");
  get_style_context()->add_class("ephemeris-sheet");
  head_.get_style_context()->add_class("ephemeris-month-head");
  head_.set_halign(Gtk::ALIGN_START);
  pack_start(head_, Gtk::PACK_SHRINK);
  grid_.set_row_homogeneous(true);
  grid_.set_column_homogeneous(true);
  grid_.set_row_spacing(2);
  grid_.set_column_spacing(2);
  pack_start(grid_, Gtk::PACK_EXPAND_WIDGET);

  Glib::Date now;
  now.set_time_current();
  set_month(now.get_month(), now.get_year());
}

Glib::ustring MonthPage::title() const
{
  const int m = static_cast<int>(month_);
  const char* name = (m >= 1 && m <= 12) ? kMonthName[m] : "";
  return Glib::ustring::compose("%1 %2", name, static_cast<int>(year_));
}

void MonthPage::set_month(Glib::Date::Month month, Glib::Date::Year year)
{
  month_ = month;
  year_ = year;
  rebuild();
}

void MonthPage::today()
{
  Glib::Date now;
  now.set_time_current();
  set_month(now.get_month(), now.get_year());
}

void MonthPage::prev_month()
{
  Glib::Date d(1, month_, year_);
  d.subtract_months(1);
  set_month(d.get_month(), d.get_year());
}

void MonthPage::next_month()
{
  Glib::Date d(1, month_, year_);
  d.add_months(1);
  set_month(d.get_month(), d.get_year());
}

Gtk::Widget* MonthPage::make_day(int day, bool in_month, bool is_today)
{
  auto* ev = Gtk::manage(new Gtk::EventBox());
  ev->set_visible_window(true);
  auto* lab = Gtk::manage(new Gtk::Label(day > 0 ? Glib::ustring::format(day) : Glib::ustring()));
  lab->set_xalign(1.0);
  lab->set_yalign(0.0);
  ev->add(*lab);
  ev->get_style_context()->add_class("ephemeris-day");
  if (!in_month)
    ev->get_style_context()->add_class("ephemeris-day-out");
  if (is_today) {
    ev->get_style_context()->add_class("ephemeris-day-today");
    lab->get_style_context()->add_class("ephemeris-day-today");
  }
  if (in_month && day > 0) {
    const int d = day;
    ev->add_events(Gdk::BUTTON_PRESS_MASK);
    ev->signal_button_press_event().connect([this, d](GdkEventButton* e) {
      if (!e || e->button != 1)
        return false;
      Glib::Date date(d, month_, year_);
      signal_day_chosen_.emit(date);
      return true;
    });
  }
  return ev;
}

void MonthPage::rebuild()
{
  for (auto* ch : grid_.get_children())
    grid_.remove(*ch);

  for (int i = 0; i < 7; ++i) {
    auto* lab = Gtk::manage(new Gtk::Label(kDow[i]));
    lab->get_style_context()->add_class("ephemeris-dow");
    lab->set_xalign(1.0);
    grid_.attach(*lab, i, 0, 1, 1);
  }

  Glib::Date first(1, month_, year_);
  const int start = monday_index(first.get_weekday());

  Glib::Date now;
  now.set_time_current();
  const bool this_month = now.get_month() == month_ && now.get_year() == year_;
  const int today_d = now.get_day();

  Glib::Date cursor = first;
  cursor.subtract_days(start);

  for (int row = 0; row < 6; ++row) {
    for (int col = 0; col < 7; ++col) {
      const bool in_month = cursor.get_month() == month_ && cursor.get_year() == year_;
      const int d = cursor.get_day();
      const bool is_today = this_month && in_month && d == today_d;
      auto* w = make_day(d, in_month, is_today);
      grid_.attach(*w, col, row + 1, 1, 1);
      cursor.add_days(1);
    }
  }
  head_.set_text(title());
  grid_.show_all();
}

}  // namespace ephemeris
