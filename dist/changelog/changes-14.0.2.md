Qt Creator 14.0.2
=================

Qt Creator version 14.0.2 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=v14.0.1..v14.0.2>

Editing
-------

* Fixed performance issues when long lines are highlighted
* Fixed a crash after completion
  (QTCREATORBUG-31563)

### C++

* Fixed generating getters and setters for static pointer members
  (QTCREATORBUG-27547)
* Fixed a crash while parsing
  (QTCREATORBUG-31569)
* Built-in
    * Fixed an issue with empty lists in member initializations
      (QTCREATORBUG-30797)
    * Fixed an issue with empty lists in the placement version of `new`
      (QTCREATORBUG-30798)

### QML

* Fixed that automatic formatting on save with `qmlformat` ignored
  `.qmlformat.ini`
  (QTCREATORBUG-29668)
* `qmlls`
    * Fixed semantic highlighting
      (QTCREATORBUG-31148)
    * Fixed that enabling `qmlls` disabled code snippets
      (QTCREATORBUG-31322)

### Python

* Fixed the state of the `Generate Kit` button

### Language Server Protocol

* Fixed the line number for diagnostics

### Widget Designer

* Fixed issues with switching to Design mode when external editor windows are
  open
  (QTCREATORBUG-31378)
* Fixed a crash when renaming widgets
  (QTCREATORBUG-31519)

Projects
--------

* Fixed a crash when editing the project environment
  (QTCREATORBUG-31483)

### CMake

* Fixed that `Clear CMake Configuration` was visible regardless of the used
  build system
* Presets
    * Fixed the handling of the `PATH` environment variable
      (QTCREATORBUG-31439)

### vcpkg

* Fixed that the wrong `vcpkg` executable could be used

Debugging
---------

### C++

* CDB
    * Fixed the debugging of 32bit applications
      (QTCREATORBUG-31345)

Platforms
---------

### Windows

* Fixed the detection of ARM compilers and debuggers

### Android

* Fixed that the debugger could be interrupted a lot during startup
  (QTCREATORBUG-29928)

### Remote Linux

* Fixed that deployment with `rsync` preserved owner, group, and permissions
  (QTCREATORBUG-31138)

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Alexandre Laurent  
Aurélien Brooke  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Friedemann Kleint  
Jaroslaw Kobus  
Jussi Witick  
Leena Miettinen  
Marcus Tillmanns  
Oliver Wolff  
Orgad Shaneh  
Robert Löhning  
Sami Shalayel  
Semih Yavuz  
