Qt Creator 17.0.1
=================

Qt Creator version 17.0.1 contains bug fixes.
It is a free upgrade for all users.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=v17.0.0..v17.0.1>

General
-------

* Fixed a crash when Qt Creator is sent commands while it shuts down
  ([QTCREATORBUG-32673](https://bugreports.qt.io/browse/QTCREATORBUG-32673))
* Fixed that changing multiple settings that require a restart could lead to
  the menu bar to vanish
  ([QTCREATORBUG-32857](https://bugreports.qt.io/browse/QTCREATORBUG-32857),
   [QTCREATORBUG-33190](https://bugreports.qt.io/browse/QTCREATORBUG-33190))

Help
----

* Fixed that Qt for MCU documentation could be shown for desktop kits

Editing
-------

* Improved the performance of painting annotations
* Fixed a crash when closing an editor while the completion popup is shown
  ([QTCREATORBUG-33079](https://bugreports.qt.io/browse/QTCREATORBUG-33079))
* Fixed a crash when trying to open a file from history that has vanished

### C++

* ClangFormat
    * Fixed that the code style preview was not scrollable
      ([QTCREATORBUG-33104](https://bugreports.qt.io/browse/QTCREATORBUG-33104))

### QML

* Fixed a freeze while searching for QML resources
  ([QTCREATORBUG-33103](https://bugreports.qt.io/browse/QTCREATORBUG-33103))

### Diff

* Fixed that selecting a file from the toolbar did not jump to the file in the
  diff

### FakeVim

* Fixed that `Cmd+S` could act as `S` on macOS
  ([QTCREATORBUG-13392](https://bugreports.qt.io/browse/QTCREATORBUG-13392))

### EmacsKeys

* Fixed that the added text editor based actions no longer worked

Projects
--------

* Fixed that no macro expansion was done for the run environment
  ([QTCREATORBUG-33095](https://bugreports.qt.io/browse/QTCREATORBUG-33095))
* Fixed that `Run in terminal` did not keep the terminal open
  ([QTCREATORBUG-33102](https://bugreports.qt.io/browse/QTCREATORBUG-33102))
* Fixed that `Run as root user` was not available for custom executable run
  configurations
* Fixed a crash when writing files fails during wizard completion
  ([QTCREATORBUG-33110](https://bugreports.qt.io/browse/QTCREATORBUG-33110))
* Fixed a crash when closing a project while parsing is still running

### CMake

* Improved the order of targets in the `cm` locator filter
  ([QTCREATORBUG-33210](https://bugreports.qt.io/browse/QTCREATORBUG-33210))
* Fixed that too many files were filtered out of the project files list
  ([QTCREATORBUG-32095](https://bugreports.qt.io/browse/QTCREATORBUG-32095),
   [QTCREATORBUG-33152](https://bugreports.qt.io/browse/QTCREATORBUG-33152),
   [QTCREATORBUG-33163](https://bugreports.qt.io/browse/QTCREATORBUG-33163),
   [QTCREATORBUG-33180](https://bugreports.qt.io/browse/QTCREATORBUG-33180))
* Fixed that setting a non-existing toolchain file resulted in a successful
  configuration instead of the expected error
  ([QTCREATORBUG-33290](https://bugreports.qt.io/browse/QTCREATORBUG-33290))
* Fixed adding the new file to the project after
  `Move Component into Separate File`
  ([QTCREATORBUG-33298](https://bugreports.qt.io/browse/QTCREATORBUG-33298))

### qmake

* Fixed that it was not possible to provide an absolute path for the `mkspec`
  ([QTCREATORBUG-33155](https://bugreports.qt.io/browse/QTCREATORBUG-33155))

Debugging
---------

* Fixed a crash during shutdown
* Fixed that floating docks were not re-docked when resetting to default layout

Analyzer
--------

* Fixed the flame graph dropdown in dark styles
  ([QTCREATORBUG-33034](https://bugreports.qt.io/browse/QTCREATORBUG-33034))

### Axivion

* Fixed the display of freshly fetched issues from a local dashboard
  ([QTCREATORBUG-33012](https://bugreports.qt.io/browse/QTCREATORBUG-33012))
* Fixed the handling of path mappings
* Fixed that starting a local build was not possible in some configurations

Platforms
---------

### Linux

* Fixed a performance regression when selecting large amounts of text in editors
  ([QTCREATORBUG-33073](https://bugreports.qt.io/browse/QTCREATORBUG-33073))

### macOS

* Fixed that opening files from Finder only worked if Qt Creator was already
  running
  ([QTCREATORBUG-33164](https://bugreports.qt.io/browse/QTCREATORBUG-33164))
* Fixed that iOS toolchains could be selected for desktop kits

### Android

* Fixed that debugging C++ stopped in JIT code during startup
  ([QTCREATORBUG-33110](https://bugreports.qt.io/browse/QTCREATORBUG-33110))
* Fixed an issue with debugging on newer devices
  ([QTCREATORBUG-33203](https://bugreports.qt.io/browse/QTCREATORBUG-33203))

### iOS

* Fixed a crash when starting an app on the Simulator fails
  ([QTCREATORBUG-33189](https://bugreports.qt.io/browse/QTCREATORBUG-33189))

### Remote Linux

* Improved the error message when device tests fail
  ([QTCREATORBUG-32933](https://bugreports.qt.io/browse/QTCREATORBUG-32933))
* Fixed an issue with deployed files becoming corrupted
  ([QTCREATORBUG-33317](https://bugreports.qt.io/browse/QTCREATORBUG-33317))
* Fixed that the command bridge could be killed when operations take a long time

Credits for these changes go to:
--------------------------------
Alexandre Laurent  
André Pönitz  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Jaroslaw Kobus  
Krzysztof Chrusciel  
Leena Miettinen  
Lukasz Papierkowski  
Marc Mutz  
Marcus Tillmanns  
Nicholas Bennett  
Orgad Shaneh  
Prashant Vaibhav  
Sami Shalayel  
