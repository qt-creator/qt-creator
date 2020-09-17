Qt Creator 4.13.1
=================

Qt Creator version 4.13.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.13.0..v4.13.1

Editing
-------

* Fixed whitespace cleaning (QTCREATORBUG-24565)
* Fixed selection color (QTCREATORBUG-24479)

### C++

* Fixed crash with adjacent raw string literals (QTCREATORBUG-24577)
* Fixed highlighting of template aliases (QTCREATORBUG-24552)

### QML

* Fixed wrong diagnostics for `ListElement` (QDS-2602)

### Language Client

* Fixed performance issue

Projects
--------

* Fixed parsing of QTest application output (QTCREATORBUG-24560)
* Fixed visibility of output in output panes (QTCREATORBUG-24411)

### qmake

* Fixed handling of unset environment variables (QTCREATORBUG-21729)
* Fixed that changes to sub-projects triggered full re-parse
  (QTCREATORBUG-24572)

### CMake

* Fixed removal of CMake tools

### Qbs

* Fixed install step

Debugging
---------

### GDB

* Fixed disabling of environment variables

Analyzer
--------

### Clang

* Updated Clazy to version 1.7

Qt Quick Designer
-----------------

* Improved composition of custom materials (QDS-2657)
* Fixed available items for MCU (QDS-2681, QDS-2512)
* Fixed visual artifacts when changing states
* Fixed rich text editor styling
* Fixed layout issues in state editor (QDS-2623, QDS-2615)
* Fixed support for `.hrd` images (QDS-2128)

Platforms
---------

### Android

* Fixed service handling in manifest editor (QTCREATORBUG-24557)

### macOS

* Fixed Clazy (QTCREATORBUG-24567)
* Fixed debugger locals view for newest LLDB (QTCREATORBUG-24596)

Credits for these changes go to:
--------------------------------
Aleksei German  
André Pönitz  
Andy Shaw  
Christian Kandeler  
Christian Stenger  
David Schulz  
Dominik Holland  
Eike Ziller  
Henning Gruendl  
Johanna Vanhatapio  
Kai Köhne  
Knud Dollereder  
Leena Miettinen  
Mahmoud Badri  
Marco Bubke  
Michael Winkelmann  
Miikka Heikkinen  
Orgad Shaneh  
Sergey Belyashov  
Thomas Hartmann  
Venugopal Shivashankar  
Vikas Pachdha  
Ville Voutilainen  
