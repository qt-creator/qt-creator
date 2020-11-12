Qt Creator 4.13.3
=================

Qt Creator version 4.13.3 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.13.2..v4.13.3

General
-------

* Updated prebuilt binaries to Qt 5.15.2 which fixes drag & drop on macOS

Editing
-------

### QML

* Fixed reformatting of required properties (QTCREATORBUG-24376)
* Fixed importing without specific version for Qt 6 (QTCREATORBUG-24533)

Projects
--------

* Fixed auto-scrolling of compile output window (QTCREATORBUG-24728)
* Fixed GitHub Actions for Qt Creator plugin wizard (QTCREATORBUG-24412)
* Fixed crash with `Manage Sessions` (QTCREATORBUG-24797)

Qt Quick Designer
-----------------

* Fixed crash when opening malformed `.ui.qml` file (QTCREATORBUG-24587)

Debugging
---------

### CDB

* Fixed pretty printing of `std::vector` and `std::string` in release mode

Analyzer
--------

### QML Profiler

* Fixed crash with `Analyze Current Range` (QTCREATORBUG-24730)

Platforms
---------

### Android

* Fixed modified state of manifest editor when changing app icons
  (QTCREATORBUG-24700)

Credits for these changes go to:
--------------------------------
Alexandru Croitor  
Christian Kandeler  
Christian Stenger  
David Schulz  
Dominik Holland  
Eike Ziller  
Fawzi Mohamed  
Friedemann Kleint  
Ivan Komissarov  
Johanna Vanhatapio  
Leena Miettinen  
Lukasz Ornatek  
Robert Löhning  
Tim Jenssen  
Ville Voutilainen  
Xiaofeng Wang  
