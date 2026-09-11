# Ephemeris

**Vended by Grok Build**

![Ephemeris month page on LCOS](brand/LCOS_ephemeris_calendar.png)

A **paper appointment book** for The Lunduke Computer Operating System (LCOS). The window is Lotus Organizer, not Outlook.

Binary: `ephemeris`. Unlicense.

LCOS itself: [https://github.com/BryanLunduke/LCOS](https://github.com/BryanLunduke/LCOS)

![Two-day spread](brand/LCOS_ephemeris_day_pages.png)

![To Do](brand/LCOS_ephemeris_todo.png)

![Contacts](brand/LCOS_ephemeris_contacts.png)

![About](brand/LCOS_ephemeris_about.png)

## Status

**v0.1.0 (M0–M5).** Lotus Organizer-shaped binder: Calendar, To Do, Contacts, print, `.deb` + tarball + AppImage. See [INSTALL.md](INSTALL.md).

| Doc | What |
|---|---|
| [DEVELOPMENT.md](DEVELOPMENT.md) | Locked decisions, architecture, milestones M0–M5 |

## Build

```
sudo apt install build-essential meson ninja-build pkg-config g++ libgtkmm-3.0-dev libxml2-dev
meson setup build
meson compile -C build
./build/ephemeris
```

Install: [INSTALL.md](INSTALL.md).

## License

[The Unlicense](https://unlicense.org). See [UNLICENSE](UNLICENSE).
