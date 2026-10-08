# Bug backlog

Reviewed 2026-10-01 against the 1.1.0 sources.

`meson test` runs `tests/test_calendar.cpp` (`calendar`), `tests/test_byday.cpp` (`byday`), `tests/test_span.cpp` (`span`), `tests/test_allday.cpp` (`allday`), `tests/test_window.cpp` (`window`), `tests/test_weekly.cpp` (`weekly`), `tests/test_monthday.cpp` (`monthday`), `tests/test_params.cpp` (`params`), and `tests/test_case.cpp` (`casefold`), and `tests/test_vcard.cpp` (`vcard`). `calendar` checks a rejected document, an all-day event with an exclusive `DTEND`, a cancelled event dropped, a daily `COUNT`, weekly `BYDAY=MO,WE` in weekday order, a folded summary, a vCard round trip, a calendar that mixes a monthly `BYDAY=1MO` rule with a one-day event, and a calendar that mixes an overnight timed event with a same-day event. A monthly rule from 31 January with no `BYDAY` and `COUNT=3` is 31 January, 31 March, and 31 May. February and 30 April are absent. `monthday` also checks a 30th, a 28th that does land in February, and a yearly 29 February. `calendar` mixes that 31st with a one-off on 2 February. `params` checks a `SUMMARY` with `LANGUAGE`, a folded parameterized summary, an `RRULE` with a parameter, a `STATUS` with a parameter, a `DURATION` with a parameter, and a `VALUE=DATE` stamp that also carries a time. `calendar` mixes a parameterized summary and a parameterized calendar name with a plain event. `casefold` checks a lower-case `BEGIN`/`SUMMARY`, a summary value that keeps its own case, a lower-case `RRULE`, `STATUS`, `DURATION`, and `VALUE=DATE`, and an empty `begin:vcalendar`. `calendar` also mixes that lower-case event with an upper-case one. `vcard` checks a grouped `TEL` and `EMAIL`, a second phone that does not replace the first, a lower-case grouped name, a grouped `FN`, and a folded grouped phone. `calendar` mixes that grouped card with a plain card and reads both back after export. `vcard` also checks an escaped semicolon in `N`, a semicolon in the given name, a comma, a backslash, a grouped `N`, and a round trip through export. `calendar` round-trips a `Smith; Jr` card beside a plain card. `vcard` keeps a card that has only a phone number, including one with a note and a grouped `TEL`, and an empty card is still dropped. A phone-only card and an email-only card are worth keeping. A card with only a note is not. `begin:vcard` and `End:VCard` open and close a card, and the name, phone, and email keep their own case. `calendar` reads that phone-only card beside a named card. `recur` checks that completing a monthly 31st skips the short month and a second completion does not stay there, that a yearly 29 February lands on the next leap day, that a daily interval advances by days, and that a saved binder reloads those next dates. `date` checks that `2026-02-31` leaves a previous 10 January in place, that a blank date stays blank, that 29 February is kept only in a leap year, and that a binder file keeps a real 10 January beside a rejected 31 February. `keepdate` checks that `tomorrow` and `2026/02/01` leave a previous due, start, and until in place, that a real ISO date still replaces one, and that a saved binder reloads those dates. `cache` checks that an HTML error page, an empty body, and a bare event leave a saved calendar in place, that a real calendar and a lower-case `begin:vcalendar` replace it, and that a second subscription is left alone. `grid` checks that a 09:30 appointment overlapping a 09:00 appointment has its own row, that two appointments starting together each have a row, and that a saved binder reloads both. `planner` checks that a later range covering the same day is the one painted on top, that both events are still listed for that day, and that a saved binder reloads them. `week` checks that the next-seven-days window is today and the six days after it, that the seventh day after today is outside it, that the last-seven-days window is today and the six days before it, and that a saved binder reloads those dues. `byday` checks ordinals, weekday lists, and yearly `BYDAY`. `span` checks a next-morning end, an end at the next midnight, a `DURATION` that crosses midnight, one daily instance of an overnight event, and a window that begins on the second day. `allday` checks a two-day `VALUE=DATE` event, a longer vacation, a one-day exclusive `DTEND`, a window that begins on the second day, and one daily instance of a multi-day event. `calendar` also mixes a two-day all-day event with a timed event on the first day. `window` checks a daily rule that starts 900 days before the window, a daily rule from 1990, a `COUNT` that is already spent before the window, an every-other-day phase, an overnight daily slice on the first visible day, and the 800-instance cap inside a long window. `weekly` checks `BYDAY=SU,TU` with `COUNT=1` and `COUNT=2` from a Tuesday, the same rule from a Sunday, and a repeated weekday token. `notes` (`tests/test_notes.cpp`) checks a plain note round trip, a note with bold, italic, and a bullet, a missing journal date that uses the stamp day, an explicit journal date, categories that can be added, renamed, and deleted, a typed category that stores the finished name, a new note saved as white while a yellow note stays yellow, an empty paragraph after a bullet that is not an empty bullet, and a planner key named Streaming that loads as Projects.

## Open

None.

## Closed

### A lower-case vCard envelope is ignored

- Severity: data-loss
- Confidence: high
- Where: `src/vcard.cpp` `parse_vcf`
- Trigger: Open a vCard whose envelope is `begin:vcard` / `end:vcard`, or mixed case such as `End:VCard`. Property names inside the card may already be lower case.
- Outcome: `BEGIN:VCARD` and `END:VCARD` are compared as exact strings. The card never opens, so the name, phone, and email are dropped. Property names inside an upper-case envelope were already matched without regard to case.
- Fixed in v1.1.22: The envelope is matched without regard to case. Values keep their own case.

### A contact with no name is dropped

- Severity: data-loss
- Confidence: high
- Where: `src/contacts_page.cpp` `on_add_contact`, `edit_item`; `src/binder.hpp` `Contact::keepable`
- Trigger: Add or edit a contact, leave first and last blank, and fill in a phone or an email. Press OK.
- Outcome: The dialog has already accepted the card. The page then returns without storing it, because both names are empty. Opening a vCard keeps that same card.
- Fixed in v1.1.21: A card with a name, a phone, or an email is kept. A blank card, or a card with only a note, is still dropped. The list shows it as Unnamed.

### Notepad "Last seven days" is eight dates

- Severity: incorrect
- Confidence: high
- Where: `src/notepad_page.cpp` `NotepadPage::visible`
- Trigger: Notepad, view "Last seven days". A note stamped seven days before today.
- Outcome: The window keeps a julian delta of 0 through 7. That is today and the seven days before it: eight dates. The to-do list's next seven days was already today and the six days after it.
- Fixed in v1.1.20: Last seven days is today and the six days before it. A note from seven days ago is outside the list. A future note stays out.

### "Seven days" is eight dates

- Severity: incorrect
- Confidence: medium
- Where: `src/todo_page.cpp` `in_next_days`
- Trigger: The next-seven-days list. It calls `in_next_days(..., 7)`.
- Outcome: The test is `delta >= 0 && delta <= days`. That is today plus the seven days after it: eight dates.
- Fixed in v1.1.19: The window is today and the six days after it. The seventh day after today is outside the list. A due date before today stays out.

### The planner opens only the first event that covers a day

- Severity: incorrect
- Confidence: high
- Where: `src/planner_page.cpp` `planner_hits`
- Trigger: Two planner ranges cover the same day. The later one was painted on top.
- Outcome: `covering_id` returns the first event in insertion order whose range contains the day. Open, edit, and delete use that id. The later event cannot be opened until the earlier covering events are removed.
- Fixed in v1.1.18: The day is painted with the later event. A day with one event opens that event. A day with more than one event lists each of them, later first, and opens the one you pick.

### The day grid opens only the first appointment in a slot

- Severity: incorrect
- Confidence: high
- Where: `src/day_spread.cpp` `day_grid_rows`
- Trigger: A 09:00–10:00 event and a 09:30–10:30 event on the same day, both inside the grid.
- Outcome: `covering` returns the first in-grid appointment whose range contains the minute. The later event is in the grid, so it is not in the overflow list. The slot stays on the first hit, and the second event cannot be opened.
- Fixed in v1.1.17: Each in-grid appointment has a row on the slot where it starts, and that row opens that appointment. Two appointments that start in the same half hour each have a row. A later slot still opens an appointment that is continuing when nothing new starts there. An all-day appointment and an appointment outside 08:00–18:00 stay off the grid.

### A bad subscription body replaces a good cache

- Severity: data-loss
- Confidence: high
- Where: `src/remote_cal.cpp` `apply_ics`
- Trigger: Refresh a subscribed calendar and the HTTP body is non-empty but not a calendar (an HTML error page, for example).
- Outcome: `apply_ics` writes the body and `rename`s it onto the cache path before `parse_ics`. A previously good `.ics` is already gone. The parse result is not required to contain `BEGIN:VCALENDAR` before the rename. An empty body is not applied.
- Fixed in v1.1.16: A body without `BEGIN:VCALENDAR` leaves the saved calendar in place. An HTML error page, an empty body, and a bare event do not replace it. A real calendar still replaces it, including a lower-case `begin:vcalendar`. A second subscription is left alone. The refresh counts a rejected body as a failure.

### A date the parser rejects clears the old due date

- Severity: data-loss
- Confidence: high
- Where: `src/todo_page.cpp` start, due, and until in the to-do dialog
- Trigger: Edit a to-do that already has a due date and type `2026/02/01` or `tomorrow`. The same pattern clears start and until.
- Outcome: `date_from_iso` returns false. `has_due` is set false and the dialog still closes (`TodoDlg::ok`). The previous due date is deleted. This is separate from the impossible numeric day, which is rejected and leaves the previous date unchanged.
- Fixed in v1.1.15: Text that does not parse leaves the previous start, due, or until in place. A real ISO date still replaces it. Turning the checkbox off still clears that field. An appointment until uses the same rule. The next-seven-days list still counts eight dates.

### An impossible day can be saved as the previous day

- Severity: incorrect
- Confidence: high
- Where: `src/binder.cpp` `date_from_iso`
- Trigger: `date_from_iso` is called with `2026-02-31` when `out` is already a valid date, such as 2026-01-10. The to-do dialog and the binder both call it that way.
- Outcome: The check allows any day from 1 to 31. `set_dmy` of 31 February logs `GLib-CRITICAL g_date_set_dmy: assertion 'g_date_valid_dmy' failed` and leaves the previous day. `valid()` is still true, so the function returns true. The dialog saves 10 January while the entry showed 31 February. A default-constructed date stays invalid and the function returns false. The same call aborts under `G_DEBUG=fatal-criticals`. The iCalendar parser rejects this date. This path does not.
- Fixed in v1.1.14: A day the month does not have is rejected, and the previous date is left unchanged. 31 February does not become 10 January. 29 February in a leap year is still accepted. A date the to-do dialog cannot parse still clears the old due date.

### Completing a monthly or yearly to-do permanently moves the day

- Severity: incorrect
- Confidence: high
- Where: `src/binder.cpp` `add_recur`, called from `complete_todo`
- Trigger: Complete a to-do due 2026-01-31 with a monthly repeat. Or a yearly to-do due 2024-02-29.
- Outcome: `complete_todo` calls `add_recur`, which calls `Glib::Date::add_months` or `add_years`. GLib clamps: 31 January plus one month is 28 February, and the next month stays the 28th (measured: 2026-01-31 → 2026-02-28 → 2026-03-28). A yearly 29 February becomes 28 February and stays the 28th, including in 2028. A daily interval does not drift.
- Fixed in v1.1.13: The next date skips a month or year that does not contain that day. 31 January monthly becomes 31 March, then 31 May. 29 February yearly becomes 29 February in the next leap year. A daily interval still advances by days. An impossible day in the date field is still a separate defect.

### A card that has only a phone number is discarded

- Severity: data-loss
- Confidence: high
- Where: `src/vcard.cpp` keep condition in `parse_vcf`
- Trigger: A vCard with `TEL` (or `TEL` and `NOTE`) and no `N`, `FN`, or `EMAIL`.
- Outcome: The keep condition required a first name, last name, or email. The card was not added. The phone number was gone.
- Fixed in v1.1.12: A card with a phone number is kept. A grouped `TEL` counts. An empty card is still dropped. Completing a to-do still uses its own date arithmetic.

### An escaped semicolon in N does not round-trip

- Severity: incorrect
- Confidence: high
- Where: `src/vcard.cpp` `N` parsing in `parse_vcf`
- Trigger: A last name that contains `;`. The app's own export writes `N:Smith\; Jr;Greg`. Import that card.
- Outcome: `unescape` ran before the `;` split, so the escaped semicolon became a field separator. The name was split in the wrong place.
- Fixed in v1.1.11: `N` is split on semicolons that are not escaped, and each field is unescaped after that. `Smith\; Jr;Greg` is last name `Smith; Jr` and given name `Greg`. A card that has only a phone number is still discarded.

### A grouped vCard property is dropped

- Severity: incorrect
- Confidence: high
- Where: `src/vcard.cpp` `prop_name`
- Trigger: `item1.TEL` or `A.EMAIL`, the grouping Apple and other exporters use.
- Outcome: `prop_name` cut at `;` and uppercased. It did not cut at `.`. The name stayed `ITEM1.TEL` and did not match `TEL` or `EMAIL`. The phone or email was dropped.
- Fixed in v1.1.10: The group prefix before the first `.` is removed before the name is matched. Parameters after `;` still apply. A card that has only a phone number is still discarded. An escaped semicolon in `N` still does not round-trip.

### Property names are matched case-sensitively

- Severity: incorrect
- Confidence: medium
- Where: `src/ics.cpp` property parser in `parse_ics`
- Trigger: `begin:vevent`, or `summary:Meet`. RFC 5545 names are case-insensitive.
- Outcome: `BEGIN:VEVENT` was an exact string compare. A lower-case begin line was not an event, so the component was skipped. A lower-case `SUMMARY` missed the title lookup.
- Fixed in v1.1.9: Component names, property names, parameter names, and the fixed tokens this parser compares are matched without regard to case. A summary value keeps the case it was written in. `begin:vevent` with `summary:Meet` is an event titled Meet.

### A SUMMARY with parameters has a blank title

- Severity: incorrect
- Confidence: high
- Where: `src/ics.cpp` property parser in `parse_ics`
- Trigger: `SUMMARY;LANGUAGE=en:Meet`.
- Outcome: The map key was the text before the colon, parameters included. The title lookup is `ev["SUMMARY"]`, which was empty. The event was stored with a blank title. The same applied to any property matched by exact name, including `RRULE`, `STATUS`, and `DURATION`.
- Fixed in v1.1.8: The name is stored without its parameters, so `SUMMARY;LANGUAGE=en:Meet` is titled Meet. `DTSTART` and `DTEND` still pass their parameters through, so `VALUE=DATE` and `TZID` keep working. A lower-case property name is still a separate defect.

### A monthly or yearly 31st is clamped to the last day of the short month

- Severity: incorrect
- Confidence: high
- Where: `src/ics.cpp` monthly and yearly loop in `expand`
- Trigger: `DTSTART` on 31 January, `FREQ=MONTHLY`. The same clamp hit a yearly 29 February in a non-leap year.
- Outcome: If the wanted day was past the length of the month, the instance was placed on the last day. February became the 28th (or the 29th in a leap year) instead of being skipped. March was still the 31st, because each month was clamped on its own.
- Fixed in v1.1.7: A month that has no such day is skipped and does not consume `COUNT`. 31 January monthly is 31 January, 31 March, 31 May. A 28th still lands in February. A yearly 29 February keeps leap years only. Completing a to-do still uses its own date arithmetic.

### Weekly BYDAY is walked in list order, and the first miss ends the rule

- Severity: incorrect
- Confidence: high
- Where: `src/ics.cpp` weekly loop in `expand`
- Trigger: `DTSTART` on a Tuesday, `RRULE:FREQ=WEEKLY;BYDAY=SU,TU;COUNT=1`.
- Outcome: The week was walked in the order the list was written. Sunday of that week is after Tuesday, so it was emitted first and consumed `COUNT`. `take_instance` returning false left `expand` entirely. The Tuesday, which is earlier in the same week and is the start date, was never emitted.
- Fixed in v1.1.6: The week's weekdays are sorted Monday through Sunday before expansion, and a repeated token is kept once. `COUNT` therefore consumes dates in calendar order. A list that was already in weekday order is unchanged.

### A long-running daily event can vanish from the visible window

- Severity: incorrect
- Confidence: high
- Where: `src/ics.cpp` `take_instance`, daily loop in `expand`
- Trigger: A daily rule with no `COUNT` and no `UNTIL`, whose `DTSTART` is years before the ±24 month window.
- Outcome: The cap was 800 instances counted from `DTSTART`. `take_instance` incremented `emitted` even when the day was outside the window. Those 800 could be spent before the window started, and the event did not appear.
- Fixed in v1.1.5: The safety cap counts an instance only when a slice falls in the window. `COUNT` still counts from `DTSTART`, including days the window does not show. An unbounded daily rule keeps its interval and skips ahead to the window, including a span that starts the day before. Weekly and monthly rules share the cap change. Their own loop bounds are unchanged.

### A multi-day all-day event appears only on the first day

- Severity: incorrect
- Confidence: high
- Where: `src/ics.cpp` `expand`, `take_instance`
- Trigger: `VALUE=DATE` with `DTSTART:20260928` and `DTEND:20260930` (two days), or a longer vacation.
- Outcome: The exclusive `DTEND` was shortened by one day, then a non-recurring `expand` emitted only `start.date`. The event appeared on the start day and not on the following days.
- Fixed in v1.1.4: After the exclusive `DTEND` is shortened, an all-day span is one full day per covered date and still one recurrence instance. A one-day event, including an exclusive `DTEND` on the next day, stays a single day. Timed spans are unchanged.

### A timed event that crosses midnight is shown as 30 minutes

- Severity: incorrect
- Confidence: high
- Where: `src/ics.cpp` `take_instance`, `expand`, `duration_minutes`
- Trigger: `DTSTART` at 22:00 and `DTEND` the next day at 01:00, or a `DURATION` the parser turns into a next-day end. The same for an end at the next day's 00:00.
- Outcome: Duration was `end.mins - start.mins`. When that was not positive, `end_min` became `start_min + 30`. The event was drawn as half an hour on the start day.
- Fixed in v1.1.3: A timed span that ends on a later day is one instance drawn on each covered date. The first day runs from the start time to midnight, a middle day fills the day, and the last day runs from midnight to the end time. An end at 00:00 adds no empty next-day row. When `DTEND` is absent, `DURATION` sets the end. `DTEND` wins when both are present. All-day events are unchanged.

### Monthly and yearly rules ignore BYDAY

- Severity: incorrect
- Confidence: high
- Where: `src/ics.cpp` `parse_byday`, `byday_in_month`, `byday_in_year`
- Trigger: `RRULE:FREQ=MONTHLY;BYDAY=1MO`, `FREQ=MONTHLY;BYDAY=MO,WE,FR`, or `FREQ=YEARLY;BYDAY=1MO`.
- Outcome: The ordinal was discarded and the monthly and yearly branch repeated `DTSTART`'s day of the month.
- Fixed in v1.1.2: `BYDAY` keeps its ordinal. Monthly rules expand inside each month and yearly rules inside each year, in date order. `1MO` is the first Monday, `-1FR` the last Friday, and a bare weekday is every matching day. A rule with no `BYDAY` still repeats on the start day.
