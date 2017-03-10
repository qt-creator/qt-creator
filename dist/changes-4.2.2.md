Qt Creator version 4.2.2 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline v4.2.1..v4.2.2

All Projects

* Fixed available kits after selecting Qt 5.8 as minimal required version
  in wizard (QTCREATORBUG-17574)
* Fixed that `Run in terminal` was sometimes ignored (QTCREATORBUG-17608)
* Fixed that `This file is not part of any project` was shown in editor after
  adding new file to project (QTCREATORBUG-17743)

Qt Support

* Fixed ABI detection of static Qt builds

Qbs Projects

* Fixed duplicate include paths (QTCREATORBUG-17381)
* Fixed that generated object files where shown in Locator and Advanced Search
  (QTCREATORBUG-17382)

C++ Support

* Fixed that inline namespaces were used in generated code (QTCREATORBUG-16086)

Debugging

* GDB
    * Fixed performance regression when resolving enum names
      (QTCREATORBUG-17598)

Version Control Systems

* Git
    * Fixed crash when committing and pushing to Gerrit (QTCREATORBUG-17634)
    * Fixed searching for patterns starting with dash in `Files in File System`
      when using `git grep`
    * Fixed discarding changes before performing other actions (such as Pull)
      (QTCREATORBUG-17156)

Platform Specific

Android

* Fixed that installing package with lower version number than already installed
  package silently failed (QTCREATORBUG-17789)
* Fixed crash when re-running application after stopping it (QTCREATORBUG-17691)

iOS

* Fixed running applications on devices with iOS 10.1 and later
  (QTCREATORBUG-17818)

BareMetal

* Fixed debugging with OpenOCD in TCP/IP mode on Windows (QTCREATORBUG-17765)
