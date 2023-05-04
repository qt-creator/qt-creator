Qt Creator 10.0.1
=================

Qt Creator version 10.0.1 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v10.0.0..v10.0.1

General
-------

* Enabled example categories for Qt 6.5.1 and later

Editing
-------

* Fixed layout issues in the editor toolbar (QTCREATORBUG-28765)

### C++

* Clangd
    * Fixed the renaming of macros and namespaces
    * Fixed that renaming appended `_new` to the replacement by default
      (QTCREATORBUG-28321, QTCREATORBUG-28910)
    * Fixed that Cuda files were not passed to Clangd (QTCREATORBUG-28984)
* Clang Format
    * Fixed the formatting for advanced C++ (QTCREATORBUG-29033)
    * Fixed the updating of the coding style preview (QTCREATORBUG-29043)
    * Fixed the indentation of `QML_*` macros (QTCREATORBUG-29086)

### QML

* Fixed a crash when trying to open non-existing `.qml` files
  (QTCREATORBUG-29021)

Projects
--------

* Re-added a Qt Quick Application wizard that works with Qt 5 and other build
  systems than CMake
* Fixed that additional empty lines could be added to files created by wizards
  (QTCREATORBUG-29040)

### CMake

* Added missing `RUNTIME DESTINATION` properties to the `install` commands of
  wizard-generated projects (QTCREATORBUG-28999)
* Fixed that macros were not expanded for all configure cache variables
  (QTCREATORBUG-28982)
* Fixed switching from `.c` files to their header (QTCREATORBUG-28991)
* Presets
    * Fixed that boolean values for cache variables were interpreted as string
      values (QTCREATORBUG-29078)
    * Fixed inheritance over multiple levels
      (QTCREATORBUG-29076)

Debugging
---------

### Clang

* Fixed the pretty printers of `std::string` for Clang 15 and later

Analyzer
--------

### Clang

* Fixed starting Clazy and Clang-Tidy while a build is running
  (QTCREATORBUG-29044)

Platforms
---------

### Android

* Fixed that changes to the `JDK Location` did not take effect immediately
  (QTCREATORBUG-28827)
* Fixed debugging on Android Automotive devices (QTCREATORBUG-28851)

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Artem Sokolovskii  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Haowei Hsu  
Jaroslaw Kobus  
Jussi Witick  
Leena Miettinen  
Marcus Tillmanns  
Orgad Shaneh  
Patrik Teivonen  
Robert Löhning  
Sivert Krøvel  
Thiago Macieira  
Ulf Hermann  
Zoltan Gera  
