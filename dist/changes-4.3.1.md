Qt Creator version 4.3.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v4.3.0..v4.3.1

General

* Fixed that wizards overwrote existing files even when told not to do so
  (QTCREATORBUG-18284)

Editing

* Fixed memory leak in code completion (QTCREATORBUG-18326)

All Projects

* Fixed that links in `Application Output` stopped working after application
  stops (QTCREATORBUG-18134)
* Fixed that `Application Output` was no longer editable (QTCREATORBUG-18418)

QMake Projects

* Fixed `Add Library` (QTCREATORBUG-18263)

CMake Projects

* Fixed crash when restoring session with multiple CMake projects
  (QTCREATORBUG-18258)
* Fixed that `test` target was missing (QTCREATORBUG-18323)
* Fixed that `STATIC` and `INTERNAL` variables were shown in project
  configuration
* Fixed that CMake `message`s were not shown in `Issues` pane
  (QTCREATORBUG-18318)
* Fixed issues with CMake variables that contain `//` or `#`
  (QTCREATORBUG-18385)
* Fixed that deployment information could contain empty items
  (QTCREATORBUG-18406)
* Fixed that targets were duplicated when importing project (QTCREATORBUG-18409)
* Fixed that building application failed first time and after build error
  when using CMake < 3.7 (QTCREATORBUG-18290, QTCREATORBUG-18382)

Qbs Projects

* Fixed crash when renaming files (QTCREATORBUG-18440)

Autotools Projects

* Fixed regressions in project tree (QTCREATORBUG-18371)

C++ Support

* Fixed crash when requesting refactoring operations on invalid code
  (QTCREATORBUG-18355)

QML Support

* Fixed crash when changing kit environment (QTCREATORBUG-18335)

Valgrind

* Fixed crash when running analyzer for iOS and Android (QTCREATORBUG-18254)

Version Control Systems

* Fixed filtering of untracked files in commit editor
  when multiple projects are open
* Git
    * Fixed that ref names were missing for `Show`
* Mercurial
    * Fixed extra options in diff and log (QTCREATORBUG-17987)
* Gerrit
    * Fixed parsing output from Gerrit 2.14

Test Integration

* Fixed that changing QML file triggered full rescan for tests
  (QTCREATORBUG-18315)
* Fixed issues with multiple build targets
  (QTCREATORBUG-17783, QTCREATORBUG-18357)

Platform Specific

Windows

* Fixed checking whether example should be copied to writable location
  (QTCREATORBUG-18184)
* Fixed issues with MSVC2017 and CMake (QTCREATORBUG-17925)

macOS

* Fixed performance issue on HiDPI displays (QTBUG-61384)

WinRT

* Fixed running MSVC 2017 based applications (QTCREATORBUG-18288)

Android

* Fixed detection of MIPS64 toolchains
* Fixed that 64-bit ABIs were missing in AVD creation dialog

iOS

* Fixed running on iOS 10.3 devices (QTCREATORBUG-18380)
* Fixed crash that could occur at startup while device is connected
  (QTCREATORBUG-18226)

BareMetal

* Fixed crash on shutdown
