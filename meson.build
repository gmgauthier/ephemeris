project(
  'ephemeris',
  'cpp',
  version: '1.1.13',
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
libsoup = dependency('libsoup-3.0')

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
  'src/contacts_page.cpp',
  'src/planner_page.cpp',
  'src/notepad_page.cpp',
  'src/vcard.cpp',
  'src/mention.cpp',
  'src/binder.cpp',
  'src/day_spread.cpp',
  'src/settings.cpp',
  'src/ics.cpp',
  'src/fetch.cpp',
  'src/remote_cal.cpp',
  'src/subscribe_dialog.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml, libsoup],
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

ephemeris_tests = executable(
  'ephemeris-tests',
  'tests/test_calendar.cpp',
  'src/ics.cpp',
  'src/vcard.cpp',
  'src/binder.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml],
)
test('calendar', ephemeris_tests)

ephemeris_byday_tests = executable(
  'ephemeris-byday-tests',
  'tests/test_byday.cpp',
  'src/ics.cpp',
  'src/vcard.cpp',
  'src/binder.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml],
)
test('byday', ephemeris_byday_tests)

ephemeris_span_tests = executable(
  'ephemeris-span-tests',
  'tests/test_span.cpp',
  'src/ics.cpp',
  'src/vcard.cpp',
  'src/binder.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml],
)
test('span', ephemeris_span_tests)

ephemeris_allday_tests = executable(
  'ephemeris-allday-tests',
  'tests/test_allday.cpp',
  'src/ics.cpp',
  'src/vcard.cpp',
  'src/binder.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml],
)
test('allday', ephemeris_allday_tests)

ephemeris_window_tests = executable(
  'ephemeris-window-tests',
  'tests/test_window.cpp',
  'src/ics.cpp',
  'src/vcard.cpp',
  'src/binder.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml],
)
test('window', ephemeris_window_tests)

ephemeris_weekly_tests = executable(
  'ephemeris-weekly-tests',
  'tests/test_weekly.cpp',
  'src/ics.cpp',
  'src/vcard.cpp',
  'src/binder.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml],
)
test('weekly', ephemeris_weekly_tests)

ephemeris_monthday_tests = executable(
  'ephemeris-monthday-tests',
  'tests/test_monthday.cpp',
  'src/ics.cpp',
  'src/vcard.cpp',
  'src/binder.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml],
)
test('monthday', ephemeris_monthday_tests)

ephemeris_params_tests = executable(
  'ephemeris-params-tests',
  'tests/test_params.cpp',
  'src/ics.cpp',
  'src/vcard.cpp',
  'src/binder.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml],
)
test('params', ephemeris_params_tests)

ephemeris_case_tests = executable(
  'ephemeris-case-tests',
  'tests/test_case.cpp',
  'src/ics.cpp',
  'src/vcard.cpp',
  'src/binder.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml],
)
test('casefold', ephemeris_case_tests)

ephemeris_vcard_tests = executable(
  'ephemeris-vcard-tests',
  'tests/test_vcard.cpp',
  'src/ics.cpp',
  'src/vcard.cpp',
  'src/binder.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml],
)
test('vcard', ephemeris_vcard_tests)

ephemeris_recur_tests = executable(
  'ephemeris-recur-tests',
  'tests/test_recur.cpp',
  'src/ics.cpp',
  'src/vcard.cpp',
  'src/binder.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml],
)
test('recur', ephemeris_recur_tests)

ephemeris_date_tests = executable(
  'ephemeris-date-tests',
  'tests/test_date.cpp',
  'src/ics.cpp',
  'src/vcard.cpp',
  'src/binder.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml],
)
test('date', ephemeris_date_tests)
