/* SPDX-License-Identifier: Unlicense */

#include "mention.hpp"

namespace ephemeris {
namespace {

struct MentionCols : Gtk::TreeModelColumnRecord {
  MentionCols()
  {
    add(display);
    add(first);
    add(last);
  }
  Gtk::TreeModelColumn<Glib::ustring> display;
  Gtk::TreeModelColumn<Glib::ustring> first;
  Gtk::TreeModelColumn<Glib::ustring> last;
};

MentionCols& cols()
{
  static MentionCols c;
  return c;
}

bool starts_folded(const Glib::ustring& hay, const Glib::ustring& needle)
{
  if (needle.empty())
    return true;
  const Glib::ustring h = hay.casefold();
  const Glib::ustring n = needle.casefold();
  if (n.size() > h.size())
    return false;
  return h.compare(0, n.size(), n) == 0;
}

int last_at_before(const Glib::ustring& text, int pos)
{
  if (pos < 0)
    pos = 0;
  if (static_cast<Glib::ustring::size_type>(pos) > text.size())
    pos = static_cast<int>(text.size());
  for (int i = pos - 1; i >= 0; --i) {
    if (text[i] == '@')
      return i;
  }
  return -1;
}

}  // namespace

void attach_mentions(Gtk::Entry& entry, Binder* binder)
{
  if (!binder)
    return;
  auto store = Gtk::ListStore::create(cols());
  for (const Contact& c : binder->contacts()) {
    const Glib::ustring name = c.display_name();
    if (name.empty() || name == "Unnamed")
      continue;
    auto row = *store->append();
    row[cols().display] = name;
    row[cols().first] = c.first;
    row[cols().last] = c.last;
  }
  auto completion = Gtk::EntryCompletion::create();
  completion->set_model(store);
  completion->set_text_column(cols().display);
  completion->set_inline_completion(false);
  completion->set_popup_completion(true);
  completion->set_popup_single_match(true);
  completion->set_minimum_key_length(0);
  completion->set_match_func([&entry](const Glib::ustring&,
                                      const Gtk::TreeModel::const_iterator& iter) {
    if (!iter)
      return false;
    const Glib::ustring text = entry.get_text();
    const int at = last_at_before(text, entry.get_position());
    if (at < 0)
      return false;
    const int pos = entry.get_position();
    const Glib::ustring frag =
        (pos > at + 1) ? text.substr(at + 1, pos - at - 1) : Glib::ustring();
    const Glib::ustring display = (*iter)[cols().display];
    const Glib::ustring first = (*iter)[cols().first];
    const Glib::ustring last = (*iter)[cols().last];
    if (starts_folded(display, frag) || starts_folded(first, frag) ||
        starts_folded(last, frag))
      return true;
    if (!last.empty() && !first.empty())
      return starts_folded(last + ", " + first, frag);
    return false;
  });
  completion->signal_match_selected().connect(
      [&entry](const Gtk::TreeModel::iterator& iter) -> bool {
        if (!iter)
          return false;
        const Glib::ustring name = (*iter)[cols().display];
        const Glib::ustring text = entry.get_text();
        const int pos = entry.get_position();
        const int at = last_at_before(text, pos);
        if (at < 0)
          return false;
        const Glib::ustring before = text.substr(0, at);
        const Glib::ustring after =
            static_cast<Glib::ustring::size_type>(pos) < text.size() ? text.substr(pos)
                                                                     : Glib::ustring();
        entry.set_text(before + name + after);
        entry.set_position(at + static_cast<int>(name.size()));
        return true;
      });
  entry.set_completion(completion);
}

}  // namespace ephemeris
