# Qt Creator 4.10.1

Qt Creator version 4.10.1 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.10.0..v4.10.1

## Editing

* Fixed file saving with some text encodings
* Fixed `Preserve case` in advanced search and replace (QTCREATORBUG-19696)
* Fixed crash when changing editor font (QTCREATORBUG-22933)

## Help

* Fixed that text moved around when resizing and zooming (QTCREATORBUG-4756)

## All Projects

* Fixed `Qt Creator Plugin` wizard (QTCREATORBUG-22945)

## Debugging

* Fixed more layout restoration issues (QTCREATORBUG-22286, QTCREATORBUG-22415, QTCREATORBUG-22938)

### LLDB

* Fixed wrong empty command line argument when debugging (QTCREATORBUG-22975)

## Qt Quick Designer

* Removed transformations from list of unsupported types
* Fixed update of animation curve editor

## Platform Specific

### macOS

* Fixed debugging with Xcode 11 (QTCREATORBUG-22955)
* Fixed window stacking order after closing file dialog (QTCREATORBUG-22906)
* Fixed window size after exiting fullscreen

### QNX

* Fixed that QNX compiler could not be selected for C

## Credits for these changes go to:

Aleksei German  
Alexander Akulich  
Andre Hartmann  
André Pönitz  
Christian Kandeler  
Christian Stenger  
Cristian Adam  
David Schulz  
Eike Ziller  
Knud Dollereder  
Leena Miettinen  
Lisandro Damián Nicanor Pérez Meyer  
Nikolai Kosjar  
Orgad Shaneh  
Richard Weickelt  
Sergey Belyashov  
Thomas Hartmann  
