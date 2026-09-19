/* SPDX-License-Identifier: Unlicense */

#pragma once

#include "binder.hpp"

#include <string>
#include <vector>

namespace ephemeris {

std::vector<Contact> parse_vcf(const std::string& text);
std::string contacts_to_vcf(const std::vector<Contact>& contacts);

}  // namespace ephemeris
