/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <string>
#include <vector>

namespace ephemeris {

struct CalSub {
  std::string url;
  std::string title;
};

struct Settings {
  int window_x = -1;
  int window_y = -1;
  int window_w = 720;
  int window_h = 520;
  std::string last_path;
  std::string last_section = "calendar";
  std::string last_cal = "month";
  std::string last_date;
  std::vector<CalSub> calendars;

  void load();
  void save() const;
};

}  // namespace ephemeris
