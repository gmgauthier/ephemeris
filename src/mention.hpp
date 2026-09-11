/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "binder.hpp"

#include <gtkmm.h>

namespace ephemeris {

void attach_mentions(Gtk::Entry& entry, Binder* binder);

}  // namespace ephemeris
