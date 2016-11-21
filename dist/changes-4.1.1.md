Qt Creator version 4.1.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v4.1.0..v4.1.1

General

* Fixed issues with output pane height
  (QTCREATORBUG-15986, QTCREATORBUG-16829)

Editing

* Fixed performance of cleaning whitespace (QTCREATORBUG-16420)
* Fixed selection color in help viewer for dark theme (QTCREATORBUG-16375)

Help

* Fixed that no results could be shown in Locator (QTCREATORBUG-16753)

QMake Projects

* Fixed issue with make steps in deploy configurations (QTCREATORBUG-16795)

Qbs Projects

* Fixed handling of generated files (QTCREATORBUG-16976)

QML Support

* Fixed handling of circular dependencies (QTCREATORBUG-16585)

Debugging

* Fixed scrolling in memory editor (QTCREATORBUG-16751)
* Fixed expansion of items in tool tip (QTCREATORBUG-16947)
* GDB
    * Fixed handling of built-in pretty printers from new versions of GDB
      (QTCREATORBUG-16758)
    * Fixed that remote working directory was used for local process
      (QTCREATORBUG-16211)
* CDB
    * Fixed display order of vectors in vectors (QTCREATORBUG-16813)
    * Fixed display of QList contents (QTCREATORBUG-16750)
* QML
    * Fixed that expansion state was reset when stepping

QML Profiler

* Separated compile events from other QML/JS events in statistics and
  flamegraph, since compilation can happen asynchronously

Beautifier

* Fixed that beautifier was not enabled for Objective-C/C++ files
  (QTCREATORBUG-16806)

Platform Specific

macOS

* Fixed issue with detecting LLDB through `xcrun`

Android

* Added API level 24 for Android 7
* Fixed debugging on Android 6+ with NDK r11+ (QTCREATORBUG-16721)

iOS

* Fixed simulator support with Xcode 8 (QTCREATORBUG-16942)
  Known issue: Qt Creator is blocked until simulator finishes starting
* Fixed that standard paths reported by QStandardPaths were wrong when
  running on simulator (QTCREATORBUG-13655)
* Fixed QML debugging on device (QTCREATORBUG-15812)

QNX

* Fixed QML debugging (QTCREATORBUG-17208)
