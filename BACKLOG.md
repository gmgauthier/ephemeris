# Ephemeris backlog

Current release: **v1.1.16**. Last updated: 2026-10-01.

Lotus Organizer (ring binder). Local paper book only. Binary `ephemeris`. Suite catalog: `lcos-projects/PRODUCT-BACKLOG.md`. Plan: [DEVELOPMENT.md](DEVELOPMENT.md). How to land work: [DEVELOPMENT.md](DEVELOPMENT.md#process) — `feature/` / `fix/` branches, PRs to `master`, lint gate, semver on shipped PRs.

## High Priority

None.

## Low Priority

- Anniversary section
- Calls section
- Alarms

## Out of Scope

- Mail, IMAP, SMTP, or any online account. Mail is a later *mode* of Dispatch, not an Ephemeris tab
- CalDAV / native `.ics` as identity
- Evolution re-theme
- Outlook chrome, Exchange task assignment, Task Requests, flagged mail-as-tasks
- Day-page contact show-through
- YOLO-dex index cards (people records already live here as Contacts)
- Photoreal leather
- Custom title bar. `GTK_THEME` in the environment still wins; else Clearlooks-Phenix, then Clearlooks, then Adwaita:light (process only)
- Bryan’s seal

## Shipped

**v1.1.16** — A refresh that is not a calendar leaves the saved calendar in place. A real calendar still replaces it.

**v1.1.15** — Text that does not parse leaves the previous start, due, or until in place. Turning the checkbox off still clears that field.

**v1.1.14** — A day the month does not have is rejected. 31 February does not replace the previous date.

**v1.1.13** — Completing a monthly or yearly to-do keeps that day. A short month is skipped.

**v1.1.12** — A vCard that has only a phone number is kept. An empty card is still dropped.

**v1.1.11** — A semicolon inside a vCard `N` field round-trips. `Smith\; Jr` stays the last name.

**v1.1.10** — A grouped vCard property such as `item1.TEL` or `A.EMAIL` keeps the phone or email.

**v1.1.9** — iCalendar names match without regard to case. A summary value keeps the case it was written in.

**v1.1.8** — A property that carries parameters still matches its name, so a `SUMMARY` with `LANGUAGE` keeps its title.

**v1.1.7** — A monthly or yearly day the month does not have is skipped, and it does not consume `COUNT`.

**v1.1.6** — Weekly `BYDAY` expands in weekday order, so `COUNT` consumes the earlier day.

**v1.1.5** — Unbounded daily events that started long before the window stay visible. `COUNT` still counts from the start date.

**v1.1.4** — Multi-day all-day events are drawn on each covered day.

**v1.1.3** — Timed events that cross midnight are drawn on each covered day. A `DURATION` sets the end when `DTEND` is absent.

**v1.1.2** — Monthly and yearly `BYDAY`, including ordinals such as `1MO` and `-1FR`.

**v1.1.1** — Headless meson test suite, and known defects recorded in BUG-BACKLOG.md.

**v1.1.0** — Outlook-class To Do (start, status, percent, category, notes, regenerating recurrence, views). Outlook-class Notepad (colour, categories, list views, search).

**v1.0.1** — Planner click-drag paints a span; one title, key, and From/To for the whole selection.

**v1.0.0** — Planner year wall-chart (Holiday / Visit / Travel / Streaming / Other); local appointment recurrence; Notepad with timestamps; vCard import/export; month-day tooltips.

**v0.2.0** — HTTPS iCalendar URL subscribe as a read-only overlay on Calendar. Share-link keys in `~/.config/ephemeris/ephemeris.ini` (0600). Cache under `~/.local/share/ephemeris/calendars/`. No CalDAV.

**v0.1.0 (M0–M5)** — Monday-first month + two-day spread (08:00–18:00, 30-minute slots); To Do with due-date show-through; Contacts (first, last, phone, email-as-a-field, timezone, notes ≤1000; A–Z jump; `@` name completion on appointments and tasks); one `.ephemeris` XML file; print day / month / To Do / Contacts; `.deb` / tarball / AppImage.
