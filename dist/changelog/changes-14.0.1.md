Qt Creator 14.0.1
=================

Qt Creator version 14.0.1 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=v14.0.0..v14.0.1>

General
-------

* Fixed wrong colors when using a light theme on a dark system
  ([QTCREATORBUG-31226](https://bugreports.qt.io/browse/QTCREATORBUG-31226))

Editing
-------

* Fixed a crash when selecting a context menu item for an editor that was
  closed in the meantime
  ([QTCREATORBUG-31232](https://bugreports.qt.io/browse/QTCREATORBUG-31232))

### C++

* Fixed a crash in the type hierarchy builder
  ([QTCREATORBUG-31318](https://bugreports.qt.io/browse/QTCREATORBUG-31318))
* Fixed the highlighting of non-plain character literals
  ([QTCREATORBUG-31342](https://bugreports.qt.io/browse/QTCREATORBUG-31342))

### QML

* Fixed that context help could show help from the wrong module
  ([QTCREATORBUG-31280](https://bugreports.qt.io/browse/QTCREATORBUG-31280))

### FakeVim

* Fixed that invalid values could be set for the tabstop size
  ([QTCREATORBUG-28082](https://bugreports.qt.io/browse/QTCREATORBUG-28082))

Projects
--------

### CMake

* Fixed the application of changes to the `Initial Arguments`
  ([QTCREATORBUG-31320](https://bugreports.qt.io/browse/QTCREATORBUG-31320))

Debugging
---------

### C++

* Fixed pretty printing for Qt 4
  ([QTCREATORBUG-31355](https://bugreports.qt.io/browse/QTCREATORBUG-31355))

Analyzer
--------

### Axivion

* Fixed a crash when creating links from the issues table to column data

### Valgrind

* Fixed missing error kinds for newer Valgrind versions in the parser
  ([QTCREATORBUG-31376](https://bugreports.qt.io/browse/QTCREATORBUG-31376))

Platforms
---------

### Docker

* Fixed a crash when adding a Docker device while an application is running
  on a Docker device
  ([QTCREATORBUG-31364](https://bugreports.qt.io/browse/QTCREATORBUG-31364))

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Alexandre Laurent  
André Pönitz  
Andrew Shark  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
Eike Ziller  
Leena Miettinen  
Marco Bubke  
Marcus Tillmanns  
Mehdi Salem  
