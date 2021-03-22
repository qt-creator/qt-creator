Qt Creator 4.14.2
=================

Qt Creator version 4.14.2 contains bug fixes.

The most important changes are listed in this document. For a complete
list of changes, see the Git log for the Qt Creator sources that
you can check out from the public Git repository. For example:

    git clone git://code.qt.io/qt-creator/qt-creator.git
    git log --cherry-pick --pretty=oneline origin/v4.14.1..v4.14.2

General
-------

### Building Qt Creator with CMake

* Fixed installation location of desktop and appstream files

Help
----

* Fixed crash with `Previous/Next Open Document in History` (QDS-3743)

Editing
-------

* Re-added generic highlighting for Autoconf files (QTCREATORBUG-25391)

Debugging
---------

### LLDB

* Fixed performance issue (QTCREATORBUG-25185, QTCREATORBUG-25217)

Platforms
---------

### macOS

* Fixed vanishing controls in Welcome mode in Dark Mode (QTCREATORBUG-25405)

Credits for these changes go to:
--------------------------------
Alessandro Portale  
Christian Stenger  
Christophe Giboudeaux  
David Schulz  
Eike Ziller  
Heiko Becker  
Ivan Komissarov  
Miikka Heikkinen  
Robert Löhning  
