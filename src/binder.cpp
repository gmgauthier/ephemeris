/* SPDX-License-Identifier: Unlicense */

#include "binder.hpp"

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <glib.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <algorithm>
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

}  // namespace

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
  if (m < 1 || m > 12 || d < 1 || d > 31)
    return false;
  out.set_dmy(static_cast<Glib::Date::Day>(d), static_cast<Glib::Date::Month>(m),
              static_cast<Glib::Date::Year>(y));
  return out.valid();
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
  path_.clear();
  error_.clear();
  next_id_ = 1;
  next_todo_id_ = 1;
  open_ = false;
  dirty_ = false;
}

void Binder::create_new()
{
  close();
  open_ = true;
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
    if (date_eq(a.date, date))
      out.push_back(a);
  }
  std::sort(out.begin(), out.end(), [](const Appointment& x, const Appointment& y) {
    return x.start_min < y.start_min;
  });
  return out;
}

bool Binder::has_on(const Glib::Date& date) const
{
  for (const auto& a : appts_) {
    if (date_eq(a.date, date))
      return true;
  }
  return false;
}

std::set<int> Binder::days_in_month(Glib::Date::Month month, Glib::Date::Year year) const
{
  std::set<int> days;
  for (const auto& a : appts_) {
    if (a.date.get_month() == month && a.date.get_year() == year)
      days.insert(a.date.get_day());
  }
  for (const auto& t : todos_) {
    if (t.has_due && t.due.get_month() == month && t.due.get_year() == year)
      days.insert(t.due.get_day());
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
  todos_.push_back(n);
  dirty_ = true;
  return n.id;
}

bool Binder::update_todo(const Todo& t)
{
  for (auto& x : todos_) {
    if (x.id == t.id) {
      x = t;
      dirty_ = true;
      return true;
    }
  }
  return false;
}

bool Binder::remove_todo(int id)
{
  auto it = std::remove_if(todos_.begin(), todos_.end(),
                           [id](const Todo& t) { return t.id == id; });
  if (it == todos_.end())
    return false;
  todos_.erase(it, todos_.end());
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
       << format_hm(a.start_min) << "\" end=\"" << format_hm(a.end_min) << "\">"
       << xml_escape(a.text) << "</appointment>\n";
  }
  for (const auto& t : todos_) {
    os << "  <todo id=\"" << t.id << "\" done=\"" << (t.done ? "true" : "false")
       << "\" priority=\"" << t.priority << "\"";
    if (t.has_due)
      os << " due=\"" << date_iso(t.due) << "\"";
    os << ">" << xml_escape(t.text) << "</todo>\n";
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
  int max_id = 0;
  int max_todo = 0;
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
      t.text = node_text(n);
      if (t.id > max_todo)
        max_todo = t.id;
      loaded_todos.push_back(std::move(t));
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
  appts_ = std::move(loaded);
  todos_ = std::move(loaded_todos);
  path_ = path;
  next_id_ = max_id + 1;
  if (next_id_ < 1)
    next_id_ = 1;
  next_todo_id_ = max_todo + 1;
  if (next_todo_id_ < 1)
    next_todo_id_ = 1;
  open_ = true;
  dirty_ = false;
  return true;
}

}  // namespace ephemeris
