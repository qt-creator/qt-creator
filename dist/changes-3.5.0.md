Qt Creator version 3.5 contains bug fixes and new features.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/3.4..origin/3.5

General

* Increased minimum requirements for compilation of Qt Creator to
  MSVC 2013 and GCC 4.7, and Qt 5.4.0
* Added variants with native separators to Qt Creator variables that
  represent file paths
* Changed the way inconsistent enabled states were handled by the
  plugin manager. Disabling plugins is now only a hint; if another
  (enabled) plugin needs it, it is implicitly enabled. Before, the
  other plugin was implicitly disabled.
* Added support for `~` as shortcut for user's home directory to
  path input fields
* Added filtering to About Plugins
* Added `-load all` and `-noload all` command line options that
  enable and disable all plugins respectively
* Made `-load` command line option implicitly enable all required
  plugins, and `-noload` disable all plugins requiring the
  disabled plugin. Multiple `-load` and `-noload` options are
  interpreted in the order given on the command line.
* Fixed issues with raising the Qt Creator window on Gnome desktop
  (QTCREATORBUG-13845)
* Fixed appearance on high DPI displays on Windows and Linux
  (QTCREATORBUG-11179)

Editing

* Added option to jump directly to specific column in addition to
  line number when opening files through locator or command line
* Made global file search use multiple threads (QTCREATORBUG-10298)

Help

QMake Projects

CMake Projects

* Made it possible to register multiple CMake executables
* Fixed default shadow build directory name

Qbs Projects

QML-Only Projects (.qmlproject)

Debugging

Analyzer

QML Profiler

* Made saving and loading trace data asynchronous to avoid
  locking up UI (QTCREATORBUG-11822)

C++ Support

* Added separate icon for structs

QML Support

Version Control Systems

FakeVim

* Added support for `C-r{register}`

Platform Specific

Windows

OS X

* Added locator filter that uses Spotlight for locating files

Linux

Android

* Made it possible to create AVD without SD card (QTCREATORBUG-13590)
* Added warning if emulator is not OpenGL enabled
  (QTCREATORBUG-13615)
* Fixed listing of Google AVDs (QTCREATORBUG-13980)

Remote Linux

BareMetal

Credits for these changes go to:
