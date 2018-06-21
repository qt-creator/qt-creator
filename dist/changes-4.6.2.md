Qt Creator version 4.6.2 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v4.6.1..v4.6.2

General

QMake Projects

* Fixed reparsing after changes (QTCREATORBUG-20113)

Qt Support

* Fixed detection of Qt Quick Compiler in Qt 5.11 (QTCREATORBUG-19993)

C++ Support

* Fixed flags for C files with MSVC (QTCREATORBUG-20198)

Debugging

* Fixed crash when attaching to remote process (QTCREATORBUG-20331)

Platform Specific

macOS

* Fixed signature of pre-built binaries (QTCREATORBUG-20370)

Android

* Fixed path to C++ includes (QTCREATORBUG-20340)

QNX

* Fixed restoring deploy steps (QTCREATORBUG-20248)

Credits for these changes go to:  
Alessandro Portale  
André Pönitz  
Christian Stenger  
Eike Ziller  
Ivan Donchevskii  
Oswald Buddenhagen  
Robert Löhning  
Ulf Hermann  
Vikas Pachdha  
