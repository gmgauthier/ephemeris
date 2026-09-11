# Ephemeris development plan

A gtkmm-3 **paper appointment book** for LCOS. The *window* is Lotus Organizer 1.x / 97: a ring-bound Filofax, not Outlook.

Display name: **Ephemeris**  
Binary / repo / package: `ephemeris`  
License: The Unlicense (`UNLICENSE`)  
Repos: https://gitea.scriptorium/gmgauthier/ephemeris (origin), https://github.com/gmgauthier/ephemeris

Reference window: `brand/ui-reference.svg`

## Status (2026-09-12)

**M5 in tree.** Packaging: `debian/`, `scripts/release.sh`, `INSTALL.md`. Tag `v0.1.0`.

## 1. Locked decisions

| Decision | Choice |
|---|---|
| Product | Original. Chrome is Lotus Organizer, not Outlook 97 |
| Name | Ephemeris. Binary `ephemeris`. APP_ID `org.gmgauthier.Ephemeris` |
| Toolkit | C++17, gtkmm-3.0, GTK3 CSS, Meson |
| Metaphor | One ring-bound book. Section tabs on the right |
| v1 sections | **Calendar** + **To Do** + **Contacts** |
| Contacts | First, Last, Phone, Email, Timezone, Notes (1000 plain text). Sort by last name. A–Z jump. Evergreen tab. `@` name completion in appointment and To Do add. No vCard |
| Parked | Notepad, Planner, Anniversary, Calls, recurrence, alarms, CalDAV, vCard import, standalone Address Book |
| v1 format | UTF-8 XML, extension `.ephemeris`. libxml2. Not `.ics` native, not Lotus `.ORG` |
| Calendar | Monday-first month. Two-day spread (M1). 08:00–18:00, 30-minute slots |
| Network | None |
| Brand | LCOS beige / navy. No Bryan’s seal |
| License | The Unlicense |

## 2. Milestones

- **M0** — binder window
- **M1** — XML file + appointments on the day spread
- **M2** — To Do + show-through
- **M3** — print + keys
- **M4** — chrome polish
- **M4.5** — Contacts tab + `@` mentions
- **M5** — `.deb` + tarball + AppImage, tag `v0.1.0` (this slice)

## 3. Traps

- Evolution / CalDAV / native `.ics`
- vCard import, YOLO-dex index cards, day-page contact show-through
- Photoreal leather
- Custom title bar; do not override `GTK_THEME`
