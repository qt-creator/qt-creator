Qt Creator 12.0.2
=================

Qt Creator version 12.0.2 contains bug fixes.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v12.0.1..v12.0.2

General
-------

### External Tools

* Fixed that the output could be interspersed with newlines
  ([QTCREATORBUG-29977](https://bugreports.qt.io/browse/QTCREATORBUG-29977))

Editing
-------

* Fixed that modified documents lost their file icon, which potentially included
  a short freeze
  ([QTCREATORBUG-29999](https://bugreports.qt.io/browse/QTCREATORBUG-29999))
* Fixed a crash when opening bookmarks
  ([QTCREATORBUG-30283](https://bugreports.qt.io/browse/QTCREATORBUG-30283))

### C++

* Clang Format
    * Fixed the update of the preview when settings change
      ([QTCREATORBUG-30089](https://bugreports.qt.io/browse/QTCREATORBUG-30089))
    * Fixed an issue with `CRLF` line endings
* Fixed a freeze when looking up symbols
  ([QTCREATORBUG-30155](https://bugreports.qt.io/browse/QTCREATORBUG-30155))
* Fixed a crash while parsing
  ([QTCREATORBUG-30044](https://bugreports.qt.io/browse/QTCREATORBUG-30044))

### Language Server Protocol

* Fixed a crash when completing

### Markdown

* Fixed that clicking on file links cleared the preview instead of opening the
  file
  ([QTCREATORBUG-30120](https://bugreports.qt.io/browse/QTCREATORBUG-30120))

Projects
--------

* Fixed that trying to stop remote processes that were no longer reachable
  resulted in a broken run control state
* Fixed a potential infinite loop
  ([QTCREATORBUG-30067](https://bugreports.qt.io/browse/QTCREATORBUG-30067))
* Fixed a crash when navigating in the Projects view
  ([QTCREATORBUG-30035](https://bugreports.qt.io/browse/QTCREATORBUG-30035))
* Fixed that custom compiler settings could vanish after restart
  ([QTCREATORBUG-30133](https://bugreports.qt.io/browse/QTCREATORBUG-30133))
* Fixed the restoring of per project C++ file name settings

### CMake

* Fixed that automatic re-configuration on saving files while a build is
  running could fail
  ([QTCREATORBUG-30048](https://bugreports.qt.io/browse/QTCREATORBUG-30048))
* Fixed that the automatically added library path was wrong for targets with
  the same name as special CMake targets (like "test")
  ([QTCREATORBUG-30050](https://bugreports.qt.io/browse/QTCREATORBUG-30050))
* Fixed that the `cm` locator filter did not show all targets
  ([QTCREATORBUG-29946](https://bugreports.qt.io/browse/QTCREATORBUG-29946))
* Fixed adding files with the wizards when triggered through `File > New File`
  ([QTCREATORBUG-30170](https://bugreports.qt.io/browse/QTCREATORBUG-30170))
* Fixed adding QML files to CMake files when variables like `${PROJECT_NAME}`
  are used for the target name
  ([QTCREATORBUG-30218](https://bugreports.qt.io/browse/QTCREATORBUG-30218))
* Fixed adding files to `OBJECT` libraries
  ([QTCREATORBUG-29914](https://bugreports.qt.io/browse/QTCREATORBUG-29914))
* CMake Presets
    * Fixed that display names were not updated when reloading presets
      ([QTCREATORBUG-30237](https://bugreports.qt.io/browse/QTCREATORBUG-30237))

### Conan

* Fixed that macOS sysroot was not passed on to Conan
  ([QTCREATORBUG-29978](https://bugreports.qt.io/browse/QTCREATORBUG-29978))
* Fixed that the MSVC runtime library was not passed on to Conan
  ([QTCREATORBUG-30169](https://bugreports.qt.io/browse/QTCREATORBUG-30169))

### Autotools

* Fixed that makefiles where no longer recognized as project files

Debugging
---------

### CMake

* Fixed that debugging required a successful build first
  ([QTCREATORBUG-30045](https://bugreports.qt.io/browse/QTCREATORBUG-30045))

Terminal
--------

* Fixed a crash when double-clicking
  ([QTCREATORBUG-30144](https://bugreports.qt.io/browse/QTCREATORBUG-30144))

Platforms
---------

### Android

* Fixed that the prompt for configuring the Android setup was no longer shown
  ([QTCREATORBUG-30131](https://bugreports.qt.io/browse/QTCREATORBUG-30131))
* Fixed that Qt ABI detection could be wrong
  ([QTCREATORBUG-30146](https://bugreports.qt.io/browse/QTCREATORBUG-30146))

### iOS

* Fixed that multiple dialogs informing about devices that are not in developer
  mode were opened simultaneously
* Fixed that a wrong warning about the provisioning profile could be shown
  ([QTCREATORBUG-30158](https://bugreports.qt.io/browse/QTCREATORBUG-30158))

### Remote Linux

* Fixed that deployment could fail when trying to kill the potentially running
  application
  ([QTCREATORBUG-30024](https://bugreports.qt.io/browse/QTCREATORBUG-30024))

### Boot2Qt

* Fixed that SSH operations could silently fail after the connection got lost
  ([QTCREATORBUG-29982](https://bugreports.qt.io/browse/QTCREATORBUG-29982))

### WASM

* Fixed issues with spaces in the `emsdk` path
  ([QTCREATORBUG-29981](https://bugreports.qt.io/browse/QTCREATORBUG-29981))

Credits for these changes go to:
--------------------------------
Alessandro Portale  
André Pönitz  
Artem Sokolovskii  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
Cristián Maureira-Fredes  
David Faure  
David Schulz  
Eike Ziller  
Fabian Vogt  
Jaroslaw Kobus  
Leena Miettinen  
Marcus Tillmanns  
Mathias Hasselmann  
Robert Löhning  
Sivert Krøvel  
Thiago Macieira  
Yasser Grimes  
