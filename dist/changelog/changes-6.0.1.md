Qt Creator 6.0.1
================

Qt Creator version 6.0.1 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v6.0.0..v6.0.1

Editing
-------

* Fixed cursor position after pasting and moving (QTCREATORBUG-26635)
* Fixed deletion of start or end of word when camel case navigation is off
  (QTCREATORBUG-26646)
* Fixed crash when removing built-in snippets (QTCREATORBUG-26648)

### C++

* Clangd
    * Improved indexing performance on macOS
    * Fixed issues with refactoring (QTCREATORBUG-26649)
    * Fixed warnings for multiple `/Tx` options with MSVC (QTCREATORBUG-26664)

### Language Client

* Fixed sending of `textDocument/didChange` notifications (QTCREATORBUG-26651)

Projects
--------

* Fixed canceling GUI processes as build steps (QTCREATORBUG-26687)

### CMake

* Fixed initial CMake arguments for Windows ARM targets (QTCREATORBUG-26636)

### Qbs

* Fixed support for C++23 with MSVC (QTCREATORBUG-26663)

Debugging
---------

* Fixed interrupting console applications

### GDB

* Fixed `PATH` for debugging MinGW applications (QTCREATORBUG-26670)

Test Integration
----------------

### Google Test

* Fixed that application arguments could change order

Platforms
---------

### Linux

* Removed dependency of prebuilt packages on libGLX and libOpenGL
  (QTCREATORBUG-26652)

### macOS

* Fixed running applications that access Bluetooth (QTCREATORBUG-26666)
* Fixed saving of file system case sensitivity setting

### Android

* Fixed keystore handling on Windows (QTCREATORBUG-26647)

### Boot2Qt

* Fixed flashing wizard on Windows

### QNX

* Fixed codemodel issues (QTCREATORBUG-23483)

Credits for these changes go to:
--------------------------------
Aleksei German  
Alessandro Portale  
André Pönitz  
Antti Määttä  
BogDan Vatra  
Christiaan Janssen  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Dawid Śliwa  
Eike Ziller  
Henning Gruendl  
Jaroslaw Kobus  
Joni Poikelin  
Kai Köhne  
Kaj Grönholm  
Knud Dollereder  
Leena Miettinen  
Mahmoud Badri  
Marco Bubke  
Miikka Heikkinen  
Oliver Wolff  
Petar Perisin  
Robert Löhning  
Samuel Ghinet  
Sergey Levin  
Tapani Mattila  
Thomas Hartmann  
Vikas Pachdha  
Youri Westerman  
