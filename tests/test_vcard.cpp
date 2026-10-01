/* SPDX-License-Identifier: Unlicense */

#include "vcard.hpp"
#include "check.hpp"

#include <string>

int main()
{
  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "N:Smith;Ada;;;\r\n"
        "item1.TEL;TYPE=CELL:555-0100\r\n"
        "item2.TEL;TYPE=HOME:555-0199\r\n"
        "A.EMAIL;TYPE=INTERNET:ada@example.com\r\n"
        "item1.NOTE:lab\\, west\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 1);
    CHECK(people[0].last == "Smith");
    CHECK(people[0].first == "Ada");
    CHECK(people[0].phone == "555-0100");
    CHECK(people[0].email == "ada@example.com");
    CHECK(people[0].notes == "lab, west");
  }

  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "FN:Ada Smith\r\n"
        "item1.tel:555-0101\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 1);
    CHECK(people[0].first == "Ada Smith");
    CHECK(people[0].phone == "555-0101");
    CHECK(people[0].email.empty());
  }

  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "item1.FN:Grouped Name\r\n"
        "item1.EMAIL:grouped@example.com\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 1);
    CHECK(people[0].first == "Grouped Name");
    CHECK(people[0].email == "grouped@example.com");
  }

  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "N:Plain;Pat;;;\r\n"
        "TEL:555\r\n"
        "EMAIL:pat@example.com\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 1);
    CHECK(people[0].last == "Plain");
    CHECK(people[0].first == "Pat");
    CHECK(people[0].phone == "555");
    CHECK(people[0].email == "pat@example.com");
  }

  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "N:Fold;Sam;;;\r\n"
        "item1.TEL;TYPE=CELL:555\r\n"
        " -0102\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 1);
    CHECK(people[0].last == "Fold");
    CHECK(people[0].first == "Sam");
    CHECK(people[0].phone == "555-0102");
  }

  return suite_test::done("vcard");
}
