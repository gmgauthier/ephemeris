/* SPDX-License-Identifier: Unlicense */

#include "settings.hpp"

#include <glib.h>
#include <glibmm/fileutils.h>
#include <glibmm/keyfile.h>
#include <glibmm/miscutils.h>

namespace ephemeris {
namespace {

std::string config_dir()
{
  return Glib::build_filename(Glib::get_user_config_dir(), "ephemeris");
}

std::string config_path()
{
  return Glib::build_filename(config_dir(), "ephemeris.ini");
}

int get_int(Glib::KeyFile& kf, const char* group, const char* key, int fallback)
{
  try {
    if (kf.has_key(group, key))
      return kf.get_integer(group, key);
  } catch (const Glib::Error&) {
  }
  return fallback;
}

std::string get_str(Glib::KeyFile& kf, const char* group, const char* key)
{
  try {
    if (kf.has_key(group, key))
      return kf.get_string(group, key);
  } catch (const Glib::Error&) {
  }
  return {};
}

}  // namespace

void Settings::load()
{
  Glib::KeyFile kf;
  try {
    kf.load_from_file(config_path());
  } catch (const Glib::Error&) {
    return;
  }
  window_x = get_int(kf, "window", "x", window_x);
  window_y = get_int(kf, "window", "y", window_y);
  window_w = get_int(kf, "window", "width", window_w);
  window_h = get_int(kf, "window", "height", window_h);
  last_path = get_str(kf, "session", "path");
  const std::string sec = get_str(kf, "session", "section");
  if (!sec.empty())
    last_section = sec;
  const std::string cal = get_str(kf, "session", "cal");
  if (!cal.empty())
    last_cal = cal;
  last_date = get_str(kf, "session", "date");
}

void Settings::save() const
{
  g_mkdir_with_parents(config_dir().c_str(), 0700);
  Glib::KeyFile kf;
  kf.set_integer("window", "x", window_x);
  kf.set_integer("window", "y", window_y);
  kf.set_integer("window", "width", window_w);
  kf.set_integer("window", "height", window_h);
  kf.set_string("session", "path", last_path);
  kf.set_string("session", "section", last_section);
  kf.set_string("session", "cal", last_cal);
  kf.set_string("session", "date", last_date);
  try {
    kf.save_to_file(config_path());
  } catch (const Glib::Error&) {
  }
}

}  // namespace ephemeris
