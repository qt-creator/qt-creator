Qt Creator 18.0.2
=================

Qt Creator version 18.0.2 contains bug fixes and new features.
It is a free upgrade for all users.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=v18.0.1..v18.0.2>

General
-------
* Fixed a potential freeze when updating the `Paste` action
  ([QTBUG-142708](https://bugreports.qt.io/browse/QTBUG-142708))

Projects
--------

* Fixed the persistence of the `Always save files before build` option
  ([QTCREATORBUG-33886](https://bugreports.qt.io/browse/QTCREATORBUG-33886))

### CMake

* vcpkg
    * Fixed an issue with custom toolchain files
      ([QTCREATORBUG-33854](https://bugreports.qt.io/browse/QTCREATORBUG-33854))

### Qt Safe Renderer

* Added a wizard template for QSR 2.2
* Fixed the QSR 2.1 wizard templates for remote targets

Analyzer
--------

### Clang

* Fixed the documentation URLs for Clazy checks

Terminal
--------

* Fixed an issue if WinPTY is used (Windows 10)
  ([QTCREATORBUG-33876](https://bugreports.qt.io/browse/QTCREATORBUG-33876))

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Christian Kandeler  
Cristian Adam  
David Schulz  
Eike Ziller  
Jaroslaw Kobus  
Jussi Witick  
Kai Köhne  
Lukasz Papierkowski  
Marcus Tillmanns  
