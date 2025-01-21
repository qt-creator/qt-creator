Qt Creator 15.0.1
=================

Qt Creator version 15.0.1 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=v15.0.0..v15.0.1>

General
-------

* Fixed the loading of plugins from the user-specific plugin directory
* Fixed a crash when selecting a dynamic library directly in the plugin
  installation wizard
* Fixed the extraction of plugins from or to paths with spaces

Help
----

* Fixed opening links to Academy
  ([QTCREATORBUG-32134](https://bugreports.qt.io/browse/QTCREATORBUG-32134))

Editing
-------

* Fixed a crash when editors are closed while the document switching window
  (`Ctrl+Tab`) is open
* Fixed a missing re-highlighting after the color scheme is changed
  ([QTCREATORBUG-32116](https://bugreports.qt.io/browse/QTCREATORBUG-32116))

### C++

* Fixed the `Move Definition` refactoring action for template methods
  ([QTCREATORBUG-31678](https://bugreports.qt.io/browse/QTCREATORBUG-31678))
* Clangd
    * Updated prebuilt binaries to LLVM 19.1.6
    * Fixed a potential crash when canceling indexing
* Built-in
    * Fixed a wrong warning for range-based `for` loops
      ([QTCREATORBUG-32043](https://bugreports.qt.io/browse/QTCREATORBUG-32043))

### QML

* qmlls
    * Turned off by default because of issues with QML modules
    * Fixed that triggering refactoring actions was not available with
      shortcut and Locator
      ([QTCREATORBUG-31977](https://bugreports.qt.io/browse/QTCREATORBUG-31977))
    * Fixed that auto-completion sometimes did not appear
      ([QTCREATORBUG-32013](https://bugreports.qt.io/browse/QTCREATORBUG-32013))
    * Fixed that a client handler could be created for empty executable paths

Projects
--------

* Fixed the GitHub action created by the Qt Creator plugin wizard
  ([QTCREATORBUG-32090](https://bugreports.qt.io/browse/QTCREATORBUG-32090))

### CMake

* Fixed that user-defined `UTILITY` targets were missing from Locator
  ([QTCREATORBUG-32080](https://bugreports.qt.io/browse/QTCREATORBUG-32080))
* Fixed a potential crash with CMake Presets
* Fixed an issue after updating MSVC
  ([QTCREATORBUG-32165](https://bugreports.qt.io/browse/QTCREATORBUG-32165))
* Fixed that `CMakeLists.txt` could not be found when adding files when
  the `FOLDER` property was used
  ([QTCREATORBUG-32194](https://bugreports.qt.io/browse/QTCREATORBUG-32194))
* Fixed that a rebuild of an imported project could be forced
  ([QTCREATORBUG-32196](https://bugreports.qt.io/browse/QTCREATORBUG-32196))
* Fixed an issue with watching for external CMake changes
  ([QTCREATORBUG-31536](https://bugreports.qt.io/browse/QTCREATORBUG-31536))
* Fixed that files were wrongly marked as generated
  ([QTCREATORBUG-32283](https://bugreports.qt.io/browse/QTCREATORBUG-32283),
   [QTCREATORBUG-32298](https://bugreports.qt.io/browse/QTCREATORBUG-32298))
* Conan
    * Fixed the loading of projects that specify a `CMakeDeps` generator
      ([QTCREATORBUG-32076](https://bugreports.qt.io/browse/QTCREATORBUG-32076))

### Python

* Fixed that simple output from `print` was added to the `Issues` view
  ([QTCREATORBUG-32214](https://bugreports.qt.io/browse/QTCREATORBUG-32214))

### Autotools

* Fixed the initial project parsing
  ([QTCREATORBUG-32305](https://bugreports.qt.io/browse/QTCREATORBUG-32305))

Debugging
---------

* Fixed a crash when enabling or disabling all breakpoints with the context menu
* Fixed issues with big endian targets

Analyzer
--------

### Clang

* Clang-Tidy
    * Fixed that the required compilation database was not created
      ([QTCREATORBUG-32098](https://bugreports.qt.io/browse/QTCREATORBUG-32098))
    * Fixed an issue with deselecting checks
      ([QTCREATORBUG-32147](https://bugreports.qt.io/browse/QTCREATORBUG-32147))

### Axivion

* Fixed that settings changes were automatically applied
  ([QTCREATORBUG-32078](https://bugreports.qt.io/browse/QTCREATORBUG-32078))
* Fixed the handling of projects with special characters in their name
  ([QTCREATORBUG-32091](https://bugreports.qt.io/browse/QTCREATORBUG-32091))

Terminal
--------

* Fixed the reuse of terminal tabs on macOS
  ([QTCREATORBUG-32197](https://bugreports.qt.io/browse/QTCREATORBUG-32197))
* Fixed a freeze when closing a terminal on Windows
  ([QTCREATORBUG-32192](https://bugreports.qt.io/browse/QTCREATORBUG-32192))
* Fixed that terminals automatically scrolled to the end also when not at the
  bottom
  ([QTCREATORBUG-32167](https://bugreports.qt.io/browse/QTCREATORBUG-32167))

Version Control Systems
-----------------------

### Git

* Worked around a potential crash in the Branches view

Test Integration
----------------

### Boost

* Fixed a potential crash when accessing the C++ snapshot

Platforms
---------

### Android

* Fixed a potential crash after reloading packages

### Remote Linux

* Fixed the working directory for `gdbserver`
  ([QTCREATORBUG-32122](https://bugreports.qt.io/browse/QTCREATORBUG-32122))

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Andre Hartmann  
André Pönitz  
Andrii Semkiv  
Artur Twardy  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Jaroslaw Kobus  
Krzysztof Chrusciel  
Leena Miettinen  
Liu Zhangjian  
Lukasz Papierkowski  
Marcus Tillmanns  
Orgad Shaneh  
Patryk Stachniak  
Robert Löhning  
Sami Shalayel  
Tim Blechmann  
Ville Lavonius  
