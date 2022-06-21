Qt Creator 4.12.1
=================

Qt Creator version 4.12.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.12.0..v4.12.1

General
-------

* Fixed crash when changing font settings (QTCREATORBUG-14385)
* Fixed availability of `Link with Qt` information on startup (QTCREATORBUG-23900)

Editing
-------

### C++

* Fixed crash when loading settings from Qt Creator < 4.11 (QTCREATORBUG-23916)

### QML

* Fixed semantic highlighting (QTCREATORBUG-23729, QTCREATORBUG-23777)
* Fixed wrong symbol highlighting (QTCREATORBUG-23830)
* Fixed warning for `palette` property (QTCREATORBUG-23830)

Projects
--------

### qmake

* Fixed that run button could stay disabled after parsing

### CMake

* Fixed issue with JOM (QTCREATORBUG-22645)

### Qbs

* Fixed crash when updating project (QTCREATORBUG-23924)

### Compilation Database

* Fixed issues with symbolic links (QTCREATORBUG-23511)

Debugging
---------

* Fixed startup when Python's JSON module is missing (QTCREATORBUG-24004)
* Fixed pretty printing of `std::unique_ptr` with custom deleter (QTCREATORBUG-23885)

### GDB

* Fixed handling of register addresses with lowercase characters
* Fixed issue with GDB reporting zero array size in some cases (QTCREATORBUG-23998)

Qt Quick Designer
-----------------

* Fixed crash after building emulation layer (QTCREATORBUG-20364)
* Fixed crash when opening `.qml` file instead of `.qml.ui` file (QDS-2011)

Test Integration
----------------

* Fixed handling of test output (QTCREATORBUG-23939)

Platforms
---------

### Android

* Fixed crash at startup when Qt is missing in Kit (QTCREATORBUG-23963)
* Fixed `Always use this device for this project` (QTCREATORBUG-23918)
* Fixed issue with "side by side" NDK installation (QTCREATORBUG-23903)

### OpenBSD

* Fixed Qt ABI detection (QTCREATORBUG-23818)

### MCU

* Fixed various issues with Kit creation and cleanup

Credits for these changes go to:
--------------------------------
Alessandro Portale  
André Pönitz  
Assam Boudjelthia  
Brook Cronin  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Friedemann Kleint  
Henning Gruendl  
Jeremy Ephron  
Johanna Vanhatapio  
Leander Schulten  
Leena Miettinen  
Nikolai Kosjar  
Robert Löhning  
Sebastian Verling  
Sergey Belyashov  
Thiago Macieira  
Thomas Hartmann  
Tim Jenssen  
Venugopal Shivashankar  
Vikas Pachdha  
