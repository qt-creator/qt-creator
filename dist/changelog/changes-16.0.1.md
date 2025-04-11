Qt Creator 16.0.1
=================

Qt Creator version 16.0.1 contains bug fixes.
It is a free upgrade for commercial license holders.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=v16.0.0..v16.0.1>

Editing
-------

* Fixed that formatting code with the Beautifier plugin broke the text encoding
  ([QTCREATORBUG-32668](https://bugreports.qt.io/browse/QTCREATORBUG-32668))
* Fixed that jumping to a column that doesn't exist jumped to a different line
  ([QTCREATORBUG-32592](https://bugreports.qt.io/browse/QTCREATORBUG-32592))

### C++

* Fixed that autodetection of tab settings was turned on for the built-in
  code styles
  ([QTCREATORBUG-32664](https://bugreports.qt.io/browse/QTCREATORBUG-32664))

### QML

* Fixed that `Follow Symbol Under Cursor` could open a copy in the build folder
  ([QTCREATORBUG-32652](https://bugreports.qt.io/browse/QTCREATORBUG-32652))

### Language Server Protocol

* Fixed the sorting in `Type Hierarchy`

Projects
--------

* Fixed a possible crash when renaming files
* Fixed a crash when canceling the download of SDKs
  ([QTCREATORBUG-32746](https://bugreports.qt.io/browse/QTCREATORBUG-32746))

### CMake

* Improved the detection of Ninja on macOS
  ([QTCREATORBUG-32331](https://bugreports.qt.io/browse/QTCREATORBUG-32331))
* Fixed a crash when adding files
  ([QTCREATORBUG-32745](https://bugreports.qt.io/browse/QTCREATORBUG-32745))
* Fixed `Package manager auto setup` for CMake older than 3.19
  ([QTCREATORBUG-32636](https://bugreports.qt.io/browse/QTCREATORBUG-32636))

Debugging
---------

* Fixed `Open Memory Editor Showing Stack Layout`
  ([QTCREATORBUG-32542](https://bugreports.qt.io/browse/QTCREATORBUG-32542))

### QML

Fixed that variable values in `Locals` view were not marked as changed
  ([QTCREATORBUG-29344](https://bugreports.qt.io/browse/QTCREATORBUG-29344))
* Fixed the re-enabling of breakpoints
  ([QTCREATORBUG-17294](https://bugreports.qt.io/browse/QTCREATORBUG-17294))

Analyzer
--------

### QML Profiler

* Fixed attaching to a running external application
  ([QTCREATORBUG-32617](https://bugreports.qt.io/browse/QTCREATORBUG-32617))

### Perf

* Fixed stopping profiling with the tool button

Terminal
--------

* Fixed issues with the `PATH` environment variable
  ([QTCREATORBUG-32647](https://bugreports.qt.io/browse/QTCREATORBUG-32647))
* Fixed `Enable mouse tracking`

Version Control Systems
-----------------------

### Git

* Guarded against crashes in `Branches` view
  ([QTCREATORBUG-32186](https://bugreports.qt.io/browse/QTCREATORBUG-32186))

Platforms
---------

### iOS

* Fixed running on iOS 17+ devices with Xcode 15.4
  ([QTCREATORBUG-32637](https://bugreports.qt.io/browse/QTCREATORBUG-32637))

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Andre Hartmann  
Aurélien Brooke  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Jaroslaw Kobus  
Jörg Bornemann  
Krzysztof Chrusciel  
Leena Miettinen  
Lukasz Papierkowski  
Marcus Tillmanns  
Orgad Shaneh  
Robert Löhning  
Sami Shalayel  
Thiago Macieira  
