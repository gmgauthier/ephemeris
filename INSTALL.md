# Installing Ephemeris

Four ways to get a binary, in the order LCOS cares about:

| Artifact | Who it is for |
|---|---|
| **`.deb`** | LCOS, Devuan Excalibur, Debian Trixie. Preferred. |
| **Source tarball** | Distro packagers and `meson setup && ninja install`. |
| **AppImage** | Fallback for distros that do not install `.deb` files. gtkmm only. Published on the GitHub/Gitea release. |
| **Git build** | Developers. See below. |

Version comes from `meson.build` (currently `0.1.2`).

## Runtime needs

- GTK 3 / gtkmm-3.0
- libxml2, **libsoup-3.0**, **glib-networking** (HTTPS calendar URLs)

On Debian / Devuan / LCOS:

```
sudo apt install libgtkmm-3.0-1t64 libxml2 libsoup-3.0-0 glib-networking
```

(Package names on older Debian may be `libgtkmm-3.0-1v5`.)

## 1. Debian package (preferred)

From a release `.deb`:

```
sudo apt install ./dist/ephemeris_0.1.2-1_amd64.deb
```

Or, from this tree:

```
./scripts/release.sh deb
sudo apt install ./dist/ephemeris_0.1.2-1_amd64.deb
```

That installs:

- `/usr/bin/ephemeris`
- `/usr/share/applications/ephemeris.desktop`
- `/usr/share/icons/hicolor/scalable/apps/ephemeris.svg`
- `/usr/share/ephemeris/skin/lcos/lcos.css`
- `/usr/share/ephemeris/brand/icon-tile.svg`

Launch from the menu or `ephemeris`. Config is `~/.config/ephemeris/ephemeris.ini`. Binders are `.ephemeris` XML files.

Uninstall: `sudo apt remove ephemeris`.

## 2. Source tarball

`meson dist` produces `build/meson-dist/ephemeris-VERSION.tar.xz` (sample binders under `data/samples/` are git-only, not in the tarball).

```
tar -xf ephemeris-0.1.2.tar.xz
cd ephemeris-0.1.2
sudo apt install build-essential meson ninja-build pkg-config \
  libgtkmm-3.0-dev libxml2-dev libsoup-3.0-dev
meson setup build --prefix=/usr
meson compile -C build
sudo meson install -C build
```

`./scripts/release.sh tarball` runs `meson dist` for you.

## 3. AppImage (fallback)

LCOS 0.3 already runs AppImages. The image bundles gtkmm from the build host.

```
./scripts/release.sh appimage
```

Requires `linuxdeploy` on `$PATH` (see <https://github.com/linuxdeploy/linuxdeploy>). Output lands under `dist/`.

```
chmod +x Ephemeris-*.AppImage ephemeris-*.AppImage
./ephemeris-*.AppImage
```

The AppImage runtime sets `APPDIR`; Ephemeris looks for skin and brand under `$APPDIR/usr/share/ephemeris`. Leave `APPDIR` unset for `.deb` and `meson install` builds.

## 4. Developer build (no install)

```
meson setup build
meson compile -C build
./build/ephemeris
```

The binary finds CSS via `SOURCE_ROOT` in the build tree. `EPHEMERIS_DATA` overrides that.

## One command for every artifact

```
./scripts/release.sh all
```

Writes tarball, `.deb`, and AppImage (if `linuxdeploy` is there) under `dist/`. The GitHub/Gitea release includes the AppImage as the non-deb fallback.

## What this project will not ship

- Mail, CalDAV, native `.ics`, or Lotus `.ORG`
- vCard import
- A systemd unit
- Vendored Clearlooks / xfwm themes
- Sample binders in the tarball (they stay git-only)
