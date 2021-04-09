Qt Creator 4.15
===============

Qt Creator version 4.15 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/4.14..v4.15.0

General
-------

* Added locator filter for global file index on Linux (`locate`) and Windows
  (`Everything`)
* Added option for globally changing base environment for running tools
  (QTCREATORBUG-22123)
* Added option for text codec used for tools (QTCREATORBUG-24776)
* Fixed that `General Messages` pane popped up too often (QTCREATORBUG-24667)

Help
----

* Added shared `Zoom` setting (QTCREATORBUG-23731, QTCREATORBUG-25109,
  QTCREATORBUG-25230)

Editing
-------

* Added action for pasting without auto-formatting (QTCREATORBUG-20887)
* Fixed that completion could block Qt Creator (QTCREATORBUG-25419)

### C++

* Added `Create Getter and Setter Member Functions` refactoring action
  (QTCREATORBUG-1532)
* Added `Generate Constructor` refactoring action
* Added filtering of `Find References to Symbol Under Cursor` based on access
  type (QTCREATORBUG-19373)
* Added `Open in Editor` and `Open Type Hierarchy` to context menu on items in
  type hierarchy
* Added highlighting of previous class when navigating in type hierarchy
* Added type aliases to `C++ Classes, Enums and Functions` locator filter
  (QTCREATORBUG-5800)
* Added parentheses highlighting for ternary operator (QTCREATORBUG-1410)
* Improved type name minimization for `Add definition` (QTCREATORBUG-8030)
* Fixed type hierarchy with templates classes and typedefs
* Fixed that `-include` compile option was ignored by code model
  (QTCREATORBUG-20602)
* Fixed highlighting of raw string literals (QTCREATORBUG-16183)
* Fixed issue with declaration and definition matching in presence of macros
  (QTCREATORBUG-24739)
* Fixed issue with struct type alias (QTCREATORBUG-24875)
* Fixed issue with function attributes (QTCREATORBUG-24650, QTCREATORBUG-24636)
* Fixed highlighting in macros with indirection (QTCREATORBUG-21522)
* Fixed highlighting in multi-dimensional arrays (QTCREATORBUG-21534)
* Fixed switching between declaration and definition for custom conversion
  operators (QTCREATORBUG-21168)
* Fixed that fix-its with outdated information could be applied
  (QTCREATORBUG-21818)
* Fixed tooltip for some include directives (QTCREATORBUG-21194)
* Fixed include completion for files with non-standard file extensions
  (QTCREATORBUG-25154)
* Fixed highlighting of comments with continuation lines (QTCREATORBUG-23297)
* Fixed issues with `Add definition` (QTCREATORBUG-14661, QTCREATORBUG-14524,
  QTCREATORBUG-14524, QTCREATORBUG-25560)
* Fixed real-time updating of `Class View`
* Fixed that function parameter hint showed inapplicable overloads
  (QTCREATORBUG-650)

### QML

* Added support for inline components (QTCREATORBUG-24766, QTCREATORBUG-24705)
* Fixed issues with multiple import paths (QTCREATORBUG-24405)
* Fixed reformatting of arrow functions (QTCREATORBUG-25198)
* Fixed reformatting of JavaScript spread operator (QTCREATORBUG-23402)

### Language Client

* Added support for new formatting options in LSP 3.15.0
* Added support for versioned diagnostics
* Added support for server progress messages
* Improved Java language server support

### Java

* Simplified configuration of Java language server
* Improved support for Java language server

Projects
--------

* Added `Open Terminal Here` for project nodes (QTCREATORBUG-25107)
* Added option for running application as root user (QTCREATORBUG-2831,
  QTCREATORBUG-25330)
* Fixed detection of `rcc` and `uic` for Qt 6 (QTBUG-88791)
* Fixed detection of Designer, Linguist, `qmlscene` and `qmlplugindump` for Qt 6
  cross-builds

### qmake

* Fixed freeze when executable run with `system` call waits for input
  (QTCREATORBUG-25194)

### CMake

* Added support for multiconfig generators (QTCREATORBUG-24984)
* Added filesystem node to project tree (QTCREATORBUG-24677)
* Added `install/strip` and `package` targets (QTCREATORBUG-22047,
  QTCREATORBUG-22620)
* Added automatic run of conan install on initial CMake call
  (QTCREATORBUG-25362)
* Added batch editing for CMake configuration
* Added `Re-configure with Initial Parameters` button
* Made it possible to copy CMake variables from configuration
  (QTCREATORBUG-24781)
* Removed utility targets from CMake target locator filters (QTCREATORBUG-24718)
* Fixed that configuration changes were lost when CMake configuration fails
  (QTCREATORBUG-24593)
* Fixed Qt detection when importing builds of Qt6-based projects
  (QTCREATORBUG-25100)
* Fixed importing builds of Qt6 tests (QTBUG-88776)
* Fixed which file is opened for `Open CMake target` locator filter
  (QTCREATORBUG-25166)
* Fixed `Save all files before build` for `Build for Run Configuration`
  (QTCREATORBUG-25276)
* Fixed that only source file name was copied to clipboard when adding class
  (QTCREATORBUG-24301, QTCREATORBUG-25212)
* Fixed reparsing of project with `Auto-run CMake`
* Fixed that removed targets stayed selected for building (QTCREATORBUG-25477)

### Qbs

* Added Android target ABI selection

### Python

* Added support for PySide6 to wizards (QTCREATORBUG-25340)

### Meson

* Added support for `extra_files` (QTCREATORBUG-24824)
* Added support for custom Meson parameters

### Conan

* Added auto-detection of conan file in project root

Debugging
---------

* Added option to show simple values as text annotations
* Added option to copy selected items from stack view (QTCREATORBUG-24701)
* Added visualization of hit breakpoint in `Breakpoints` view
  (QTCREATORBUG-6999)
* Fixed type display for automatically dereferenced pointers
  (QTCREATORBUG-20907)
* Fixed that debugging repeatedly stopped with `SIGSTOP` (QTCREATORBUG-25073,
  QTCREATORBUG-25082, QTCREATORBUG-25227)

### QML

* Fixed breakpoints in `.mjs` files (QTCREATORBUG-25328)

Analyzer
--------

### Clang

* Added option for disabling diagnostic types from result list
  (QTCREATORBUG-24852)
* Added support for individual `clazy` check options (QTCREATORBUG-24977)
* Added help link to diagnostic tooltip (QTCREATORBUG-25163)

Version Control Systems
-----------------------

* Added simple commit message verification

Test Integration
----------------

* Added basic support for `ctest` (QTCREATORBUG-23332)

### Google Test

* Fixed detection of tests that start with a number (QTCREATORBUG-25498)

FakeVim
-------

* Added support for `\u` `\U` `\l` `\L` in substitute command
* Added emulation of `vim-exchange` and `vim-surround` plugins
* Fixed dot command for pasting (QTCREATORBUG-25281)

Platforms
---------

### Android

* Fixed `android-*-deployment-settings.json` detection (QTCREATORBUG-25209)

### iOS

* Added support for CMake projects with Qt 6 (QTCREATORBUG-23574)
* Fixed launch of applications on iOS 14 (QTCREATORBUG-24672)
* Fixed `Attach to Running Application` for long executable paths
  (QTCREATORBUG-25286)

### Remote Linux

* Fixed issues with remote process PID parsing (QTCREATORBUG-25306)
* Fixed issues with `rsync` on Windows (QTCREATORBUG-25333)

### MCU

* Added error reporting when creating MCU kits fails (QTCREATORBUG-25258)
* Improved detection of Ultralight library (QTCREATORBUG-25469)
* Fixed that examples were missing from `Welcome` screen (QTCREATORBUG-25320)

### WASM

* Improved handling of Emscripten detection and setup (QTCREATORBUG-23126,
  QTCREATORBUG-23160, QTCREATORBUG-23561, QTCREATORBUG-23741,
  QTCREATORBUG-24814, QTCREATORBUG-24822)
* Added support for Qt 6 applications with CMake (QTCREATORBUG-25519)
* Fixed ABI detection for Qt 5.15 (QTCREATORBUG-24891)
* Fixed running of `em++.bat` in some environments on Windows

Credits for these changes go to:
--------------------------------
Aleksei German  
Alessandro Portale  
Alexandru Croitor  
Alexis Jeandet  
Allan Sandfeld Jensen  
Andre Hartmann  
André Pönitz  
Bernhard Beschow  
Björn Schäpers  
BogDan Vatra  
Christiaan Janssen  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Dawid Śliwa  
Denis Shienkov  
Dmitriy Purgin  
Eike Ziller  
Erik Verbruggen  
Fabio Falsini  
Fawzi Mohamed  
Friedemann Kleint  
Henning Gruendl  
Jacopo Martellini  
Jaroslaw Kobus  
Johanna Vanhatapio  
Kai Köhne  
Kevin Funk  
Knud Dollereder  
Leander Schulten  
Leena Miettinen  
Mahmoud Badri  
Marco Bubke  
Matti Paaso  
Mattias Johansson  
Maximilian Goldstein  
Michael Weghorn  
Michael Winkelmann  
Miikka Heikkinen  
Miina Puuronen  
Mitch Curtis  
Nikolai Kosjar  
Orgad Shaneh  
Oswald Buddenhagen  
Raphaël Cotty  
Robert Löhning  
Sergey Levin  
Thomas Hartmann  
Tim Jenssen  
Timon Riedelbauch  
Tom Praschan  
Tuomo Pelkonen  
Ulf Hermann  
Vikas Pachdha  
