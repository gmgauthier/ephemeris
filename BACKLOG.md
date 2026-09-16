# Ephemeris backlog

Current release: **v0.1.0**. Last updated: 2026-09-15.

Lotus Organizer (ring binder). Local paper book only. Binary `ephemeris`. Suite catalog: `lcos-projects/PRODUCT-BACKLOG.md`. Plan: [DEVELOPMENT.md](DEVELOPMENT.md). How to land work: [DEVELOPMENT.md](DEVELOPMENT.md#process) — `feature/` / `fix/` branches, PRs to `master`, lint gate, semver on shipped PRs.

## High Priority

Nothing queued. v1 (M0–M5, including Contacts) is tagged. Parked sections live under Low Priority. Do **not** start a second Organizer.

## Low Priority

- Notepad section
- Planner section
- Anniversary section
- Calls section
- Recurrence
- Alarms
- vCard import

## Out of Scope

- Mail, IMAP, SMTP, or any online account. Mail is a later *mode* of Dispatch, not an Ephemeris tab
- CalDAV / native `.ics` as identity
- Evolution re-theme
- Day-page contact show-through
- YOLO-dex index cards (people records already live here as Contacts)
- Photoreal leather
- Custom title bar. `GTK_THEME` in the environment still wins; else Clearlooks-Phenix, then Clearlooks, then Adwaita:light (process only)
- Bryan’s seal

## Shipped

**v0.1.0 (M0–M5)** — Monday-first month + two-day spread (08:00–18:00, 30-minute slots); To Do with due-date show-through; Contacts (first, last, phone, email-as-a-field, timezone, notes ≤1000; A–Z jump; `@` name completion on appointments and tasks); one `.ephemeris` XML file; print day / month / To Do / Contacts; `.deb` / tarball / AppImage.
