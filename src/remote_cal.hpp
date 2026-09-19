/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "binder.hpp"
#include "settings.hpp"

#include <set>
#include <string>
#include <vector>

namespace ephemeris {

class RemoteCalendars {
 public:
  void set_subs(std::vector<CalSub> subs);
  const std::vector<CalSub>& subs() const
  {
    return subs_;
  }
  void load_caches();
  bool apply_ics(const std::string& url, const std::string& ics);
  void remove(const std::string& url);

  std::vector<Appointment> for_date(const Glib::Date& date) const;
  std::set<int> days_in_month(Glib::Date::Month month, Glib::Date::Year year) const;

 private:
  void expand_all();
  void window(Glib::Date& from, Glib::Date& to) const;

  std::vector<CalSub> subs_;
  std::vector<Appointment> items_;
};

std::string calendar_cache_path(const std::string& url);
void delete_calendar_cache(const std::string& url);

}  // namespace ephemeris
