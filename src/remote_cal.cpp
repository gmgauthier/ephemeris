/* SPDX-License-Identifier: Unlicense */

#include "remote_cal.hpp"
#include "ics.hpp"

#include <glib.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <sys/stat.h>
#include <unistd.h>

namespace ephemeris {
namespace {

std::string cache_dir()
{
  const std::string dir = Glib::build_filename(Glib::get_user_data_dir(), "ephemeris", "calendars");
  g_mkdir_with_parents(dir.c_str(), 0700);
  return dir;
}

}  // namespace

std::string calendar_cache_path(const std::string& url)
{
  gchar* sum = g_compute_checksum_for_string(G_CHECKSUM_SHA256, url.c_str(), url.size());
  std::string name = sum ? sum : "cal";
  g_free(sum);
  name += ".ics";
  return Glib::build_filename(cache_dir(), name);
}

void delete_calendar_cache(const std::string& url)
{
  ::unlink(calendar_cache_path(url).c_str());
}

void RemoteCalendars::window(Glib::Date& from, Glib::Date& to) const
{
  from.set_time_current();
  from.subtract_months(24);
  to.set_time_current();
  to.add_months(24);
}

void RemoteCalendars::set_subs(std::vector<CalSub> subs)
{
  subs_ = std::move(subs);
}

void RemoteCalendars::expand_all()
{
  items_.clear();
  Glib::Date from, to;
  window(from, to);
  for (auto& sub : subs_) {
    const std::string path = calendar_cache_path(sub.url);
    if (!Glib::file_test(path, Glib::FILE_TEST_IS_REGULAR))
      continue;
    std::ifstream in(path.c_str());
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ParsedIcs parsed = parse_ics(text, from, to);
    if (!parsed.title.empty())
      sub.title = parsed.title;
    for (auto& a : parsed.items) {
      a.calendar = sub.title.empty() ? Glib::ustring(sub.url) : Glib::ustring(sub.title);
      a.remote = true;
      items_.push_back(std::move(a));
    }
  }
}

void RemoteCalendars::load_caches()
{
  expand_all();
}

bool RemoteCalendars::apply_ics(const std::string& url, const std::string& ics)
{
  const std::string path = calendar_cache_path(url);
  const std::string tmp = path + ".tmp";
  {
    std::ofstream out(tmp.c_str(), std::ios::binary | std::ios::trunc);
    if (!out)
      return false;
    out << ics;
    if (!out)
      return false;
  }
  ::chmod(tmp.c_str(), 0600);
  if (std::rename(tmp.c_str(), path.c_str()) != 0) {
    ::unlink(tmp.c_str());
    return false;
  }
  Glib::Date from, to;
  window(from, to);
  ParsedIcs parsed = parse_ics(ics, from, to);
  for (auto& sub : subs_) {
    if (sub.url == url) {
      if (!parsed.title.empty())
        sub.title = parsed.title;
      break;
    }
  }
  expand_all();
  return parsed.error.empty();
}

void RemoteCalendars::remove(const std::string& url)
{
  subs_.erase(
      std::remove_if(subs_.begin(), subs_.end(), [&url](const CalSub& s) { return s.url == url; }),
      subs_.end());
  delete_calendar_cache(url);
  expand_all();
}

std::vector<Appointment> RemoteCalendars::for_date(const Glib::Date& date) const
{
  std::vector<Appointment> out;
  for (const auto& a : items_) {
    if (a.date.compare(date) == 0)
      out.push_back(a);
  }
  return out;
}

std::set<int> RemoteCalendars::days_in_month(Glib::Date::Month month, Glib::Date::Year year) const
{
  std::set<int> days;
  for (const auto& a : items_) {
    if (a.date.get_month() == month && a.date.get_year() == year)
      days.insert(a.date.get_day());
  }
  return days;
}

}  // namespace ephemeris
