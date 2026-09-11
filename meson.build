project(
  'ephemeris',
  'cpp',
  version: '0.1.0',
  license: 'Unlicense',
  meson_version: '>=0.56',
  default_options: [
    'cpp_std=c++17',
    'warning_level=2',
    'prefix=/usr',
  ],
)

gtkmm = dependency('gtkmm-3.0')
libxml = dependency('libxml-2.0')

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('APP_ID', 'org.gmgauthier.Ephemeris')
conf.set_quoted('DATADIR', get_option('prefix') / get_option('datadir') / 'ephemeris')
conf.set_quoted('SOURCE_ROOT', meson.project_source_root())

configure_file(
  output: 'config.hpp',
  configuration: conf,
)

inc = include_directories('.', 'src')

executable(
  'ephemeris',
  'src/main.cpp',
  'src/application.cpp',
  'src/paths.cpp',
  'src/main_window.cpp',
  'src/about_dialog.cpp',
  'src/rings.cpp',
  'src/tab_strip.cpp',
  'src/month_page.cpp',
  'src/todo_page.cpp',
  'src/binder.cpp',
  'src/day_spread.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml],
  install: true,
  install_dir: get_option('bindir'),
)

install_data(
  'data/ephemeris.desktop',
  install_dir: get_option('datadir') / 'applications',
)

install_data(
  'data/skin/lcos/lcos.css',
  install_dir: get_option('datadir') / 'ephemeris' / 'skin' / 'lcos',
)

install_data(
  'brand/icon-tile.svg',
  install_dir: get_option('datadir') / 'ephemeris' / 'brand',
)

install_data(
  'brand/icon-tile.svg',
  rename: 'ephemeris.svg',
  install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
)
