/* SPDX-License-Identifier: Unlicense */

#include "contacts_page.hpp"

#include <algorithm>

namespace ephemeris {
namespace {

enum class ContactDlg { cancel, ok, del };

const char* kTimezones[] = {
    "",
    "UTC",
    "America/New_York",
    "America/Chicago",
    "America/Denver",
    "America/Los_Angeles",
    "America/Anchorage",
    "Pacific/Honolulu",
    "America/Toronto",
    "America/Vancouver",
    "America/Mexico_City",
    "America/Sao_Paulo",
    "Europe/London",
    "Europe/Dublin",
    "Europe/Paris",
    "Europe/Berlin",
    "Europe/Rome",
    "Europe/Madrid",
    "Europe/Amsterdam",
    "Europe/Athens",
    "Europe/Moscow",
    "Africa/Cairo",
    "Africa/Johannesburg",
    "Asia/Dubai",
    "Asia/Kolkata",
    "Asia/Bangkok",
    "Asia/Singapore",
    "Asia/Shanghai",
    "Asia/Tokyo",
    "Asia/Seoul",
    "Australia/Perth",
    "Australia/Sydney",
    "Pacific/Auckland",
};

Gtk::Entry* labeled_entry(Gtk::Box& box, const char* label, const Glib::ustring& value,
                          const char* placeholder)
{
  box.pack_start(*Gtk::manage(new Gtk::Label(label, Gtk::ALIGN_START)), Gtk::PACK_SHRINK);
  auto* e = Gtk::manage(new Gtk::Entry());
  e->set_text(value);
  if (placeholder)
    e->set_placeholder_text(placeholder);
  box.pack_start(*e, Gtk::PACK_SHRINK);
  return e;
}

ContactDlg run_contact_dialog(Gtk::Window& parent, Contact& c, bool existing)
{
  Gtk::Dialog dlg(existing ? "Edit contact" : "New contact", parent, true);
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  if (existing)
    dlg.add_button("_Delete", Gtk::RESPONSE_REJECT);
  dlg.add_button("_OK", Gtk::RESPONSE_ACCEPT);
  dlg.set_default_response(Gtk::RESPONSE_ACCEPT);
  dlg.set_default_size(420, 0);

  auto* box = dlg.get_content_area();
  box->set_border_width(10);
  box->set_spacing(6);

  auto* first = labeled_entry(*box, "First", c.first, "First name");
  first->set_activates_default(true);
  auto* last = labeled_entry(*box, "Last", c.last, "Last name");
  last->set_activates_default(true);
  auto* phone = labeled_entry(*box, "Phone", c.phone, nullptr);
  auto* email = labeled_entry(*box, "Email", c.email, nullptr);

  box->pack_start(*Gtk::manage(new Gtk::Label("Timezone", Gtk::ALIGN_START)), Gtk::PACK_SHRINK);
  auto* tz = Gtk::manage(new Gtk::ComboBoxText());
  tz->append("none", "(none)");
  bool have_tz = c.timezone.empty();
  for (const char* z : kTimezones) {
    if (!z[0])
      continue;
    tz->append(z, z);
    if (c.timezone == z)
      have_tz = true;
  }
  if (!have_tz && !c.timezone.empty())
    tz->append(c.timezone, c.timezone);
  tz->set_active_id(c.timezone.empty() ? Glib::ustring("none") : c.timezone);

  box->pack_start(*tz, Gtk::PACK_SHRINK);

  box->pack_start(*Gtk::manage(new Gtk::Label("Notes (plain text, 1000 characters)",
                                              Gtk::ALIGN_START)),
                  Gtk::PACK_SHRINK);
  auto* notes = Gtk::manage(new Gtk::TextView());
  notes->set_wrap_mode(Gtk::WRAP_WORD_CHAR);
  notes->get_buffer()->set_text(c.notes);
  auto* nsw = Gtk::manage(new Gtk::ScrolledWindow());
  nsw->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  nsw->set_min_content_height(140);
  nsw->add(*notes);
  box->pack_start(*nsw, Gtk::PACK_EXPAND_WIDGET);
  dlg.show_all();

  const int resp = dlg.run();
  if (resp == Gtk::RESPONSE_REJECT)
    return ContactDlg::del;
  if (resp != Gtk::RESPONSE_ACCEPT)
    return ContactDlg::cancel;
  c.first = first->get_text();
  c.last = last->get_text();
  c.phone = phone->get_text();
  c.email = email->get_text();
  c.timezone = tz->get_active_id();
  if (c.timezone == "none")
    c.timezone.clear();
  c.notes = notes->get_buffer()->get_text();
  if (c.notes.size() > static_cast<Glib::ustring::size_type>(kContactNotesMax))
    c.notes = c.notes.substr(0, kContactNotesMax);
  return ContactDlg::ok;
}

}  // namespace

ContactsPage::ContactsPage() : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 8)
{
  get_style_context()->add_class("ephemeris-page");
  set_margin_start(16);
  set_margin_end(20);
  set_margin_top(12);
  set_margin_bottom(16);

  auto* top = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  auto* head = Gtk::manage(new Gtk::Label());
  head->set_markup("<b>Contacts</b>");
  head->set_halign(Gtk::ALIGN_START);
  head->get_style_context()->add_class("ephemeris-month-head");
  auto* add = Gtk::manage(new Gtk::Button("Add"));
  add->signal_clicked().connect(sigc::mem_fun(*this, &ContactsPage::on_add_contact));
  top->pack_start(*head, Gtk::PACK_EXPAND_WIDGET);
  top->pack_end(*add, Gtk::PACK_SHRINK);
  pack_start(*top, Gtk::PACK_SHRINK);

  az_.set_homogeneous(true);
  az_.get_style_context()->add_class("ephemeris-az");
  pack_start(az_, Gtk::PACK_SHRINK);

  scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  scroll_.add(list_);
  pack_start(scroll_, Gtk::PACK_EXPAND_WIDGET);
}

void ContactsPage::set_binder(Binder* b)
{
  binder_ = b;
  refresh();
}

void ContactsPage::rebuild_az(const std::vector<Contact>& people)
{
  for (auto* ch : az_.get_children())
    az_.remove(*ch);
  bool present[26] = {};
  for (const Contact& c : people) {
    const gunichar ch = c.last_initial();
    if (ch >= 'A' && ch <= 'Z')
      present[ch - 'A'] = true;
  }
  for (int i = 0; i < 26; ++i) {
    const char letter = static_cast<char>('A' + i);
    auto* btn = Gtk::manage(new Gtk::Button(Glib::ustring(1, letter)));
    btn->set_relief(Gtk::RELIEF_NONE);
    btn->set_can_focus(false);
    btn->get_style_context()->add_class("ephemeris-az-letter");
    if (present[i])
      btn->get_style_context()->add_class("ephemeris-az-filled");
    else
      btn->get_style_context()->add_class("ephemeris-az-empty");
    btn->signal_clicked().connect([this, letter]() { jump_to(letter); });
    az_.pack_start(*btn, Gtk::PACK_EXPAND_WIDGET);
  }
  az_.show_all();
}

void ContactsPage::refresh()
{
  for (auto* ch : list_.get_children())
    list_.remove(*ch);
  rows_.clear();
  if (!binder_) {
    rebuild_az({});
    return;
  }
  const auto people = binder_->contacts();
  rebuild_az(people);
  for (const Contact& c : people) {
    Gtk::Widget* row = make_row(c);
    list_.pack_start(*row, Gtk::PACK_SHRINK);
    rows_.push_back({c.last_initial(), row});
  }
  list_.show_all();
}

Gtk::Widget* ContactsPage::make_row(const Contact& c)
{
  auto* row = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 12));
  row->get_style_context()->add_class("ephemeris-contact-line");
  auto* name = Gtk::manage(new Gtk::Label(c.sort_label()));
  name->set_xalign(0.0);
  name->set_ellipsize(Pango::ELLIPSIZE_END);
  auto* phone = Gtk::manage(new Gtk::Label(c.phone));
  phone->set_xalign(0.0);
  phone->set_ellipsize(Pango::ELLIPSIZE_END);
  auto* email = Gtk::manage(new Gtk::Label(c.email));
  email->set_xalign(0.0);
  email->set_ellipsize(Pango::ELLIPSIZE_END);
  row->pack_start(*name, Gtk::PACK_EXPAND_WIDGET);
  row->pack_start(*phone, Gtk::PACK_EXPAND_WIDGET);
  row->pack_start(*email, Gtk::PACK_EXPAND_WIDGET);

  auto* ev = Gtk::manage(new Gtk::EventBox());
  ev->add(*row);
  ev->add_events(Gdk::BUTTON_PRESS_MASK);
  const int id = c.id;
  ev->signal_button_press_event().connect([this, id](GdkEventButton* e) {
    if (!e || e->button != 1)
      return false;
    edit_item(id);
    return true;
  });
  return ev;
}

void ContactsPage::jump_to(gunichar letter)
{
  Gtk::Widget* hit = nullptr;
  Gtk::Widget* next = nullptr;
  for (const auto& row : rows_) {
    if (row.first == letter) {
      hit = row.second;
      break;
    }
    if (!next && row.first > letter)
      next = row.second;
  }
  Gtk::Widget* target = hit ? hit : next;
  if (!target)
    return;
  Gtk::Allocation a = target->get_allocation();
  auto adj = scroll_.get_vadjustment();
  const double y = static_cast<double>(a.get_y());
  const double page = adj->get_page_size();
  if (y < adj->get_value() || y > adj->get_value() + page - 24.0)
    adj->set_value(std::min(y, adj->get_upper() - page));
}

void ContactsPage::on_add_contact()
{
  if (!binder_)
    return;
  auto* win = dynamic_cast<Gtk::Window*>(get_toplevel());
  if (!win)
    return;
  Contact c;
  if (run_contact_dialog(*win, c, false) != ContactDlg::ok)
    return;
  if (c.first.empty() && c.last.empty())
    return;
  binder_->add_contact(c);
  signal_changed_.emit();
  refresh();
}

void ContactsPage::edit_item(int id)
{
  if (!binder_)
    return;
  const Contact* cur = binder_->find_contact(id);
  if (!cur)
    return;
  auto* win = dynamic_cast<Gtk::Window*>(get_toplevel());
  if (!win)
    return;
  Contact c = *cur;
  const ContactDlg r = run_contact_dialog(*win, c, true);
  if (r == ContactDlg::del) {
    binder_->remove_contact(id);
    signal_changed_.emit();
    refresh();
    return;
  }
  if (r != ContactDlg::ok)
    return;
  if (c.first.empty() && c.last.empty())
    return;
  binder_->update_contact(c);
  signal_changed_.emit();
  refresh();
}

}  // namespace ephemeris
