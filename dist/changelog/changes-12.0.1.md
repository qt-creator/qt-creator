Qt Creator 12.0.1
=================

Qt Creator version 12.0.1 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v12.0.0..v12.0.1

General
-------

* Fixed opening files with drag and drop on Qt Creator
  ([QTCREATORBUG-29961](https://bugreports.qt.io/browse/QTCREATORBUG-29961))

Editing
-------

### C++

* Fixed a crash while parsing
  ([QTCREATORBUG-29847](https://bugreports.qt.io/browse/QTCREATORBUG-29847))
* Fixed a freeze when hovering over a class declaration
  ([QTCREATORBUG-29975](https://bugreports.qt.io/browse/QTCREATORBUG-29975))
* Fixed the renaming of virtual functions
* Fixed `Select Block Up` for string literals
  ([QTCREATORBUG-29844](https://bugreports.qt.io/browse/QTCREATORBUG-29844))
* Fixed the conversion between comment styles for certain indented comments
* Clang Format
    * Fixed the indentation after multi-byte UTF-8 characters
      ([QTCREATORBUG-29927](https://bugreports.qt.io/browse/QTCREATORBUG-29927))

### Widget Designer

* Fixed that the buttons for editing signals and slots and editing buddies
  were switched
  ([QTCREATORBUG-30017](https://bugreports.qt.io/browse/QTCREATORBUG-30017))

Projects
--------

* Fixed the restoring of custom Kit data
  ([QTCREATORBUG-29970](https://bugreports.qt.io/browse/QTCREATORBUG-29970))
* Fixed overlapping labels in the target selector
  ([QTCREATORBUG-29990](https://bugreports.qt.io/browse/QTCREATORBUG-29990))
* Fixed the label for `Custom Executable` run configurations
  ([QTCREATORBUG-29983](https://bugreports.qt.io/browse/QTCREATORBUG-29983))

### CMake

* Fixed a crash when opening projects
  ([QTCREATORBUG-29965](https://bugreports.qt.io/browse/QTCREATORBUG-29965))
* Fixed a crash when editing CMake files without a project
  ([QTCREATORBUG-30023](https://bugreports.qt.io/browse/QTCREATORBUG-30023))
* Fixed that directories were marked as invalid for the `Staging Directory`
  ([QTCREATORBUG-29997](https://bugreports.qt.io/browse/QTCREATORBUG-29997))

### Qbs

* Fixed a crash when parsing projects

Analyzer
--------

### Valgrind

* Fixed stopping the Valgrind process
  ([QTCREATORBUG-29948](https://bugreports.qt.io/browse/QTCREATORBUG-29948))

Version Control Systems
-----------------------

### Git

* Fixed that empty blame annotations are shown after saving a file outside of
  the version control directory
  ([QTCREATORBUG-29991](https://bugreports.qt.io/browse/QTCREATORBUG-29991))

Platforms
---------

### Linux

* Added an error dialog for errors when loading the Qt platform plugin
  ([QTCREATORBUG-30004](https://bugreports.qt.io/browse/QTCREATORBUG-30004))

### Boot2Qt

* Fixed deployment on Windows
  ([QTCREATORBUG-29971](https://bugreports.qt.io/browse/QTCREATORBUG-29971))

### MCU

* Fixed `Replace existing kits` after changing MCU SDK path
  ([QTCREATORBUG-29960](https://bugreports.qt.io/browse/QTCREATORBUG-29960))

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Andre Hartmann  
Artem Sokolovskii  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
Eike Ziller  
Friedemann Kleint  
Jaroslaw Kobus  
Marcus Tillmanns  
Orgad Shaneh  
Robert Löhning  
Samuli Piippo  
Tor Arne Vestbø  
Yasser Grimes  
