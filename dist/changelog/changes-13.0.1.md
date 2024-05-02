Qt Creator 13.0.1
=================

Qt Creator version 13.0.1 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v13.0.0..v13.0.1

General
-------

* Fixed a crash when hiding tool tips
  ([QTCREATORBUG-30738](https://bugreports.qt.io/browse/QTCREATORBUG-30738))

Help
----

* Examples Browser
    * Fixed that two columns were shown for the wrong category
      ([QTCREATORBUG-30634](https://bugreports.qt.io/browse/QTCREATORBUG-30634))
    * Fixed that the clear button was missing after typing in the filter input

* Fixed links to QML properties
  ([QTCREATORBUG-30625](https://bugreports.qt.io/browse/QTCREATORBUG-30625))

Editing
-------

* Fixed that backspace could delete more than one whitespace in the middle of a line
  ([QTCREATORBUG-30725](https://bugreports.qt.io/browse/QTCREATORBUG-30725))

### C++

* Fixed that completion was shown for number literals
  ([QTCREATORBUG-30607](https://bugreports.qt.io/browse/QTCREATORBUG-30607))
* Fixed that Flex and Bison files were opened in the C++ editor
  ([QTCREATORBUG-30686](https://bugreports.qt.io/browse/QTCREATORBUG-30686))

### QML

* Fixed a crash when creating a `Qt Quick Application` when the QML language server is enabled
  ([QTCREATORBUG-30739](https://bugreports.qt.io/browse/QTCREATORBUG-30739))

### Models

* Fixed a crash when selecting elements
  ([QTCREATORBUG-30413](https://bugreports.qt.io/browse/QTCREATORBUG-30413))

Projects
--------

* Fixed that the text editor for environment changes showed a blinking cursor even when not focused
  ([QTCREATORBUG-30640](https://bugreports.qt.io/browse/QTCREATORBUG-30640))
* Fixed that the option `Start build processes with low priority` did not persist
  ([QTCREATORBUG-30696](https://bugreports.qt.io/browse/QTCREATORBUG-30696))

### CMake

* Fixed that `Add build library search path to PATH` missed paths to libraries that were built by
  the project
  ([QTCREATORBUG-30644](https://bugreports.qt.io/browse/QTCREATORBUG-30644))
* Fixed the handling of `source_group`
  ([QTCREATORBUG-30602](https://bugreports.qt.io/browse/QTCREATORBUG-30602),
   [QTCREATORBUG-30620](https://bugreports.qt.io/browse/QTCREATORBUG-30620))
* Fixed that renaming files did not adapt `set_source_file_properties` calls
  ([QTCREATORBUG-30174](https://bugreports.qt.io/browse/QTCREATORBUG-30174))
* Fixed a crash when combining presets
  ([QTCREATORBUG-30755](https://bugreports.qt.io/browse/QTCREATORBUG-30755))

Debugging
---------

* Fixed a crash when enabling QML debugging
  ([QTCREATORBUG-30706](https://bugreports.qt.io/browse/QTCREATORBUG-30706))
* LLDB
    * Fixed that `Additional Attach Commands` were not used

Analyzer
--------

### Clang

* Fixed the documentation link for `clang-tidy` checks
  ([QTCREATORBUG-30658](https://bugreports.qt.io/browse/QTCREATORBUG-30658))

Terminal
--------

* Fixed the `TERM` environment variable, which broke the functioning of certain command line tools
  ([QTCREATORBUG-30737](https://bugreports.qt.io/browse/QTCREATORBUG-30737))

Platforms
---------

### Android

* Updated the command line tools that are installed with `Set Up SDK`
* Fixed that Qt ABI detection was wrong directly after `Set Up SDK`
  ([QTCREATORBUG-30568](https://bugreports.qt.io/browse/QTCREATORBUG-30568))

### iOS

* Fixed a crash when starting multiple applications in Simulators
  ([QTCREATORBUG-30666](https://bugreports.qt.io/browse/QTCREATORBUG-30666))

### Remote Linux

* Fixed that deployment could block Qt Creator until finished
* Fixed that it was not possible to change the device name
  ([QTCREATORBUG-30622](https://bugreports.qt.io/browse/QTCREATORBUG-30622))

Credits for these changes go to:
--------------------------------
Ahmad Samir  
Alessandro Portale  
BogDan Vatra  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Faure  
David Schulz  
Eike Ziller  
Jaroslaw Kobus  
Jussi Witick  
Leena Miettinen  
Marcus Tillmanns  
Robert Löhning  
