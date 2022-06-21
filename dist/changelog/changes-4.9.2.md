# Qt Creator 4.9.2

Qt Creator version 4.9.2 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.9.1..v4.9.2

## General

* Fixed display of shortcuts in `Keyboard` preferences (QTCREATORBUG-22333)

## Editing

* Fixed disabled editor close button in Design mode (QTCREATORBUG-22553)

### Syntax Highlighting

* Fixed highlighting issue while editing (QTCREATORBUG-22290)

## All Projects

* Fixed saving state of `Hide Empty Directories`
* Fixed crash that could happen after project parsing failed

## C++ Support

* Fixed expansion of `%DATE%` in license templates (QTCREATORBUG-22440)

## Qt Quick Designer

* Fixed crash on malformed QML (QDS-778)

## Platform Specific

### macOS

* Re-enabled graphics card switching that was disabled as a workaround
  for OpenGL issues on macOS 10.14.4 (QTCREATORBUG-22215)

## Credits for these changes go to:

Christian Kandeler  
Christian Stenger  
David Schulz  
Eike Ziller  
Leena Miettinen  
Michl Voznesensky  
Robert Löhning  
Thomas Hartmann  
Tim Jenssen  
Tobias Hunger  
Ulf Hermann  
