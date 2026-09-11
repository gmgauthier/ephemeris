/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <glibmm/date.h>
#include <glibmm/ustring.h>

#include <set>
#include <string>
#include <vector>

namespace ephemeris {

struct Appointment {
  int id = 0;
  Glib::Date date;
  int start_min = 8 * 60;
  int end_min = 8 * 60 + 30;
  Glib::ustring text;
};

struct Todo {
  int id = 0;
  Glib::Date due;
  bool has_due = false;
  bool done = false;
  int priority = 0;  // 0 none, 1–3
  Glib::ustring text;
};

class Binder {
 public:
  bool is_open() const { return open_; }
  bool dirty() const { return dirty_; }
  const std::string& path() const { return path_; }
  const std::string& error() const { return error_; }
  std::string display_name() const;

  void close();
  void create_new();
  bool open(const std::string& path);
  bool save();
  bool save_as(const std::string& path);

  std::vector<Appointment> for_date(const Glib::Date& date) const;
  bool has_on(const Glib::Date& date) const;
  std::set<int> days_in_month(Glib::Date::Month month, Glib::Date::Year year) const;

  int add_appointment(const Appointment& a);
  bool update_appointment(const Appointment& a);
  bool remove_appointment(int id);
  const Appointment* find(int id) const;

  std::vector<Todo> todos() const;
  std::vector<Todo> todos_due_on(const Glib::Date& date) const;
  int open_todo_count() const;
  int add_todo(const Todo& t);
  bool update_todo(const Todo& t);
  bool remove_todo(int id);
  const Todo* find_todo(int id) const;

 private:
  bool write_file(const std::string& path) const;

  std::vector<Appointment> appts_;
  std::vector<Todo> todos_;
  std::string path_;
  std::string error_;
  int next_id_ = 1;
  int next_todo_id_ = 1;
  bool open_ = false;
  bool dirty_ = false;
};

Glib::ustring format_hm(int mins);
int parse_hm(const Glib::ustring& s);
std::string date_iso(const Glib::Date& d);
bool date_from_iso(const std::string& s, Glib::Date& out);

}  // namespace ephemeris
