/* SPDX-License-Identifier: Unlicense */

#include "main_window.hpp"
#include "about_dialog.hpp"
#include "config.hpp"
#include "paths.hpp"

#include <iostream>

#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

namespace ephemeris {

MainWindow::MainWindow()
{
  settings_.load();
  set_title("Ephemeris");
  set_default_size(settings_.window_w > 0 ? settings_.window_w : 720,
                   settings_.window_h > 0 ? settings_.window_h : 520);
  get_style_context()->add_class("ephemeris-window");
  accel_ = Gtk::AccelGroup::create();
  add_accel_group(accel_);
  load_css();
  build_menu();
  build_toolbar();

  binder_.create_new();
  spread_.set_binder(&binder_);
  todo_.set_binder(&binder_);
  spread_.signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_binder_changed));
  todo_.signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_binder_changed));
  spread_.signal_goto_todo().connect(
      sigc::bind(sigc::mem_fun(*this, &MainWindow::show_section), Section::todo));

  pages_.add(month_, "month");
  pages_.add(spread_, "spread");
  pages_.add(todo_, "todo");
  pages_.set_visible_child("month");
  month_.set_hexpand(true);
  month_.set_vexpand(true);
  spread_.set_hexpand(true);
  spread_.set_vexpand(true);
  todo_.set_hexpand(true);
  todo_.set_vexpand(true);

  tabs_.signal_section().connect(sigc::mem_fun(*this, &MainWindow::show_section));
  month_.signal_day_chosen().connect(sigc::mem_fun(*this, &MainWindow::on_day));

  sheet_.set_visible_window(true);
  sheet_.get_style_context()->add_class("ephemeris-page");
  sheet_.add(pages_);
  sheet_.set_hexpand(true);
  sheet_.set_vexpand(true);
  tabs_.set_valign(Gtk::ALIGN_START);
  tabs_.set_hexpand(false);

  book_.set_spacing(0);
  book_.pack_start(rings_, Gtk::PACK_SHRINK);
  book_.pack_start(sheet_, Gtk::PACK_EXPAND_WIDGET);
  book_.pack_start(tabs_, Gtk::PACK_SHRINK);

  root_.pack_start(menubar_, Gtk::PACK_SHRINK);
  root_.pack_start(toolbar_, Gtk::PACK_SHRINK);
  root_.pack_start(book_, Gtk::PACK_EXPAND_WIDGET);
  status_ctx_ = status_.get_context_id("main");
  root_.pack_start(status_, Gtk::PACK_SHRINK);
  add(root_);
  if (settings_.window_w > 0 && settings_.window_h > 0)
    resize(settings_.window_w, settings_.window_h);
  if (settings_.window_x >= 0 && settings_.window_y >= 0)
    move(settings_.window_x, settings_.window_y);
  signal_hide().connect(sigc::mem_fun(*this, &MainWindow::persist));
  update_title();
  set_status("Calendar — " + month_.title());
  show_all();
  restore_session();
}

void MainWindow::load_css()
{
  const std::string css_path = find_data_file("skin/lcos/lcos.css");
  if (css_path.empty()) {
    std::cerr << "ephemeris: lcos.css not found\n";
    return;
  }
  try {
    auto css = Gtk::CssProvider::create();
    css->load_from_path(css_path);
    Gtk::StyleContext::add_provider_for_screen(
        Gdk::Screen::get_default(), css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  } catch (const Glib::Error& e) {
    std::cerr << "ephemeris: CSS: " << e.what() << "\n";
  }
}

Gtk::MenuItem* MainWindow::add_item(Gtk::Menu& menu, const Glib::ustring& label,
                                    const sigc::slot<void()>& slot, guint key,
                                    Gdk::ModifierType mods)
{
  auto* item = Gtk::manage(new Gtk::MenuItem(label, true));
  item->signal_activate().connect(slot);
  if (key != 0)
    item->add_accelerator("activate", accel_, key, mods, Gtk::ACCEL_VISIBLE);
  menu.append(*item);
  return item;
}

void MainWindow::build_menu()
{
  auto add_menu = [this](const Glib::ustring& label, Gtk::Menu& menu) {
    auto* top = Gtk::manage(new Gtk::MenuItem(label, true));
    top->set_submenu(menu);
    menubar_.append(*top);
  };

  auto* file = Gtk::manage(new Gtk::Menu());
  add_item(*file, "_New", sigc::mem_fun(*this, &MainWindow::on_new), GDK_KEY_n,
           Gdk::CONTROL_MASK);
  add_item(*file, "_Open…", sigc::mem_fun(*this, &MainWindow::on_open), GDK_KEY_o,
           Gdk::CONTROL_MASK);
  add_item(*file, "_Save", sigc::mem_fun(*this, &MainWindow::on_save), GDK_KEY_s,
           Gdk::CONTROL_MASK);
  add_item(*file, "Save _As…", sigc::mem_fun(*this, &MainWindow::on_save_as), GDK_KEY_s,
           Gdk::CONTROL_MASK | Gdk::SHIFT_MASK);
  file->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
  add_item(*file, "_Print…",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet), Glib::ustring("Print")),
           GDK_KEY_p, Gdk::CONTROL_MASK);
  file->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
  add_item(*file, "E_xit", sigc::mem_fun(*this, &MainWindow::on_quit));
  add_menu("_File", *file);

  auto* edit = Gtk::manage(new Gtk::Menu());
  add_item(*edit, "_Delete",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet), Glib::ustring("Delete")));
  add_menu("_Edit", *edit);

  auto* section = Gtk::manage(new Gtk::Menu());
  cal_item_ = Gtk::manage(new Gtk::RadioMenuItem(section_group_, "_Calendar", true));
  todo_item_ = Gtk::manage(new Gtk::RadioMenuItem(section_group_, "_To Do", true));
  cal_item_->set_active(true);
  cal_item_->signal_activate().connect([this]() {
    if (!suppress_section_ && cal_item_->get_active())
      show_section(Section::calendar);
  });
  todo_item_->signal_activate().connect([this]() {
    if (!suppress_section_ && todo_item_->get_active())
      show_section(Section::todo);
  });
  section->append(*cal_item_);
  section->append(*todo_item_);
  add_menu("_Section", *section);

  auto* help = Gtk::manage(new Gtk::Menu());
  add_item(*help, "_About Ephemeris", sigc::mem_fun(*this, &MainWindow::on_about));
  add_menu("_Help", *help);
}

void MainWindow::build_toolbar()
{
  toolbar_.set_border_width(4);
  auto add_btn = [this](const char* label, const sigc::slot<void()>& slot) {
    auto* b = Gtk::manage(new Gtk::Button(label));
    b->signal_clicked().connect(slot);
    toolbar_.pack_start(*b, Gtk::PACK_SHRINK);
  };
  add_btn("Today", sigc::mem_fun(*this, &MainWindow::on_today));
  add_btn("Month", sigc::mem_fun(*this, &MainWindow::on_month_btn));
  add_btn("Prev", sigc::mem_fun(*this, &MainWindow::on_prev));
  add_btn("Next", sigc::mem_fun(*this, &MainWindow::on_next));
}

void MainWindow::set_status(const Glib::ustring& text)
{
  status_.pop(status_ctx_);
  status_.push(text, status_ctx_);
}

void MainWindow::update_title()
{
  Glib::ustring t = "Ephemeris - ";
  t += binder_.display_name();
  if (binder_.dirty())
    t += "*";
  set_title(t);
}

void MainWindow::refresh_marks()
{
  month_.set_marks(binder_.days_in_month(month_.month(), month_.year()));
}

void MainWindow::show_month()
{
  cal_view_ = CalView::month;
  show_section(Section::calendar);
}

void MainWindow::show_spread(const Glib::Date& left)
{
  spread_.set_left_date(left);
  cal_view_ = CalView::spread;
  show_section(Section::calendar);
}

void MainWindow::show_section(Section s)
{
  suppress_section_ = true;
  if (s == Section::calendar) {
    if (cal_view_ == CalView::spread) {
      pages_.set_visible_child("spread");
      char buf[64];
      g_date_strftime(buf, sizeof(buf), "%A %d %B",
                      const_cast<GDate*>(spread_.left_date().gobj()));
      set_status(Glib::ustring("Calendar — ") + buf);
    } else {
      pages_.set_visible_child("month");
      set_status("Calendar — " + month_.title());
    }
    tabs_.set_section(Section::calendar);
    if (cal_item_)
      cal_item_->set_active(true);
  } else {
    pages_.set_visible_child("todo");
    tabs_.set_section(Section::todo);
    if (todo_item_)
      todo_item_->set_active(true);
    todo_.refresh();
    set_status("To Do");
  }
  suppress_section_ = false;
}

void MainWindow::on_today()
{
  Glib::Date now;
  now.set_time_current();
  month_.today();
  refresh_marks();
  show_spread(now);
}

void MainWindow::on_month_btn()
{
  refresh_marks();
  show_month();
}

void MainWindow::on_prev()
{
  if (cal_view_ == CalView::spread && pages_.get_visible_child_name() == "spread") {
    spread_.prev_spread();
    show_section(Section::calendar);
    return;
  }
  month_.prev_month();
  refresh_marks();
  show_month();
}

void MainWindow::on_next()
{
  if (cal_view_ == CalView::spread && pages_.get_visible_child_name() == "spread") {
    spread_.next_spread();
    show_section(Section::calendar);
    return;
  }
  month_.next_month();
  refresh_marks();
  show_month();
}

void MainWindow::on_day(const Glib::Date& date)
{
  show_spread(date);
}

void MainWindow::on_binder_changed()
{
  update_title();
  refresh_marks();
  todo_.refresh();
  if (cal_view_ == CalView::spread)
    spread_.refresh();
}

void MainWindow::show_error(const Glib::ustring& message)
{
  Gtk::MessageDialog dlg(*this, message, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
  dlg.set_title("Ephemeris");
  dlg.run();
}

bool MainWindow::confirm_discard()
{
  if (!binder_.is_open() || !binder_.dirty())
    return true;
  Gtk::MessageDialog dlg(*this, "Save changes to " + binder_.display_name() + "?", false,
                         Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
  dlg.set_title("Ephemeris");
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dlg.add_button("_Discard", Gtk::RESPONSE_REJECT);
  dlg.add_button("_Save", Gtk::RESPONSE_ACCEPT);
  dlg.set_default_response(Gtk::RESPONSE_ACCEPT);
  const int resp = dlg.run();
  if (resp == Gtk::RESPONSE_ACCEPT)
    return do_save();
  return resp == Gtk::RESPONSE_REJECT;
}

std::string MainWindow::ensure_suffix(const std::string& path) const
{
  const std::string suf = ".ephemeris";
  if (path.size() >= suf.size() &&
      path.compare(path.size() - suf.size(), suf.size(), suf) == 0)
    return path;
  return path + suf;
}

std::string MainWindow::samples_dir() const
{
  const std::string p = std::string(SOURCE_ROOT) + "/data/samples";
  if (Glib::file_test(p, Glib::FILE_TEST_IS_DIR))
    return p;
  return Glib::get_home_dir();
}

bool MainWindow::do_save()
{
  if (!binder_.is_open())
    return true;
  if (binder_.path().empty())
    return do_save_as();
  if (!binder_.save()) {
    show_error(binder_.error().empty() ? "Could not save." : binder_.error());
    return false;
  }
  update_title();
  return true;
}

bool MainWindow::do_save_as()
{
  if (!binder_.is_open())
    return false;
  Gtk::FileChooserDialog dlg(*this, "Save Binder", Gtk::FILE_CHOOSER_ACTION_SAVE);
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dlg.add_button("_Save", Gtk::RESPONSE_ACCEPT);
  dlg.set_do_overwrite_confirmation(true);
  auto filter = Gtk::FileFilter::create();
  filter->set_name("Ephemeris binder");
  filter->add_pattern("*.ephemeris");
  dlg.add_filter(filter);
  if (!binder_.path().empty())
    dlg.set_filename(binder_.path());
  else {
    dlg.set_current_folder(Glib::get_home_dir());
    dlg.set_current_name("Untitled.ephemeris");
  }
  if (dlg.run() != Gtk::RESPONSE_ACCEPT)
    return false;
  const std::string path = ensure_suffix(dlg.get_filename());
  dlg.hide();
  if (!binder_.save_as(path)) {
    show_error(binder_.error().empty() ? "Could not save." : binder_.error());
    return false;
  }
  update_title();
  return true;
}

void MainWindow::on_new()
{
  if (!confirm_discard())
    return;
  binder_.create_new();
  spread_.set_binder(&binder_);
  todo_.set_binder(&binder_);
  month_.today();
  refresh_marks();
  show_month();
  update_title();
}

void MainWindow::on_open()
{
  if (!confirm_discard())
    return;
  Gtk::FileChooserDialog dlg(*this, "Open Binder", Gtk::FILE_CHOOSER_ACTION_OPEN);
  dlg.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dlg.add_button("_Open", Gtk::RESPONSE_ACCEPT);
  auto filter = Gtk::FileFilter::create();
  filter->set_name("Ephemeris binder");
  filter->add_pattern("*.ephemeris");
  dlg.add_filter(filter);
  dlg.set_current_folder(samples_dir());
  if (dlg.run() != Gtk::RESPONSE_ACCEPT)
    return;
  const std::string path = dlg.get_filename();
  dlg.hide();
  if (!binder_.open(path)) {
    show_error(binder_.error().empty() ? "Could not open binder." : binder_.error());
    return;
  }
  spread_.set_binder(&binder_);
  todo_.set_binder(&binder_);
  month_.today();
  refresh_marks();
  show_month();
  update_title();
}

void MainWindow::on_save()
{
  do_save();
}

void MainWindow::on_save_as()
{
  do_save_as();
}

void MainWindow::on_quit()
{
  if (!confirm_discard())
    return;
  hide();
}

bool MainWindow::on_delete_event(GdkEventAny* event)
{
  if (!confirm_discard())
    return true;
  return Gtk::Window::on_delete_event(event);
}

void MainWindow::on_about()
{
  AboutDialog dlg(*this);
  dlg.run();
}

void MainWindow::persist()
{
  int x = 0, y = 0, w = 0, h = 0;
  get_position(x, y);
  get_size(w, h);
  settings_.window_x = x;
  settings_.window_y = y;
  settings_.window_w = w;
  settings_.window_h = h;
  if (binder_.is_open() && !binder_.path().empty())
    settings_.last_path = binder_.path();
  else
    settings_.last_path.clear();
  settings_.last_section =
      (pages_.get_visible_child_name() == "todo") ? "todo" : "calendar";
  settings_.last_cal = (cal_view_ == CalView::spread) ? "spread" : "month";
  settings_.last_date = date_iso(spread_.left_date());
  settings_.save();
}

void MainWindow::restore_session()
{
  if (settings_.last_path.empty() ||
      !Glib::file_test(settings_.last_path, Glib::FILE_TEST_IS_REGULAR))
    return;
  if (!binder_.open(settings_.last_path)) {
    set_status("Could not restore last binder.");
    return;
  }
  spread_.set_binder(&binder_);
  todo_.set_binder(&binder_);
  Glib::Date d;
  if (!settings_.last_date.empty() && date_from_iso(settings_.last_date, d)) {
    month_.set_month(d.get_month(), d.get_year());
    spread_.set_left_date(d);
  } else {
    month_.today();
  }
  refresh_marks();
  update_title();
  if (settings_.last_cal == "spread")
    cal_view_ = CalView::spread;
  else
    cal_view_ = CalView::month;
  if (settings_.last_section == "todo")
    show_section(Section::todo);
  else
    show_section(Section::calendar);
}

void MainWindow::on_not_yet(const Glib::ustring& feature)
{
  set_status(feature + " — coming in a later milestone.");
}

}  // namespace ephemeris
