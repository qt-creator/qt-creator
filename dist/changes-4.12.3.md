Qt Creator 4.12.3
=================

Qt Creator version 4.12.3 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.12.2..v4.12.3

Editing
-------

* Fixed missing update of completions after cursor navigation (QTCREATORBUG-24071)

### QML

* Fixed line number for string literals (QTCREATORBUG-23777)

### GLSL

* Fixed freeze (QTCREATORBUG-24070)

Projects
--------

### CMake

* Fixed issue with `Add build library search path` and older CMake versions (QTCREATORBUG-23997)
* Fixed that projects without name were considered invalid (QTCREATORBUG-24044)

Debugging
---------

* Fixed QDateTime pretty printer for Qt 5.14 and newer
* Fixed QJson pretty printer for Qt 5.15 and newer (QTCREATORBUG-23827)

Platforms
---------

### Android

* Fixed that installing OpenSSL for Android in the settings could delete current working directory
  (QTCREATORBUG-24173)

### MCU

* Fixed issue with saving settings (QTCREATORBUG-24048)

Credits for these changes go to:
--------------------------------
Alessandro Portale  
André Pönitz  
Assam Boudjelthia  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Leena Miettinen  
Tobias Hunger  
