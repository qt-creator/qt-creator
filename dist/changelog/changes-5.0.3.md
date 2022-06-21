Qt Creator 5.0.3
================

Qt Creator version 5.0.3 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v5.0.2..v5.0.3

Editing
-------

### QML

* Fixed possible crash on shutdown

### Image Viewer

* Fixed crash when opening invalid movie (QTCREATORBUG-26377)

Projects
--------

### qmake

* Fixed handling of `QMAKE_EXTRA_COMPILERS` (QTCREATORBUG-26323)

Platforms
---------

### macOS

* Fixed crash when opening qmake projects on ARM Macs (QTBUG-97085)

### Android

* Fixed issue in installation step with qmake projects (QTCREATORBUG-26357)

Credits for these changes go to:
--------------------------------
Assam Boudjelthia  
Christian Kandeler  
Eike Ziller  
Henning Gruendl  
Jaroslaw Kobus  
Johanna Vanhatapio  
Jörg Bornemann  
Kai Köhne  
Kaj Grönholm  
Leena Miettinen  
Robert Löhning  
Thomas Hartmann  
Tim Jenssen  
