/* SPDX-License-Identifier: Unlicense */

#include "check.hpp"
#include "remote_cal.hpp"

#include <glibmm/fileutils.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

void remove_tree(const std::string& dir)
{
  DIR* d = opendir(dir.c_str());
  if (!d)
    return;
  while (dirent* ent = readdir(d)) {
    if (std::strcmp(ent->d_name, ".") == 0 || std::strcmp(ent->d_name, "..") == 0)
      continue;
    const std::string path = dir + "/" + ent->d_name;
    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
      remove_tree(path);
    else
      ::unlink(path.c_str());
  }
  closedir(d);
  ::rmdir(dir.c_str());
}

class TempHome {
 public:
  TempHome()
  {
    char tmpl[] = "/tmp/ephemeris-cache-XXXXXX";
    if (char* made = mkdtemp(tmpl))
      dir_ = made;
  }

  ~TempHome()
  {
    if (!dir_.empty())
      remove_tree(dir_);
  }

  bool ok() const
  {
    return !dir_.empty();
  }
  const std::string& dir() const
  {
    return dir_;
  }

 private:
  std::string dir_;
};

std::string ymd(const Glib::Date& d)
{
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%04u%02u%02u", static_cast<unsigned>(d.get_year()),
                static_cast<unsigned>(d.get_month()), static_cast<unsigned>(d.get_day()));
  return buf;
}

std::string calendar(const char* name, const char* summary, const std::string& stamp)
{
  return std::string("BEGIN:VCALENDAR\nX-WR-CALNAME:") + name +
         "\nBEGIN:VEVENT\nDTSTART;VALUE=DATE:" + stamp + "\nSUMMARY:" + summary +
         "\nEND:VEVENT\nEND:VCALENDAR\n";
}

std::string read_file(const std::string& path)
{
  std::ifstream in(path.c_str());
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

bool has_text(const std::vector<ephemeris::Appointment>& rows, const char* text)
{
  for (const auto& row : rows) {
    if (row.text == text)
      return true;
  }
  return false;
}

}  // namespace

int main()
{
  TempHome home;
  CHECK(home.ok());
  if (!home.ok())
    return suite_test::done("cache");
  setenv("XDG_DATA_HOME", home.dir().c_str(), 1);

  Glib::Date today;
  today.set_time_current();
  const std::string stamp = ymd(today);
  const char* work = "https://cal.example/work.ics";
  const char* side = "https://cal.example/side.ics";
  const std::string work_path = ephemeris::calendar_cache_path(work);
  CHECK(work_path.compare(0, home.dir().size(), home.dir()) == 0);
  if (work_path.compare(0, home.dir().size(), home.dir()) != 0)
    return suite_test::done("cache");

  ephemeris::RemoteCalendars remotes;
  remotes.set_subs({{work, ""}, {side, ""}});

  CHECK(remotes.apply_ics(work, calendar("Work", "Standup", stamp)));
  CHECK(remotes.subs()[0].title == "Work");
  CHECK(read_file(work_path).find("Standup") != std::string::npos);
  CHECK(has_text(remotes.for_date(today), "Standup"));

  CHECK(remotes.apply_ics(side, calendar("Side", "Side", stamp)));
  CHECK(has_text(remotes.for_date(today), "Side"));

  const char* html =
      "<!DOCTYPE html>\n<html><head><title>502 Bad Gateway</title></head>"
      "<body>Bad Gateway</body></html>\n";
  CHECK(!remotes.apply_ics(work, html));
  {
    const std::string body = read_file(work_path);
    CHECK(body.find("Standup") != std::string::npos);
    CHECK(body.find("<html>") == std::string::npos);
  }
  CHECK(!Glib::file_test(work_path + ".tmp", Glib::FILE_TEST_EXISTS));
  CHECK(remotes.subs()[0].title == "Work");
  CHECK(has_text(remotes.for_date(today), "Standup"));
  CHECK(has_text(remotes.for_date(today), "Side"));

  CHECK(!remotes.apply_ics(work, ""));
  CHECK(read_file(work_path).find("Standup") != std::string::npos);

  const std::string fragment =
      "BEGIN:VEVENT\nDTSTART;VALUE=DATE:" + stamp + "\nSUMMARY:Fragment\nEND:VEVENT\n";
  CHECK(!remotes.apply_ics(work, fragment));
  CHECK(read_file(work_path).find("Standup") != std::string::npos);
  CHECK(read_file(work_path).find("Fragment") == std::string::npos);
  CHECK(!has_text(remotes.for_date(today), "Fragment"));

  const std::string lower = std::string(
                                "begin:vcalendar\nx-wr-calname:Other\nbegin:vevent\n"
                                "dtstart;value=date:") +
                            stamp + "\nsummary:Later\nend:vevent\nend:vcalendar\n";
  CHECK(remotes.apply_ics(work, lower));
  CHECK(remotes.subs()[0].title == "Other");
  CHECK(read_file(work_path).find("Later") != std::string::npos);
  CHECK(read_file(work_path).find("Standup") == std::string::npos);
  CHECK(has_text(remotes.for_date(today), "Later"));
  CHECK(!has_text(remotes.for_date(today), "Standup"));
  CHECK(has_text(remotes.for_date(today), "Side"));

  CHECK(remotes.apply_ics(work, calendar("Brief", "Brief", stamp)));
  CHECK(has_text(remotes.for_date(today), "Brief"));
  CHECK(!has_text(remotes.for_date(today), "Later"));
  CHECK(has_text(remotes.for_date(today), "Side"));

  {
    ephemeris::RemoteCalendars again;
    again.set_subs({{work, ""}, {side, ""}});
    again.load_caches();
    CHECK(again.subs()[0].title == "Brief");
    CHECK(has_text(again.for_date(today), "Brief"));
    CHECK(has_text(again.for_date(today), "Side"));
  }

  CHECK(remotes.apply_ics(work, "BEGIN:VCALENDAR\nEND:VCALENDAR\n"));
  CHECK(!has_text(remotes.for_date(today), "Brief"));
  CHECK(has_text(remotes.for_date(today), "Side"));
  CHECK(read_file(work_path).find("BEGIN:VCALENDAR") != std::string::npos);
  CHECK(read_file(work_path).find("Brief") == std::string::npos);

  CHECK(remotes.apply_ics(work, calendar("Old", "Old", "19900115")));
  CHECK(read_file(work_path).find("Old") != std::string::npos);
  CHECK(!has_text(remotes.for_date(today), "Old"));
  CHECK(has_text(remotes.for_date(today), "Side"));
  {
    const std::string side_body = read_file(ephemeris::calendar_cache_path(side));
    CHECK(side_body.find("Side") != std::string::npos);
    CHECK(side_body.find("Old") == std::string::npos);
  }

  return suite_test::done("cache");
}
