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

  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "TEL;TYPE=CELL:555-0100\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 1);
    CHECK(people[0].phone == "555-0100");
    CHECK(people[0].first.empty());
    CHECK(people[0].last.empty());
    CHECK(people[0].email.empty());
  }

  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "item1.TEL:555-0199\r\n"
        "NOTE:desk\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 1);
    CHECK(people[0].phone == "555-0199");
    CHECK(people[0].notes == "desk");
    CHECK(people[0].first.empty());
    CHECK(people[0].last.empty());
    CHECK(people[0].email.empty());
  }

  {
    const char* text =
        "BEGIN:VCARD\r\n"
        "END:VCARD\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.empty());
  }

  {
    ephemeris::Contact card;
    card.phone = "555-0100";
    card.notes = "desk";
    const auto again = ephemeris::parse_vcf(ephemeris::contacts_to_vcf({card}));
    CHECK(again.size() == 1);
    CHECK(again[0].phone == "555-0100");
    CHECK(again[0].notes == "desk");
  }

  {
    // Add and edit keep a card the file would keep.
    ephemeris::Contact phone;
    phone.phone = "555-0100";
    CHECK(phone.keepable());
    ephemeris::Contact email;
    email.email = "a@b.test";
    CHECK(email.keepable());
    ephemeris::Contact named;
    named.last = "Smith";
    CHECK(named.keepable());
    ephemeris::Contact blank;
    CHECK(!blank.keepable());
    ephemeris::Contact notes;
    notes.notes = "desk";
    CHECK(!notes.keepable());
  }

  {
    // The envelope is matched without regard to case. Values keep their case.
    const char* text =
        "begin:vcard\r\n"
        "n:Smith;Ada\r\n"
        "tel:555-0100\r\n"
        "end:vcard\r\n"
        "Begin:VCARD\r\n"
        "FN:Ada Lovelace\r\n"
        "EMAIL:Ada@Example.test\r\n"
        "End:VCard\r\n";
    const auto people = ephemeris::parse_vcf(text);
    CHECK(people.size() == 2);
    CHECK(people[0].last == "Smith");
    CHECK(people[0].first == "Ada");
    CHECK(people[0].phone == "555-0100");
    CHECK(people[1].first == "Ada Lovelace");
    CHECK(people[1].email == "Ada@Example.test");
    CHECK(people[1].last.empty());
  }

  return suite_test::done("vcard");
}
