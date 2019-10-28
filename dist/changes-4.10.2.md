# Qt Creator 4.10.2

Qt Creator version 4.10.2 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.10.1..v4.10.2

## General

* Added experimental SerialTerminal plugin to prebuilt binaries

## Projects

### CMake

* Fixed issue with deployment to remote Linux devices (QTCREATORBUG-21235)

### Qbs

* Fixed executable path in run configuration when custom installation directory
  is specified (QTCREATORBUG-23039)

## Debugging

* Fixed crash when logging output (QTCREATORBUG-22733)
* Fixed crash when removing previously dragged breakpoint (QTCREATORBUG-23107)
* Fixed highlighting of watch item when value changes

## Test Integration

### GTest

* Fixed `Run Under Cursor` (QTCREATORBUG-23068)

### Boost

* Fixed handling of parameterized tests

## Platform Specific

### Windows

* Fixed detection of Visual C++ Build Tools 2015 and MSVC 2010
  (QTCREATORBUG-22960)

### iOS

* Fixed deployment to iOS 13 devices (QTCREATORBUG-22950)

## Credits for these changes go to:

Alexis Murzeau  
Andre Hartmann  
Antonio Di Monaco  
Christian Kandeler  
Christian Stenger  
David Schulz  
Eike Ziller  
Laurent Montel  
Leena Miettinen  
Mariana Meireles  
Michael Weghorn  
Nikolai Kosjar  
Vikas Pachdha  
