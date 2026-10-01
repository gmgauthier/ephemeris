/* SPDX-License-Identifier: Unlicense */

#include "binder.hpp"

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <glib.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <sstream>

namespace ephemeris {
namespace {

std::string xml_escape(const Glib::ustring& in)
{
  std::string out;
  out.reserve(in.bytes() + 8);
  for (const char c : in.raw()) {
    switch (c) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      default:
        out += c;
        break;
    }
  }
  return out;
}

std::string xml_escape_attr(const Glib::ustring& in)
{
  std::string out;
  out.reserve(in.bytes() + 8);
  for (const char c : in.raw()) {
    switch (c) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      case '\'':
        out += "&apos;";
        break;
      default:
        out += c;
        break;
    }
  }
  return out;
}

void clamp_notes(Contact& c)
{
  if (c.notes.size() > static_cast<Glib::ustring::size_type>(kContactNotesMax))
    c.notes = c.notes.substr(0, kContactNotesMax);
}

std::string node_name(xmlNode* n)
{
  if (!n || !n->name)
    return {};
  return reinterpret_cast<const char*>(n->name);
}

std::string node_prop(xmlNode* n, const char* key)
{
  if (!n)
    return {};
  xmlChar* v = xmlGetProp(n, BAD_CAST key);
  if (!v)
    return {};
  std::string out(reinterpret_cast<const char*>(v));
  xmlFree(v);
  return out;
}

Glib::ustring node_text(xmlNode* n)
{
  if (!n)
    return {};
  xmlChar* v = xmlNodeGetContent(n);
  if (!v)
    return {};
  Glib::ustring out(reinterpret_cast<const char*>(v));
  xmlFree(v);
  return out;
}

bool date_eq(const Glib::Date& a, const Glib::Date& b)
{
  return a.get_julian() == b.get_julian();
}

void default_planner_keys(std::array<Glib::ustring, kPlannerKeyCount>& keys)
{
  keys[0] = "Holiday";
  keys[1] = "Visit";
  keys[2] = "Travel";
  keys[3] = "Streaming";
  keys[4] = "Other";
}

}  // namespace

const char* recur_attr(Recur r)
{
  switch (r) {
    case Recur::daily:
      return "daily";
    case Recur::weekly:
      return "weekly";
    case Recur::monthly:
      return "monthly";
    case Recur::yearly:
      return "yearly";
    case Recur::none:
    default:
      return "";
  }
}

Recur parse_recur(const std::string& s)
{
  if (s == "daily")
    return Recur::daily;
  if (s == "weekly")
    return Recur::weekly;
  if (s == "monthly")
    return Recur::monthly;
  if (s == "yearly")
    return Recur::yearly;
  return Recur::none;
}

const char* status_attr(TodoStatus s)
{
  switch (s) {
    case TodoStatus::in_progress:
      return "in_progress";
    case TodoStatus::waiting:
      return "waiting";
    case TodoStatus::deferred:
      return "deferred";
    case TodoStatus::completed:
      return "completed";
    case TodoStatus::not_started:
    default:
      return "not_started";
  }
}

TodoStatus parse_status(const std::string& s)
{
  if (s == "in_progress")
    return TodoStatus::in_progress;
  if (s == "waiting")
    return TodoStatus::waiting;
  if (s == "deferred")
    return TodoStatus::deferred;
  if (s == "completed")
    return TodoStatus::completed;
  return TodoStatus::not_started;
}

const char* status_label(TodoStatus s)
{
  switch (s) {
    case TodoStatus::in_progress:
      return "In progress";
    case TodoStatus::waiting:
      return "Waiting";
    case TodoStatus::deferred:
      return "Deferred";
    case TodoStatus::completed:
      return "Completed";
    case TodoStatus::not_started:
    default:
      return "Not started";
  }
}

const char* colour_attr(NoteColour c)
{
  switch (c) {
    case NoteColour::blue:
      return "blue";
    case NoteColour::green:
      return "green";
    case NoteColour::pink:
      return "pink";
    case NoteColour::white:
      return "white";
    case NoteColour::yellow:
    default:
      return "yellow";
  }
}

NoteColour parse_colour(const std::string& s)
{
  if (s == "blue")
    return NoteColour::blue;
  if (s == "green")
    return NoteColour::green;
  if (s == "pink")
    return NoteColour::pink;
  if (s == "white")
    return NoteColour::white;
  return NoteColour::yellow;
}

const char* colour_label(NoteColour c)
{
  switch (c) {
    case NoteColour::blue:
      return "Blue";
    case NoteColour::green:
      return "Green";
    case NoteColour::pink:
      return "Pink";
    case NoteColour::white:
      return "White";
    case NoteColour::yellow:
    default:
      return "Yellow";
  }
}

const char* colour_hex(NoteColour c)
{
  switch (c) {
    case NoteColour::blue:
      return "#A8D4FF";
    case NoteColour::green:
      return "#B7E0A8";
    case NoteColour::pink:
      return "#F4B6C8";
    case NoteColour::white:
      return "#F7F5EF";
    case NoteColour::yellow:
    default:
      return "#FFF2A8";
  }
}

namespace {

bool month_has_day(int year, int month, int day)
{
  if (year < 1 || year > 9999 || month < 1 || month > 12 || day < 1 || day > 31)
    return false;
  const auto m = static_cast<Glib::Date::Month>(month);
  const int dim = Glib::Date::get_days_in_month(m, static_cast<Glib::Date::Year>(year));
  return day <= dim;
}

void set_ymd(Glib::Date& d, int year, int month, int day)
{
  d.set_dmy(static_cast<Glib::Date::Day>(day), static_cast<Glib::Date::Month>(month),
            static_cast<Glib::Date::Year>(year));
}

// A month without this day is skipped. The day number is kept.
void add_months_keeping_day(Glib::Date& d, int step)
{
  const int day = static_cast<int>(d.get_day());
  int year = static_cast<int>(d.get_year());
  int month = static_cast<int>(d.get_month());
  for (int i = 0; i < 480; ++i) {
    const long long index = (static_cast<long long>(month) - 1) + step;
    const long long next_year = static_cast<long long>(year) + index / 12;
    if (next_year < 1 || next_year > 9999)
      return;
    year = static_cast<int>(next_year);
    month = static_cast<int>(index % 12) + 1;
    if (!month_has_day(year, month, day))
      continue;
    set_ymd(d, year, month, day);
    return;
  }
}

void add_years_keeping_day(Glib::Date& d, int step)
{
  const int day = static_cast<int>(d.get_day());
  const int month = static_cast<int>(d.get_month());
  int year = static_cast<int>(d.get_year());
  for (int i = 0; i < 480; ++i) {
    if (static_cast<long long>(year) + step > 9999)
      return;
    year += step;
    if (!month_has_day(year, month, day))
      continue;
    set_ymd(d, year, month, day);
    return;
  }
}

}  // namespace

void add_recur(Glib::Date& d, Recur r, int interval)
{
  if (!d.valid())
    return;
  const int n = interval > 0 ? interval : 1;
  switch (r) {
    case Recur::daily:
      d.add_days(n);
      break;
    case Recur::weekly:
      d.add_days(7 * n);
      break;
    case Recur::monthly:
      add_months_keeping_day(d, n);
      break;
    case Recur::yearly:
      add_years_keeping_day(d, n);
      break;
    case Recur::none:
    default:
      break;
  }
}

void normalize_todo(Todo& t)
{
  if (t.percent < 0)
    t.percent = 0;
  if (t.percent > 100)
    t.percent = 100;
  if (t.recur_interval < 1)
    t.recur_interval = 1;
  if (t.done || t.status == TodoStatus::completed || t.percent == 100) {
    t.done = true;
    t.status = TodoStatus::completed;
    t.percent = 100;
    if (!t.has_completed) {
      t.has_completed = true;
      t.completed.set_time_current();
    }
  } else {
    t.done = false;
    if (t.status == TodoStatus::completed)
      t.status = TodoStatus::not_started;
    t.has_completed = false;
  }
}

bool stamp_to_date(const std::string& stamped, Glib::Date& out)
{
  if (stamped.size() < 10)
    return false;
  return date_from_iso(stamped.substr(0, 10), out);
}

const char* planner_color(int category)
{
  static const char* colors[] = {"#C45C4A", "#1660C4", "#1B4D3E", "#B8860B", "#5C4A6B"};
  if (category < 0 || category >= kPlannerKeyCount)
    return colors[4];
  return colors[category];
}

std::string now_stamp()
{
  GDateTime* dt = g_date_time_new_now_local();
  if (!dt)
    return {};
  gchar* s = g_date_time_format(dt, "%Y-%m-%d %H:%M");
  g_date_time_unref(dt);
  std::string out = s ? s : "";
  g_free(s);
  return out;
}

Glib::ustring format_hm(int mins)
{
  if (mins < 0)
    mins = 0;
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%02d:%02d", mins / 60, mins % 60);
  return buf;
}

int parse_hm(const Glib::ustring& s)
{
  int h = 0, m = 0;
  if (std::sscanf(s.c_str(), "%d:%d", &h, &m) != 2)
    return 8 * 60;
  if (h < 0)
    h = 0;
  if (h > 23)
    h = 23;
  if (m < 0)
    m = 0;
  if (m > 59)
    m = 59;
  return h * 60 + m;
}

std::string date_iso(const Glib::Date& d)
{
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", static_cast<int>(d.get_year()),
                static_cast<int>(d.get_month()), d.get_day());
  return buf;
}

bool date_from_iso(const std::string& s, Glib::Date& out)
{
  int y = 0, m = 0, d = 0;
  if (std::sscanf(s.c_str(), "%d-%d-%d", &y, &m, &d) != 3)
    return false;
  if (y < 1 || y > 9999 || m < 1 || m > 12 || d < 1 || d > 31)
    return false;
  const auto day = static_cast<Glib::Date::Day>(d);
  const auto month = static_cast<Glib::Date::Month>(m);
  const auto year = static_cast<Glib::Date::Year>(y);
  // set_dmy of an impossible day leaves the previous date and logs a critical.
  if (!Glib::Date::valid_dmy(day, month, year))
    return false;
  out.set_dmy(day, month, year);
  return out.valid();
}

bool apply_iso_date(bool enabled, const std::string& text, Glib::Date& date)
{
  if (!enabled)
    return false;
  if (date_from_iso(text, date))
    return true;
  // Unparseable text leaves a previous valid date in place.
  return date.valid();
}

Glib::ustring Contact::display_name() const
{
  if (!first.empty() && !last.empty())
    return first + " " + last;
  if (!last.empty())
    return last;
  if (!first.empty())
    return first;
  return "Unnamed";
}

Glib::ustring Contact::sort_label() const
{
  if (!last.empty() && !first.empty())
    return last + ", " + first;
  if (!last.empty())
    return last;
  if (!first.empty())
    return first;
  return "Unnamed";
}

gunichar Contact::last_initial() const
{
  const Glib::ustring& src = last.empty() ? first : last;
  if (src.empty())
    return 0;
  const gunichar ch = g_unichar_toupper(src[0]);
  if (ch >= 'A' && ch <= 'Z')
    return ch;
  return 0;
}

std::string Binder::display_name() const
{
  if (path_.empty())
    return "Untitled";
  return Glib::path_get_basename(path_);
}

void Binder::close()
{
  appts_.clear();
  todos_.clear();
  contacts_.clear();
  planner_.clear();
  default_planner_keys(planner_keys_);
  notes_.clear();
  next_note_id_ = 1;
  path_.clear();
  error_.clear();
  next_id_ = 1;
  next_todo_id_ = 1;
  next_contact_id_ = 1;
  next_planner_id_ = 1;
  open_ = false;
  dirty_ = false;
}

void Binder::create_new()
{
  close();
  open_ = true;
}

bool Binder::occurs_on(const Appointment& a, const Glib::Date& date) const
{
  if (!a.date.valid() || !date.valid())
    return false;
  if (date.compare(a.date) < 0)
    return false;
  if (a.has_until && a.until.valid() && date.compare(a.until) > 0)
    return false;
  const int interval = a.recur_interval > 0 ? a.recur_interval : 1;
  if (a.recur == Recur::none)
    return date_eq(a.date, date);
  const long delta = static_cast<long>(date.get_julian()) - static_cast<long>(a.date.get_julian());
  if (a.recur == Recur::daily)
    return delta % interval == 0;
  if (a.recur == Recur::weekly) {
    if (date.get_weekday() != a.date.get_weekday())
      return false;
    return (delta / 7) % interval == 0;
  }
  if (a.recur == Recur::monthly) {
    const int months =
        (static_cast<int>(date.get_year()) - static_cast<int>(a.date.get_year())) * 12 +
        (static_cast<int>(date.get_month()) - static_cast<int>(a.date.get_month()));
    if (months < 0 || months % interval != 0)
      return false;
    const int dim = Glib::Date::get_days_in_month(date.get_month(), date.get_year());
    const int want = a.date.get_day();
    return date.get_day() == (want > dim ? dim : want);
  }
  if (a.recur == Recur::yearly) {
    const int years = static_cast<int>(date.get_year()) - static_cast<int>(a.date.get_year());
    if (years < 0 || years % interval != 0)
      return false;
    if (date.get_month() != a.date.get_month())
      return false;
    const int dim = Glib::Date::get_days_in_month(date.get_month(), date.get_year());
    const int want = a.date.get_day();
    return date.get_day() == (want > dim ? dim : want);
  }
  return false;
}

const Appointment* Binder::find(int id) const
{
  for (const auto& a : appts_) {
    if (a.id == id)
      return &a;
  }
  return nullptr;
}

std::vector<Appointment> Binder::for_date(const Glib::Date& date) const
{
  std::vector<Appointment> out;
  for (const auto& a : appts_) {
    if (!occurs_on(a, date))
      continue;
    Appointment copy = a;
    copy.date = date;
    out.push_back(std::move(copy));
  }
  std::sort(out.begin(), out.end(),
            [](const Appointment& x, const Appointment& y) { return x.start_min < y.start_min; });
  return out;
}

bool Binder::has_on(const Glib::Date& date) const
{
  for (const auto& a : appts_) {
    if (occurs_on(a, date))
      return true;
  }
  return false;
}

std::set<int> Binder::days_in_month(Glib::Date::Month month, Glib::Date::Year year) const
{
  std::set<int> days;
  const int dim = Glib::Date::get_days_in_month(month, year);
  for (int d = 1; d <= dim; ++d) {
    Glib::Date date(static_cast<Glib::Date::Day>(d), month, year);
    if (has_on(date))
      days.insert(d);
    for (const auto& t : todos_) {
      if (t.has_due && date_eq(t.due, date))
        days.insert(d);
    }
    for (const auto& p : planner_) {
      if (p.start.valid() && p.end.valid() && date.compare(p.start) >= 0 &&
          date.compare(p.end) <= 0)
        days.insert(d);
    }
  }
  return days;
}

int Binder::add_appointment(const Appointment& a)
{
  if (!open_)
    create_new();
  Appointment n = a;
  n.id = next_id_++;
  if (n.end_min <= n.start_min)
    n.end_min = n.start_min + 30;
  appts_.push_back(n);
  dirty_ = true;
  return n.id;
}

bool Binder::update_appointment(const Appointment& a)
{
  for (auto& x : appts_) {
    if (x.id == a.id) {
      x = a;
      if (x.end_min <= x.start_min)
        x.end_min = x.start_min + 30;
      dirty_ = true;
      return true;
    }
  }
  return false;
}

bool Binder::remove_appointment(int id)
{
  auto it = std::remove_if(appts_.begin(), appts_.end(),
                           [id](const Appointment& a) { return a.id == id; });
  if (it == appts_.end())
    return false;
  appts_.erase(it, appts_.end());
  dirty_ = true;
  return true;
}

bool todo_less(const Todo& a, const Todo& b)
{
  if (a.done != b.done)
    return !a.done;
  if (a.has_due != b.has_due)
    return a.has_due;
  if (a.has_due && a.due.get_julian() != b.due.get_julian())
    return a.due.get_julian() < b.due.get_julian();
  const int pa = a.priority == 0 ? 9 : a.priority;
  const int pb = b.priority == 0 ? 9 : b.priority;
  if (pa != pb)
    return pa < pb;
  return a.id < b.id;
}

std::vector<Todo> Binder::todos() const
{
  std::vector<Todo> out = todos_;
  std::sort(out.begin(), out.end(), todo_less);
  return out;
}

std::vector<Todo> Binder::todos_due_on(const Glib::Date& date) const
{
  std::vector<Todo> out;
  for (const auto& t : todos_) {
    if (t.has_due && date_eq(t.due, date))
      out.push_back(t);
  }
  std::sort(out.begin(), out.end(), todo_less);
  return out;
}

int Binder::open_todo_count() const
{
  int n = 0;
  for (const auto& t : todos_) {
    if (!t.done)
      ++n;
  }
  return n;
}

const Todo* Binder::find_todo(int id) const
{
  for (const auto& t : todos_) {
    if (t.id == id)
      return &t;
  }
  return nullptr;
}

int Binder::add_todo(const Todo& t)
{
  if (!open_)
    create_new();
  Todo n = t;
  n.id = next_todo_id_++;
  normalize_todo(n);
  todos_.push_back(n);
  dirty_ = true;
  return n.id;
}

bool Binder::update_todo(const Todo& t)
{
  for (auto& x : todos_) {
    if (x.id == t.id) {
      x = t;
      normalize_todo(x);
      dirty_ = true;
      return true;
    }
  }
  return false;
}

bool Binder::complete_todo(int id)
{
  Todo prev;
  bool found = false;
  for (auto& x : todos_) {
    if (x.id != id)
      continue;
    if (x.done)
      return true;
    prev = x;
    x.done = true;
    x.status = TodoStatus::completed;
    x.percent = 100;
    normalize_todo(x);
    dirty_ = true;
    found = true;
    break;
  }
  if (!found)
    return false;
  if (prev.recur == Recur::none || (!prev.has_due && !prev.has_start))
    return true;
  Todo n = prev;
  n.done = false;
  n.status = TodoStatus::not_started;
  n.percent = 0;
  n.has_completed = false;
  if (n.has_due)
    add_recur(n.due, n.recur, n.recur_interval);
  if (n.has_start)
    add_recur(n.start, n.recur, n.recur_interval);
  if (n.has_until && n.until.valid()) {
    const Glib::Date& check = n.has_due ? n.due : n.start;
    if (check.valid() && check.compare(n.until) > 0)
      return true;
  }
  add_todo(n);
  return true;
}

std::vector<Glib::ustring> Binder::todo_categories() const
{
  std::set<Glib::ustring> names;
  for (const auto& t : todos_) {
    if (!t.category.empty())
      names.insert(t.category);
  }
  return {names.begin(), names.end()};
}

bool Binder::remove_todo(int id)
{
  auto it =
      std::remove_if(todos_.begin(), todos_.end(), [id](const Todo& t) { return t.id == id; });
  if (it == todos_.end())
    return false;
  todos_.erase(it, todos_.end());
  dirty_ = true;
  return true;
}

bool contact_less(const Contact& a, const Contact& b)
{
  const Glib::ustring la = a.last.casefold();
  const Glib::ustring lb = b.last.casefold();
  if (la != lb)
    return la < lb;
  const Glib::ustring fa = a.first.casefold();
  const Glib::ustring fb = b.first.casefold();
  if (fa != fb)
    return fa < fb;
  return a.id < b.id;
}

std::vector<Contact> Binder::contacts() const
{
  std::vector<Contact> out = contacts_;
  std::sort(out.begin(), out.end(), contact_less);
  return out;
}

const Contact* Binder::find_contact(int id) const
{
  for (const auto& c : contacts_) {
    if (c.id == id)
      return &c;
  }
  return nullptr;
}

int Binder::add_contact(const Contact& c)
{
  if (!open_)
    create_new();
  Contact n = c;
  clamp_notes(n);
  n.id = next_contact_id_++;
  contacts_.push_back(n);
  dirty_ = true;
  return n.id;
}

bool Binder::update_contact(const Contact& c)
{
  for (auto& x : contacts_) {
    if (x.id == c.id) {
      x = c;
      clamp_notes(x);
      dirty_ = true;
      return true;
    }
  }
  return false;
}

std::vector<PlannerEvent> Binder::planner_events() const
{
  return planner_;
}

std::vector<PlannerEvent> Binder::planner_on(const Glib::Date& date) const
{
  std::vector<PlannerEvent> out;
  for (const auto& p : planner_) {
    if (p.start.valid() && p.end.valid() && date.compare(p.start) >= 0 && date.compare(p.end) <= 0)
      out.push_back(p);
  }
  return out;
}

int Binder::add_planner(const PlannerEvent& e)
{
  if (!open_)
    create_new();
  PlannerEvent n = e;
  n.id = next_planner_id_++;
  if (n.end.valid() && n.start.valid() && n.end.compare(n.start) < 0)
    std::swap(n.start, n.end);
  if (n.category < 0 || n.category >= kPlannerKeyCount)
    n.category = 0;
  planner_.push_back(n);
  dirty_ = true;
  return n.id;
}

bool Binder::update_planner(const PlannerEvent& e)
{
  for (auto& x : planner_) {
    if (x.id == e.id) {
      x = e;
      if (x.end.valid() && x.start.valid() && x.end.compare(x.start) < 0)
        std::swap(x.start, x.end);
      dirty_ = true;
      return true;
    }
  }
  return false;
}

bool Binder::remove_planner(int id)
{
  auto it = std::remove_if(planner_.begin(), planner_.end(),
                           [id](const PlannerEvent& e) { return e.id == id; });
  if (it == planner_.end())
    return false;
  planner_.erase(it, planner_.end());
  dirty_ = true;
  return true;
}

const PlannerEvent* Binder::find_planner(int id) const
{
  for (const auto& p : planner_) {
    if (p.id == id)
      return &p;
  }
  return nullptr;
}

std::array<Glib::ustring, kPlannerKeyCount> Binder::planner_keys() const
{
  return planner_keys_;
}

std::vector<Note> Binder::notes() const
{
  return notes_;
}

int Binder::add_note(const Note& n)
{
  if (!open_)
    create_new();
  Note x = n;
  x.id = next_note_id_++;
  if (x.stamped.empty())
    x.stamped = now_stamp();
  notes_.push_back(x);
  dirty_ = true;
  return x.id;
}

std::vector<Glib::ustring> Binder::note_categories() const
{
  std::set<Glib::ustring> names;
  for (const auto& n : notes_) {
    if (!n.category.empty())
      names.insert(n.category);
  }
  return {names.begin(), names.end()};
}

bool Binder::update_note(const Note& n)
{
  for (auto& x : notes_) {
    if (x.id == n.id) {
      x = n;
      dirty_ = true;
      return true;
    }
  }
  return false;
}

bool Binder::remove_note(int id)
{
  auto it =
      std::remove_if(notes_.begin(), notes_.end(), [id](const Note& n) { return n.id == id; });
  if (it == notes_.end())
    return false;
  notes_.erase(it, notes_.end());
  dirty_ = true;
  return true;
}

const Note* Binder::find_note(int id) const
{
  for (const auto& n : notes_) {
    if (n.id == id)
      return &n;
  }
  return nullptr;
}

void Binder::set_planner_key(int index, const Glib::ustring& name)
{
  if (index < 0 || index >= kPlannerKeyCount)
    return;
  Glib::ustring n = name;
  if (n.empty())
    return;
  planner_keys_[static_cast<size_t>(index)] = std::move(n);
  dirty_ = true;
}

bool Binder::remove_contact(int id)
{
  auto it = std::remove_if(contacts_.begin(), contacts_.end(),
                           [id](const Contact& c) { return c.id == id; });
  if (it == contacts_.end())
    return false;
  contacts_.erase(it, contacts_.end());
  dirty_ = true;
  return true;
}

bool Binder::write_file(const std::string& path) const
{
  std::ostringstream os;
  os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  os << "<ephemeris version=\"1\">\n";
  for (const auto& a : appts_) {
    os << "  <appointment id=\"" << a.id << "\" date=\"" << date_iso(a.date) << "\" start=\""
       << format_hm(a.start_min) << "\" end=\"" << format_hm(a.end_min) << "\"";
    if (a.recur != Recur::none) {
      os << " recur=\"" << recur_attr(a.recur) << "\" interval=\"" << a.recur_interval << "\"";
      if (a.has_until && a.until.valid())
        os << " until=\"" << date_iso(a.until) << "\"";
    }
    os << ">" << xml_escape(a.text) << "</appointment>\n";
  }
  for (int i = 0; i < kPlannerKeyCount; ++i) {
    os << "  <planner-key index=\"" << i << "\">"
       << xml_escape(planner_keys_[static_cast<size_t>(i)]) << "</planner-key>\n";
  }
  for (const auto& p : planner_) {
    os << "  <planner id=\"" << p.id << "\" start=\"" << date_iso(p.start) << "\" end=\""
       << date_iso(p.end) << "\" category=\"" << p.category << "\">" << xml_escape(p.text)
       << "</planner>\n";
  }
  for (const auto& n : notes_) {
    os << "  <note id=\"" << n.id << "\" title=\"" << xml_escape_attr(n.title) << "\" stamped=\""
       << xml_escape_attr(n.stamped) << "\" colour=\"" << colour_attr(n.colour) << "\" category=\""
       << xml_escape_attr(n.category) << "\">" << xml_escape(n.body) << "</note>\n";
  }
  for (const auto& t : todos_) {
    os << "  <todo id=\"" << t.id << "\" done=\"" << (t.done ? "true" : "false") << "\" priority=\""
       << t.priority << "\" status=\"" << status_attr(t.status) << "\" percent=\"" << t.percent
       << "\" text=\"" << xml_escape_attr(t.text) << "\" category=\"" << xml_escape_attr(t.category)
       << "\"";
    if (t.has_due)
      os << " due=\"" << date_iso(t.due) << "\"";
    if (t.has_start)
      os << " start=\"" << date_iso(t.start) << "\"";
    if (t.has_completed && t.completed.valid())
      os << " completed=\"" << date_iso(t.completed) << "\"";
    if (t.recur != Recur::none) {
      os << " recur=\"" << recur_attr(t.recur) << "\" interval=\"" << t.recur_interval << "\"";
      if (t.has_until && t.until.valid())
        os << " until=\"" << date_iso(t.until) << "\"";
    }
    os << ">" << xml_escape(t.notes) << "</todo>\n";
  }
  for (const auto& c : contacts_) {
    os << "  <contact id=\"" << c.id << "\" first=\"" << xml_escape_attr(c.first) << "\" last=\""
       << xml_escape_attr(c.last) << "\" phone=\"" << xml_escape_attr(c.phone) << "\" email=\""
       << xml_escape_attr(c.email) << "\" timezone=\"" << xml_escape_attr(c.timezone) << "\">"
       << xml_escape(c.notes) << "</contact>\n";
  }
  os << "</ephemeris>\n";
  try {
    Glib::file_set_contents(path, os.str());
  } catch (const Glib::Error&) {
    return false;
  }
  return true;
}

bool Binder::save()
{
  error_.clear();
  if (path_.empty()) {
    error_ = "No file name.";
    return false;
  }
  if (!write_file(path_)) {
    error_ = "Could not write " + path_ + ".";
    return false;
  }
  dirty_ = false;
  return true;
}

bool Binder::save_as(const std::string& path)
{
  error_.clear();
  if (path.empty()) {
    error_ = "No file name.";
    return false;
  }
  if (!write_file(path)) {
    error_ = "Could not write " + path + ".";
    return false;
  }
  path_ = path;
  dirty_ = false;
  open_ = true;
  return true;
}

bool Binder::open(const std::string& path)
{
  error_.clear();
  xmlDoc* doc = xmlReadFile(path.c_str(), nullptr, XML_PARSE_NONET | XML_PARSE_NOBLANKS);
  if (!doc) {
    error_ = "Not an Ephemeris binder.";
    return false;
  }
  xmlNode* root = xmlDocGetRootElement(doc);
  if (!root || node_name(root) != "ephemeris") {
    xmlFreeDoc(doc);
    error_ = "Not an Ephemeris binder.";
    return false;
  }

  std::vector<Appointment> loaded;
  std::vector<Todo> loaded_todos;
  std::vector<Contact> loaded_contacts;
  std::vector<PlannerEvent> loaded_planner;
  std::vector<Note> loaded_notes;
  int max_note = 0;
  std::array<Glib::ustring, kPlannerKeyCount> loaded_keys;
  default_planner_keys(loaded_keys);
  int max_id = 0;
  int max_todo = 0;
  int max_contact = 0;
  int max_planner = 0;
  for (xmlNode* n = root->children; n; n = n->next) {
    if (n->type != XML_ELEMENT_NODE)
      continue;
    if (node_name(n) == "appointment") {
      Appointment a;
      const std::string id_s = node_prop(n, "id");
      if (!id_s.empty())
        a.id = static_cast<int>(g_ascii_strtoll(id_s.c_str(), nullptr, 10));
      date_from_iso(node_prop(n, "date"), a.date);
      a.start_min = parse_hm(node_prop(n, "start"));
      const std::string end = node_prop(n, "end");
      a.end_min = end.empty() ? a.start_min + 30 : parse_hm(end);
      a.recur = parse_recur(node_prop(n, "recur"));
      const std::string iv = node_prop(n, "interval");
      if (!iv.empty())
        a.recur_interval = std::max(1, static_cast<int>(g_ascii_strtoll(iv.c_str(), nullptr, 10)));
      const std::string until = node_prop(n, "until");
      a.has_until = !until.empty() && date_from_iso(until, a.until);
      a.text = node_text(n);
      if (a.id > max_id)
        max_id = a.id;
      loaded.push_back(std::move(a));
    } else if (node_name(n) == "todo") {
      Todo t;
      const std::string id_s = node_prop(n, "id");
      if (!id_s.empty())
        t.id = static_cast<int>(g_ascii_strtoll(id_s.c_str(), nullptr, 10));
      t.done = node_prop(n, "done") == "true";
      t.priority = static_cast<int>(g_ascii_strtoll(node_prop(n, "priority").c_str(), nullptr, 10));
      const std::string due = node_prop(n, "due");
      t.has_due = !due.empty() && date_from_iso(due, t.due);
      const std::string start = node_prop(n, "start");
      t.has_start = !start.empty() && date_from_iso(start, t.start);
      const std::string completed = node_prop(n, "completed");
      t.has_completed = !completed.empty() && date_from_iso(completed, t.completed);
      t.status = parse_status(node_prop(n, "status"));
      const std::string pct = node_prop(n, "percent");
      if (!pct.empty())
        t.percent = static_cast<int>(g_ascii_strtoll(pct.c_str(), nullptr, 10));
      t.category = node_prop(n, "category");
      t.recur = parse_recur(node_prop(n, "recur"));
      const std::string iv = node_prop(n, "interval");
      if (!iv.empty())
        t.recur_interval = std::max(1, static_cast<int>(g_ascii_strtoll(iv.c_str(), nullptr, 10)));
      const std::string until = node_prop(n, "until");
      t.has_until = !until.empty() && date_from_iso(until, t.until);
      const std::string subj = node_prop(n, "text");
      if (!subj.empty()) {
        t.text = subj;
        t.notes = node_text(n);
      } else {
        t.text = node_text(n);
      }
      if (t.done && t.status == TodoStatus::not_started)
        t.status = TodoStatus::completed;
      normalize_todo(t);
      if (t.id > max_todo)
        max_todo = t.id;
      loaded_todos.push_back(std::move(t));
    } else if (node_name(n) == "contact") {
      Contact c;
      const std::string id_s = node_prop(n, "id");
      if (!id_s.empty())
        c.id = static_cast<int>(g_ascii_strtoll(id_s.c_str(), nullptr, 10));
      c.first = node_prop(n, "first");
      c.last = node_prop(n, "last");
      c.phone = node_prop(n, "phone");
      c.email = node_prop(n, "email");
      c.timezone = node_prop(n, "timezone");
      c.notes = node_text(n);
      clamp_notes(c);
      if (c.id > max_contact)
        max_contact = c.id;
      loaded_contacts.push_back(std::move(c));
    } else if (node_name(n) == "planner-key") {
      const int idx = static_cast<int>(g_ascii_strtoll(node_prop(n, "index").c_str(), nullptr, 10));
      if (idx >= 0 && idx < kPlannerKeyCount) {
        Glib::ustring name = node_text(n);
        if (!name.empty())
          loaded_keys[static_cast<size_t>(idx)] = std::move(name);
      }
    } else if (node_name(n) == "planner") {
      PlannerEvent p;
      const std::string id_s = node_prop(n, "id");
      if (!id_s.empty())
        p.id = static_cast<int>(g_ascii_strtoll(id_s.c_str(), nullptr, 10));
      date_from_iso(node_prop(n, "start"), p.start);
      date_from_iso(node_prop(n, "end"), p.end);
      p.category = static_cast<int>(g_ascii_strtoll(node_prop(n, "category").c_str(), nullptr, 10));
      if (p.category < 0 || p.category >= kPlannerKeyCount)
        p.category = 0;
      p.text = node_text(n);
      if (p.end.valid() && p.start.valid() && p.end.compare(p.start) < 0)
        std::swap(p.start, p.end);
      if (p.id > max_planner)
        max_planner = p.id;
      loaded_planner.push_back(std::move(p));
    } else if (node_name(n) == "note") {
      Note note;
      const std::string id_s = node_prop(n, "id");
      if (!id_s.empty())
        note.id = static_cast<int>(g_ascii_strtoll(id_s.c_str(), nullptr, 10));
      note.title = node_prop(n, "title");
      note.stamped = node_prop(n, "stamped");
      note.colour = parse_colour(node_prop(n, "colour"));
      note.category = node_prop(n, "category");
      note.body = node_text(n);
      if (note.id > max_note)
        max_note = note.id;
      loaded_notes.push_back(std::move(note));
    }
  }
  xmlFreeDoc(doc);

  for (auto& a : loaded) {
    if (a.id < 1)
      a.id = ++max_id;
  }
  for (auto& t : loaded_todos) {
    if (t.id < 1)
      t.id = ++max_todo;
  }
  for (auto& c : loaded_contacts) {
    if (c.id < 1)
      c.id = ++max_contact;
  }
  for (auto& p : loaded_planner) {
    if (p.id < 1)
      p.id = ++max_planner;
  }
  appts_ = std::move(loaded);
  todos_ = std::move(loaded_todos);
  contacts_ = std::move(loaded_contacts);
  for (auto& n : loaded_notes) {
    if (n.id < 1)
      n.id = ++max_note;
  }
  planner_ = std::move(loaded_planner);
  planner_keys_ = std::move(loaded_keys);
  notes_ = std::move(loaded_notes);
  path_ = path;
  next_id_ = max_id + 1;
  if (next_id_ < 1)
    next_id_ = 1;
  next_todo_id_ = max_todo + 1;
  if (next_todo_id_ < 1)
    next_todo_id_ = 1;
  next_contact_id_ = max_contact + 1;
  if (next_contact_id_ < 1)
    next_contact_id_ = 1;
  next_planner_id_ = max_planner + 1;
  if (next_planner_id_ < 1)
    next_planner_id_ = 1;
  next_note_id_ = max_note + 1;
  if (next_note_id_ < 1)
    next_note_id_ = 1;
  open_ = true;
  dirty_ = false;
  return true;
}

}  // namespace ephemeris
