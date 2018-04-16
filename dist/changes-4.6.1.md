Qt Creator version 4.6.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v4.6.0..v4.6.1

General

* Locator
    * Fixed min and max functions when using more than two arguments
      in JavaScript filter (QTCREATORBUG-20167)

Editing

* Fixed crash when closing file with generic highlighting (QTCREATORBUG-20247)

All Projects

* Fixed that `.qrc` files were not listed as project files in Locator and
  searches (QTCREATORBUG-20220)

QMake Projects

* Fixed that run and build buttons could stay disabled after project parsing
  (QTCREATORBUG-20203)

CMake Projects

* Fixed that build steps for `clean` were missing (QTCREATORBUG-19823)

Qbs Projects

* Fixed performance issue (QTCREATORBUG-20175)

C++ Support

* Clang Code Model
    * Fixed issue with parsing type_traits from GCC 7 (QTCREATORBUG-18757)
    * Fixed warnings about unknown warning options (QTCREATORBUG-17460)
    * Fixed wrong warning about overriding files from precompiled headers
      (QTCREATORBUG-20125)

QML Support

* Made Qt 5.11 known to wizards

Debugging

* Fixed pointer address value for arrays
* QML
    * Fixed that `console.info` was not shown in Debugger Console
      (QTCREATORBUG-20117)

QML Profiler

* Fixed issue with spaces in path (QTCREATORBUG-20260)

Qt Quick Designer

* Fixed issue with `AbstractButton` enums
* Fixed issue with deferred properties

Test Integration

* Fixed issue with non-ASCII characters in Qt Quick test output
  (QTCREATORBUG-20105)

Platform Specific

Android

* Fixed deployment issue for 32-bit applications (QTCREATORBUG-20084)
* Fixed crash when trying to set up a broken NDK (QTCREATORBUG-20217)
* Fixed debugging on Android 8 or later
