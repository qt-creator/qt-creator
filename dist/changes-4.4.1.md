Qt Creator version 4.4.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v4.4.0..v4.4.1

FakeVim

* Fixed recognition of shortened `tabnext` and `tabprevious` commands
  (QTCREATORBUG-18843)

All Projects

* Fixed `Add Existing Files` for top-level project nodes (QTCREATORBUG-18896)

C++ Support

* Improved handling of parsing failures (QTCREATORBUG-18864)
* Fixed crash with invalid raw string literal (QTCREATORBUG-18941)
* Fixed that code model did not use sysroot as reported from the build system
  (QTCREATORBUG-18633)
* Fixed highlighting of `float` in C files (QTCREATORBUG-18879)
* Fixed `Convert to Camel Case` (QTCREATORBUG-18947)

Debugging

* Fixed that custom `solib-search-path` startup commands were ignored
  (QTCREATORBUG-18812)
* Fixed `Run in terminal` when debugging external application
  (QTCREATORBUG-18912)
* Fixed pretty printing of `CHAR` and `WCHAR`

Clang Static Analyzer

* Fixed options passed to analyzer on Windows

Qt Quick Designer

* Fixed usage of `shift` modifier when reparenting layouts

SCXML Editor

* Fixed eventless transitions (QTCREATORBUG-18345)

Test Integration

* Fixed test result output when debugging

Platform Specific

Windows

* Fixed auto-detection of CMake 3.9 and later

Android

* Fixed issues with new Android SDK (26.1.1) (QTCREATORBUG-18962)
* Fixed search path for QML modules when debugging

QNX

* Fixed debugging (QTCREATORBUG-18804, QTCREATORBUG-17901)
* Fixed QML profiler startup (QTCREATORBUG-18954)
