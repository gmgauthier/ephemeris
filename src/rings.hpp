/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <gtkmm.h>

namespace ephemeris {

class Rings : public Gtk::DrawingArea {
 public:
  Rings();

 protected:
  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
};

}  // namespace ephemeris
