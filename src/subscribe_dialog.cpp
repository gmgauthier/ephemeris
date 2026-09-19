/* SPDX-License-Identifier: Unlicense */

#include "subscribe_dialog.hpp"

#include <gtk/gtk.h>

namespace ephemeris {

SubscribeDialog::SubscribeDialog(Gtk::Window& parent)
    : Gtk::Dialog("Subscribe calendar", parent, true)
{
  set_resizable(false);
  add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  add_button("_Subscribe", Gtk::RESPONSE_OK);
  set_default_response(Gtk::RESPONSE_OK);

  url_.set_placeholder_text("https://example.com/calendar.ics");
  url_.set_activates_default(true);
  url_.set_width_chars(52);

  auto* box = get_content_area();
  box->set_border_width(12);
  box->set_spacing(8);
  auto* l = Gtk::manage(new Gtk::Label("Calendar _URL", true));
  l->set_mnemonic_widget(url_);
  box->pack_start(*l, Gtk::PACK_SHRINK);
  auto* hint =
      Gtk::manage(new Gtk::Label("HTTPS iCalendar (.ics). Read-only overlay. Not CalDAV."));
  hint->set_xalign(0.0);
  hint->set_line_wrap(true);
  box->pack_start(*hint, Gtk::PACK_SHRINK);
  box->pack_start(url_, Gtk::PACK_SHRINK);
  show_all_children();
  gtk_entry_grab_focus_without_selecting(url_.gobj());
}

Glib::ustring SubscribeDialog::url() const
{
  Glib::ustring u = url_.get_text();
  auto b = u.begin();
  auto e = u.end();
  while (b != e && (*b == ' ' || *b == '\t'))
    ++b;
  while (e != b) {
    auto p = e;
    --p;
    if (*p != ' ' && *p != '\t')
      break;
    e = p;
  }
  return Glib::ustring(b, e);
}

}  // namespace ephemeris
