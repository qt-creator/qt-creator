Qt Creator version 4.0.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v4.0.0..v4.0.1

CMake Projects

* Added notification when `CMakeCache.txt` changes and introduces a
  conflict with the build configuration settings, with the option
  to adapt the build configuration settings
* Made it possible to add arbitrary CMake variables (QTCREATORBUG-16238)
* Fixed that build configurations could not override kit settings, and added
  a warning to build configurations that override kit settings
* Fixed that `yes` was not considered as boolean `true` value
* Fixed race between persisting and parsing CMake configuration
  (QTCREATORBUG-16258)

QML Support

* Added wizard for Qt Quick Controls 2
* Fixed that `pragma` directives were removed when reformatting

Debugging

* Fixed QObject property expansion (QTCREATORBUG-15798)
* Fixed updating evaluated expressions
* Fixed crash on spontaneous debugger exit (QTCREATORBUG-16233)
* GDB
    * Fixed issues with restarting debugger (QTCREATORBUG-16355)
* QML
    * Restored expression evaluation by using the selection tool
      (QTCREATORBUG-16300)

Valgrind

* Fixed crash when changing filter

Clang Static Analyzer

* Fixed executing Clang with absolute path on Windows (QTCREATORBUG-16234)

Test Integration

* Fixed running tests on Windows with Qbs (QTCREATORBUG-16323)

Beautifier

* Fixed regression with `clang-format` and predefined style set to `File`
  (QTCREATORBUG-16239)

Platform Specific

Windows

* Fixed detection of Microsoft Visual C++ Build Tools
* Fixed that tool tips could stay visible even after switching applications
  (QTCREATORBUG-15882)

iOS

* Added missing human readable error messages (QTCREATORBUG-16328)
