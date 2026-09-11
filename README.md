# Ephemeris

**Vended by Grok Build**

A **paper appointment book** for The Lunduke Computer Operating System (LCOS). The window is Lotus Organizer, not Outlook.

Binary: `ephemeris`. Unlicense.

LCOS itself: [https://github.com/BryanLunduke/LCOS](https://github.com/BryanLunduke/LCOS)

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
