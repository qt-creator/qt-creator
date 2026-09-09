Qt Creator 20.0.2
=================

Qt Creator version 20.0.2 contains bug fixes.
It is a free upgrade for all users.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=v20.0.1..v20.0.2>

General
-------

Fixed

* The default search parameters for `Advanced Find`
* A crash with the navigation views during shutdown

Editing
-------

Fixed

* A crash when files are modified externally

### QML

Fixed

* That the wrong QML Language Server binary could be downloaded on Linux
  (QTCREATORBUG-34931)

Projects
--------

Fixed

* The `Always ask before stopping applications` option

### CMake

Fixed

* A crash when reloading CMake Presets

Platforms
---------

### macOS

Fixed

* An issue with the code model with Qt 6.12 and later
* That `bbedit` could not be run from the built-in terminal
  (QTCREATORBUG-34886)

### Android

Fixed

* Issues with the latest Android command line tools
  (QTCREATORBUG-34905, QTCREATORBUG-34917)

### iOS

Fixed

* Issues with the individual Simulator versus Device builds that are
  provided with Qt 6.12 and later
  (QTCREATORBUG-34896)
* Issues with Xcode 27
* That some command line arguments could not be passed to user applications
  on devices
  (QTCREATORBUG-34802)
* A crash when starting an application on the Simulator fails
  (QTCREATORBUG-34853)

Credits for these changes go to:
--------------------------------
Alexandre Laurent  
André Pönitz  
Aurélien Brooke  
BogDan Vatra  
Christian Kandeler  
Eike Ziller  
Marcus Tillmanns  
Patrik Teivonen  
Sami Shalayel  
Thiago Macieira  
