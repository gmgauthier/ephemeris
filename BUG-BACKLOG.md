# Bug backlog

Reviewed 2026-10-01 against the 1.1.0 sources.

`meson test` runs `tests/test_calendar.cpp` (`calendar`). It checks a rejected document, an all-day event with an exclusive `DTEND`, a cancelled event dropped, a daily `COUNT`, weekly `BYDAY=MO,WE` in weekday order, a folded summary, and a vCard round trip. A monthly rule from 31 January is required to appear on 31 January and 31 March. February is not asserted, so the suite does not canonize the clamp below.

## Open

### Monthly and yearly rules ignore BYDAY

- Severity: incorrect
- Confidence: high
- Where: `src/ics.cpp:60`, `src/ics.cpp:312`
- Trigger: `RRULE:FREQ=MONTHLY;BYDAY=1MO`, or `FREQ=MONTHLY;BYDAY=MO,WE,FR`.
- Outcome: `byday_index` strips a leading `+`, `-`, or digits, so `1MO` becomes `MO` and the ordinal is discarded. The monthly and yearly branch never reads `rule.byday`. The event repeats on `DTSTART`'s day of the month. Weekly `BYDAY` does work when the list is in weekday order. `ics.hpp` describes `BYDAY` as supported.

### A timed event that crosses midnight is shown as 30 minutes

- Severity: incorrect
- Confidence: high
- Where: `src/ics.cpp:259`
- Trigger: `DTSTART` at 22:00 and `DTEND` the next day at 01:00, or a `DURATION` the parser turns into a next-day end. The same for an end at the next day's 00:00.
- Outcome: Duration is `end.mins - start.mins`. When that is not positive, `end_min` becomes `start_min + 30`. The event is drawn as half an hour.

### A multi-day all-day event appears only on the first day

- Severity: incorrect
- Confidence: high
- Where: `src/ics.cpp:267`, `src/ics.cpp:371`
- Trigger: `VALUE=DATE` with `DTSTART:20260928` and `DTEND:20260930` (two days), or a longer vacation.
- Outcome: The exclusive `DTEND` is shortened by one day, then a non-recurring `expand` emits only `start.date`. The event appears on the start day and not on the following days.

### A long-running daily event can vanish from the visible window

- Severity: incorrect
- Confidence: high
- Where: `src/ics.cpp:239`, `src/ics.cpp:275`
- Trigger: A daily rule with no `COUNT` and no `UNTIL`, whose `DTSTART` is years before the ±24 month window.
- Outcome: The cap is 800 instances counted from `DTSTART`. `take_instance` increments `emitted` even when the day is outside the window. Those 800 can be spent before the window starts, and the event does not appear.

### Weekly BYDAY is walked in list order, and the first miss ends the rule

- Severity: incorrect
- Confidence: high
- Where: `src/ics.cpp:300`
- Trigger: `DTSTART` on a Tuesday, `RRULE:FREQ=WEEKLY;BYDAY=SU,TU;COUNT=1`.
- Outcome: The week is walked in the order the list is written. Sunday of that week is after Tuesday, so it is emitted first and consumes `COUNT`. `take_instance` returning false leaves `expand` entirely (`return`, not `continue`). The Tuesday, which is earlier in the same week and is the start date, is never emitted. `BYDAY=MO,WE` in weekday order does not hit this.

### A monthly or yearly 31st is clamped to the last day of the short month

- Severity: incorrect
- Confidence: high
- Where: `src/ics.cpp:319`
- Trigger: `DTSTART` on 31 January, `FREQ=MONTHLY`.
- Outcome: If the wanted day is past the length of the month, the instance is placed on the last day (`want_day > dim ? dim : want_day`). February becomes the 28th (or the 29th in a leap year) instead of being skipped. March is still the 31st, because each month is clamped on its own. Completing a to-do uses a different path and then stays on the clamped day. See below.

### A SUMMARY with parameters has a blank title

- Severity: incorrect
- Confidence: high
- Where: `src/ics.cpp:388`, `src/ics.cpp:376`
- Trigger: `SUMMARY;LANGUAGE=en:Meet`.
- Outcome: The map key is the text before the colon, parameters included. The title lookup is `ev["SUMMARY"]`, which is empty. The event is stored with a blank title. The same applies to any property matched by exact name.

### Property names are matched case-sensitively

- Severity: incorrect
- Confidence: medium
- Where: `src/ics.cpp:350`
- Trigger: `begin:vevent`, or `summary:Meet`. RFC 5545 names are case-insensitive.
- Outcome: `BEGIN:VEVENT` is an exact string compare. A lower-case begin line is not an event, so the component is skipped.

### A grouped vCard property is dropped

- Severity: incorrect
- Confidence: high
- Where: `src/vcard.cpp:59`
- Trigger: `item1.TEL` or `A.EMAIL`, the grouping Apple and other exporters use.
- Outcome: `prop_name` cuts at `;` and uppercases. It does not cut at `.`. The name stays `ITEM1.TEL` and does not match `TEL` or `EMAIL`. The phone or email is dropped.

### An escaped semicolon in N does not round-trip

- Severity: incorrect
- Confidence: high
- Where: `src/vcard.cpp:99`
- Trigger: A last name that contains `;`. The app's own export writes `N:Smith\; Jr;Greg`. Import that card.
- Outcome: `unescape` runs before the `;` split, so the escaped semicolon becomes a field separator. The name is split in the wrong place.

### A card that has only a phone number is discarded

- Severity: data-loss
- Confidence: high
- Where: `src/vcard.cpp:88`
- Trigger: A vCard with `TEL` (or `TEL` and `NOTE`) and no `N`, `FN`, or `EMAIL`.
- Outcome: The keep condition requires a first name, last name, or email. The card is not added. The phone number is gone.

### Completing a monthly or yearly to-do permanently moves the day

- Severity: incorrect
- Confidence: high
- Where: `src/binder.cpp:689`, `src/binder.cpp:279`
- Trigger: Complete a to-do due 2026-01-31 with a monthly repeat. Or a yearly to-do due 2024-02-29.
- Outcome: `complete_todo` calls `add_recur`, which calls `Glib::Date::add_months` or `add_years`. GLib clamps: 31 January plus one month is 28 February, and the next month stays the 28th (measured: 2026-01-31 → 2026-02-28 → 2026-03-28). A yearly 29 February becomes 28 February and stays the 28th, including in 2028. A daily interval does not drift.

### An impossible day can be saved as the previous day

- Severity: incorrect
- Confidence: high
- Where: `src/binder.cpp:374`
- Trigger: `date_from_iso` is called with `2026-02-31` when `out` is already a valid date, such as 2026-01-10. The to-do dialog and the binder both call it that way.
- Outcome: The check allows any day from 1 to 31. `set_dmy` of 31 February logs `GLib-CRITICAL g_date_set_dmy: assertion 'g_date_valid_dmy' failed` and leaves the previous day. `valid()` is still true, so the function returns true. The dialog saves 10 January while the entry showed 31 February. A default-constructed date stays invalid and the function returns false. The same call aborts under `G_DEBUG=fatal-criticals`. The iCalendar parser rejects this date. This path does not.

### A date the parser rejects clears the old due date

- Severity: data-loss
- Confidence: high
- Where: `src/todo_page.cpp:191`
- Trigger: Edit a to-do that already has a due date and type `2026/02/01` or `tomorrow`. The same pattern clears start (`src/todo_page.cpp:188`) and until (`src/todo_page.cpp:196`).
- Outcome: `date_from_iso` returns false. `has_due` is set false and the dialog still closes (`TodoDlg::ok`). The previous due date is deleted. This is separate from the impossible numeric day above, which returns true.

### A bad subscription body replaces a good cache

- Severity: data-loss
- Confidence: high
- Where: `src/remote_cal.cpp:96`
- Trigger: Refresh a subscribed calendar and the HTTP body is non-empty but not a calendar (an HTML error page, for example).
- Outcome: `apply_ics` writes the body and `rename`s it onto the cache path before `parse_ics`. A previously good `.ics` is already gone. The parse result is not required to contain `BEGIN:VCALENDAR` before the rename. An empty body is not applied.

### The day grid opens only the first appointment in a slot

- Severity: incorrect
- Confidence: high
- Where: `src/day_spread.cpp:25`
- Trigger: A 09:00–10:00 event and a 09:30–10:30 event on the same day, both inside the grid.
- Outcome: `covering` returns the first in-grid appointment whose range contains the minute. The later event is in the grid, so it is not in the overflow list. The slot stays on the first hit, and the second event cannot be opened.

### The planner opens only the first event that covers a day

- Severity: incorrect
- Confidence: high
- Where: `src/planner_page.cpp:17`
- Trigger: Two planner ranges cover the same day. The later one was painted on top.
- Outcome: `covering_id` returns the first event in insertion order whose range contains the day. Open, edit, and delete use that id. The later event cannot be opened until the earlier covering events are removed.

### "Seven days" is eight dates

- Severity: incorrect
- Confidence: medium
- Where: `src/todo_page.cpp:203`
- Trigger: The next-seven-days list. It calls `in_next_days(..., 7)`.
- Outcome: The test is `delta >= 0 && delta <= days`. That is today plus the seven days after it: eight dates.

## Closed

None.
