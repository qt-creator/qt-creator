Qt Creator version 4.7 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.6..v4.7.0

General

Editing

Help

All Projects

* Moved kit settings to separate options category
* Added shortcut for showing current document in project tree
  (QTCREATORBUG-19625)

CMake Projects

Qbs Projects

C++ Support

* Fixed auto-insertion of closing brace and semicolon after classes
  (QTCREATORBUG-19726)
* Clang Code Model
    * Implemented outline pane, outline dropdown and
      `C++ Symbols in Current Document` locator filter
* Built-in Code Model
    * Added support for nested namespaces (QTCREATORBUG-16774)

QML Support

* Updated parser to Qt 5.10, adding support for user-defined enums
* Fixed that linter warning `M127` was shown as error (QTCREATORBUG-19534)

Debugging

QML Profiler

* Improved performance of timeline

Qt Quick Designer

Version Control Systems

Diff Viewer

Image Viewer

* Added option to export SVGs in multiple resolutions simultaneously

Test Integration

Model Editor

Platform Specific

Windows

* Fixed that querying MSVC tool chains at startup could block Qt Creator

Android

Remote Linux

Credits for these changes go to:  
