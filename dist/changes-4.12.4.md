Qt Creator 4.12.4
=================

Qt Creator version 4.12.4 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.12.3..v4.12.4

Editing
-------

* Fixed crash when searching in binary files (QTCREATORBUG-21473, QTCREATORBUG-23978)

### QML

* Fixed completion of signals from singletons (QTCREATORBUG-24124)
* Fixed import scanning after code model reset (QTCREATORBUG-24082)

Projects
--------

### CMake

* Fixed search for `ninja` when it is installed with the online installer (QTCREATORBUG-24082)

Platforms
---------

### iOS

* Fixed C++ debugging on devices (QTCREATORBUG-23995)

### MCU

* Adapted to changes in Qt for MCU 1.3

Credits for these changes go to:
--------------------------------
Alessandro Portale  
André Pönitz  
Christian Kamm  
Christian Stenger  
Eike Ziller  
Fawzi Mohamed  
Friedemann Kleint  
Robert Löhning  
Venugopal Shivashankar  
