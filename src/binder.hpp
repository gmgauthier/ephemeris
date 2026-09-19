/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <glib.h>
#include <glibmm/date.h>
#include <glibmm/ustring.h>

#include <array>
#include <set>
#include <string>
#include <vector>

namespace ephemeris {

enum class Recur { none, daily, weekly, monthly, yearly };

struct Appointment {
  int id = 0;
  Glib::Date date;
  int start_min = 8 * 60;
  int end_min = 8 * 60 + 30;
  Glib::ustring text;
  bool remote = false;
  Glib::ustring calendar;
  Recur recur = Recur::none;
  int recur_interval = 1;
  bool has_until = false;
  Glib::Date until;
};

constexpr int kPlannerKeyCount = 5;

struct PlannerEvent {
  int id = 0;
  Glib::Date start;
  Glib::Date end;
  int category = 0;
  Glib::ustring text;
};

struct Note {
  int id = 0;
  Glib::ustring title;
  Glib::ustring body;
  std::string stamped;
};

struct Todo {
  int id = 0;
  Glib::Date due;
  bool has_due = false;
  bool done = false;
  int priority = 0;  // 0 none, 1–3
  Glib::ustring text;
};

constexpr int kContactNotesMax = 1000;

struct Contact {
  int id = 0;
  Glib::ustring first;
  Glib::ustring last;
  Glib::ustring phone;
  Glib::ustring email;
  Glib::ustring timezone;
  Glib::ustring notes;

  Glib::ustring display_name() const;
  Glib::ustring sort_label() const;
  gunichar last_initial() const;
};

class Binder {
 public:
  bool is_open() const
  {
    return open_;
  }
  bool dirty() const
  {
    return dirty_;
  }
  const std::string& path() const
  {
    return path_;
  }
  const std::string& error() const
  {
    return error_;
  }
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
  bool occurs_on(const Appointment& a, const Glib::Date& date) const;

  std::vector<PlannerEvent> planner_events() const;
  std::vector<PlannerEvent> planner_on(const Glib::Date& date) const;
  int add_planner(const PlannerEvent& e);
  bool update_planner(const PlannerEvent& e);
  bool remove_planner(int id);
  const PlannerEvent* find_planner(int id) const;
  std::array<Glib::ustring, kPlannerKeyCount> planner_keys() const;
  void set_planner_key(int index, const Glib::ustring& name);

  std::vector<Note> notes() const;
  int add_note(const Note& n);
  bool update_note(const Note& n);
  bool remove_note(int id);
  const Note* find_note(int id) const;

  std::vector<Todo> todos() const;
  std::vector<Todo> todos_due_on(const Glib::Date& date) const;
  int open_todo_count() const;
  int add_todo(const Todo& t);
  bool update_todo(const Todo& t);
  bool remove_todo(int id);
  const Todo* find_todo(int id) const;

  std::vector<Contact> contacts() const;
  int add_contact(const Contact& c);
  bool update_contact(const Contact& c);
  bool remove_contact(int id);
  const Contact* find_contact(int id) const;

 private:
  bool write_file(const std::string& path) const;

  std::vector<Appointment> appts_;
  std::vector<Todo> todos_;
  std::vector<Contact> contacts_;
  std::vector<PlannerEvent> planner_;
  std::array<Glib::ustring, kPlannerKeyCount> planner_keys_{};
  std::vector<Note> notes_;
  int next_note_id_ = 1;
  std::string path_;
  std::string error_;
  int next_id_ = 1;
  int next_todo_id_ = 1;
  int next_contact_id_ = 1;
  int next_planner_id_ = 1;
  bool open_ = false;
  bool dirty_ = false;
};

Glib::ustring format_hm(int mins);
int parse_hm(const Glib::ustring& s);
std::string date_iso(const Glib::Date& d);
bool date_from_iso(const std::string& s, Glib::Date& out);
const char* recur_attr(Recur r);
Recur parse_recur(const std::string& s);
const char* planner_color(int category);
std::string now_stamp();

}  // namespace ephemeris
