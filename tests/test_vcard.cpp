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

  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "N:Smith\\; Jr;Greg;;;\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 1);
    CHECK(people[0].last == "Smith; Jr");
    CHECK(people[0].first == "Greg");
  }

  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "N:Smith;Greg\\; Alan;;;\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 1);
    CHECK(people[0].last == "Smith");
    CHECK(people[0].first == "Greg; Alan");
  }

  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "N:Smith\\, Jr;Greg;;;\r\n"
        "NOTE:see\\; west\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 1);
    CHECK(people[0].last == "Smith, Jr");
    CHECK(people[0].first == "Greg");
    CHECK(people[0].notes == "see; west");
  }

  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "N:Smith\\\\;Greg;;;\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 1);
    CHECK(people[0].last == "Smith\\");
    CHECK(people[0].first == "Greg");
  }

  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "item1.N:Smith\\; Jr;Greg;;;\r\n"
        "item1.EMAIL:greg@example.com\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 1);
    CHECK(people[0].last == "Smith; Jr");
    CHECK(people[0].first == "Greg");
    CHECK(people[0].email == "greg@example.com");
  }

  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "FN:Smith\\; Jr\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 1);
    CHECK(people[0].first == "Smith; Jr");
    CHECK(people[0].last.empty());
  }

  {
    ephemeris::Contact card;
    card.last = "Smith; Jr";
    card.first = "Greg, Alan";
    card.phone = "555";
    card.notes = "see; west";
    const auto again = ephemeris::parse_vcf(ephemeris::contacts_to_vcf({card}));
    CHECK(again.size() == 1);
    CHECK(again[0].last == "Smith; Jr");
    CHECK(again[0].first == "Greg, Alan");
    CHECK(again[0].phone == "555");
    CHECK(again[0].notes == "see; west");
  }

  return suite_test::done("vcard");
}
