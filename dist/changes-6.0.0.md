Qt Creator 6
===============

Qt Creator version 6 contains bug fixes and new features.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/5.0..v6.0.0

General
-------

* Moved launching of tools to external process
* Merged `CppTools` plugin into `CppEditor` plugin

Editing
-------

* Added support for multiple cursor editing (QTCREATORBUG-16013)
* Added import and export for font settings (QTCREATORBUG-6833)
* Fixed missing permissions update when files change (QTCREATORBUG-22447)

### C++

* Updated to LLVM 13
* Added option for saving open files automatically after refactoring
  (QTCREATORBUG-25924)
* Added information about source to tooltip on diagnostics
* Added highlighting color option for namespaces (QTCREATORBUG-16580)
* Made pure virtual functions optional in `Create implementations for all member
  functions` (QTCREATORBUG-26468)
* Fixed `Insert Definition` for template types (QTCREATORBUG-26113,
  QTCREATORBUG-26397)
* Fixed that `Find References` did not work for some template and namespace
  combinations (QTCREATORBUG-26520)
* Fixed canceling of C++ parsing on configuration change (QTCREATORBUG-24890)
* Fixed crash when checking for refactoring actions (QTCREATORBUG-26316)
* Fixed wrong target compiler option (QTCREATORBUG-25615)
* Fixed parentheses matching (QTCREATORBUG-26400)
* Fixed documentation comment generation for template types (QTCREATORBUG-9620)
* Clangd
  * Added warning for older `clangd` versions
  * Added support for completion and function hint
  * Added option for `Insert header files on completion`
  * Improved location of generated `compile_commands.json` (QTCREATORBUG-26431)
  * Fixed missing reparsing after refactorings (QTCREATORBUG-26523)
  * Fixed that parameters were incorrectly highlighted as output parameters
  * Fixed highlighting of string literals in macros (QTCREATORBUG-26553)
  * Fixed icon of signals and slots in completion list (QTCREATORBUG-26555)
  * Fixed header completion for Qt headers (QTCREATORBUG-26482)
  * Fixed code model update after UI header change
  * Fixed that `Find References` could show results for deleted files
    (QTCREATORBUG-26574)
  * Fixed that highlighting of current symbol could vanish (QTCREATORBUG-26339)
  * Fixed that nested items were not synchronized with cursor position in
    outline (QTCREATORBUG-26509)

### QML

* Improved wizards for Qt 6.2 (QTCREATORBUG-26170)
* Simplified wizards
* Fixed wrong warning on JavaScript equality checks (QTCREATORBUG-25917)

### Language Server Protocol

* Added support for `activeParameter` of signature information
  (QTCREATORBUG-26346)

Projects
--------

* Added option to close all projects except one to Projects' view context menu
* Added option to cancel file renaming (QTCREATORBUG-26268)
* Added `Show in File System View` to project context menu
* Added `Advanced Find` scope for `Files in All Project Directories`
* Fixed stale directories in `Files in All Project Directories` locator filter
* Fixed redundant output on process crash (QTCREATORBUG-26049)
* Fixed duplicates in file rename dialog (QTCREATORBUG-26268)
* Fixed variable expansion for working directory (QTCREATORBUG-26274)
* Fixed possible warning when opening files from compile output
  (QTCREATORBUG-26422)
* Fixed that re-detecting compilers removed compilers from kits
  (QTCREATORBUG-25697)
* Fixed GitHub action created by Qt Creator plugin wizard for Qt 6
* Fixed crash when canceling device test dialog (QTCREATORBUG-26285)

### CMake

* Removed separate `<Headers>` node from project tree (QTCREATORBUG-18206,
  QTCREATORBUG-24609, QTCREATORBUG-25407)
* Improved performance while loading large projects
* Fixed that CMake warnings and project loading errors were not added to
  `Issues` pane (QTCREATORBUG-26231)
* Fixed header file handling when mentioned in target sources
  (QTCREATORBUG-23783, QTCREATORBUG-23843, QTCREATORBUG-26201,
  QTCREATORBUG-26238, QTCREATORBUG-21452, QTCREATORBUG-25644,
  QTCREATORBUG-25782)
* Fixed that generated files were selected for analyzing (QTCREATORBUG-25125)
* Fixed importing of Qt projects (QTCREATORBUG-25767)

### qmake

* Fixed crash when canceling parsing (QTCREATORBUG-26333)

### Compilation Database

* Fixed that headers were not shown as part of the project (QTCREATORBUG-26356)

### Conan

* Added `QT_CREATOR_CONAN_BUILD_POLICY` used for `BUILD` property of
  `conan_cmake_run`

Debugging
---------

### GDB

* Fixed issue with non-English locale (QTCREATORBUG-26384)
* Fixed variable expansion for `Additional Startup Commands`
  (QTCREATORBUG-26382)

### CDB

* Added hint for missing Qt debug information
* Improved pretty printing for Qt 6 without debug information

Version Control Systems
-----------------------

### Git

* Added option to `Show HEAD` when amending commit (QTCREATORBUG-25004)
* Fixed wrong modified state of log viewer

Test Integration
----------------

* Added option to run tests without deployment

## CTest

* Added options page (QTCREATORBUG-26029)

Platforms
---------

### Windows

* Added support for MSVC 2022

### macOS

* Changed prebuilt binaries to universal Intel + ARM
* Made dark theme the default in dark system mode
* Fixed issues with dark system mode (QTCREATORBUG-21520, QTCREATORBUG-26427,
  QTCREATORBUG-26428)

### Android

* Removed device selection dialog in favor of device selection in target
  selector (QTCREATORBUG-23991)
* Added details to device settings (QTCREATORBUG-23991)
* Added filter field for Android SDK manager
* Fixed that NDK 22 and later could not be added
* Fixed creation of Android template files (QTCREATORBUG-26580)

### iOS

* Fixed that no tasks were created for build issues (QTCREATORBUG-26541)

### WebAssembly

* Fixed running applications (QTCREATORBUG-25905, QTCREATORBUG-26189,
  QTCREATORBUG-26562)

### MCU

* Added preliminary support for SDK 2.0

### Docker

* Various improvements

Credits for these changes go to:
--------------------------------
Aleksei German  
Alessandro Portale  
Alex Richardson  
Alp Öz  
Andre Hartmann  
André Pönitz  
Artem Sokolovskii  
Artur Shepilko  
Assam Boudjelthia  
BogDan Vatra  
Christiaan Janssen  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Fawzi Mohamed  
Henning Gruendl  
Ihor Dutchak  
Ivan Komissarov  
Jaroslaw Kobus  
Johanna Vanhatapio  
Jonas Karlsson  
Jonas Singe  
Kai Köhne  
Kama Wójcik  
Knud Dollereder  
Leena Miettinen  
Li Xi  
Loren Burkholder  
Mahmoud Badri  
Marco Bubke  
Martin Kampas  
Miikka Heikkinen  
Miina Puuronen  
Oliver Wolff  
Orgad Shaneh  
Petar Perisin  
Piotr Mikolajczyk  
Robert Löhning  
Samuel Ghinet  
Shantanu Tushar  
Tapani Mattila  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
Tomi Korpipaa  
Tony Leinonen  
Tor Arne Vestbø  
Tuomo Pelkonen  
Vikas Pachdha  
Vladimir Serdyuk  
