Qt Creator 16.0.2
=================

Qt Creator version 16.0.2 contains bug fixes.
It is a free upgrade for commercial license holders.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=v16.0.1..v16.0.2>

Editing
-------

* Fixed the position of annotation tooltips for horizontally scrolled editors
  ([QTCREATORBUG-32658](https://bugreports.qt.io/browse/QTCREATORBUG-32658))
* Fixed a crash when pressing the `Delete` key while the focus is in an empty
  `Bookmarks` view
  ([QTCREATORBUG-32774](https://bugreports.qt.io/browse/QTCREATORBUG-32774))
* Fixed a crash when opening big files
  ([QTCREATORBUG-32875](https://bugreports.qt.io/browse/QTCREATORBUG-32875))

### Qt Quick Designer

* Fixed a crash when switching to a `.ui.qml` file the first time, if `Design`
  mode is already active
  ([QTCREATORBUG-32854](https://bugreports.qt.io/browse/QTCREATORBUG-32854))

Analyzer
--------

### Coco

* Fixed a crash in the build step
  ([QTCREATORBUG-32850](https://bugreports.qt.io/browse/QTCREATORBUG-32850))

Version Control Systems
-----------------------

### Git

* Fixed a crash when clicking links in the tooltip for `Instant Blame`

Platforms
---------

### macOS

* Fixed issues with configuring CMake projects with CMake 4.0.0 and CMake 4.0.1
  ([QTCREATORBUG-32877](https://bugreports.qt.io/browse/QTCREATORBUG-32877))

### Android

* Fixed a crash when setting up Android projects
  ([QTCREATORBUG-32849](https://bugreports.qt.io/browse/QTCREATORBUG-32849))

### iOS

* Fixed QML profiling on Simulator and devices with iOS 16 or earlier

### MCU

* Fixed an issue with optional packages
  ([QTCREATORBUG-32777](https://bugreports.qt.io/browse/QTCREATORBUG-32777))

Credits for these changes go to:
--------------------------------
André Pönitz  
Aurélien Brooke  
Christian Stenger  
David Schulz  
Eike Ziller  
Jaroslaw Kobus  
Karim Abdelrahman  
Krzysztof Chrusciel  
Leena Miettinen  
Marcus Tillmanns  
Markus Redeker  
