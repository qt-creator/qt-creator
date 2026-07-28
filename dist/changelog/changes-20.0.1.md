Qt Creator 20.0.1
=================

Qt Creator version 20.0.1 contains bug fixes.
It is a free upgrade for all users.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=v20.0.0..v20.0.1>

General
-------

### Agent Client Protocol

Added

* More search paths for finding tools
  (like Homebrew on macOS and Chocolatey on Windows)
  ([QTCREATORBUG-34710](https://bugreports.qt.io/browse/QTCREATORBUG-34710))

Changed

* Improved the default working directory for sessions

Fixed

* That the tool button was not available for Markdown editors
  ([QTCREATORBUG-34724](https://bugreports.qt.io/browse/QTCREATORBUG-34724))

### Model Context Protocol

* Fixed a freeze when running projects on Windows
  ([QTCREATORBUG-34720](https://bugreports.qt.io/browse/QTCREATORBUG-34720))
* Fixed issues when a client requested an older protocol version even though
  that would be supported

Editing
-------

Fixed

* A crash when reloading large files

### C++

Fixed

* That the built-in model could freeze on completion on macOS
  ([QTCREATORBUG-34721](https://bugreports.qt.io/browse/QTCREATORBUG-34721))
* A crash when pre-processing specially compiled files like `.ui` and `.scxml`

### FakeVim

Fixed

* A freeze when using `w` on special characters
  ([QTCREATORBUG-25873](https://bugreports.qt.io/browse/QTCREATORBUG-25873))
* Painting the override cursor at the end of the text
  ([QTCREATORBUG-29553](https://bugreports.qt.io/browse/QTCREATORBUG-29553))
* That line numbers could be misaligned when showing them relative to the cursor
  ([QTCREATORBUG-26802](https://bugreports.qt.io/browse/QTCREATORBUG-26802))

Projects
--------

Fixed

* A crash when cloning a manual toolchain and applying without changes

### CMake

Fixed

* Presets
    * That the configure environment was lost after re-opening the project
      ([QTCREATORBUG-34763](https://bugreports.qt.io/browse/QTCREATORBUG-34763))
    * That Presets could not be used without a Qt version
      ([QTCREATORBUG-34764](https://bugreports.qt.io/browse/QTCREATORBUG-34764))
    * The handling of trace modes
      ([QTCREATORBUG-34762](https://bugreports.qt.io/browse/QTCREATORBUG-34762))
    * That Preset kits from other projects were selectable for a project
    * That vendor extensions for build configurations were missing
      ([QTCREATORBUG-34800](https://bugreports.qt.io/browse/QTCREATORBUG-34800))

### Workspace

Fixed

* That the executable and working directory for preset run configurations could
  not be relative

Debugging
---------

Fixed

* A crash when double-clicking a break point during shutdown
* A crash when stopping while a memory editor is open

### C++

Fixed

* Pretty printing Qt types for namespace Qt builds
  ([QTCREATORBUG-33211](https://bugreports.qt.io/browse/QTCREATORBUG-33211))

### QML

Fixed

* A crash when updating the Locals view
* A freeze when updating the Locals view
  ([QTCREATORBUG-34790](https://bugreports.qt.io/browse/QTCREATORBUG-34790))

Analyzer
--------

### Clang

Fixed

* Saving the selection of a custom configuration

### Axivion

Fixed

* The availability of starting a single file analysis

Version Control Systems
-----------------------

### Git

Fixed

* The interpretation of the file inclusion and exclusion patterns in the global
  `Files in File System` search
  ([QTCREATORBUG-33817](https://bugreports.qt.io/browse/QTCREATORBUG-33817))
* That `Show file at revision` failed when triggered from `Instant Blame`

### Perforce

Fixed

* An issue with jumping to the source location from diff views
  ([QTCREATORBUG-34680](https://bugreports.qt.io/browse/QTCREATORBUG-34680))
* That editor margins were not shown in diff views
  ([QTCREATORBUG-34680](https://bugreports.qt.io/browse/QTCREATORBUG-34680))

Platforms
---------

### macOS

Fixed

* That it was not possible to change the runtime app icon
  ([QTCREATORBUG-34718](https://bugreports.qt.io/browse/QTCREATORBUG-34718))
* Confusing output when running an application in an external terminal
  ([QTCREATORBUG-34775](https://bugreports.qt.io/browse/QTCREATORBUG-34775))

### Android

Fixed

* That the Android SDK settings marked the page as dirty even when no changes
  were made
  ([QTCREATORBUG-34263](https://bugreports.qt.io/browse/QTCREATORBUG-34263))

### Remote Linux

Fixed

* That the fallback deployment method could produce corrupted files
  ([QTCREATORBUG-34734](https://bugreports.qt.io/browse/QTCREATORBUG-34734))

### MCU

Fixed

* A crash when opening `.qmlproject` projects
  ([QTCREATORBUG-34714](https://bugreports.qt.io/browse/QTCREATORBUG-34714))

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Andre Hartmann  
André Pönitz  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Jaroslaw Kobus  
Leena Miettinen  
Marcus Tillmanns  
Orgad Shaneh  
