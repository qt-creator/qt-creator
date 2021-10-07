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

### C++

* Updated to LLVM 13
* Added completion and function hint to `clangd` support
* Added option for saving open files automatically after refactoring
  (QTCREATORBUG-25924)
* Fixed `Insert Definition` for templates with value parameters
  (QTCREATORBUG-26113)
* Fixed canceling of C++ parsing on configuration change (QTCREATORBUG-24890)

### QML

* Improved wizards for Qt 6.2 (QTCREATORBUG-26170)
* Simplified wizards

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

### CMake

* Removed separate `<Headers>` node from project tree (QTCREATORBUG-18206,
  QTCREATORBUG-24609, QTCREATORBUG-25407)
* Fixed that CMake warnings and project loading errors were not added to
  `Issues` pane (QTCREATORBUG-26231)
* Fixed header file handling when mentioned in target sources
  (QTCREATORBUG-23783, QTCREATORBUG-23843, QTCREATORBUG-26201,
  QTCREATORBUG-26238, QTCREATORBUG-21452, QTCREATORBUG-25644,
  QTCREATORBUG-25782)
* Fixed that generated files were selected for analyzing (QTCREATORBUG-25125)

### qmake

* Fixed crash when canceling parsing (QTCREATORBUG-26333)

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

### macOS

* Changed prebuilt binaries to universal Intel + ARM

### Android

* Removed device selection dialog in favor of device selection in target
  selector (QTCREATORBUG-23991)
* Added details to device settings (QTCREATORBUG-23991)
* Added filter field for Android SDK manager

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
Christiaan Janssen  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Fawzi Mohamed  
Henning Gruendl  
Ihor Dutchak  
Jaroslaw Kobus  
Johanna Vanhatapio  
Jonas Karlsson  
Kai Köhne  
Kama Wójcik  
Knud Dollereder  
Li Xi  
Loren Burkholder  
Mahmoud Badri  
Marco Bubke  
Martin Kampas  
Miikka Heikkinen  
Miina Puuronen  
Orgad Shaneh  
Petar Perisin  
Piotr Mikolajczyk  
Samuel Ghinet  
Shantanu Tushar  
Tasuku Suzuki  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
Tony Leinonen  
Tor Arne Vestbø  
Vladimir Serdyuk  
