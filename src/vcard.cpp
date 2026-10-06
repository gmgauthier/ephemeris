/* SPDX-License-Identifier: Unlicense */

#include "vcard.hpp"

#include <sstream>

namespace ephemeris {
namespace {

std::string unfold(const std::string& in)
{
  std::string s;
  s.reserve(in.size());
  for (size_t i = 0; i < in.size(); ++i) {
    if (in[i] == '\r')
      continue;
    if (in[i] == '\n' && i + 1 < in.size() && (in[i + 1] == ' ' || in[i + 1] == '\t')) {
      ++i;
      continue;
    }
    s += in[i];
  }
  return s;
}

std::string unescape(const std::string& in)
{
  std::string out;
  for (size_t i = 0; i < in.size(); ++i) {
    if (in[i] == '\\' && i + 1 < in.size()) {
      const char n = in[i + 1];
      if (n == 'n' || n == 'N')
        out += '\n';
      else
        out += n;
      ++i;
      continue;
    }
    out += in[i];
  }
  return out;
}

std::string escape(const Glib::ustring& in)
{
  std::string out;
  for (const char c : in.raw()) {
    if (c == '\\' || c == ',' || c == ';') {
      out += '\\';
      out += c;
    } else if (c == '\n')
      out += "\\n";
    else
      out += c;
  }
  return out;
}

std::string ascii_upper(std::string s)
{
  for (char& c : s) {
    if (c >= 'a' && c <= 'z')
      c = static_cast<char>(c - 'a' + 'A');
  }
  return s;
}

/* Group is an optional prefix before the first '.'. Parameters follow ';'. */
std::string prop_name(const std::string& key)
{
  const auto sc = key.find(';');
  std::string n = sc == std::string::npos ? key : key.substr(0, sc);
  const auto dot = n.find('.');
  if (dot != std::string::npos)
    n = n.substr(dot + 1);
  for (char& c : n) {
    if (c >= 'a' && c <= 'z')
      c = static_cast<char>(c - 'a' + 'A');
  }
  return n;
}

/* Split on ';' that is not escaped. Pieces stay escaped for unescape(). */
std::vector<std::string> split_escaped(const std::string& raw)
{
  std::vector<std::string> fields;
  std::string cur;
  for (size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] == '\\' && i + 1 < raw.size()) {
      cur += raw[i];
      cur += raw[++i];
      continue;
    }
    if (raw[i] == ';') {
      fields.push_back(cur);
      cur.clear();
      continue;
    }
    cur += raw[i];
  }
  fields.push_back(cur);
  return fields;
}

}  // namespace

std::vector<Contact> parse_vcf(const std::string& text)
{
  std::vector<Contact> out;
  std::istringstream in(unfold(text));
  std::string line;
  Contact cur;
  bool in_card = false;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    const std::string folded = ascii_upper(line);
    if (folded == "BEGIN:VCARD") {
      cur = {};
      in_card = true;
      continue;
    }
    if (folded == "END:VCARD") {
      if (in_card && cur.keepable())
        out.push_back(cur);
      in_card = false;
      continue;
    }
    if (!in_card)
      continue;
    const auto c = line.find(':');
    if (c == std::string::npos)
      continue;
    const std::string name = prop_name(line.substr(0, c));
    const std::string raw_val = line.substr(c + 1);
    if (name == "N") {
      const auto fields = split_escaped(raw_val);
      if (!fields.empty())
        cur.last = unescape(fields[0]);
      if (fields.size() > 1)
        cur.first = unescape(fields[1]);
    } else if (name == "FN" && cur.first.empty() && cur.last.empty()) {
      cur.first = unescape(raw_val);
    } else if (name == "TEL" && cur.phone.empty())
      cur.phone = unescape(raw_val);
    else if (name == "EMAIL" && cur.email.empty())
      cur.email = unescape(raw_val);
    else if (name == "NOTE" && cur.notes.empty())
      cur.notes = unescape(raw_val);
  }
  return out;
}

std::string contacts_to_vcf(const std::vector<Contact>& contacts)
{
  std::ostringstream os;
  for (const auto& c : contacts) {
    os << "BEGIN:VCARD\r\nVERSION:3.0\r\n";
    os << "N:" << escape(c.last) << ";" << escape(c.first) << ";;;\r\n";
    os << "FN:" << escape(c.display_name()) << "\r\n";
    if (!c.phone.empty())
      os << "TEL:" << escape(c.phone) << "\r\n";
    if (!c.email.empty())
      os << "EMAIL:" << escape(c.email) << "\r\n";
    if (!c.notes.empty())
      os << "NOTE:" << escape(c.notes) << "\r\n";
    os << "END:VCARD\r\n";
  }
  return os.str();
}

}  // namespace ephemeris
