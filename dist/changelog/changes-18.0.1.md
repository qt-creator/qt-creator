Qt Creator 18.0.1
=================

Qt Creator version 18.0.1 contains bug fixes.
It is a free upgrade for all users.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=v18.0.0..v18.0.1>

Editing
-------

* Fixed issues with applying text suggestions
  ([QTCREATORBUG-33466](https://bugreports.qt.io/browse/QTCREATORBUG-33466))

### C++

* Fixed that a closing quote was added when typing a quote within a string
  ([QTCREATORBUG-33733](https://bugreports.qt.io/browse/QTCREATORBUG-33733))
* Fixed that refactoring operations could create code in `qobject.h`
  ([QTCREATORBUG-33752](https://bugreports.qt.io/browse/QTCREATORBUG-33752))
* Fixed issues with Clangd from Intel oneAPI
  ([QTCREATORBUG-33763](https://bugreports.qt.io/browse/QTCREATORBUG-33763))

### QML

* Fixed that too many `qmlls` instances were run after changing settings
  ([QTCREATORBUG-33732](https://bugreports.qt.io/browse/QTCREATORBUG-33732))
* Fixed performance issues with gathering the information for the QML code model
  ([QTCREATORBUG-33715](https://bugreports.qt.io/browse/QTCREATORBUG-33715),
   [QTCREATORBUG-33723](https://bugreports.qt.io/browse/QTCREATORBUG-33723))

### Language Server Protocol

* Fixed issues with the document outline dropdown
  ([QTCREATORBUG-33668](https://bugreports.qt.io/browse/QTCREATORBUG-33668))

### Images

* Fixed viewing remote images

Projects
--------

* Improved the performance of the output views
  ([QTCREATORBUG-33756](https://bugreports.qt.io/browse/QTCREATORBUG-33756))
* Fixed a memory leak when scanning project trees for some project types

### CMake

* Fixed a performance issue when detecting `ninja`
  ([QTCREATORBUG-33709](https://bugreports.qt.io/browse/QTCREATORBUG-33709))
* Fixed that `find_package` calls were added for existing components
  ([QTCREATORBUG-33761](https://bugreports.qt.io/browse/QTCREATORBUG-33761))
* Fixed that changes to the `PATH` environment variable could be ignored
  when configuring projects
  ([QTCREATORBUG-33674](https://bugreports.qt.io/browse/QTCREATORBUG-33674))
* Fixed that the `Don't Save` button in the `Apply Configuration Changes`
  dialog did nothing, not even close the dialog
* Fixed the installation of a missing `Quick3DPhysics` dependency with the
  Qt Online Installer
  ([QTCREATORBUG-33570](https://bugreports.qt.io/browse/QTCREATORBUG-33570))

### qmake

* Fixed opening projects on Windows for ARM

### Compilation Database

* Fixed opening remote projects
  ([QTCREATORBUG-33739](https://bugreports.qt.io/browse/QTCREATORBUG-33739))

Analyzer
--------

### QML Profiler

* Fixed a crash when attaching the profiler to a running application and no
  run configuration is available

### Axivion

* Fixed the state of the `Local Build` button
  ([QTCREATORBUG-33729](https://bugreports.qt.io/browse/QTCREATORBUG-33729))
* Fixed issues with anonymous dashboard authentication
  ([QTCREATORBUG-33627](https://bugreports.qt.io/browse/QTCREATORBUG-33627))
* Fixed `Open Issue in Dashboard`
  ([QTCREATORBUG-33728](https://bugreports.qt.io/browse/QTCREATORBUG-33728))

Platforms
---------

### Android

* Fixed that some action buttons were not shown for new Android Virtual
  Devices (AVD)
  ([QTCREATORBUG-33727](https://bugreports.qt.io/browse/QTCREATORBUG-33727))

### Remote Linux

* Fixed a freeze at startup if CMake tools are configured for disconnected
  devices
* Fixed a crash when running auto-detection on a disconnected device
* Fixed issues when the remote temporary directory is mounted without
  executable permission
  ([QTCREATORBUG-33678](https://bugreports.qt.io/browse/QTCREATORBUG-33678))

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Alexandru Croitor  
André Pönitz  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Jaroslaw Kobus  
Karim Pinter  
Krzysztof Chrusciel  
Leena Miettinen  
Lukasz Papierkowski  
Marcus Tillmanns  
Oliver Wolff  
Sami Shalayel  
Teea Poldsam  
