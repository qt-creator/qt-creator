Qt Creator 19.0.2
=================

Qt Creator version 19.0.2 contains bug fixes.
It is a free upgrade for all users.

The most important changes are listed in this document. For a complete list of
changes, see the Git log for the Qt Creator sources that you can check out from
the public Git repository or view online at

<https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=v19.0.1..v19.0.2>

General
-------

Fixed

* Setting English as the UI language on systems with non-English locale
  ([QTCREATORBUG-34457](https://bugreports.qt.io/browse/QTCREATORBUG-34457))

Editing
-------

Fixed

* A freeze when unindenting widely indented lines
  ([QTCREATORBUG-33260](https://bugreports.qt.io/browse/QTCREATORBUG-33260))

### Language Server Protocol

Fixed

* A crash when shutting down language servers

Platforms
---------

### Android

Fixed

* That projects created with the wizards had issues with accessibility

Credits for these changes go to:
--------------------------------
Christian Stenger  
David Schulz  
Eike Ziller  
Frank Osterfeld
