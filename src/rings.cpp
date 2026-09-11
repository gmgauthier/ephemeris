/* SPDX-License-Identifier: Unlicense */

#include "rings.hpp"

namespace ephemeris {

Rings::Rings()
{
  set_size_request(28, -1);
  set_hexpand(false);
  set_vexpand(true);
}

bool Rings::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  const int w = get_allocated_width();
  const int h = get_allocated_height();
  /* Navy spine (#0B1D38), eggshell paper meeting the page on the right. */
  cr->set_source_rgb(0.043, 0.114, 0.220);
  cr->rectangle(0, 0, w, h);
  cr->fill();
  cr->set_source_rgb(0.969, 0.961, 0.937);
  cr->rectangle(w - 6.0, 0, 6.0, h);
  cr->fill();

  const double cx = (w - 6.0) / 2.0;
  const int n = 4;
  for (int i = 0; i < n; ++i) {
    const double cy = h * (i + 1) / (n + 1.0);
    cr->set_line_width(3.0);
    cr->set_source_rgb(0.910, 0.949, 1.0);
    cr->arc(cx, cy, 8, 0, 2 * G_PI);
    cr->stroke();
    cr->set_line_width(1.4);
    cr->set_source_rgb(0.55, 0.68, 0.85);
    cr->arc(cx, cy, 8, 0, 2 * G_PI);
    cr->stroke();
  }
  return true;
}

}  // namespace ephemeris
