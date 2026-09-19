/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "binder.hpp"

#include <string>
#include <vector>

namespace ephemeris {

struct ParsedIcs {
  std::string title;
  std::vector<Appointment> items;
  std::string error;
};

/* Expand RRULE occurrences into Appointment rows (remote=true).
 * Window is [from, to] inclusive. FREQ DAILY/WEEKLY/MONTHLY/YEARLY with
 * INTERVAL, COUNT, UNTIL, BYDAY. No CalDAV; this is a file/URL parse. */
ParsedIcs parse_ics(const std::string& text, const Glib::Date& from, const Glib::Date& to);

}  // namespace ephemeris
