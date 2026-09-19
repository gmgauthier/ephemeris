/* SPDX-License-Identifier: Unlicense */

#include "ics.hpp"

#include <glib.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <sstream>
#include <vector>

namespace ephemeris {
namespace {

std::string unfold(const std::string& in)
{
  std::string s;
  s.reserve(in.size());
  for (size_t i = 0; i < in.size(); ++i) {
    if (in[i] == '\r')
      continue;
    if (in[i] == '\n' && i + 1 < in.size() && (in[i + 1] == ' ' || in[i + 1] == '\t')) {
      ++i;
      continue;
    }
    s += in[i];
  }
  return s;
}

std::string ics_unescape(const std::string& in)
{
  std::string out;
  out.reserve(in.size());
  for (size_t i = 0; i < in.size(); ++i) {
    if (in[i] == '\\' && i + 1 < in.size()) {
      const char n = in[i + 1];
      if (n == 'n' || n == 'N')
        out += '\n';
      else
        out += n;
      ++i;
      continue;
    }
    out += in[i];
  }
  return out;
}

int monday_index(Glib::Date::Weekday wd)
{
  const int n = static_cast<int>(wd);
  if (n <= 0)
    return 0;
  return (n + 6) % 7;
}

int byday_index(const std::string& tok)
{
  static const char* names[] = {"MO", "TU", "WE", "TH", "FR", "SA", "SU"};
  std::string t = tok;
  while (!t.empty() &&
         (t[0] == '+' || t[0] == '-' || std::isdigit(static_cast<unsigned char>(t[0]))))
    t.erase(t.begin());
  for (int i = 0; i < 7; ++i) {
    if (t == names[i])
      return i;
  }
  return -1;
}

struct Stamp {
  Glib::Date date;
  int mins = 0;
  bool all_day = false;
  bool utc = false;
  std::string tzid;
  bool ok = false;
};

int parse_hhmm(const std::string& t)
{
  if (t.size() < 4)
    return 0;
  const int h = (t[0] - '0') * 10 + (t[1] - '0');
  const int m = (t[2] - '0') * 10 + (t[3] - '0');
  if (h < 0 || h > 23 || m < 0 || m > 59)
    return 0;
  return h * 60 + m;
}

Stamp parse_stamp(const std::string& key, const std::string& value)
{
  Stamp s;
  std::string params = key;
  const auto sc = params.find(';');
  if (sc != std::string::npos)
    params = params.substr(sc + 1);
  else
    params.clear();
  std::string tzid;
  bool value_date = false;
  size_t p = 0;
  while (p < params.size()) {
    const size_t semi = params.find(';', p);
    const std::string piece =
        params.substr(p, semi == std::string::npos ? std::string::npos : semi - p);
    const auto eq = piece.find('=');
    if (eq != std::string::npos) {
      const std::string n = piece.substr(0, eq);
      const std::string v = piece.substr(eq + 1);
      if (n == "TZID")
        tzid = v;
      else if (n == "VALUE" && v == "DATE")
        value_date = true;
    }
    if (semi == std::string::npos)
      break;
    p = semi + 1;
  }

  std::string v = value;
  if (!v.empty() && v.back() == 'Z') {
    s.utc = true;
    v.pop_back();
  }
  if (v.size() >= 8) {
    const int y = std::atoi(v.substr(0, 4).c_str());
    const int mo = std::atoi(v.substr(4, 2).c_str());
    const int d = std::atoi(v.substr(6, 2).c_str());
    if (y >= 1 && mo >= 1 && mo <= 12 && d >= 1 && d <= 31) {
      s.date.set_dmy(static_cast<Glib::Date::Day>(d), static_cast<Glib::Date::Month>(mo),
                     static_cast<Glib::Date::Year>(y));
      s.ok = s.date.valid();
    }
  }
  if (v.size() >= 15 && v[8] == 'T' && !value_date)
    s.mins = parse_hhmm(v.substr(9, 6));
  else
    s.all_day = true;
  s.tzid = tzid;
  return s;
}

void to_local(Stamp& s)
{
  if (!s.ok || s.all_day)
    return;
  GTimeZone* tz = nullptr;
  if (s.utc)
    tz = g_time_zone_new_utc();
  else if (!s.tzid.empty())
    tz = g_time_zone_new_identifier(s.tzid.c_str());
  if (!tz)
    return;
  GDateTime* dt = g_date_time_new(tz, s.date.get_year(), static_cast<int>(s.date.get_month()),
                                  s.date.get_day(), s.mins / 60, s.mins % 60, 0);
  g_time_zone_unref(tz);
  if (!dt)
    return;
  GDateTime* loc = g_date_time_to_local(dt);
  g_date_time_unref(dt);
  if (!loc)
    return;
  s.date.set_dmy(static_cast<Glib::Date::Day>(g_date_time_get_day_of_month(loc)),
                 static_cast<Glib::Date::Month>(g_date_time_get_month(loc)),
                 static_cast<Glib::Date::Year>(g_date_time_get_year(loc)));
  s.mins = g_date_time_get_hour(loc) * 60 + g_date_time_get_minute(loc);
  g_date_time_unref(loc);
}

enum class Freq { none, daily, weekly, monthly, yearly };

struct RRule {
  Freq freq = Freq::none;
  int interval = 1;
  int count = 0;
  bool has_until = false;
  Glib::Date until;
  std::vector<int> byday;
};

RRule parse_rrule(const std::string& raw)
{
  RRule r;
  std::string s = raw;
  size_t p = 0;
  while (p < s.size()) {
    const size_t semi = s.find(';', p);
    const std::string piece = s.substr(p, semi == std::string::npos ? std::string::npos : semi - p);
    const auto eq = piece.find('=');
    if (eq != std::string::npos) {
      const std::string n = piece.substr(0, eq);
      const std::string v = piece.substr(eq + 1);
      if (n == "FREQ") {
        if (v == "DAILY")
          r.freq = Freq::daily;
        else if (v == "WEEKLY")
          r.freq = Freq::weekly;
        else if (v == "MONTHLY")
          r.freq = Freq::monthly;
        else if (v == "YEARLY")
          r.freq = Freq::yearly;
      } else if (n == "INTERVAL")
        r.interval = std::max(1, std::atoi(v.c_str()));
      else if (n == "COUNT")
        r.count = std::max(0, std::atoi(v.c_str()));
      else if (n == "UNTIL") {
        Stamp u = parse_stamp("UNTIL", v);
        to_local(u);
        if (u.ok) {
          r.has_until = true;
          r.until = u.date;
        }
      } else if (n == "BYDAY") {
        size_t q = 0;
        while (q < v.size()) {
          const size_t c = v.find(',', q);
          const int idx =
              byday_index(v.substr(q, c == std::string::npos ? std::string::npos : c - q));
          if (idx >= 0)
            r.byday.push_back(idx);
          if (c == std::string::npos)
            break;
          q = c + 1;
        }
      }
    }
    if (semi == std::string::npos)
      break;
    p = semi + 1;
  }
  return r;
}

/* COUNT counts every instance from DTSTART, including those outside [from,to]. */
bool take_instance(std::vector<Appointment>& out, const Appointment& proto, const Glib::Date& d,
                   const Glib::Date& from, const Glib::Date& to, int& emitted, int count,
                   bool has_until, const Glib::Date& until)
{
  if (has_until && d.compare(until) > 0)
    return false;
  ++emitted;
  if (d.compare(from) >= 0 && d.compare(to) <= 0) {
    Appointment a = proto;
    a.date = d;
    out.push_back(std::move(a));
  }
  if (count > 0 && emitted >= count)
    return false;
  return true;
}

void expand(const Appointment& proto, const Stamp& start, const Stamp& end, const RRule& rule,
            const Glib::Date& from, const Glib::Date& to, std::vector<Appointment>& out)
{
  const int dur = end.ok ? (end.all_day ? 0 : end.mins - start.mins) : 30;
  Appointment base = proto;
  base.start_min = start.all_day ? 0 : start.mins;
  base.end_min = start.all_day ? 24 * 60 : (end.ok && !end.all_day ? end.mins : start.mins + 30);
  if (!start.all_day && base.end_min <= base.start_min)
    base.end_min = base.start_min + (dur > 0 ? dur : 30);
  base.remote = true;

  if (rule.freq == Freq::none) {
    int emitted = 0;
    take_instance(out, base, start.date, from, to, emitted, 0, false, start.date);
    return;
  }

  const int interval = rule.interval > 0 ? rule.interval : 1;
  int emitted = 0;
  const int cap = rule.count > 0 ? rule.count : 800;

  if (rule.freq == Freq::daily) {
    Glib::Date d = start.date;
    while (emitted < cap) {
      if (d.compare(to) > 0)
        break;
      if (!take_instance(out, base, d, from, to, emitted, rule.count, rule.has_until, rule.until))
        break;
      d.add_days(interval);
    }
    return;
  }

  if (rule.freq == Freq::weekly) {
    std::vector<int> days = rule.byday;
    if (days.empty())
      days.push_back(monday_index(start.date.get_weekday()));
    Glib::Date week0 = start.date;
    week0.subtract_days(monday_index(week0.get_weekday()));
    for (int w = 0; w < 4000 && emitted < cap; w += interval) {
      Glib::Date ws = week0;
      ws.add_days(w * 7);
      if (ws.compare(to) > 0)
        break;
      for (int bd : days) {
        Glib::Date d = ws;
        d.add_days(bd);
        if (d.compare(start.date) < 0)
          continue;
        if (!take_instance(out, base, d, from, to, emitted, rule.count, rule.has_until, rule.until))
          return;
      }
    }
    return;
  }

  if (rule.freq == Freq::monthly || rule.freq == Freq::yearly) {
    const int step = rule.freq == Freq::yearly ? 12 * interval : interval;
    const int want_day = start.date.get_day();
    Glib::Date origin(1, start.date.get_month(), start.date.get_year());
    for (int n = 0; n < 800 && emitted < cap; ++n) {
      Glib::Date cur = origin;
      cur.add_months(n * step);
      const int dim = Glib::Date::get_days_in_month(cur.get_month(), cur.get_year());
      cur.set_day(static_cast<Glib::Date::Day>(want_day > dim ? dim : want_day));
      if (cur.compare(start.date) < 0)
        continue;
      if (cur.compare(to) > 0)
        break;
      if (!take_instance(out, base, cur, from, to, emitted, rule.count, rule.has_until, rule.until))
        break;
    }
  }
}

}  // namespace

ParsedIcs parse_ics(const std::string& text, const Glib::Date& from, const Glib::Date& to)
{
  ParsedIcs out;
  const std::string body = unfold(text);
  std::istringstream in(body);
  std::string line;
  std::map<std::string, std::string> ev;
  bool in_event = false;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (line.compare(0, 12, "X-WR-CALNAME") == 0) {
      const auto c = line.find(':');
      if (c != std::string::npos && out.title.empty())
        out.title = ics_unescape(line.substr(c + 1));
      continue;
    }
    if (line == "BEGIN:VEVENT") {
      ev.clear();
      in_event = true;
      continue;
    }
    if (line == "END:VEVENT") {
      in_event = false;
      const auto st = ev.find("STATUS");
      if (st != ev.end() && st->second == "CANCELLED")
        continue;
      Stamp start, end;
      for (const auto& kv : ev) {
        if (kv.first.compare(0, 7, "DTSTART") == 0)
          start = parse_stamp(kv.first, kv.second);
        else if (kv.first.compare(0, 5, "DTEND") == 0)
          end = parse_stamp(kv.first, kv.second);
      }
      to_local(start);
      to_local(end);
      if (!start.ok)
        continue;
      if (start.all_day && end.ok && end.all_day && end.date.compare(start.date) > 0) {
        /* DATE DTEND is exclusive */
        end.date.subtract_days(1);
      }
      Appointment proto;
      proto.text = ics_unescape(ev.count("SUMMARY") ? ev["SUMMARY"] : "");
      proto.remote = true;
      RRule rule;
      if (ev.count("RRULE"))
        rule = parse_rrule(ev["RRULE"]);
      expand(proto, start, end, rule, from, to, out.items);
      continue;
    }
    if (!in_event)
      continue;
    const auto c = line.find(':');
    if (c == std::string::npos)
      continue;
    ev[line.substr(0, c)] = line.substr(c + 1);
  }
  if (out.items.empty() && out.title.empty() && text.find("BEGIN:VCALENDAR") == std::string::npos)
    out.error = "Not an iCalendar file";
  return out;
}

}  // namespace ephemeris
