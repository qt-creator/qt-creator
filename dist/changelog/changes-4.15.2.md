Qt Creator 4.15.2
=================

Qt Creator version 4.15.2 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.15.1..v4.15.2

Projects
--------

### CMake

* Improved performance after project load and reparse
* Fixed crash on session switch (QTCREATORBUG-25837)

### qmake

* Fixed issues with executing system calls (QTCREATORBUG-25970)

Test Integration
----------------

### CTest

* Fixed test detection if `ctest` takes long to run (QTCREATORBUG-25851)

Platforms
---------

### WASM

* Fixed Python version that is on Windows (QTCREATORBUG-25897)

Credits for these changes go to:
--------------------------------
Ahmad Samir  
Alessandro Portale  
Christian Stenger  
Cristian Adam  
Eike Ziller  
Ivan Komissarov  
Kai Köhne  
Knud Dollereder  
Michael Winkelmann  
Mitch Curtis  
Robert Löhning  
Thomas Hartmann  
Tim Blechmann  
Tim Jenssen  
Tuomo Pelkonen  
