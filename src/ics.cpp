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

/* nth == 0 means every weekday. A positive nth is from the start of the
 * month or year, a negative nth from the end (RFC 5545 BYDAY). */
struct ByDay {
  int weekday = -1;
  int nth = 0;
};

ByDay parse_byday(const std::string& tok)
{
  static const char* names[] = {"MO", "TU", "WE", "TH", "FR", "SA", "SU"};
  ByDay b;
  size_t i = 0;
  int sign = 1;
  if (i < tok.size() && (tok[i] == '+' || tok[i] == '-')) {
    if (tok[i] == '-')
      sign = -1;
    ++i;
  }
  int n = 0;
  bool has_n = false;
  while (i < tok.size() && std::isdigit(static_cast<unsigned char>(tok[i]))) {
    has_n = true;
    n = n * 10 + (tok[i] - '0');
    ++i;
  }
  if (has_n && n > 0)
    b.nth = sign * n;
  std::string name = tok.substr(i);
  for (char& ch : name)
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  for (int w = 0; w < 7; ++w) {
    if (name == names[w]) {
      b.weekday = w;
      break;
    }
  }
  return b;
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

/* RFC 5545 dur-value, minute resolution. Seconds are dropped. A leading '-' is
 * rejected so the caller leaves the end unset. */
bool duration_minutes(const std::string& raw, long& out_mins)
{
  size_t i = 0;
  if (i < raw.size() && (raw[i] == '+' || raw[i] == '-')) {
    if (raw[i] == '-')
      return false;
    ++i;
  }
  if (i >= raw.size() || raw[i] != 'P')
    return false;
  ++i;
  bool any = false;
  bool in_time = false;
  long weeks = 0;
  long days = 0;
  long hours = 0;
  long minutes = 0;
  while (i < raw.size()) {
    if (raw[i] == 'T') {
      if (in_time)
        return false;
      in_time = true;
      ++i;
      continue;
    }
    if (!std::isdigit(static_cast<unsigned char>(raw[i])))
      return false;
    long n = 0;
    while (i < raw.size() && std::isdigit(static_cast<unsigned char>(raw[i]))) {
      n = n * 10 + (raw[i] - '0');
      if (n > 1000000)
        return false;
      ++i;
    }
    if (i >= raw.size())
      return false;
    const char u = raw[i++];
    if (!in_time) {
      if (u == 'W')
        weeks = n;
      else if (u == 'D')
        days = n;
      else
        return false;
    } else if (u == 'H') {
      hours = n;
    } else if (u == 'M') {
      minutes = n;
    } else if (u == 'S') {
      /* Appointments are stored in minutes. */
    } else {
      return false;
    }
    any = true;
  }
  if (!any)
    return false;
  out_mins = ((weeks * 7L + days) * 24L + hours) * 60L + minutes;
  return true;
}

/* End stamp is DTSTART plus a duration, still in DTSTART's zone. */
Stamp stamp_plus_minutes(const Stamp& start, long add_mins)
{
  Stamp end = start;
  if (!start.ok || start.all_day || add_mins < 0)
    return Stamp{};
  const long day_mins = 24L * 60L;
  const long total = static_cast<long>(start.mins) + add_mins;
  const long add_days = total / day_mins;
  end.mins = static_cast<int>(total % day_mins);
  end.all_day = false;
  if (add_days > 0)
    end.date.add_days(static_cast<int>(add_days));
  end.ok = end.date.valid();
  return end;
}

enum class Freq { none, daily, weekly, monthly, yearly };

struct RRule {
  Freq freq = Freq::none;
  int interval = 1;
  int count = 0;
  bool has_until = false;
  Glib::Date until;
  std::vector<ByDay> byday;
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
          const ByDay bd =
              parse_byday(v.substr(q, c == std::string::npos ? std::string::npos : c - q));
          if (bd.weekday >= 0)
            r.byday.push_back(bd);
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

void append_nth(std::vector<Glib::Date>& out, const std::vector<Glib::Date>& hits, int nth)
{
  if (hits.empty())
    return;
  if (nth == 0) {
    out.insert(out.end(), hits.begin(), hits.end());
    return;
  }
  if (nth > 0) {
    if (nth <= static_cast<int>(hits.size()))
      out.push_back(hits[static_cast<size_t>(nth - 1)]);
    return;
  }
  const int idx = static_cast<int>(hits.size()) + nth;
  if (idx >= 0 && idx < static_cast<int>(hits.size()))
    out.push_back(hits[static_cast<size_t>(idx)]);
}

std::vector<Glib::Date> weekdays_between(const Glib::Date& first, int span, int weekday)
{
  std::vector<Glib::Date> hits;
  for (int i = 0; i < span; ++i) {
    Glib::Date d = first;
    d.add_days(i);
    if (monday_index(d.get_weekday()) == weekday)
      hits.push_back(d);
  }
  return hits;
}

std::vector<Glib::Date> dates_for_byday(const Glib::Date& first, int span,
                                        const std::vector<ByDay>& rules)
{
  std::vector<Glib::Date> out;
  for (const ByDay& bd : rules)
    append_nth(out, weekdays_between(first, span, bd.weekday), bd.nth);
  std::sort(out.begin(), out.end(),
            [](const Glib::Date& a, const Glib::Date& b) { return a.compare(b) < 0; });
  out.erase(std::unique(out.begin(), out.end(),
                        [](const Glib::Date& a, const Glib::Date& b) { return a.compare(b) == 0; }),
            out.end());
  return out;
}

std::vector<Glib::Date> byday_in_month(const Glib::Date& month_day1,
                                       const std::vector<ByDay>& rules)
{
  const int dim = Glib::Date::get_days_in_month(month_day1.get_month(), month_day1.get_year());
  return dates_for_byday(month_day1, dim, rules);
}

std::vector<Glib::Date> byday_in_year(int year, const std::vector<ByDay>& rules)
{
  const Glib::Date jan(1, Glib::Date::JANUARY, static_cast<Glib::Date::Year>(year));
  const int span = Glib::Date::is_leap_year(static_cast<Glib::Date::Year>(year)) ? 366 : 365;
  return dates_for_byday(jan, span, rules);
}

/* COUNT counts every instance from DTSTART, including those outside [from,to].
 * The safety cap (count == 0) counts only an instance that places a slice in the
 * window. day_span > 0 means the event covers later days. For a timed event,
 * proto.start_min is the first day's clock time and proto.end_min is the clock
 * time on the last day. For an all-day event both ends are the full day. Each
 * slice is drawn on its own date. The whole span still counts as one instance. */
bool take_instance(std::vector<Appointment>& out, const Appointment& proto, const Glib::Date& d,
                   const Glib::Date& from, const Glib::Date& to, int& emitted, int count,
                   bool has_until, const Glib::Date& until, int day_span)
{
  if (has_until && d.compare(until) > 0)
    return false;
  bool placed = false;
  auto push_day = [&](const Glib::Date& day, int start_min, int end_min) {
    if (end_min <= start_min)
      return;
    if (day.compare(from) < 0 || day.compare(to) > 0)
      return;
    Appointment a = proto;
    a.date = day;
    a.start_min = start_min;
    a.end_min = end_min;
    out.push_back(std::move(a));
    placed = true;
  };
  if (day_span <= 0) {
    if (d.compare(from) >= 0 && d.compare(to) <= 0)
      push_day(d, proto.start_min, proto.end_min);
  } else {
    for (int i = 0; i <= day_span; ++i) {
      Glib::Date day = d;
      day.add_days(i);
      if (i == 0)
        push_day(day, proto.start_min, 24 * 60);
      else if (i == day_span)
        push_day(day, 0, proto.end_min);
      else
        push_day(day, 0, 24 * 60);
    }
  }
  if (count > 0)
    ++emitted;
  else if (placed)
    ++emitted;
  if (count > 0 && emitted >= count)
    return false;
  return true;
}

void expand(const Appointment& proto, const Stamp& start, const Stamp& end, const RRule& rule,
            const Glib::Date& from, const Glib::Date& to, std::vector<Appointment>& out)
{
  int day_span = 0;
  const bool same_kind =
      end.ok && start.date.valid() && end.date.valid() && start.all_day == end.all_day;
  if (same_kind) {
    /* All-day DTEND is already exclusive-shortened, so this is an inclusive span. */
    day_span = static_cast<int>(end.date.get_julian()) - static_cast<int>(start.date.get_julian());
    if (day_span < 0)
      day_span = 0;
  }
  const int dur = end.ok ? (end.all_day ? 0 : end.mins - start.mins) : 30;
  Appointment base = proto;
  base.start_min = start.all_day ? 0 : start.mins;
  base.end_min = start.all_day ? 24 * 60 : (end.ok && !end.all_day ? end.mins : start.mins + 30);
  if (!start.all_day && day_span == 0 && base.end_min <= base.start_min)
    base.end_min = base.start_min + (dur > 0 ? dur : 30);
  base.remote = true;

  if (rule.freq == Freq::none) {
    int emitted = 0;
    take_instance(out, base, start.date, from, to, emitted, 0, false, start.date, day_span);
    return;
  }

  const int interval = rule.interval > 0 ? rule.interval : 1;
  int emitted = 0;
  const int cap = rule.count > 0 ? rule.count : 800;

  if (rule.freq == Freq::daily) {
    Glib::Date d = start.date;
    /* Keep the interval. A span that begins just before the window still shows. */
    if (rule.count == 0 && d.compare(from) < 0) {
      const int delta = static_cast<int>(from.get_julian()) - static_cast<int>(d.get_julian());
      int back = day_span > 0 ? day_span : 0;
      if (back > delta)
        back = delta;
      const int steps = (delta - back) / interval;
      if (steps > 0)
        d.add_days(steps * interval);
    }
    while (emitted < cap) {
      if (d.compare(to) > 0)
        break;
      if (!take_instance(out, base, d, from, to, emitted, rule.count, rule.has_until, rule.until,
                         day_span))
        break;
      d.add_days(interval);
    }
    return;
  }

  if (rule.freq == Freq::weekly) {
    std::vector<int> days;
    for (const ByDay& bd : rule.byday)
      days.push_back(bd.weekday);
    if (days.empty())
      days.push_back(monday_index(start.date.get_weekday()));
    /* Walk the week in weekday order. List order would let a later day consume
     * COUNT, and the return below would then skip the earlier day. */
    std::sort(days.begin(), days.end());
    days.erase(std::unique(days.begin(), days.end()), days.end());
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
        if (!take_instance(out, base, d, from, to, emitted, rule.count, rule.has_until, rule.until,
                           day_span))
          return;
      }
    }
    return;
  }

  if (rule.freq == Freq::monthly || rule.freq == Freq::yearly) {
    if (!rule.byday.empty()) {
      const bool yearly = rule.freq == Freq::yearly;
      for (int n = 0; n < 800 && emitted < cap; ++n) {
        std::vector<Glib::Date> dates;
        if (yearly) {
          const int year = start.date.get_year() + n * interval;
          dates = byday_in_year(year, rule.byday);
          Glib::Date jan(1, Glib::Date::JANUARY, static_cast<Glib::Date::Year>(year));
          if (jan.compare(to) > 0)
            break;
        } else {
          Glib::Date origin(1, start.date.get_month(), start.date.get_year());
          origin.add_months(n * interval);
          if (origin.compare(to) > 0)
            break;
          dates = byday_in_month(origin, rule.byday);
        }
        bool stop = false;
        for (const Glib::Date& d : dates) {
          if (d.compare(start.date) < 0)
            continue;
          if (d.compare(to) > 0) {
            stop = true;
            break;
          }
          if (!take_instance(out, base, d, from, to, emitted, rule.count, rule.has_until,
                             rule.until, day_span)) {
            stop = true;
            break;
          }
        }
        if (stop)
          break;
      }
      return;
    }

    const int step = rule.freq == Freq::yearly ? 12 * interval : interval;
    const int want_day = start.date.get_day();
    Glib::Date origin(1, start.date.get_month(), start.date.get_year());
    for (int n = 0; n < 800 && emitted < cap; ++n) {
      Glib::Date cur = origin;
      cur.add_months(n * step);
      if (cur.compare(to) > 0)
        break;
      const int dim = Glib::Date::get_days_in_month(cur.get_month(), cur.get_year());
      /* RFC 5545: a month with no such day is skipped, not clamped. */
      if (want_day > dim)
        continue;
      cur.set_day(static_cast<Glib::Date::Day>(want_day));
      if (cur.compare(start.date) < 0)
        continue;
      if (cur.compare(to) > 0)
        break;
      if (!take_instance(out, base, cur, from, to, emitted, rule.count, rule.has_until, rule.until,
                         day_span))
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
      if (!end.ok && start.ok && !start.all_day) {
        const auto dur = ev.find("DURATION");
        if (dur != ev.end()) {
          long mins = 0;
          if (duration_minutes(dur->second, mins))
            end = stamp_plus_minutes(start, mins);
        }
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
