Qt Creator 13.0.2
=================

Qt Creator version 13.0.2 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v13.0.1..v13.0.2

General
-------

* Fixed that the `-client` option could start a new Qt Creator instance instead
  of using a running one (which affects for example version control operations)
  ([QTCREATORBUG-30624](https://bugreports.qt.io/browse/QTCREATORBUG-30624))

Editing
-------

* Fixed that closing files with the tool button didn't add an entry to the
  navigation history

### Widget Designer

* Fixed that `Use Qt module name in #include-directive` used Qt 4 module names
  ([QTCREATORBUG-30751](https://bugreports.qt.io/browse/QTCREATORBUG-30751))

Projects
--------

### Meson

* Fixed a crash when selecting kits
  ([QTCREATORBUG-30698](https://bugreports.qt.io/browse/QTCREATORBUG-30698))

Terminal
--------

* Fixed the handling of environment variables with an equal sign `=` in the
  value
  ([QTCREATORBUG-30844](https://bugreports.qt.io/browse/QTCREATORBUG-30844))

Version Control Systems
-----------------------

### Git

* Fixed a crash in `Instant Blame` when reloading externally modified files
  ([QTCREATORBUG-30824](https://bugreports.qt.io/browse/QTCREATORBUG-30824))

Platforms
---------

### Windows

* Fixed missing paths with `Add build library search path to PATH` for CMake
  projects
  ([QTCREATORBUG-30556](https://bugreports.qt.io/browse/QTCREATORBUG-30556),
   [QTCREATORBUG-30827](https://bugreports.qt.io/browse/QTCREATORBUG-30827),
   [QTCREATORBUG-30932](https://bugreports.qt.io/browse/QTCREATORBUG-30932))

### Android

* Fixed a crash when re-connecting devices
  ([QTCREATORBUG-30645](https://bugreports.qt.io/browse/QTCREATORBUG-30645),
   [QTCREATORBUG-30770](https://bugreports.qt.io/browse/QTCREATORBUG-30770))

### Remote Linux

* Fixed passing more than one argument to `rsync`
  ([QTCREATORBUG-30795](https://bugreports.qt.io/browse/QTCREATORBUG-30795))

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Leena Miettinen  
Marcus Tillmanns  
Robert Löhning  
