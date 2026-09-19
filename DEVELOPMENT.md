# Ephemeris development plan

A gtkmm-3 **paper appointment book** for LCOS. The *window* is Lotus Organizer 1.x / 97: a ring-bound Filofax, not Outlook.

Display name: **Ephemeris**  
Binary / repo / package: `ephemeris`  
License: The Unlicense (`UNLICENSE`)  
Repos: https://gitea.scriptorium/gmgauthier/ephemeris (origin), https://github.com/gmgauthier/ephemeris

Reference window: `brand/ui-reference.svg`

## Status (2026-09-19)

**v0.1.2.** Packaging: `debian/`, `scripts/release.sh`, `INSTALL.md`. Tags `v0.1.0`, `v0.1.1`, `v0.1.2`.

## 1. Locked decisions

| Decision | Choice |
|---|---|
| Product | Original. Chrome is Lotus Organizer, not Outlook 97 |
| Name | Ephemeris. Binary `ephemeris`. APP_ID `org.gmgauthier.Ephemeris` |
| Toolkit | C++17, gtkmm-3.0, GTK3 CSS, Meson |
| Metaphor | One ring-bound book. Section tabs on the right |
| v1 sections | **Calendar** + **To Do** + **Contacts** |
| Contacts | First, Last, Phone, Email (a field, not a mailbox), Timezone, Notes (1000 plain text). Sort by last name. A–Z jump. Evergreen tab. `@` name completion in appointment and To Do add. No vCard |
| Network | Optional HTTPS iCalendar (.ics) URL subscribe, read-only overlay. No CalDAV, no account, no mail. |
| Parked | Notepad, Planner, Anniversary, Calls, recurrence, alarms, vCard import |
| v1 format | UTF-8 XML, extension `.ephemeris`. libxml2. Not `.ics` native, not Lotus `.ORG` |
| Calendar | Monday-first month. Two-day spread (M1). 08:00–18:00, 30-minute slots |
| Brand | LCOS beige / navy. No Bryan’s seal |
| License | The Unlicense |
| Versioning | Semantic (`MAJOR.MINOR.PATCH`). `meson.build` is the source of truth. Debian changelog and git tag `vX.Y.Z` match it. See **Process**. |

## 2. Milestones

- **M0** — binder window
- **M1** — XML file + appointments on the day spread
- **M2** — To Do + show-through
- **M3** — print + keys
- **M4** — chrome polish
- **M4.5** — Contacts tab + `@` mentions
- **M5** — `.deb` + tarball + AppImage, tag `v0.1.0` (this slice)

## 3. Traps

- Evolution / CalDAV / native `.ics` as the binder identity (HTTPS ICS subscribe is overlay only)
- Mail, IMAP, SMTP, or any online account
- vCard import, YOLO-dex index cards, day-page contact show-through
- Photoreal leather
- Custom title bar. `GTK_THEME` in the environment still wins; else Clearlooks-Phenix, then Clearlooks, then Adwaita:light (process only)

## Process

Do not commit to `master`. Every change lands through a pull request.

### Branches

- `feature/<short-name>` — new user-visible work
- `fix/<short-name>` — bugs, packaging nits, regressions

Open a pull request into `master`. Merge only after review.

### Gates

A pull request must pass **lint** before merge. CI runs `./scripts/lint.sh` (no `--fix`). Locally:

- `./scripts/lint.sh --fix` — clang-format rewrites `src/`
- `./scripts/lint.sh` — SPDX headers, no tabs, clang-format `--dry-run --Werror`, cppcheck (`warning`) on `src/`
- `meson compile` with this tree’s `warning_level=2` is clean (no new warnings)

Do not pass `--fix` in CI. Do not merge a red PR.

**Tests** are required when they exist (`meson test -C build`). Until a test suite lands, the gate is lint plus a clean compile plus a manual pass of the change.

### Semantic versioning

Every **shipped** pull request — merged to `master` and tagged as a release — bumps the version. `meson.build` is the source of truth. Keep these in lockstep in the same PR:

- `meson.build` `version:`
- `debian/changelog` (new stanza)
- git tag `vMAJOR.MINOR.PATCH` after merge

Then `./scripts/release.sh` produces `.deb`, tarball, and AppImage.

| Bump | When |
|---|---|
| **PATCH** (`x.y.Z`) | Bug fix or packaging. No new user-facing feature. |
| **MINOR** (`x.Y.0`) | New backward-compatible feature. |
| **MAJOR** (`X.0.0`) | Breaking change: native file format, dropped config keys, removed UI users rely on. |

While the version is `0.y.z`, still bump MINOR and PATCH this way. Do not treat 0.x as a free-for-all. The Debian revision (`-1`, `-2`) is only for rebuilding the same upstream version with no source change.
