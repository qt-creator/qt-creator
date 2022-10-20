Qt Creator 8.0.2
================

Qt Creator version 8.0.2 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v8.0.1..v8.0.2

Editing
-------

* Fixed importing color schemes
* Fixed moving text between editors by drag & drop (QTCREATORBUG-28125,
  QTCREATORBUG-28126)
* Fixed that the default font could be blurry (QTCREATORBUG-28106,
  QTCREATORBUG-28139)

### C++

* Re-added non-Clangd version of `Extract Function` (QTCREATORBUG-28030)

### Language Client

* Fixed issues after resetting server (QTCREATORBUG-27596)

### SCXML

* Fixed crash when closing document (QTCREATORBUG-28027)

Version Control
---------------

* Fixed handling of `\r` in tool output (QTCREATORBUG-27615)

Test Integration
----------------

* GTest
    * Fixed running selected tests (QTCREATORBUG-28153)
* Catch2
    * Fixed crash when running tests (QTCREATORBUG-28269)

Platforms
---------

### Android

* Fixed deployment in release mode (QTCREATORBUG-28163)

### Remote Linux

* Fixed issue with deployment (QTCREATORBUG-28167)
* Fixed key deployment on Windows (QTCREATORBUG-28092)
* Fixed killing remote processes (QTCREATORBUG-28072)

Credits for these changes go to:
--------------------------------
Aleksei German  
Ali Kianian  
André Pönitz  
Antti Määttä  
Brook Cronin  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Fawzi Mohamed  
Henning Gruendl  
Ivan Komissarov  
Jaroslaw Kobus  
Knud Dollereder  
Leena Miettinen  
Mahmoud Badri  
Marco Bubke  
Marcus Tillmanns  
Mats Honkamaa  
Miikka Heikkinen  
Petri Virkkunen  
Pranta Dastider  
Robert Löhning  
Samuel Ghinet  
Sivert Krøvel  
Sona Kurazyan  
Thomas Hartmann  
Tim Jenssen  
Vikas Pachdha  
