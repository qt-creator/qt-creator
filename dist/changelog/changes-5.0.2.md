Qt Creator 5.0.2
================

Qt Creator version 5.0.2 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v5.0.1..v5.0.2

Help
----

* Fixed that Qt 5 context help was shown even if Qt 6 documentation is available
  (QTCREATORBUG-26292)

Projects
--------

* Fixed canceling of builds (QTCREATORBUG-26271)

### CMake

* Changed the `File System` special node to be shown only on parsing failure
  (QTCREATORBUG-25994, QTCREATORBUG-25974)
* Fixed loading of projects without targets (QTCREATORBUG-25509)
* Fixed that no targets where shown in added build step (QTCREATORBUG-25759)
* Fixed that `ninja` could not be found after changing Qt installation location
  (QTCREATORBUG-26289)

Debugging
---------

### GDB

* Fixed debugging of terminal applications with GDB < 10 (QTCREATORBUG-26299)

Platforms
---------

### macOS

* Fixed issue with absolute RPATH in `clazy-standalone` (QTCREATORBUG-26196)

### Android

* Fixed that wrong deployment file could be used (QTCREATORBUG-25793)

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Assam Boudjelthia  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
Eike Ziller  
Henning Gruendl  
Ivan Komissarov  
Jaroslaw Kobus  
Johanna Vanhatapio  
Kai Köhne  
Knud Dollereder  
Leena Miettinen  
Mahmoud Badri  
Marco Bubke  
Orgad Shaneh  
Robert Löhning  
Thomas Hartmann  
