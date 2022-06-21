Qt Creator 4.15.1
=================

Qt Creator version 4.15.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.15.0..v4.15.1

General
-------

* Fixed crash in `Search Results` pane (QTCREATORBUG-25713)
* Fixed crash when showing tooltips after screen configuration changes
  (QTCREATORBUG-25747)
* Fixed environment selection for external tools (QTCREATORBUG-25634)

Editing
-------

* Fixed crash when opening settings from tooltip (QTCREATORBUG-25623)
* Fixed hiding of function hints (QTCREATORBUG-25664)
* Fixed vanishing text marks (QTCREATORBUG-25427)

### C++

* Fixed freeze when updating project while indexing is running

### QML

* Fixed wrong warning for blocks with `case` and `let` (QTCREATORBUG-24214)

### QRC

* Fixed that `compress-algo` tags were removed (QTCREATORBUG-25706)

Projects
--------

* Fixed restoration of `Projects` mode layout (QTCREATORBUG-25551)

### Wizards

* Fixed `Fetch data asynchronously` for list and table models

### CMake

* Fixed issues when switching configurations or running CMake while parsing
  (QTCREATORBUG-25588, QTCREATORBUG-25287)
* Fixed crash when cancelling scanning the project tree (QTCREATORBUG-24564)
* Fixed custom targets missing in Locator (QTCREATORBUG-25726)

Debugging
---------

### GDB

* Fixed crash (QTCREATORBUG-25745)

Test Integration
----------------

* Fixed selection of individual tests (QTCREATORBUG-25702)

### Catch2

* Fixed issues with Catch2 3.0 (QTCREATORBUG-25582)

### GoogleTest

* Fixed crash with empty test name

Platforms
---------

### Windows

* Fixed issues with `clang-cl` toolchain (QTCREATORBUG-25690,
  QTCREATORBUG-25693, QTCREATORBUG-25698)

### Remote Linux

* Fixed install step (QTCREATORBUG-25359)

### Android

* Improved startup time (QTCREATORBUG-25463)
* Fixed `Checking pending licenses` (QTCREATORBUG-25667)

### MCU

* Added support for Cypress Traveo II (UL-4242)
* Fixed CMake generator for GHS compiler (UL-4247)

Credits for these changes go to:
--------------------------------
Alessandro Portale  
André Pönitz  
Christiaan Janssen  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Erik Verbruggen  
Ivan Komissarov  
Jaroslaw Kobus  
Knud Dollereder  
Leena Miettinen  
Marco Bubke  
Michael Winkelmann  
Miikka Heikkinen  
Mikhail Khachayants  
Mitch Curtis  
Tapani Mattila  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
