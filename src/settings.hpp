/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <string>

namespace ephemeris {

struct Settings {
  int window_x = -1;
  int window_y = -1;
  int window_w = 720;
  int window_h = 520;
  std::string last_path;
  std::string last_section = "calendar";
  std::string last_cal = "month";
  std::string last_date;

  void load();
  void save() const;
};

}  // namespace ephemeris
