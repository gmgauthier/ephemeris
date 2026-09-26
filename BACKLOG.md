# Ephemeris backlog

Current release: **v1.0.1**. Last updated: 2026-09-26.

Lotus Organizer (ring binder). Local paper book only. Binary `ephemeris`. Suite catalog: `lcos-projects/PRODUCT-BACKLOG.md`. Plan: [DEVELOPMENT.md](DEVELOPMENT.md). How to land work: [DEVELOPMENT.md](DEVELOPMENT.md#process) — `feature/` / `fix/` branches, PRs to `master`, lint gate, semver on shipped PRs.

## High Priority

None.

## Low Priority

- Anniversary section
- Calls section
- Alarms
- **Outlook-class To Do.** Keep the Organizer To Do tab. Add the classic Outlook Tasks fields that the list still lacks: start date, status (not started / in progress / waiting / deferred — completed is today’s done), % complete, categories, a notes body, recurrence on a task (regenerating, not a Calendar appointment). Views: simple list (today), detailed, active, next seven days, overdue, by category. Reminders use the parked **Alarms** item — do not invent a second alarm stack. No task assignment, no Task Request, no flagged mail-as-tasks (Mail is Dispatch).
- **Outlook-class Notepad.** Keep the Organizer Notepad tab (pages in the binder, not yellow desktop stickies). Add Outlook Notes-style colour, categories, and list views (icons / list / last seven days / by category / by colour) on top of title + body + stamp. Search notes. Do not add OLE, do not forward a page as mail, do not open a second notes guest.

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

**v1.0.1** — Planner click-drag paints a span; one title, key, and From/To for the whole selection.

**v1.0.0** — Planner year wall-chart (Holiday / Visit / Travel / Streaming / Other); local appointment recurrence; Notepad with timestamps; vCard import/export; month-day tooltips.

**v0.2.0** — HTTPS iCalendar URL subscribe as a read-only overlay on Calendar. Share-link keys in `~/.config/ephemeris/ephemeris.ini` (0600). Cache under `~/.local/share/ephemeris/calendars/`. No CalDAV.

**v0.1.0 (M0–M5)** — Monday-first month + two-day spread (08:00–18:00, 30-minute slots); To Do with due-date show-through; Contacts (first, last, phone, email-as-a-field, timezone, notes ≤1000; A–Z jump; `@` name completion on appointments and tasks); one `.ephemeris` XML file; print day / month / To Do / Contacts; `.deb` / tarball / AppImage.
