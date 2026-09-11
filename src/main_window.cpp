/* SPDX-License-Identifier: Unlicense */

#include "main_window.hpp"
#include "about_dialog.hpp"
#include "paths.hpp"

#include <iostream>

namespace ephemeris {

MainWindow::MainWindow()
{
  set_title("Ephemeris");
  set_default_size(720, 520);
  get_style_context()->add_class("ephemeris-window");
  accel_ = Gtk::AccelGroup::create();
  add_accel_group(accel_);
  load_css();
  build_menu();
  build_toolbar();

  pages_.add(month_, "calendar");
  pages_.add(todo_, "todo");
  pages_.set_visible_child("calendar");
  month_.set_hexpand(true);
  month_.set_vexpand(true);
  todo_.set_hexpand(true);
  todo_.set_vexpand(true);

  tabs_.signal_section().connect(sigc::mem_fun(*this, &MainWindow::show_section));
  month_.signal_day_chosen().connect(sigc::mem_fun(*this, &MainWindow::on_day));

  pages_.get_style_context()->add_class("ephemeris-page");
  sheet_.add(pages_);
  sheet_.add_overlay(tabs_);
  tabs_.set_halign(Gtk::ALIGN_END);
  tabs_.set_valign(Gtk::ALIGN_START);
  tabs_.set_hexpand(false);
  sheet_.set_hexpand(true);
  sheet_.set_vexpand(true);

  book_.set_spacing(0);
  book_.pack_start(rings_, Gtk::PACK_SHRINK);
  book_.pack_start(sheet_, Gtk::PACK_EXPAND_WIDGET);

  root_.pack_start(menubar_, Gtk::PACK_SHRINK);
  root_.pack_start(toolbar_, Gtk::PACK_SHRINK);
  root_.pack_start(book_, Gtk::PACK_EXPAND_WIDGET);
  status_ctx_ = status_.get_context_id("main");
  status_.push("Calendar — " + month_.title(), status_ctx_);
  root_.pack_start(status_, Gtk::PACK_SHRINK);
  add(root_);
  show_all();
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
  add_item(*file, "_New",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet), Glib::ustring("New")),
           GDK_KEY_n, Gdk::CONTROL_MASK);
  add_item(*file, "_Open…",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet), Glib::ustring("Open")),
           GDK_KEY_o, Gdk::CONTROL_MASK);
  add_item(*file, "_Save",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet), Glib::ustring("Save")),
           GDK_KEY_s, Gdk::CONTROL_MASK);
  add_item(*file, "Save _As…",
           sigc::bind(sigc::mem_fun(*this, &MainWindow::on_not_yet), Glib::ustring("Save As")));
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
  add_btn("Prev", sigc::mem_fun(*this, &MainWindow::on_prev));
  add_btn("Next", sigc::mem_fun(*this, &MainWindow::on_next));
}

void MainWindow::show_section(Section s)
{
  suppress_section_ = true;
  if (s == Section::calendar) {
    pages_.set_visible_child("calendar");
    tabs_.set_section(Section::calendar);
    if (cal_item_)
      cal_item_->set_active(true);
    status_.pop(status_ctx_);
    status_.push("Calendar — " + month_.title(), status_ctx_);
  } else {
    pages_.set_visible_child("todo");
    tabs_.set_section(Section::todo);
    if (todo_item_)
      todo_item_->set_active(true);
    status_.pop(status_ctx_);
    status_.push("To Do — list comes in M2.", status_ctx_);
  }
  suppress_section_ = false;
}

void MainWindow::on_today()
{
  month_.today();
  show_section(Section::calendar);
}

void MainWindow::on_prev()
{
  month_.prev_month();
  show_section(Section::calendar);
}

void MainWindow::on_next()
{
  month_.next_month();
  show_section(Section::calendar);
}

void MainWindow::on_day(const Glib::Date& date)
{
  char buf[64];
  g_date_strftime(buf, sizeof(buf), "%A %d %B %Y", const_cast<GDate*>(date.gobj()));
  status_.pop(status_ctx_);
  status_.push(Glib::ustring("Day spread for ") + buf + " — comes in M1.", status_ctx_);
}

void MainWindow::on_quit()
{
  hide();
}

void MainWindow::on_about()
{
  AboutDialog dlg(*this);
  dlg.run();
}

void MainWindow::on_not_yet(const Glib::ustring& feature)
{
  status_.pop(status_ctx_);
  status_.push(feature + " — coming in a later milestone.", status_ctx_);
}

}  // namespace ephemeris
