Qt Creator 10
=============

Qt Creator version 10 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/9.0..v10.0.0

General
-------

* Added support for temporarily dragging progress details out of the way
  (QTCREATORBUG-28078)

Editing
-------

* Added `Follow Symbol` for `http(s)` string literals (QTCREATORBUG-14967)
* Added environment expansion to file system locator filter (QTCREATORBUG-242)
* Added `Temporarily hide inline annotations` for types of annotations
* Improved cursor visibility with indentation visualization (QTCREATORBUG-28645)
* Fixed editor scrolling when pressing backspace (QTCREATORBUG-28316)
* Fixed performance of rendering many search results (QTCREATORBUG-21056)

### C++

* Added renaming of includes when renaming `.ui` files (QTCREATORBUG-14259)
* Added automatic refactoring of C++ code when forms in `.ui` files are renamed
  (QTCREATORBUG-1179)
* Added option for ignoring files for indexing (QTCREATORBUG-28313)
* Added `Tools > C++ > Find Unused Functions`, and `Find Unused C/C++ Functions`
  to the project context menu (QTCREATORBUG-6772)
* Fixed text codec when rewriting headers as part of renaming
  (QTCREATORBUG-28164)
* Fixed color of whitespace visualization in string literals
  (QTCREATORBUG-26693, QTCREATORBUG-28284)
* Fixed `Move Definition` for template functions (QTCREATORBUG-28186)
* Clangd
    * Made temporary disabling of global indexing possibly by cancelling in the
      progress indicator
    * Added support for highlighting angle brackets
* Built-in
    * Added support for spaceship operator (QTCREATORBUG-27503)
    * Fixed handling of `= default` (QTCREATORBUG-28102)
* ClangFormat
    * Enabled by default
    * Added project setting for `ClangFormat` (QTCREATORBUG-28188)

### Language Server Protocol

* Added `Restart` action to menu in editor tool bar
* Added `Call Hierarchy` (QTCREATORBUG-11660)

### QML

* Updated code model to Qt 6.5
* Added experimental support for QML language server
* Added color preview tooltip (QTCREATORBUG-28446)
* Added option for applying `qmlformat` on file save (QTCREATORBUG-28192,
  QTCREATORBUG-26602)
* Added `Follow Symbol` for QRC paths in string literals (QTCREATORBUG-28087)
* Adapted wizard to new features in Qt 6.4 and Qt 6.5 (QTBUG-47996)
* Fixed freeze when closing file (QTCREATORBUG-28206)
* Fixed that `QtObject` was not recognized (QTCREATORBUG-28287,
  QTCREATORBUG-28375)

### Python

* Added interpreter selector to editor toolbar (PYSIDE-2154)

Projects
--------

* Added `Build > Run Generator` for exporting projects to other build systems
  (QTCREATORBUG-28149)
* Added option for browsing remote file system for remote builds and targets
* Added support for opening remote terminal
* Fixed that wizards did not create target directories (QTCREATORBUG-28346)
* Fixed that absolute paths could be shown when relative paths would be
  preferable (QTCREATORBUG-288)

### CMake

* Added deployment method with `cmake --install` (QTCREATORBUG-25880)
* Added option for using `cmake-format` for CMake files
  ([cmake-format Documentation](https://cmake-format.readthedocs.io/en/latest/))
* Added option for showing advanced configure items by default
* Moved `Autorun CMake` to global settings
* Changed environment for running CMake to be based on build environment by
  default (QTCREATORBUG-28513)
* Fixed that `Package manager auto setup` created dependency of project build to
  Qt Creator installation

### Qbs

* Added `Profile` build variant (QTCREATORBUG-27206)
* Fixed that generated files were not made known to the code model

### Python

* Removed wizard for dynamically loaded `.ui` projects (QTCREATORBUG-25807)

### Qt Quick UI Prototype

* Added support for running on remote Linux devices

Debugging
---------

### C++

* Added pretty printers for `variant`, `optional` and `tuple` from `libcpp`
  (QTCREATORBUG-25865)
* Fixed highlighting in disassembly view
* Fixed skipping `std::function` details when stepping

Analyzer
--------

### Clang

* Split `Clang-Tidy and Clazy` into separate `Clang-Tidy` and `Clazy` analyzers

Version Control Systems
-----------------------

* Moved support for `Fossil` SCM into mainline repository
* Removed settings for prompting to submit (QTCREATORBUG-22233)
* Added links to file names in diff output (QTCREATORBUG-27309)
* Fixed blame on symbolic links (QTCREATORBUG-20792)
* Fixed saving of files before applying action on chunks (QTCREATORBUG-22506)
* Fixed line ending preservation when reverting chunks (QTCREATORBUG-12690)

### Git

* Improved tracking of external changes (QTCREATORBUG-21089)
* Added editor annotation for blame information (instant blame), with setting
  (opt-out), and action to show it manually for the current line
  (QTCREATORBUG-23299)

Test Integration
----------------

* Improved `Run` and `Debug Test Under Cursor` (QTCREATORBUG-28496)
* Improved number of files that are scanned for tests
* Improved output handling (QTCREATORBUG-28706)
* Made expensive checking for tests in derived `TestCase` objects optional

Platforms
---------

### macOS

* Changed kits to prefer Xcode toolchain over the wrappers in `/bin`

### Android

* Removed service management from manifest editor (QTCREATORBUG-28024)

### Remote Linux

* Fixed that opening file dialog unnecessarily queried for password for remote
  devices

### Docker

* Added support for remote code model via remote Clangd
* Added support for loading and attaching to core dumps from remote devices
* Added support for using ClangFormat on remote files
* Added option to enable necessary capabilities for debugging with LLDB
* Fixed issue with space in file paths (QTCREATORBUG-28476)
* Fixed that auto-detection controls were shown for devices registered by the
  installer

Credits for these changes go to:
--------------------------------
Aleksei German  
Alessandro Portale  
Alexander Pershin  
Ali Kianian  
Amr Essam  
Andre Hartmann  
André Pönitz  
Antti Määttä  
Artem Sokolovskii  
Artur Shepilko  
Assam Boudjelthia  
BogDan Vatra  
Burak Hancerli  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
Cristián Maureira-Fredes  
David Schulz  
Dmitry Bravikov  
Eike Ziller  
Fabian Kosmale  
Fawzi Mohamed  
Henning Gruendl  
Jaroslaw Kobus  
Jussi Witick  
Kai Köhne  
Knud Dollereder  
Knut Petter Svendsen  
Leena Miettinen  
Łukasz Wojniłowicz  
Mahmoud Badri  
Marc Mutz  
Marco Bubke  
Marcus Tillmanns  
Mats Honkamaa  
Miikka Heikkinen  
Mikhail Khachayants  
Orgad Shaneh  
Oswald Buddenhagen  
Philip Van Hoof  
Pranta Dastider  
Robert Löhning  
Sami Shalayel  
Samuel Ghinet  
Sergey Levin  
Sivert Krøvel  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
Tomáš Juřena  
Topi Reinio  
Ulf Hermann  
Vikas Pachdha  
Xavier Besson  
Yasser Grimes  
