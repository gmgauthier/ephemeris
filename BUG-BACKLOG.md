# Bug backlog

Reviewed 2026-10-01 against the 1.1.0 sources.

`meson test` runs `tests/test_calendar.cpp` (`calendar`), `tests/test_byday.cpp` (`byday`), `tests/test_span.cpp` (`span`), `tests/test_allday.cpp` (`allday`), `tests/test_window.cpp` (`window`), `tests/test_weekly.cpp` (`weekly`), and `tests/test_monthday.cpp` (`monthday`). `calendar` checks a rejected document, an all-day event with an exclusive `DTEND`, a cancelled event dropped, a daily `COUNT`, weekly `BYDAY=MO,WE` in weekday order, a folded summary, a vCard round trip, a calendar that mixes a monthly `BYDAY=1MO` rule with a one-day event, and a calendar that mixes an overnight timed event with a same-day event. A monthly rule from 31 January with no `BYDAY` and `COUNT=3` is 31 January, 31 March, and 31 May. February and 30 April are absent. `monthday` also checks a 30th, a 28th that does land in February, and a yearly 29 February. `calendar` mixes that 31st with a one-off on 2 February. `byday` checks ordinals, weekday lists, and yearly `BYDAY`. `span` checks a next-morning end, an end at the next midnight, a `DURATION` that crosses midnight, one daily instance of an overnight event, and a window that begins on the second day. `allday` checks a two-day `VALUE=DATE` event, a longer vacation, a one-day exclusive `DTEND`, a window that begins on the second day, and one daily instance of a multi-day event. `calendar` also mixes a two-day all-day event with a timed event on the first day. `window` checks a daily rule that starts 900 days before the window, a daily rule from 1990, a `COUNT` that is already spent before the window, an every-other-day phase, an overnight daily slice on the first visible day, and the 800-instance cap inside a long window. `weekly` checks `BYDAY=SU,TU` with `COUNT=1` and `COUNT=2` from a Tuesday, the same rule from a Sunday, and a repeated weekday token.

## Open

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
