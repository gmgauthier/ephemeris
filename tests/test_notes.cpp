/* SPDX-License-Identifier: Unlicense */

#include "binder.hpp"
#include "check.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

Glib::Date day(int d, Glib::Date::Month m, int y)
{
  return Glib::Date(static_cast<Glib::Date::Day>(d), m, static_cast<Glib::Date::Year>(y));
}

bool same_day(const Glib::Date& a, const Glib::Date& b)
{
  return a.valid() && b.valid() && a.get_julian() == b.get_julian();
}

class TempBook {
 public:
  TempBook()
  {
    char tmpl[] = "/tmp/ephemeris-notes-XXXXXX";
    if (char* made = mkdtemp(tmpl))
      dir_ = made;
    if (!dir_.empty())
      path_ = dir_ + "/book.ephemeris";
  }

  ~TempBook()
  {
    if (!path_.empty())
      std::remove(path_.c_str());
    if (!dir_.empty())
      rmdir(dir_.c_str());
  }

  const std::string& path() const
  {
    return path_;
  }

 private:
  std::string dir_;
  std::string path_;
};

std::string slurp(const std::string& path)
{
  std::ifstream in(path);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

bool write_raw(const std::string& path, const std::string& xml)
{
  std::ofstream out(path);
  if (!out)
    return false;
  out << xml;
  return out.good();
}

}  // namespace

int main()
{
  const Glib::Date oct1 = day(1, Glib::Date::OCTOBER, 2026);
  const Glib::Date oct3 = day(3, Glib::Date::OCTOBER, 2026);
  const Glib::Date oct8 = day(8, Glib::Date::OCTOBER, 2026);

  CHECK(ephemeris::format_written_date(oct8) == "8 October 2026");
  CHECK(ephemeris::format_written_date(oct1) == "1 October 2026");
  CHECK(ephemeris::format_written_stamp("2026-10-08 14:02") == "8 October 2026, 14:02");

  Glib::Date parsed = oct1;
  CHECK(ephemeris::parse_journal_text("8 October 2026", parsed));
  CHECK(same_day(parsed, oct8));
  CHECK(ephemeris::parse_journal_text("2026-10-03", parsed));
  CHECK(same_day(parsed, oct3));
  CHECK(ephemeris::parse_journal_text("8 October 2026, 14:02", parsed));
  CHECK(same_day(parsed, oct8));
  CHECK(ephemeris::parse_journal_text("2026-10-08 09:15", parsed));
  CHECK(same_day(parsed, oct8));
  parsed = oct1;
  CHECK(!ephemeris::parse_journal_text("31 February 2026", parsed));
  CHECK(same_day(parsed, oct1));
  CHECK(!ephemeris::parse_journal_text("tomorrow", parsed));
  CHECK(same_day(parsed, oct1));

  {
    ephemeris::Binder book;
    book.create_new();
    const auto keys = book.planner_keys();
    CHECK(keys[3] == "Projects");
    CHECK(keys[0] == "Holiday");
  }

  {
    TempBook tmp;
    CHECK(!tmp.path().empty());
    const char* xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ephemeris version=\"1\">\n"
        "  <planner-key index=\"0\">Streaming</planner-key>\n"
        "  <planner-key index=\"3\">Streaming</planner-key>\n"
        "  <note id=\"1\" title=\"Old\" stamped=\"2026-10-08 14:02\" colour=\"yellow\" "
        "category=\"Work\">hello &amp; co</note>\n"
        "</ephemeris>\n";
    CHECK(write_raw(tmp.path(), xml));
    ephemeris::Binder book;
    CHECK(book.open(tmp.path()));
    CHECK(!book.dirty());
    const auto keys = book.planner_keys();
    CHECK(keys[0] == "Streaming");
    CHECK(keys[3] == "Projects");
    const ephemeris::Note* note = book.find_note(1);
    CHECK(note != nullptr);
    if (note) {
      CHECK(note->body == "hello & co");
      CHECK(note->journal.empty());
      CHECK(note->paras.size() == 1);
      CHECK(!note->paras[0].bullet);
      CHECK(note->paras[0].spans.size() == 1);
      CHECK(!note->paras[0].spans[0].bold);
      Glib::Date got;
      CHECK(ephemeris::note_journal(*note, got));
      CHECK(same_day(got, oct8));
      CHECK(note->category == "Work");
    }
    const auto cats = book.note_categories();
    CHECK(cats.size() == 1);
    CHECK(cats[0] == "Work");

    CHECK(book.save());
    CHECK(!book.dirty());
    const std::string saved = slurp(tmp.path());
    CHECK(saved.find("index=\"3\">Projects<") != std::string::npos);
    CHECK(saved.find("index=\"0\">Streaming<") != std::string::npos);
    CHECK(saved.find("<p>") == std::string::npos);
    CHECK(saved.find("journal=") == std::string::npos);
    CHECK(saved.find("hello &amp; co") != std::string::npos);

    ephemeris::Note edited = *book.find_note(1);
    CHECK(book.update_note(edited));
    CHECK(book.find_note(1)->journal == "2026-10-08");
  }

  {
    TempBook tmp;
    CHECK(write_raw(tmp.path(),
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<ephemeris version=\"1\">\n"
                    "  <planner-key index=\"3\">Films</planner-key>\n"
                    "</ephemeris>\n"));
    ephemeris::Binder book;
    CHECK(book.open(tmp.path()));
    CHECK(!book.dirty());
    CHECK(book.planner_keys()[3] == "Films");
  }

  {
    TempBook tmp;
    ephemeris::Binder book;
    ephemeris::Note plain;
    plain.title = "Plain";
    plain.stamped = "2026-10-01 09:00";
    plain.body = "one\ntwo < three";
    plain.category = "Work";
    const int id = book.add_note(plain);
    CHECK(id > 0);
    const ephemeris::Note* stored = book.find_note(id);
    CHECK(stored != nullptr);
    if (stored) {
      CHECK(stored->journal == "2026-10-01");
      CHECK(stored->body == "one\ntwo < three");
      CHECK(stored->paras.size() == 2);
    }
    CHECK(book.save_as(tmp.path()));
    const std::string saved = slurp(tmp.path());
    CHECK(saved.find("<p>") == std::string::npos);
    CHECK(saved.find("<b>") == std::string::npos);
    CHECK(saved.find("two &lt; three") != std::string::npos);

    ephemeris::Binder loaded;
    CHECK(loaded.open(tmp.path()));
    const ephemeris::Note* again = loaded.find_note(id);
    CHECK(again != nullptr);
    if (again) {
      CHECK(again->body == "one\ntwo < three");
      CHECK(again->journal == "2026-10-01");
      CHECK(again->paras.size() == 2);
      CHECK(again->paras[0].spans.size() == 1);
      CHECK(again->paras[0].spans[0].text == "one");
      CHECK(again->paras[1].spans[0].text == "two < three");
    }
  }

  {
    TempBook tmp;
    ephemeris::Binder book;
    ephemeris::Note rich;
    rich.title = "Rich";
    rich.stamped = "2026-10-08 14:02";
    rich.journal = "2026-10-03";
    rich.colour = ephemeris::NoteColour::blue;
    rich.category = "Work";
    ephemeris::NotePara para;
    para.spans.push_back(ephemeris::NoteSpan{"Hello ", false, false});
    para.spans.push_back(ephemeris::NoteSpan{"there", true, false});
    para.spans.push_back(ephemeris::NoteSpan{" ", false, false});
    para.spans.push_back(ephemeris::NoteSpan{"x", false, true});
    ephemeris::NotePara item;
    item.bullet = true;
    item.spans.push_back(ephemeris::NoteSpan{"item & co", true, true});
    rich.paras = {para, item};
    const int id = book.add_note(rich);
    CHECK(book.save_as(tmp.path()));
    const std::string saved = slurp(tmp.path());
    CHECK(saved.find("<b>there </b>") != std::string::npos);
    CHECK(saved.find("<i>x</i>") != std::string::npos);
    CHECK(saved.find("<li><b><i>item &amp; co</i></b></li>") != std::string::npos);
    CHECK(saved.find("journal=\"2026-10-03\"") != std::string::npos);
    CHECK(saved.find("•") == std::string::npos);

    ephemeris::Binder loaded;
    CHECK(loaded.open(tmp.path()));
    const ephemeris::Note* note = loaded.find_note(id);
    CHECK(note != nullptr);
    if (note) {
      CHECK(note->body == "Hello there x\nitem & co");
      CHECK(note->journal == "2026-10-03");
      CHECK(note->colour == ephemeris::NoteColour::blue);
      CHECK(note->paras.size() == 2);
      CHECK(!note->paras[0].bullet);
      CHECK(note->paras[0].spans.size() == 3);
      CHECK(note->paras[0].spans[1].text == "there ");
      CHECK(note->paras[0].spans[1].bold);
      CHECK(!note->paras[0].spans[1].italic);
      CHECK(note->paras[0].spans[2].text == "x");
      CHECK(note->paras[0].spans[2].italic);
      CHECK(note->paras[1].bullet);
      CHECK(note->paras[1].spans.size() == 1);
      CHECK(note->paras[1].spans[0].text == "item & co");
      CHECK(note->paras[1].spans[0].bold);
      CHECK(note->paras[1].spans[0].italic);
    }
    const auto on_third = loaded.notes_on(oct3);
    CHECK(on_third.size() == 1);
    CHECK(loaded.notes_on(oct8).empty());
    const auto days = loaded.note_days_in_month(Glib::Date::OCTOBER, 2026);
    CHECK(days.size() == 1);
    CHECK(days.count(3) == 1);
  }

  {
    ephemeris::Binder book;
    ephemeris::Note older;
    older.title = "Older";
    older.stamped = "2026-10-03 08:00";
    older.journal = "2026-10-03";
    ephemeris::Note newer;
    newer.title = "Newer";
    newer.stamped = "2026-10-03 18:00";
    newer.journal = "2026-10-03";
    const int old_id = book.add_note(older);
    const int new_id = book.add_note(newer);
    const auto on = book.notes_on(oct3);
    CHECK(on.size() == 2);
    CHECK(on[0].id == new_id);
    CHECK(on[1].id == old_id);
    CHECK(book.note_days_in_month(Glib::Date::OCTOBER, 2026).size() == 1);
  }

  {
    Glib::Date today;
    today.set_time_current();
    Glib::Date back = today;
    back.subtract_days(10);
    ephemeris::Binder book;
    ephemeris::Note n;
    n.title = "Backdated";
    n.stamped = ephemeris::date_iso(today) + " 12:00";
    n.journal = ephemeris::date_iso(back);
    const int id = book.add_note(n);
    Glib::Date got;
    CHECK(ephemeris::note_journal(*book.find_note(id), got));
    CHECK(same_day(got, back));
    CHECK(!ephemeris::in_last_days(got, today, 7));
    CHECK(book.notes_on(today).empty());
    CHECK(book.notes_on(back).size() == 1);
  }

  {
    TempBook tmp;
    ephemeris::Binder book;
    book.create_new();
    book.add_note_category("Later");
    book.add_note_category("Work");
    ephemeris::Note n;
    n.title = "Page";
    n.stamped = "2026-10-08 14:02";
    n.body = "words";
    n.category = "Work";
    const int id = book.add_note(n);
    CHECK(book.note_categories().size() == 2);
    CHECK(book.note_categories()[0] == "Later");
    CHECK(book.note_categories()[1] == "Work");
    CHECK(book.save_as(tmp.path()));

    ephemeris::Binder loaded;
    CHECK(loaded.open(tmp.path()));
    CHECK(loaded.note_categories().size() == 2);
    CHECK(loaded.note_categories()[0] == "Later");
    CHECK(loaded.find_note(id)->category == "Work");
    CHECK(loaded.notes().size() == 1);

    CHECK(loaded.rename_note_category("Work", "Jobs"));
    CHECK(loaded.find_note(id)->category == "Jobs");
    CHECK(loaded.note_categories().size() == 2);
    CHECK(loaded.note_categories()[1] == "Jobs");

    loaded.add_note_category("Extra");
    CHECK(loaded.rename_note_category("Jobs", "Extra"));
    CHECK(loaded.find_note(id)->category == "Extra");
    int extra = 0;
    for (const auto& c : loaded.note_categories()) {
      if (c == "Extra")
        ++extra;
    }
    CHECK(extra == 1);
    CHECK(loaded.remove_note_category("Later"));
    bool saw_later = false;
    for (const auto& c : loaded.note_categories()) {
      if (c == "Later")
        saw_later = true;
    }
    CHECK(!saw_later);
    CHECK(loaded.find_note(id)->category == "Extra");
    CHECK(loaded.remove_note_category("Extra"));
    CHECK(loaded.find_note(id)->category.empty());
    CHECK(loaded.note_categories().empty());

    CHECK(loaded.save());
    ephemeris::Binder again;
    CHECK(again.open(tmp.path()));
    CHECK(again.note_categories().empty());
    CHECK(again.find_note(id)->category.empty());
    CHECK(again.find_note(id)->body == "words");
  }

  {
    ephemeris::Note fresh;
    CHECK(fresh.colour == ephemeris::NoteColour::white);
  }

  {
    TempBook tmp;
    ephemeris::Binder book;
    ephemeris::Note n;
    n.title = "Typed";
    n.stamped = "2026-10-08 18:47";
    n.body = "words";
    const int id = book.add_note(n);
    for (const char* step : {"B", "Ba", "Ban", "Bananas"}) {
      ephemeris::Note edited = *book.find_note(id);
      edited.category = step;
      CHECK(book.update_note(edited));
    }
    CHECK(book.save_as(tmp.path()));
    const std::string saved = slurp(tmp.path());
    CHECK(saved.find("<note-category>B</note-category>") == std::string::npos);
    CHECK(saved.find("<note-category>Ba</note-category>") == std::string::npos);
    CHECK(saved.find("<note-category>Ban</note-category>") == std::string::npos);
    CHECK(saved.find("category=\"Bananas\"") != std::string::npos);

    ephemeris::Binder loaded;
    CHECK(loaded.open(tmp.path()));
    CHECK(loaded.find_note(id)->category == "Bananas");
    CHECK(loaded.note_categories().size() == 1);
    CHECK(loaded.note_categories()[0] == "Bananas");
    CHECK(loaded.add_note_category("Bananas") == false);
    CHECK(loaded.add_note_category("Testing"));
    CHECK(loaded.note_categories().size() == 2);
  }

  CHECK(ephemeris::parse_colour("white") == ephemeris::NoteColour::white);
  CHECK(ephemeris::parse_colour("") == ephemeris::NoteColour::yellow);
  CHECK(ephemeris::parse_colour("cream") == ephemeris::NoteColour::yellow);

  {
    TempBook tmp;
    ephemeris::Binder book;
    ephemeris::Note fresh;
    fresh.title = "New";
    fresh.stamped = "2026-10-08 19:00";
    fresh.body = "hello";
    const int white_id = book.add_note(fresh);
    ephemeris::Note kept;
    kept.title = "Old yellow";
    kept.stamped = "2026-10-01 09:00";
    kept.journal = "2026-10-01";
    kept.body = "stay";
    kept.colour = ephemeris::NoteColour::yellow;
    const int yellow_id = book.add_note(kept);
    CHECK(book.save_as(tmp.path()));
    const std::string saved = slurp(tmp.path());
    CHECK(saved.find("colour=\"white\"") != std::string::npos);
    CHECK(saved.find("colour=\"yellow\"") != std::string::npos);

    ephemeris::Binder loaded;
    CHECK(loaded.open(tmp.path()));
    CHECK(loaded.find_note(white_id)->colour == ephemeris::NoteColour::white);
    CHECK(loaded.find_note(yellow_id)->colour == ephemeris::NoteColour::yellow);
    const auto days = loaded.note_days_in_month(Glib::Date::OCTOBER, 2026);
    CHECK(days.count(8) == 1);
    CHECK(days.count(1) == 1);
  }

  {
    TempBook tmp;
    ephemeris::Binder book;
    ephemeris::Note n;
    n.title = "List";
    n.stamped = "2026-10-08 19:09";
    ephemeris::NotePara item;
    item.bullet = true;
    item.spans.push_back(ephemeris::NoteSpan{"Biscuits", false, false});
    ephemeris::NotePara tail;
    n.paras = {item, tail};
    const int id = book.add_note(n);
    CHECK(id > 0);
    CHECK(book.save_as(tmp.path()));
    const std::string saved = slurp(tmp.path());
    CHECK(saved.find("<li>Biscuits</li><p></p>") != std::string::npos);
    CHECK(saved.find("<li></li>") == std::string::npos);

    CHECK(write_raw(tmp.path(),
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<ephemeris version=\"1\">\n"
                    "  <note id=\"1\" title=\"List\" stamped=\"2026-10-08 19:09\" "
                    "colour=\"white\" category=\"\"><li>Biscuits</li><li></li></note>\n"
                    "</ephemeris>\n"));
    ephemeris::Binder loaded;
    CHECK(loaded.open(tmp.path()));
    const ephemeris::Note* note = loaded.find_note(1);
    CHECK(note != nullptr);
    if (note) {
      CHECK(note->paras.size() == 2);
      CHECK(note->paras[0].bullet);
      CHECK(note->paras[0].spans.size() == 1);
      CHECK(note->paras[1].bullet);
      CHECK(note->paras[1].spans.empty());
    }
  }

  return suite_test::done("notes");
}
