/* SPDX-License-Identifier: Unlicense */

#pragma once

#include <string>

namespace ephemeris {

/* Blocking GET. Call from a worker, not the UI thread. */
std::string http_get(const std::string& url, std::string& error);

}  // namespace ephemeris
